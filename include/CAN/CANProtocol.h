/**
 * @file    CANProtocol.h
 * @author  rur_member
 * @version V1.0.0
 * @date    2016/11/06
 * @brief
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CANPROTOCOL_H_
#define CANPROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"
#include "CANProtocol_Packet.h"

/* Exported types ------------------------------------------------------------*/
typedef union {
	uint8_t u8[4];
	int16_t s16;
	uint16_t u16;
	int32_t s32;
	uint32_t u32;
	float f32;
} DataConverterTypedef;

/* Exported constants --------------------------------------------------------*/
#define CANPROTOCOL_RX_PACKET_BUF_SIZE		(256)
//#define CANPROTOCOL_MEASURE_RESPNSE_TIME
#define CANPROTOCOL_MEASURE_TIMx			TIM13

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
ErrorStatus CANProtocol_config(
		CANProtocol_NodeIDTypedef thisBoardID,
		uint8_t commandCodeMask,
		void (*responseSignalCallBack)(bool));
void CANProtocol_sendPacket(
		CAN_TypeDef *CANx,
		const CANProtocol_PacketMasterType masterCode,
		const CANProtocol_NodeIDTypedef *addrID,
		const CANProtocol_DataTypedef *packetData);
void CANProtocol_sendAndWaitResponse(
		CAN_TypeDef *CANx,
		const CANProtocol_PacketMasterType masterCode,
		const CANProtocol_NodeIDTypedef *addrID,
		CANProtocol_DataTypedef *packetData);
void CANProtocol_sendResponsePacket(
		CAN_TypeDef *CANx,
		CANProtocol_PacketTypedef *packet);
ErrorStatus CANProtocol_popPacketFromRxBuffer(
		CAN_TypeDef *CANx,
		CANProtocol_PacketTypedef *packet);
uint32_t CANProtocol_getLastResponseTime(void);

#ifdef __cplusplus
}
#endif

#endif /* CANPROTOCOL_H_ */
