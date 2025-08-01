/*
 * uart_handler.c
 *
 *  Created on: Jul 17, 2025
 *      Author: 1-05
 */
#include "uart_handler.h"
#include "motor_control.h"
#include <stdio.h>

extern UART_HandleTypeDef huart1;
extern uint8_t rx_byte;

volatile uint8_t command_flag = 0;
volatile uint8_t command_param = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        command_flag = 1;
        command_param = rx_byte;
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

// UART 명령 처리 함수
void UART_ProcessCommand(uint8_t cmd)
{
    switch (cmd)
    {
        case CMD_MOVE_FORWARD:
            L9110s_MoveForward(MOTOR_SPEED_NORMAL);
            HAL_Delay(15000);  // 15초 동안 전진
            L9110s_Stop();
            UART_SendResponse(ACK_CODE);
            break;

        case CMD_MOVE_BACKWARD:
            L9110s_MoveBackward(MOTOR_SPEED_NORMAL);
            HAL_Delay(1000);  // 1초 동안 후진
            L9110s_Stop();
            UART_SendResponse(ACK_CODE);
            break;

        case CMD_TURN_LEFT:
            L9110s_TurnLeft(MOTOR_SPEED_NORMAL);
            HAL_Delay(500);   // 0.5초 동안 좌회전
            L9110s_Stop();
            UART_SendResponse(ACK_CODE);
            break;

        case CMD_TURN_RIGHT:
            L9110s_TurnRight(MOTOR_SPEED_NORMAL);
            HAL_Delay(500);   // 0.5초 동안 우회전
            L9110s_Stop();
            UART_SendResponse(ACK_CODE);
            break;

        case CMD_STOP:
            L9110s_Stop();
            UART_SendResponse(ACK_CODE);
            break;

        default:
            UART_SendResponse(NACK_CODE);
            break;
    }
}

// UART 응답 전송 함수
void UART_SendResponse(uint8_t response)
{
    HAL_UART_Transmit(&huart1, &response, 1, 100);
}


