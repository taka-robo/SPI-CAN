/**
 * @file    CANProtocol_MasterCommand.c
 * @author  rur_member
 * @version V1.0.0
 * @date    2016/11/08
 * @brief
 */

/* Includes ------------------------------------------------------------------*/
#include "CANProtocol_MasterCommand.h"

#include "CANProtocol.h"

/* Private typedef -----------------------------------------------------------*/
typedef enum {
	CANProtocolMasterCommand_Event				= 0x00,
	CANProtocolMasterCommand_WatchDog			= 0x01,
	CANProtocolMasterCommand_ResetNode			= 0x02,
	CANProtocolMasterCommand_ErrorOccurred		= 0x03,
} CANProtocolMasterCommandCode;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
_ALWAYS_INLINE bool __eq_NodeID(CANProtocol_NodeIDTypedef x, CANProtocol_NodeIDTypedef y) {
	return x.category == y.category && x.id == y.id;
}

/* Private variables ---------------------------------------------------------*/
// この基板のID
static CANProtocol_NodeIDTypedef CANProtocolMC_thisBoardID;

/* Private function prototypes -----------------------------------------------*/
static void CANProtocolMasterCommand_recvEvent(CAN_TypeDef *CANx,
											   CANProtocol_PacketTypedef *packet);
static void CANProtocolMasterCommand_recvWatchDog(CAN_TypeDef *CANx,
												  CANProtocol_PacketTypedef *packet);
static void CANProtocolMasterCommand_recvResetNode(CAN_TypeDef *CANx,
												   CANProtocol_PacketTypedef *packet);
static void CANProtocolMasterCommand_recvErrorOccurred(CAN_TypeDef *CANx,
													   CANProtocol_PacketTypedef *packet);

/* Private functions ---------------------------------------------------------*/
static void (*const CANProtocolMasterCommandFunction[256])(CAN_TypeDef*, CANProtocol_PacketTypedef*) =
{
		CANProtocolMasterCommand_recvEvent,			// 0x00
		CANProtocolMasterCommand_recvWatchDog,		// 0x01
		CANProtocolMasterCommand_recvResetNode,		// 0x02
		CANProtocolMasterCommand_recvErrorOccurred,	// 0x03
};

/**
 * ますたーコマンドの初期化
 * @param thisBoardID
 */
void CANProtocolMasterCommand_config(CANProtocol_NodeIDTypedef thisBoardID) {
	CANProtocolMC_thisBoardID = thisBoardID;
}

/**
 * マスターコマンド処理
 * @param CANx
 * @param packet
 */
void CANProtocolMasterCommand_runCommand(CAN_TypeDef *CANx,
										 CANProtocol_PacketTypedef *packet) {
	if (CANProtocolMasterCommandFunction[packet->packetData.commandCode]) {
		CANProtocolMasterCommandFunction[packet->packetData.commandCode](CANx, packet);
	}
}

////////////////////////////////////////////////////////////////////////////////

static uint32_t CANProtocolMC_eventBuffer[8];

void CANProtocolMasterCommand_sendEvent(CAN_TypeDef *CANx, const uint8_t eventNum) {
	CANProtocol_DataTypedef packet;
	packet.commandCode = CANProtocolMasterCommand_Event;
	packet.dataSize = 1;
	packet.data[0] = eventNum;
	CANProtocol_NodeIDTypedef addr;
	CANProtocol_sendPacket(CANx, CANProtocol_PacketMasterType_Master,
						   &addr, &packet);
}

void CANProtocolMasterCommand_getEventList(uint32_t event[8]) {
	LOOP (i, 8) {
		event[i] = CANProtocolMC_eventBuffer[i];
		CANProtocolMC_eventBuffer[i] &= ~event[i];
	}
}

/**
 * イベント受信
 * @param CANx
 * @param packet
 */
void CANProtocolMasterCommand_recvEvent(CAN_TypeDef *CANx,
										CANProtocol_PacketTypedef *packet) {
	const uint8_t eventNum = packet->packetData.data[0];
	CANProtocolMC_eventBuffer[eventNum / 32] |= 0x01 << eventNum % 32;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * いきてるかー？
 * @param addr
 */
ErrorStatus CANProtocolMasterCommand_watchDog(CAN_TypeDef *CANx,
											  const CANProtocol_NodeIDTypedef *addr) {
	CANProtocol_DataTypedef packet;
	packet.commandCode = CANProtocolMasterCommand_WatchDog;
	packet.dataSize = 0;
	CANProtocol_sendAndWaitResponse(CANx, CANProtocol_PacketMasterType_Master,
									addr, &packet);
	return SUCCESS;
}

/**
 * しんでませーん
 * @param CANx
 * @param packet
 */
void CANProtocolMasterCommand_recvWatchDog(CAN_TypeDef *CANx,
										   CANProtocol_PacketTypedef *packet) {
	if (!__eq_NodeID(packet->extIdHeader.reciever, CANProtocolMC_thisBoardID)) {
		return;
	}
	CANProtocol_sendResponsePacket(CANx, packet);
}

////////////////////////////////////////////////////////////////////////////////

static void (*CANProtocolMC_resetCallBack)(bool*) = 0;

void CANProtocolMasterCommand_setResetCallBack(void (*callBack)(bool *)) {
	CANProtocolMC_resetCallBack = callBack;
}

/**
 * しぬがよい
 * @param addr
 */
void CANProtocolMasterCommand_resetNode(CAN_TypeDef *CANx,
										const CANProtocol_NodeIDTypedef *addr,
										bool allNode) {
	CANProtocol_DataTypedef packet;
	packet.commandCode = CANProtocolMasterCommand_ResetNode;
	packet.dataSize = 1;
	packet.data[0] = allNode;

	delay_ms(1000);
	CANProtocol_sendPacket(CANx, CANProtocol_PacketMasterType_Master,
						   addr, &packet);
	delay_ms(1000);
}

/**
 * あべし
 * @param CANx
 * @param packet
 */
void CANProtocolMasterCommand_recvResetNode(CAN_TypeDef *CANx,
											CANProtocol_PacketTypedef *packet) {
	if (!packet->packetData.data[0] && !__eq_NodeID(packet->extIdHeader.reciever, CANProtocolMC_thisBoardID)) {
		return;
	}

	if (CANProtocolMC_resetCallBack) {
		bool cancel = false;
		CANProtocolMC_resetCallBack(&cancel);
		if (cancel) {
			return;
		}
	}

	NVIC_SystemReset();
	while (true) {
		continue;
	}
}

////////////////////////////////////////////////////////////////////////////////

static void (*CANProtocolMC_errorCallBack)(uint8_t) = 0;

/**
 * コールバックの登録
 * @param callBack
 */
void CANProtocolMasterCommand_setErrorCallBack(void (*callBack)(uint8_t)) {
	CANProtocolMC_errorCallBack = callBack;
}

 /**
 * アカン
 * @param CANx
 * @param packet
 */
void CANProtocolMasterCommand_errorOccurred(CAN_TypeDef *CANx, const uint8_t errorCode) {
	CANProtocol_DataTypedef packet;
	packet.commandCode = CANProtocolMasterCommand_ErrorOccurred;
	packet.dataSize = 1;
	packet.data[0] = errorCode;
	CANProtocol_NodeIDTypedef addr;
	CANProtocol_sendPacket(CANx, CANProtocol_PacketMasterType_Master,
						   &addr, &packet);
}

/**
 *
 * @param CANx
 * @param packet
 */
void CANProtocolMasterCommand_recvErrorOccurred(CAN_TypeDef *CANx,
												CANProtocol_PacketTypedef *packet) {
	if (CANProtocolMC_errorCallBack) {
		CANProtocolMC_errorCallBack(packet->packetData.data[0]);
	}
}
