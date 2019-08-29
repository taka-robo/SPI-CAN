/**
 * @file    CANProtocol_RingBuffer.c
 * @author  R.Y
 * @version V1.0.0
 * @date    2016/04/02
 * @brief
 */

/* Includes ------------------------------------------------------------------*/
#include "CANProtocol_RingBuffer.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
 * キューのクリア
 * @param queue
 */
void CANProtocolPacketQueue_clear(CANProtocol_PacketQueueTypedef *queue) {
	queue->readPt = 0;
	queue->writePt = 0;
}

/**
 * キューが空か否か
 * @param queue
 * @return
 */
bool CANProtocolPacketQueue_isEmpty(const CANProtocol_PacketQueueTypedef *queue) {
	return queue->writePt == queue->readPt;
}

/**
 * キューが満タンか否か
 * @param queue
 * @return
 */
bool CANProtocolPacketQueue_isFull(const CANProtocol_PacketQueueTypedef *queue) {
	uint32_t nextWriteIt = (queue->writePt + 1) % queue->size;
	return nextWriteIt == queue->readPt;
}

/**
 * キューにプッシュ
 * @param queue
 * @param packet
 * @return
 */
ErrorStatus CANProtocolPacketQueue_push(CANProtocol_PacketQueueTypedef *queue, const CANProtocol_PacketTypedef *packet) {
	if (CANProtocolPacketQueue_isFull(queue)) {
		return ERROR;
	}

	queue->buffer[queue->writePt++] = *packet;
	queue->writePt %= queue->size;
	return SUCCESS;
}

/**
 * キューをポップ
 * @param queue
 * @return
 */
ErrorStatus CANProtocolPacketQueue_pop(CANProtocol_PacketQueueTypedef *queue) {
	if (CANProtocolPacketQueue_isEmpty(queue)) {
		return ERROR;
	}

	queue->readPt++;
	queue->readPt %= queue->size;
	return SUCCESS;
}

/**
 * キューの先頭を取得
 * @param queue
 * @param packet
 * @return
 */
ErrorStatus CANProtocolPacketQueue_front(const CANProtocol_PacketQueueTypedef *queue, CANProtocol_PacketTypedef *packet) {
	if (CANProtocolPacketQueue_isEmpty(queue)) {
		return ERROR;
	}

	*packet = queue->buffer[queue->readPt];
	return SUCCESS;
}

