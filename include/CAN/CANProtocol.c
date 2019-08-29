/**
 * @file    CANProtocol.c
 * @author  rur_member
 * @version V1.0.0
 * @date    2016/11/06
 * @brief
 */

/* Includes ------------------------------------------------------------------*/
#include "CANProtocol.h"

#include "CANProtocol_MasterCommand.h"
#include "CommonMacro.h"
#include "CANProtocol_RingBuffer.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct {
	CANProtocol_PacketTypedef receivedBuffer[CANPROTOCOL_RX_PACKET_BUF_SIZE];
	CANProtocol_PacketQueueTypedef Queue;
	CANProtocol_PacketTypedef receivedResponseBuffer[16];
	volatile uint16_t waitingResponseFlag, receivedResponseFlag;
} CANProtocol_RxBufferTypedef;

/* Private define ------------------------------------------------------------*/
#define CANPROTOCOL_FIFO					CAN_FIFO0
#define CANPROTOCOL_MASTERCODE_FILTER_NUM	0
#define CANPROTOCOL_NORMALCODE_FILTER_NUM	1

/* Private macro -------------------------------------------------------------*/
#if defined(STM32F30X)
#define CAN_NUM		1
#elif defined(STM32F40_41xxx)
#define CAN_NUM		2
#endif

/* Private variables ---------------------------------------------------------*/
// Rxバッファ
static CANProtocol_RxBufferTypedef CANProtocol_rxBuffer[CAN_NUM];
// この基板のID
static CANProtocol_NodeIDTypedef CANProtocol_thisBoardID;
// 計測、表示用
static void (*CANProtocol_responseSignalCallBack)(bool) = 0;
static uint16_t CANProtocol_lastResponseTime = 0;

/* Private function prototypes -----------------------------------------------*/
_ALWAYS_INLINE ErrorStatus CANProtocol_getRxBuffer(
		CAN_TypeDef *CANx,
		CANProtocol_RxBufferTypedef **rxBuffer);
_ALWAYS_INLINE ErrorStatus CANProtocol_receivePacketFromPeriph(
		CAN_TypeDef *CANx,
		CANProtocol_PacketTypedef *packet);
_ALWAYS_INLINE void CANProtocol_sendPacketToNetwork(
		CAN_TypeDef *CANx,
		const CANProtocol_PacketTypedef *packet);
_ALWAYS_INLINE ErrorStatus CANProtocol_getFreeTimeStamp(
		CAN_TypeDef *CANx,
		uint8_t *timeStamp);
_ALWAYS_INLINE ErrorStatus CANProtocol_waitResponse(
		CAN_TypeDef *CANx,
		CANProtocol_DataTypedef *packetData,
		uint8_t timeStamp);

/* Private functions ---------------------------------------------------------*/

/**
 * 受信キューを取得する
 * @param CANx		CAN_TypeDef
 * @param rxBuffer	キューへのポインタが入る
 * @return
 */
_ALWAYS_INLINE ErrorStatus CANProtocol_getRxBuffer(CAN_TypeDef *CANx,
												   CANProtocol_RxBufferTypedef **rxBuffer) {
	switch ((uint32_t) CANx) {
	case (uint32_t) CAN1:
		*rxBuffer = &CANProtocol_rxBuffer[0];
		break;
#if 1 < CAN_NUM
	case (uint32_t) CAN2:
		*rxBuffer = &CANProtocol_rxBuffer[1];
		break;
#endif
	default:
		return ERROR;
	}
	return SUCCESS;
}

/**
 * パケットをペリフェラルから取得する
 * @param CANx
 * @param packet
 * @return
 */
_ALWAYS_INLINE ErrorStatus CANProtocol_receivePacketFromPeriph(CAN_TypeDef *CANx,
															   CANProtocol_PacketTypedef *packet) {
	if (CAN_GetFlagStatus(CANx, CAN_FLAG_FMP0) == RESET) {
		return ERROR;
	}
	// 受信してデコード
	CanRxMsg canRxMsg;
	CAN_Receive(CANx, CANPROTOCOL_FIFO, &canRxMsg);
	CANProtocolPacket_convCanRxMsg2Packet(&canRxMsg, packet);
	return SUCCESS;
}

/**
 * パケットを送信
 * @param CANx
 * @param packet
 */
_ALWAYS_INLINE void CANProtocol_sendPacketToNetwork(CAN_TypeDef *CANx,
													const CANProtocol_PacketTypedef *packet) {
	CanTxMsg canTxMsg;
	// エンコ―ドして送信
	CANProtocolPacket_convPacket2CanTxMsg(packet, &canTxMsg);
	{// Mutex Area
		__disable_irq();
		while(CAN_Transmit(CANx, &canTxMsg) == CAN_TxStatus_NoMailBox) {
			continue;
		}
		__enable_irq();
	}
}

/**
 * タイムスタンプを発行する
 * @param rxBuffer
 * @return
 */
_ALWAYS_INLINE ErrorStatus CANProtocol_getFreeTimeStamp(CAN_TypeDef *CANx, uint8_t *timeStamp) {
	CANProtocol_RxBufferTypedef *rxBuffer;
	if (CANProtocol_getRxBuffer(CANx, &rxBuffer) == ERROR)
		return ERROR;

	// タイムスタンプの空きができるまで待機
	// デッドロック発生するとしたらここ
	// すでに全てのタイムスタンプが使われている状態で最優先割り込みがここにくるとout
	while (rxBuffer->waitingResponseFlag == 0xFFFF) {
		continue;
	}
	uint16_t availableTimeStampBit;
	{// Mutex Area
		// 最小値のタイムスタンプ(bit)を発行する
		__disable_irq();
		availableTimeStampBit =
				~(rxBuffer->waitingResponseFlag) & -(~(rxBuffer->waitingResponseFlag));
		rxBuffer->waitingResponseFlag |= availableTimeStampBit;
		rxBuffer->receivedResponseFlag &= ~availableTimeStampBit;
		__enable_irq();
	}
	// タイムスタンプを数字に変換
	// やろうと思えばハッシュ関数使って超速で計算できる やろうと思えば
	LOOP (i, 16) {
		if (availableTimeStampBit & (0x0001 << i)) {
			*timeStamp = i;
			return SUCCESS;
		}
	}
	return ERROR;
}

/**
 * タイムスタンプで指定したパケットの取得待ちをする
 * @param CANx
 * @param packetData
 * @param timeStamp
 * @return
 */
_ALWAYS_INLINE ErrorStatus CANProtocol_waitResponse(CAN_TypeDef *CANx,
													CANProtocol_DataTypedef *packetData,
													uint8_t timeStamp) {
	CANProtocol_RxBufferTypedef *rxBuffer;
	if (CANProtocol_getRxBuffer(CANx, &rxBuffer) == ERROR) {
		return ERROR;
	}

	const uint16_t timeStampBit = 0x0001 << timeStamp;
#ifdef CANPROTOCOL_MEASURE_RESPNSE_TIME
	uint16_t startCount = TIM_GetCounter(CANPROTOCOL_MEASURE_TIMx);
#endif
	while (!(rxBuffer->receivedResponseFlag & timeStampBit)) {
		continue;
	}
#ifdef CANPROTOCOL_MEASURE_RESPNSE_TIME
	CANProtocol_lastResponseTime =
			(uint16_t)(TIM_GetCounter(CANPROTOCOL_MEASURE_TIMx) - startCount);
#endif

	*packetData = rxBuffer->receivedResponseBuffer[timeStamp].packetData;
	// ここフラグ下ろす順番大事なので
	rxBuffer->receivedResponseFlag &= ~timeStampBit;
	rxBuffer->waitingResponseFlag &= ~timeStampBit;
	return SUCCESS;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

/**
 * CAN_Protocolの初期化
 * @param thisBoardID
 * @param responseSignalCallBack
 * @return
 */
ErrorStatus CANProtocol_config(CANProtocol_NodeIDTypedef thisBoardID,
							   uint8_t commandCodeMask,
							   void (*responseSignalCallBack)(bool)) {
	LOOP (i, CAN_NUM) {
		CANProtocol_rxBuffer[i].Queue.buffer = CANProtocol_rxBuffer[i].receivedBuffer;
		CANProtocol_rxBuffer[i].Queue.size = CANPROTOCOL_RX_PACKET_BUF_SIZE;
	}
	CANProtocol_thisBoardID = thisBoardID;
	CANProtocol_responseSignalCallBack = responseSignalCallBack;

	//細かい事はf3のリファレンスのp.1027あたりを参照のこと
	CAN_FilterInitTypeDef CAN_filterInitStructure;
	CAN_filterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
	CAN_filterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
	CAN_filterInitStructure.CAN_FilterFIFOAssignment = CANPROTOCOL_FIFO;
	CAN_filterInitStructure.CAN_FilterActivation = ENABLE;

	uint32_t eidMask = 0, eidMach = 0;

	eidMask = 0x01 << 28;
	eidMach = CANProtocol_PacketMasterType_Master << 28;

	CAN_filterInitStructure.CAN_FilterNumber = CANPROTOCOL_MASTERCODE_FILTER_NUM;
	CAN_filterInitStructure.CAN_FilterMaskIdHigh = eidMask >> 13;
	CAN_filterInitStructure.CAN_FilterMaskIdLow = eidMask << 3;
	CAN_filterInitStructure.CAN_FilterIdHigh = eidMach >> 13;
	CAN_filterInitStructure.CAN_FilterIdLow = eidMach << 3;
	CAN_FilterInit(&CAN_filterInitStructure);

	eidMask = 0x01 << 28
			| ((0x07 << 4) | 0x0F) << 8;
//			| (commandCodeMask & 0xFF) << 0;
	eidMach = CANProtocol_PacketMasterType_Normal << 28
			| (((thisBoardID.category & 0x07) << 4) | (thisBoardID.id & 0x0F)) << 8;

	CAN_filterInitStructure.CAN_FilterNumber = CANPROTOCOL_NORMALCODE_FILTER_NUM;
	CAN_filterInitStructure.CAN_FilterMaskIdHigh = eidMask >> 13;
	CAN_filterInitStructure.CAN_FilterMaskIdLow = eidMask << 3;
	CAN_filterInitStructure.CAN_FilterIdHigh = eidMach >> 13;
	CAN_filterInitStructure.CAN_FilterIdLow = eidMach << 3;
	CAN_FilterInit(&CAN_filterInitStructure);

#ifdef CANPROTOCOL_MEASURE_RESPNSE_TIME
	// 計測用タイマ
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM13, ENABLE);
	TIM_TimeBaseInitTypeDef TIM_timeBaseInitStructure;
	TIM_TimeBaseStructInit(&TIM_timeBaseInitStructure);
	TIM_timeBaseInitStructure.TIM_Period = 0xffff;
	TIM_timeBaseInitStructure.TIM_Prescaler = 84 - 1;
	TIM_timeBaseInitStructure.TIM_ClockDivision = 0;
	TIM_timeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_timeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(CANPROTOCOL_MEASURE_TIMx, &TIM_timeBaseInitStructure);
	TIM_Cmd(CANPROTOCOL_MEASURE_TIMx, ENABLE);
#endif

	CANProtocolMasterCommand_config(thisBoardID);
	return SUCCESS;
}

/**
 * パケットを送信する
 * @param CANx
 * @param packetData
 */
void CANProtocol_sendPacket(CAN_TypeDef *CANx,
							const CANProtocol_PacketMasterType masterCode,
							const CANProtocol_NodeIDTypedef *addrID,
							const CANProtocol_DataTypedef *packetData) {
	CANProtocol_PacketTypedef packet;
	packet.extIdHeader.masterType = masterCode;
	packet.extIdHeader.responseType = CANProtocol_PacketResponseType_NoResponseReq;
	packet.extIdHeader.timeStamp = 0x00;
	packet.extIdHeader.sender = CANProtocol_thisBoardID;
	packet.extIdHeader.reciever = *addrID;
	packet.packetData = *packetData;
	CANProtocol_sendPacketToNetwork(CANx, &packet);
}

/**
 * パケットを送信し、返信待ちをする
 * @param CANx
 * @param packetData
 */
void CANProtocol_sendAndWaitResponse(CAN_TypeDef *CANx,
									 const CANProtocol_PacketMasterType masterCode,
									 const CANProtocol_NodeIDTypedef *addrID,
									 CANProtocol_DataTypedef *packetData) {
	if (CANProtocol_responseSignalCallBack) {
		(*CANProtocol_responseSignalCallBack)(true);
	}

	uint8_t timeStamp = 0x00;
	CANProtocol_getFreeTimeStamp(CANx, &timeStamp);

	CANProtocol_PacketTypedef packet;
	packet.extIdHeader.masterType = masterCode;
	packet.extIdHeader.responseType = CANProtocol_PacketResponseType_ResponseReq;
	packet.extIdHeader.timeStamp = timeStamp;
	packet.extIdHeader.sender = CANProtocol_thisBoardID;
	packet.extIdHeader.reciever = *addrID;
	packet.packetData = *packetData;
	CANProtocol_sendPacketToNetwork(CANx, &packet);

	CANProtocol_waitResponse(CANx, packetData, timeStamp);

	if (CANProtocol_responseSignalCallBack) {
		(*CANProtocol_responseSignalCallBack)(false);
	}
}

/**
 * 返信パケットを送信する
 * @param CANx
 * @param packetData
 */
void CANProtocol_sendResponsePacket(CAN_TypeDef *CANx,
									CANProtocol_PacketTypedef *packet) {
	packet->extIdHeader.responseType = CANProtocol_PacketResponseType_Response;
	packet->extIdHeader.reciever = packet->extIdHeader.sender;
	packet->extIdHeader.sender = CANProtocol_thisBoardID;
	CANProtocol_sendPacketToNetwork(CANx, packet);
}

/**
 * 受信したパケットを取得する
 * @param CANx
 * @param packet
 * @return
 */
ErrorStatus CANProtocol_popPacketFromRxBuffer(CAN_TypeDef *CANx,
											  CANProtocol_PacketTypedef *packet) {
	CANProtocol_RxBufferTypedef *rxBuffer;
	if (CANProtocol_getRxBuffer(CANx, &rxBuffer) == ERROR) {
		return ERROR;
	}
	if (CANProtocolPacketQueue_front(&rxBuffer->Queue, packet) == ERROR) {
		return ERROR;
	}
	CANProtocolPacketQueue_pop(&rxBuffer->Queue);
	return SUCCESS;
}

/**
 * 最新の返答時間を取得する
 * @return
 */
uint32_t CANProtocol_getLastResponseTime(void) {
	return CANProtocol_lastResponseTime;
}

/**
 * 受信したパケットをバッファに貯める
 * 受信割り込み用
 * @param CANx
 * @return
 */
ErrorStatus CANProtocol_saveReceivedPacketToBuffer(CAN_TypeDef *CANx) {
	CANProtocol_PacketTypedef packet;
	if(CANProtocol_receivePacketFromPeriph(CANx, &packet) != SUCCESS) {
		return ERROR;
	}
	CANProtocol_RxBufferTypedef *rxBuffer;
	if (CANProtocol_getRxBuffer(CANx, &rxBuffer) != SUCCESS) {
		return ERROR;
	}

	// 返信以外
	if (packet.extIdHeader.responseType != CANProtocol_PacketResponseType_Response) {
		// ますたーこーど
		if (packet.extIdHeader.masterType == CANProtocol_PacketMasterType_Master) {
			CANProtocolMasterCommand_runCommand(CANx, &packet);
		} else if (CANProtocolPacketQueue_push(&(rxBuffer->Queue), &packet) != SUCCESS) {
			return ERROR;
		}
	} else { // 返信
		uint8_t timeStamp = packet.extIdHeader.timeStamp;
		if (!(rxBuffer->waitingResponseFlag & (0x0001 << timeStamp))) {
			return ERROR;
		}
		rxBuffer->receivedResponseBuffer[timeStamp] = packet;
		rxBuffer->receivedResponseFlag |= 0x0001 << timeStamp;
	}
	return SUCCESS;
}
