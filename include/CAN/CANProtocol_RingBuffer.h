/**
 * @file    CANProtocol_RingBuffer.h
 * @author  R.Y
 * @version V1.0.0
 * @date    2016/04/02
 * @brief
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CANPROTOCOL_RINGBUFFER_H_
#define CANPROTOCOL_RINGBUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "CANProtocol.h"

/* Exported types ------------------------------------------------------------*/
typedef struct {
	uint32_t size;
	uint32_t readPt;
	uint32_t writePt;
	CANProtocol_PacketTypedef *buffer;
} CANProtocol_PacketQueueTypedef;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void CANProtocolPacketQueue_clear(CANProtocol_PacketQueueTypedef *queue);
bool CANProtocolPacketQueue_isEmpty(const CANProtocol_PacketQueueTypedef *queue);
bool CANProtocolPacketQueue_isFull(const CANProtocol_PacketQueueTypedef *queue);
ErrorStatus CANProtocolPacketQueue_push(CANProtocol_PacketQueueTypedef *queue,
										const CANProtocol_PacketTypedef *packet);
ErrorStatus CANProtocolPacketQueue_pop(CANProtocol_PacketQueueTypedef *queue);
ErrorStatus CANProtocolPacketQueue_front(const CANProtocol_PacketQueueTypedef *queue,
										 CANProtocol_PacketTypedef *packet);

#ifdef __cplusplus
}
#endif

#endif /* CANPROTOCOL_RINGBUFFER_H_ */
