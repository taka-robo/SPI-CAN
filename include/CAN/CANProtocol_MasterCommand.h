/**
 * @file    CANProtocol_MasterCommand.h
 * @author  rur_member
 * @version V1.0.0
 * @date    2016/11/08
 * @brief
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CANPROTOCOL_MASTERCOMMAND_H_
#define CANPROTOCOL_MASTERCOMMAND_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"
#include "CANProtocol_Packet.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void CANProtocolMasterCommand_config(CANProtocol_NodeIDTypedef thisBoardID);
void CANProtocolMasterCommand_runCommand(CAN_TypeDef *CANx,
										 CANProtocol_PacketTypedef *packet);

////////////////////////////////////////////////////////////////////////////////
void CANProtocolMasterCommand_sendEvent(CAN_TypeDef *CANx, const uint8_t eventNum);
void CANProtocolMasterCommand_getEventList(uint32_t event[8]);
////////////////////////////////////////////////////////////////////////////////
ErrorStatus CANProtocolMasterCommand_watchDog(CAN_TypeDef *CANx, const CANProtocol_NodeIDTypedef *addr);
////////////////////////////////////////////////////////////////////////////////
void CANProtocolMasterCommand_setResetCallBack(void (*callBack)(bool *));
void CANProtocolMasterCommand_resetNode(CAN_TypeDef *CANx, const CANProtocol_NodeIDTypedef *addr, bool allNode);
////////////////////////////////////////////////////////////////////////////////
void CANProtocolMasterCommand_setErrorCallBack(void (*callBack)(uint8_t));
void CANProtocolMasterCommand_errorOccurred(CAN_TypeDef *CANx, uint8_t errorCode);
////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* CANPROTOCOL_MASTERCOMMAND_H_ */
