/*
 * uart_handler.h
 *
 *  Created on: Jul 17, 2025
 *      Author: 1-05
 */

#ifndef INC_UART_HANDLER_H_
#define INC_UART_HANDLER_H_

#include "main.h"

// 라즈베리파이 코드와 동일한 명령 코드
#define CMD_MOVE_FORWARD    0xA1
#define CMD_MOVE_BACKWARD   0xA2
#define CMD_TURN_LEFT       0xA3
#define CMD_TURN_RIGHT      0xA4
#define CMD_STOP            0xA5

#define ACK_CODE            0xAA
#define NACK_CODE           0xFF

// 함수 선언
void UART_ProcessCommand(uint8_t cmd);
void UART_SendResponse(uint8_t response);

#endif /* INC_UART_HANDLER_H_ */
