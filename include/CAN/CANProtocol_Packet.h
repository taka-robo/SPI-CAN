/**
 * @file    CANProtocol_Packet.h
 * @author  rur_member
 * @version V1.0.0
 * @date    2016/11/06
 * @brief
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CANPROTOCOL_PACKET_H_
#define CANPROTOCOL_PACKET_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"
#include "CommonMacro.h"

/* Exported types ------------------------------------------------------------*/
//誰かハンガリアン記法を使わずにCで列挙を上手く使う方法を教えて下さい

/**
 * パケットのマスターコード
 * マスターコードはすべての種類のノードで実行可能な命令
 * これを受信した場合,返答は最速でなければならない
 * ex. 接続先の列挙, 生存確認
 * 1bit
 */
typedef enum {
	CANProtocol_PacketMasterType_Master = 0x00,//!< マスターコード
	CANProtocol_PacketMasterType_Normal = 0x01,//!< ノーマルコード
} CANProtocol_PacketMasterType;

/**
 * パケットの返答種別
 * 2bit
 */
typedef enum {
	CANProtocol_PacketResponseType_NoResponseReq	= 0x00,//!< パケットは一方通行で、返信を求めない
	CANProtocol_PacketResponseType_ResponseReq		= 0x01,//!< パケットは返信を求める内容のものである
	CANProtocol_PacketResponseType_Response			= 0x02,//!< パケットは返信内容である
} CANProtocol_PacketResponseType;

/**
 * CANノードの基板種別
 * 3bit
 */
typedef enum {
	CANProtocol_NodeCategory_MotherBoard		= 0x00,//!< マザーボード
	CANProtocol_NodeCategory_ControlalerBoard	= 0x01,//!< コントローラ基板
	CANProtocol_NodeCategory_GouyokuDCMD		= 0x02,//!< 強欲ドライバ（遂に来ましたよ）
	CANProtocol_NodeCategory_BLDCMD				= 0x03,//!< ブラシレスドライバ
	CANProtocol_NodeCategory_ColorLineSensor	= 0x04,//!< カラーラインセンサ
	CANProtocol_NodeCategory_SolenoidValve		= 0x05,//!< 電磁弁(未)
	CANProtocol_NodeCategory_LoggingBoard		= 0x07,//!< ログ取り基板
} CANProtocol_NodeCategory;

/**
 * CANノードの識別コード
 */
typedef struct {
	CANProtocol_NodeCategory category;
	uint8_t id;
} CANProtocol_NodeIDTypedef;

/**
 * ExtendIDのヘッダ
 */
typedef struct {
	CANProtocol_PacketMasterType masterType;
	CANProtocol_PacketResponseType responseType;
	uint8_t timeStamp;
	CANProtocol_NodeIDTypedef sender;
	CANProtocol_NodeIDTypedef reciever;
} CANProtocol_ExtIdHeaderTypedef;

/**
 * パケットの実データ部
 */
typedef struct {
	uint8_t commandCode;
	uint8_t dataSize;
	uint8_t data[8];
} CANProtocol_DataTypedef;

typedef struct {
	CANProtocol_ExtIdHeaderTypedef extIdHeader;
	CANProtocol_DataTypedef packetData;
} CANProtocol_PacketTypedef;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
_ALWAYS_INLINE void CANProtocolPacket_convCanRxMsg2Packet(const CanRxMsg *msg,
														  CANProtocol_PacketTypedef *packet) {
	uint32_t eid = msg->ExtId; uint8_t shift = 29;
	shift -= 1; packet->extIdHeader.masterType			= (eid >> shift) & 0x01;
	shift -= 2; packet->extIdHeader.responseType		= (eid >> shift) & 0x03;
	shift -= 4; packet->extIdHeader.timeStamp			= (eid >> shift) & 0x0F;
	shift -= 3; packet->extIdHeader.sender.category		= (eid >> shift) & 0x07;
	shift -= 4; packet->extIdHeader.sender.id			= (eid >> shift) & 0x0F;
	shift -= 3; packet->extIdHeader.reciever.category	= (eid >> shift) & 0x07;
	shift -= 4; packet->extIdHeader.reciever.id			= (eid >> shift) & 0x0F;
	shift -= 8; packet->packetData.commandCode			= (eid >> shift) & 0xFF;

	// Data データ部
	packet->packetData.dataSize = msg->DLC;
	LOOP (i, packet->packetData.dataSize) {
		packet->packetData.data[i] = msg->Data[i];
	}
}

_ALWAYS_INLINE void CANProtocolPacket_convPacket2CanTxMsg(const CANProtocol_PacketTypedef *packet,
														  CanTxMsg *msg) {
	uint32_t eid = 0x00000000; uint8_t shift = 29;
	shift -= 1; eid |= (packet->extIdHeader.masterType			& 0x01) << shift;
	shift -= 2; eid |= (packet->extIdHeader.responseType		& 0x03) << shift;
	shift -= 4; eid |= (packet->extIdHeader.timeStamp			& 0x0F) << shift;
	shift -= 3; eid |= (packet->extIdHeader.sender.category		& 0x07) << shift;
	shift -= 4; eid |= (packet->extIdHeader.sender.id			& 0x0F) << shift;
	shift -= 3; eid |= (packet->extIdHeader.reciever.category	& 0x07) << shift;
	shift -= 4; eid |= (packet->extIdHeader.reciever.id			& 0x0F) << shift;
	shift -= 8; eid |= (packet->packetData.commandCode			& 0xFF) << shift;
	msg->ExtId = eid;
	// ExtendID
	msg->IDE = CAN_Id_Extended;
	// リモートは使わない
	msg->RTR = CAN_RTR_Data;

	// Data データ部
	msg->DLC = packet->packetData.dataSize;
	LOOP (i, packet->packetData.dataSize) {
		msg->Data[i] = packet->packetData.data[i];
	}
}

#ifdef __cplusplus
}
#endif

#endif /* CANPROTOCOL_PACKET_H_ */
