/*
 * motor_control.c
 *
 *  Created on: Jul 17, 2025
 *      Author: 1-05
 */


#include "motor_control.h"
#include <stdio.h>

// L9110s 모터 드라이버 초기화
void L9110s_Init(void)
{
    // PWM 시작
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); // PA5 - 모터A-1A
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2); // PB3 - 모터A-1B
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); // PA6 - 모터B-1A
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); // PA7 - 모터B-1B

    // 초기 상태: 모터 정지
    L9110s_Stop();
}

// PWM 듀티 사이클 설정
void L9110s_SetPWM(L9110s_Channel_t channel, uint16_t duty)
{
    // 듀티 사이클 범위 제한 (0~999)
    if (duty > 999) duty = 999;

    switch (channel)
    {
        case MOTOR_A_1A:  // PA5 (TIM2_CH1)
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, duty);
            break;
        case MOTOR_A_1B:  // PB3 (TIM2_CH2)
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, duty);
            break;
        case MOTOR_B_1A:  // PA6 (TIM3_CH1)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty);
            break;
        case MOTOR_B_1B:  // PA7 (TIM3_CH2)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, duty);
            break;
    }
}

// L9110s 모터 제어 메인 함수
void L9110s_MotorControl(L9110s_Direction_t left_dir, uint16_t left_speed,
                         L9110s_Direction_t right_dir, uint16_t right_speed)
{
    // 좌측 모터 제어 (모터 B)
    switch (left_dir)
    {
        case L9110S_FORWARD:
            L9110s_SetPWM(MOTOR_B_1A, 0);           // 1A = LOW
            L9110s_SetPWM(MOTOR_B_1B, left_speed);  // 1B = PWM
            break;
        case L9110S_BACKWARD:
            L9110s_SetPWM(MOTOR_B_1A, left_speed);  // 1A = PWM
            L9110s_SetPWM(MOTOR_B_1B, 0);           // 1B = LOW
            break;
        case L9110S_STOP:
            L9110s_SetPWM(MOTOR_B_1A, 0);           // 1A = LOW
            L9110s_SetPWM(MOTOR_B_1B, 0);           // 1B = LOW
            break;
    }

    // 우측 모터 제어 (모터 A)
    switch (right_dir)
    {
        case L9110S_FORWARD:
            L9110s_SetPWM(MOTOR_A_1A, right_speed); // 1A = PWM
            L9110s_SetPWM(MOTOR_A_1B, 0);           // 1B = LOW
            break;
        case L9110S_BACKWARD:
            L9110s_SetPWM(MOTOR_A_1A, 0);           // 1A = LOW
            L9110s_SetPWM(MOTOR_A_1B, right_speed); // 1B = PWM
            break;
        case L9110S_STOP:
            L9110s_SetPWM(MOTOR_A_1A, 0);           // 1A = LOW
            L9110s_SetPWM(MOTOR_A_1B, 0);           // 1B = LOW
            break;
    }
}


// 전진 동작
void L9110s_MoveForward(uint16_t speed)
{
    L9110s_MotorControl(L9110S_FORWARD, speed, L9110S_FORWARD, speed);
}

// 후진 동작
void L9110s_MoveBackward(uint16_t speed)
{
    L9110s_MotorControl(L9110S_BACKWARD, speed, L9110S_BACKWARD, speed);
}

// 좌회전 동작 (좌측 모터 후진, 우측 모터 전진)
void L9110s_TurnLeft(uint16_t speed)
{
    L9110s_MotorControl(L9110S_BACKWARD, speed, L9110S_FORWARD, speed);
}

// 우회전 동작 (좌측 모터 전진, 우측 모터 후진)
void L9110s_TurnRight(uint16_t speed)
{
    L9110s_MotorControl(L9110S_FORWARD, speed, L9110S_BACKWARD, speed);
}

// 모터 정지
void L9110s_Stop(void)
{
    L9110s_MotorControl(L9110S_STOP, 0, L9110S_STOP, 0);
}
