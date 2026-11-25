/*
 * motor.h
 *
 * Created on: Oct 19, 2026
 * Author: bartol
 *
 * Dwa silniki na TB6612FNG:
 *   lewy  = kanał A: PWMA PA8 (TIM1_CH1), AIN1 PC7,  AIN2 PC9
 *   prawy = kanał B: PWMB PA9 (TIM1_CH2), BIN1 PC11, BIN2 PC12
 * STBY na sztywno do 3V3 (nie sterowany z MCU).
 */

#ifndef DEVICES_MOTOR_H_
#define DEVICES_MOTOR_H_

#include "main.h"
#include "vats_config.h"

HAL_StatusTypeDef motor_init(void);

/* Regulacja prędkości w zamkniętej pętli. dir: 'F', 'B' lub 'S'. */
void motor_set_target(char dir, uint32_t speed_rpm);

/* Test bez regulatora: stały duty 0..MOTOR_DUTY_MAX na oba silniki. */
void motor_set_open_loop(char dir, uint32_t duty);

/* Natychmiastowe zatrzymanie (wybieg), reset regulatorów. */
void motor_stop(void);

/* Wywoływać co CONTROL_PERIOD_MS, po encoder_update(). */
void motor_control_update(uint32_t dt_ms);

/* Aktualny duty ze znakiem kierunku (+ przód, - tył). */
int32_t motor_get_duty(Wheel wheel);

/* Zatrzymanie na poziomie rejestrów – bezpieczne w Error_Handler/ISR. */
void motor_emergency_stop(void);

#endif /* DEVICES_MOTOR_H_ */
