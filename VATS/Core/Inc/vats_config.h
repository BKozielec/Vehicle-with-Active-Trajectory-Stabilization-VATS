/*
 * vats_config.h
 *
 * Wspólne stałe konfiguracyjne VATS. Wszystkie wartości oznaczone
 * "NIEZWERYFIKOWANE" trzeba potwierdzić pomiarem na sprzęcie.
 */

#ifndef INC_VATS_CONFIG_H_
#define INC_VATS_CONFIG_H_

typedef enum {
    WHEEL_LEFT  = 0,   /* kanał A TB6612, enkoder TIM3 */
    WHEEL_RIGHT = 1,   /* kanał B TB6612, enkoder TIM8 */
    WHEEL_COUNT = 2
} Wheel;

/* ---------------- Timing ---------------- */
#define CONTROL_PERIOD_MS        20U    /* okres pętli regulacji prędkości */
#define TELEMETRY_PERIOD_MS      100U   /* okres wysyłania telemetrii */

/* ---------------- Enkodery ----------------
 * Liczniki w trybie TIM_ENCODERMODE_TI1 (zliczanie zboczy jednego kanału, x2).
 * ENCODER_CPR = impulsy licznika na 1 obrót koła (wału za przekładnią 50:1).
 * NIEZWERYFIKOWANE: wartość przeniesiona ze starego kodu. Pomiar: obróć koło
 * ręcznie 10 razy i podziel "enc_l"/"enc_r" z telemetrii przez 10.
 */
#define ENCODER_CPR              2000.0f
#define ENCODER_L_INVERT         0      /* 1 = odwróć znak lewego enkodera */
#define ENCODER_R_INVERT         0      /* 1 = odwróć znak prawego enkodera */

/* ---------------- Silniki ----------------
 * MOTOR_MAX_RPM: prędkość przy pełnym PWM, w skali ENCODER_CPR.
 * NIEZWERYFIKOWANE: wartość przeniesiona ze starego kodu (104 przy CPR 2000).
 * Po zmierzeniu CPR zmierz też to (komenda PWM:1000, koła w powietrzu).
 */
#define MOTOR_MAX_RPM            104.0f
#define MOTOR_DUTY_MAX           1000U  /* TIM1 ARR = 999 -> CCR 1000 = 100% */
#define MOTOR_DUTY_MIN           100U   /* offset strefy martwej (feedforward) */
#define MOTOR_DIR_CHANGE_RPM     5.0f   /* zmiana kierunku dopiero poniżej tej prędkości */
#define MOTOR_L_INVERT           0      /* 1 = lewy silnik kręci się odwrotnie do DIR */
#define MOTOR_R_INVERT           0      /* 1 = prawy silnik kręci się odwrotnie do DIR */

/* ---------------- Regulator prędkości (PI) ----------------
 * Jednostki: wejście RPM, wyjście jednostki duty 0..MOTOR_DUTY_MAX.
 * Wartości startowe przeliczone ze starego kodu, do strojenia.
 */
#define SPEED_KP                 11.5f  /* duty / RPM */
#define SPEED_KI                 40.0f  /* duty / (RPM * s) */
#define SPEED_KD                 0.0f   /* duty * s / RPM, D liczone z pomiaru */

/* ---------------- Serwo SG90 ----------------
 * Kąt w konwencji aplikacji: 0..180, 90 = prosto.
 * SERVO_ANGLE_MIN/MAX: mechaniczne limity skrętu – NIEZWERYFIKOWANE,
 * ustaw po sprawdzeniu, gdzie drążki/zwrotnice dochodzą do oporu.
 */
#define SERVO_PULSE_MIN_US       1000   /* impuls dla kąta 0 */
#define SERVO_PULSE_MAX_US       2000   /* impuls dla kąta 180 */
#define SERVO_CENTER_TRIM_US     0      /* korekta środka, jeśli koła nie są prosto przy 90 */
#define SERVO_ANGLE_CENTER       90
#define SERVO_ANGLE_MIN          45
#define SERVO_ANGLE_MAX          135

#endif /* INC_VATS_CONFIG_H_ */
