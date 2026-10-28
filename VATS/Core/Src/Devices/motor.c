/*
 * motor.c
 *
 * Created on: Oct 19, 2026
 * Author: bartol
 */

#include "Devices/motor.h"
#include "Devices/esp.h" // Pobieramy dane z modułu ESP (gettery)

// Piny kierunku dla obu silników pracujących razem (np. PC11 i PC12)
#define MOTOR_DIR1_PORT GPIOC
#define MOTOR_DIR1_PIN GPIO_PIN_11
#define MOTOR_DIR2_PORT GPIOC
#define MOTOR_DIR2_PIN GPIO_PIN_12

extern TIM_HandleTypeDef htim1;

void motor_init(void){

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);


    motor_set_direction('S');
    motor_set_speed(0);
}


void motor_set_speed(uint32_t speed) {
    if (speed > 1000) speed = 1000;

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, speed);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, speed);
}


void motor_set_direction(char dir) {
    if (dir == 'F') {
        HAL_GPIO_WritePin(MOTOR_DIR1_PORT, MOTOR_DIR1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_DIR2_PORT, MOTOR_DIR2_PIN, GPIO_PIN_RESET);
    } else if (dir == 'B') {
        HAL_GPIO_WritePin(MOTOR_DIR1_PORT, MOTOR_DIR1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_DIR2_PORT, MOTOR_DIR2_PIN, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(MOTOR_DIR1_PORT, MOTOR_DIR1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_DIR2_PORT, MOTOR_DIR2_PIN, GPIO_PIN_RESET);
    }
}

// Funkcja spinająca silniki z modułem ESP (pobiera stan i aplikuje na oba silniki)
void motor_update_from_esp(void) {
    int target_speed = ESP_GetSpeed();
    char target_dir = ESP_GetDirection();

    motor_set_speed((uint32_t)target_speed);
    motor_set_direction(target_dir);
}
