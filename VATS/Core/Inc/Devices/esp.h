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


#endif /* INC_DEVICES_ESP_H_ */
