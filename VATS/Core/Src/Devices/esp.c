/*
 * esp.c
 *
 *  Created on: Oct 28, 2026
 *      Author: bartol
 */

#include "Devices/esp.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "main.h"


#define RX_BUFFER_SIZE 64


static char rx_line_buffer[RX_BUFFER_SIZE];
static uint8_t buffer_index = 0;

static int parsed_speed =0;
static char parsed_direction='S';
static int parsed_servo_angle=0;

void ESP_ParseLine(char *command){
	if(strncmp(command,"DIR:",4)==0){
		parsed_direction=command[4];
	}
	else if(strncmp(command,"S:",2)==0){
		parsed_speed = atoi(&command[2]);
	}
	else if (strncmp(command, "SERVO:", 6) == 0) {
	    parsed_servo_angle = atoi(&command[6]);
	}
}

static uint8_t single_rx_byte;
extern UART_HandleTypeDef huart2;

void ESP_Init(void) {
    buffer_index = 0;
    memset(rx_line_buffer, 0, RX_BUFFER_SIZE);

    HAL_UART_Receive_IT(&huart2, &single_rx_byte, 1);
}


void ESP_UART_RxCallback(uint8_t rx_data) {
    if (rx_data == '\n' || rx_data == '\r') {
        if (buffer_index > 0) {
            rx_line_buffer[buffer_index] = '\0';
            ESP_ParseLine(rx_line_buffer);
            buffer_index = 0;
        }
    } else {
        if (buffer_index < (RX_BUFFER_SIZE - 1)) {
            rx_line_buffer[buffer_index++] = rx_data;
        } else {
            buffer_index = 0;
        }
    }


    HAL_UART_Receive_IT(&huart2, &single_rx_byte, 1);
}

int ESP_GetSpeed(void){
	return parsed_speed;
}
char ESP_GetDirection(void){
	return parsed_direction;
}
int ESP_GetServoAngle(void){
	return parsed_servo_angle;
}
