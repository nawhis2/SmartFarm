/*
 * motor_control.h
 *
 *  Created on: Jul 17, 2025
 *      Author: 1-05
 */

#ifndef INC_MOTOR_CONTROL_H_
#define INC_MOTOR_CONTROL_H_

#include "main.h"

// L9110s 모터 채널 정의 (핀 매핑에 따라)
typedef enum {
    MOTOR_A_1A = 0,  // PA5 (TIM2_CH1) - 좌측 모터
    MOTOR_A_1B,      // PB3 (TIM2_CH2) - 좌측 모터
    MOTOR_B_1A,      // PA6 (TIM3_CH1) - 우측 모터
    MOTOR_B_1B       // PA7 (TIM3_CH2) - 우측 모터
} L9110s_Channel_t;

// L9110s 모터 방향 정의
typedef enum {
    L9110S_FORWARD = 0,
    L9110S_BACKWARD,
    L9110S_STOP
} L9110s_Direction_t;

// 모터 속도 정의
#define MOTOR_SPEED_MAX     800
#define MOTOR_SPEED_NORMAL  600
#define MOTOR_SPEED_SLOW    300
#define MOTOR_SPEED_STOP    0

// 함수 선언
void L9110s_Init(void);
void L9110s_SetPWM(L9110s_Channel_t channel, uint16_t duty);
void L9110s_MotorControl(L9110s_Direction_t left_dir, uint16_t left_speed,
                         L9110s_Direction_t right_dir, uint16_t right_speed);
void L9110s_MoveForward(uint16_t speed);
void L9110s_MoveBackward(uint16_t speed);
void L9110s_TurnLeft(uint16_t speed);
void L9110s_TurnRight(uint16_t speed);
void L9110s_Stop(void);


#endif /* INC_MOTOR_CONTROL_H_ */
