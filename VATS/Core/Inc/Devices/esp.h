/*
 * esp.h
 *
 *  Created on: Oct 28, 2026
 *      Author: bartol
 */
#include <stdint.h>
#ifndef INC_DEVICES_ESP_H_
#define INC_DEVICES_ESP_H_

void ESP_UART_RxCallback(uint8_t rx_data);
void ESP_ParseLine(char *command);
int ESP_GetSpeed(void);
char ESP_GetDirection(void);
int ESP_GetServoAngle(void);
void ESP_Init(void);
int _write(int file, char *ptr, int len);
void esp_send_encoder_rpm(float rpm);
#endif /* INC_DEVICES_ESP_H_ */
