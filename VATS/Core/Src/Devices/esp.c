/*
 * esp.c
 *
 *  Created on: Oct 28, 2026
 *      Author: bartol
 */

#include "Devices/esp.h"
#include "usart.h"
#include "vats_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ESP_LINE_MAX        64U
#define ESP_TX_BUF_SIZE     192U
#define ESP_SPEED_MAX_RPM   10000U   /* zakres parsera; motor.c przycina do MOTOR_MAX_RPM */
#define DEBUG_TX_TIMEOUT_MS 50U

typedef struct {
    UART_HandleTypeDef *huart;
    uint8_t  rx_byte;
    char     line[ESP_LINE_MAX];
    uint8_t  len;
    bool     overflow;
    volatile bool restart_pending;
    char     tx_buf[ESP_TX_BUF_SIZE];
} UartPort;

static UartPort port_esp = { .huart = &huart1 };   /* ESP32 */
static UartPort port_vcp = { .huart = &huart2 };   /* ST-LINK VCP (USB) */
static UartPort *const ports[] = { &port_esp, &port_vcp };
#define PORT_COUNT (sizeof(ports) / sizeof(ports[0]))

/* Stan komend modyfikowany w ISR, czytany w pętli głównej. */
static volatile char     cmd_dir = 'S';
static volatile uint32_t cmd_speed_rpm = 0U;
static volatile bool     cmd_open_loop = false;
static volatile uint32_t cmd_open_loop_duty = 0U;
static volatile int32_t  cmd_servo_angle = SERVO_ANGLE_CENTER;
static volatile EspMode  cmd_mode = ESP_MODE_MANUAL;
static volatile uint32_t cmd_seq = 0U;

static UartPort *find_port(const UART_HandleTypeDef *huart)
{
    for (size_t i = 0; i < PORT_COUNT; i++) {
        if (ports[i]->huart == huart) return ports[i];
    }
    return NULL;
}

/* Liczba bez znaku, same cyfry, w zakresie 0..max. */
static bool parse_uint(const char *s, uint32_t max, uint32_t *out)
{
    if (s == NULL || *s < '0' || *s > '9') return false;
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 10);
    if (end == s || *end != '\0' || v > max) return false;
    *out = (uint32_t)v;
    return true;
}

static void parse_line(const char *line)
{
    uint32_t value = 0U;

    if (strcmp(line, "DIR:F") == 0 || strcmp(line, "DIR:B") == 0 ||
        strcmp(line, "DIR:S") == 0) {
        cmd_dir = line[4];
    } else if (strncmp(line, "S:", 2) == 0) {
        if (!parse_uint(&line[2], ESP_SPEED_MAX_RPM, &value)) return;
        cmd_speed_rpm = value;
        cmd_open_loop = false;
    } else if (strncmp(line, "PWM:", 4) == 0) {
        if (!parse_uint(&line[4], MOTOR_DUTY_MAX, &value)) return;
        cmd_open_loop_duty = value;
        cmd_open_loop = true;
    } else if (strncmp(line, "SERVO:", 6) == 0) {
        if (!parse_uint(&line[6], 180U, &value)) return;
        cmd_servo_angle = (int32_t)value;
    } else if (strcmp(line, "ESTOP") == 0) {
        cmd_dir = 'S';
        cmd_speed_rpm = 0U;
        cmd_open_loop = false;
        cmd_open_loop_duty = 0U;
    } else if (strcmp(line, "MODE:MAN") == 0) {
        cmd_mode = ESP_MODE_MANUAL;
    } else if (strcmp(line, "MODE:AUTO") == 0) {
        cmd_mode = ESP_MODE_AUTO;
    } else {
        return;   /* nieznana komenda – ignoruj */
    }
    cmd_seq++;
}

static void process_byte(UartPort *p, uint8_t b)
{
    if (b == '\n' || b == '\r') {
        if (!p->overflow && p->len > 0U) {
            p->line[p->len] = '\0';
            parse_line(p->line);
        }
        p->len = 0U;
        p->overflow = false;
        return;
    }
    if (p->overflow) {
        return;   /* odrzucamy resztę za długiej linii */
    }
    if (p->len < (ESP_LINE_MAX - 1U)) {
        p->line[p->len++] = (char)b;
    } else {
        p->overflow = true;
        p->len = 0U;
    }
}

static HAL_StatusTypeDef start_rx(UartPort *p)
{
    HAL_StatusTypeDef status = HAL_UART_Receive_IT(p->huart, &p->rx_byte, 1U);
    p->restart_pending = (status != HAL_OK);
    return status;
}

HAL_StatusTypeDef ESP_Init(void)
{
    HAL_StatusTypeDef result = HAL_OK;
    for (size_t i = 0; i < PORT_COUNT; i++) {
        UartPort *p = ports[i];
        p->len = 0U;
        p->overflow = false;
        memset(p->line, 0, sizeof(p->line));
        HAL_StatusTypeDef status = start_rx(p);
        if (status != HAL_OK) {
            result = status;
        }
    }
    return result;
}

void ESP_Poll(void)
{
    for (size_t i = 0; i < PORT_COUNT; i++) {
        if (ports[i]->restart_pending) {
            (void)start_rx(ports[i]);
        }
    }
}

void ESP_GetCommand(EspCommand *out)
{
    if (out == NULL) return;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    out->dir            = cmd_dir;
    out->speed_rpm      = cmd_speed_rpm;
    out->open_loop      = cmd_open_loop;
    out->open_loop_duty = cmd_open_loop_duty;
    out->servo_angle    = cmd_servo_angle;
    out->mode           = cmd_mode;
    out->seq            = cmd_seq;
    __set_PRIMASK(primask);
}

static int32_t round_to_i32(float v)
{
    return (int32_t)((v >= 0.0f) ? (v + 0.5f) : (v - 0.5f));
}

HAL_StatusTypeDef ESP_SendTelemetry(const EspTelemetry *t)
{
    if (t == NULL) return HAL_ERROR;

    char line[ESP_TX_BUF_SIZE];
    int len = snprintf(line, sizeof(line),
        "{\"rpm_l\":%ld,\"rpm_r\":%ld,\"pwm_l\":%ld,\"pwm_r\":%ld,"
        "\"enc_l\":%ld,\"enc_r\":%ld,\"servo\":%ld,"
        "\"up\":%lu,\"rst\":\"%s\","
        "\"yaw_ref\":0,\"yaw_meas\":0,\"t_offset\":0}\n",
        (long)round_to_i32(t->rpm_l), (long)round_to_i32(t->rpm_r),
        (long)t->duty_l, (long)t->duty_r,
        (long)t->enc_l, (long)t->enc_r, (long)t->servo_angle,
        (unsigned long)t->uptime_ms,
        (t->reset_cause != NULL) ? t->reset_cause : "?");
    if (len <= 0 || (size_t)len >= sizeof(line)) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef result = HAL_OK;
    for (size_t i = 0; i < PORT_COUNT; i++) {
        UartPort *p = ports[i];
        if (p->huart->gState != HAL_UART_STATE_READY) {
            result = HAL_BUSY;
            continue;
        }
        memcpy(p->tx_buf, line, (size_t)len);
        HAL_StatusTypeDef status = HAL_UART_Transmit_IT(p->huart, (uint8_t *)p->tx_buf, (uint16_t)len);
        if (status != HAL_OK) {
            result = status;
        }
    }
    return result;
}

/* printf -> VCP (USART2), blokująco z krótkim timeoutem. */
int _write(int file, char *ptr, int len)
{
    (void)file;
    if (ptr == NULL || len <= 0) return 0;
    if (HAL_UART_Transmit(&huart2, (uint8_t *)ptr, (uint16_t)len, DEBUG_TX_TIMEOUT_MS) != HAL_OK) {
        return -1;
    }
    return len;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    UartPort *p = find_port(huart);
    if (p == NULL) return;
    process_byte(p, p->rx_byte);
    (void)start_rx(p);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    /* ORE/FE/NE przerywa odbiór IT – porzuć bieżącą linię i wznów. */
    UartPort *p = find_port(huart);
    if (p == NULL) return;
    p->len = 0U;
    p->overflow = false;
    (void)start_rx(p);
}
