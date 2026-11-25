/*
 * esp.h
 *
 *  Created on: Oct 28, 2026
 *      Author: bartol
 *
 * Komunikacja tekstowa, linie zakończone '\n' lub '\r', 115200 8N1:
 *   - ESP32:          USART1, PB14 (TX) / PB15 (RX)
 *   - terminal na PC: USART2 = VCP ST-LINK (USB), te same komendy
 *
 * Komendy:
 *   DIR:F | DIR:B | DIR:S   kierunek jazdy
 *   S:<rpm>                 zadana prędkość [RPM], regulacja w pętli zamkniętej
 *   PWM:<0..1000>           test: stały duty bez regulatora (do S: wraca regulacja)
 *   SERVO:<0..180>          kąt skrętu, 90 = prosto
 *   ESTOP                   natychmiastowe zatrzymanie (DIR:S, S:0)
 *   MODE:MAN | MODE:AUTO    tryb (zapamiętywany, AUTO jeszcze nieobsługiwany)
 *
 * Telemetria (JSON, jedna linia) wysyłana na oba porty.
 */

#ifndef INC_DEVICES_ESP_H_
#define INC_DEVICES_ESP_H_

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ESP_MODE_MANUAL = 0,
    ESP_MODE_AUTO   = 1
} EspMode;

typedef struct {
    char     dir;             /* 'F', 'B', 'S' */
    uint32_t speed_rpm;
    bool     open_loop;
    uint32_t open_loop_duty;
    int32_t  servo_angle;
    EspMode  mode;
    uint32_t seq;             /* rośnie przy każdej przyjętej komendzie */
} EspCommand;

typedef struct {
    float   rpm_l;
    float   rpm_r;
    int32_t duty_l;
    int32_t duty_r;
    int32_t enc_l;
    int32_t enc_r;
    int32_t servo_angle;
    uint32_t uptime_ms;          /* czas od startu – spadek do 0 = reset MCU */
    const char *reset_cause;     /* przyczyna ostatniego resetu (diagnostyka zasilania) */
} EspTelemetry;

HAL_StatusTypeDef ESP_Init(void);

/* Wywoływać w pętli głównej: wznawia odbiór po błędach UART. */
void ESP_Poll(void);

/* Kopia ostatniego stanu komend (atomowo względem przerwań). */
void ESP_GetCommand(EspCommand *out);

/* Wysyłka nieblokująca (IT); HAL_BUSY, jeśli poprzednia wysyłka trwa. */
HAL_StatusTypeDef ESP_SendTelemetry(const EspTelemetry *t);

int _write(int file, char *ptr, int len);

#endif /* INC_DEVICES_ESP_H_ */
