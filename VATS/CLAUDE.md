# VATS – Vehicle with Active Trajectory Stabilization

Odpowiadaj po polsku, terminologia techniczna po angielsku. Bez komplementów, wprost wskazuj błędy.
Nie zgaduj nazw rejestrów, pinów ani parametrów – jeśli nie jesteś pewien, wskaż co sprawdzić w RM0481 / datasheecie / UM3121.
Podawaj kompletny kod (bez "// reszta bez zmian"), z obsługą błędów HAL (HAL_StatusTypeDef).

## Sprzęt
- MCU: NUCLEO-H533RE (STM32H533RET6, Cortex-M33), ST-LINK V3EC on-board
- Napęd: 2x N20E 6 V 300 RPM 50:1 z enkoderem magnetycznym (kanały C1/C2), sterownik Pololu TB6612FNG (713)
- Skręt: SG90 (Ackermann), PWM 50 Hz, 1–2 ms
- IMU: GY-6500 (MPU-6500) po I2C
- Komunikacja: ESP32 (ESP-WROOM-32) po UART – Bluetooth
- Zasilanie: 2S 18650 + BMS 2S -> VMOT TB6612; LM2596 -> 5 V -> Nucleo E5V (JP5 na 5-6), ESP32, serwo
- 3V3 z Nucleo (CN6-4 / CN7-16) -> VCC TB6612, IMU, enkodery

## Planowany przydział pinów (do potwierdzenia z .ioc)
| Funkcja | Peryferium | Piny |
|---|---|---|
| Enkoder L | TIM3 encoder mode | PC6 / PC7 |
| Enkoder R | TIM2 encoder mode | PA0 / PB3 |
| PWM silników | TIM1_CH1 / CH2 | PA8 / PA9 |
| Serwo | TIM4_CH3 | PB8 |
| IMU | I2C1 | PB6 (SCL) / PB7 (SDA) |
| ESP32 | USART3 | PC10 (TX) / PC11 (RX) |
| Kierunek TB6612 (AIN1/2, BIN1/2, STBY) | GPIO | PB12, PB13, PB2, PC8, PC9 |
VCP ST-LINK = USART2 na PA2/PA3 (niewyprowadzone na złącza) – do printf/debug.

## Znane problemy
- Wejście AIN1 na obecnym module TB6612 uszkodzone -> kanał A nie ma jazdy wstecz
  (PWM=0 przy IN1/IN2 daje short brake). Planowana wymiana modułu.
- W trakcie diagnozy: brak 3V3 na CN7-16 przy zasilaniu E5V – sprawdzić przed podłączaniem czujników.

## Build
Projekt CMake wygenerowany z CubeMX 6.19 (FW_H5 1.5.1), budowany w VS Code rozszerzeniem
STM32CubeIDE for Visual Studio Code. Folder projektu: VATS/ (repo git piętro wyżej).
Własny kod tylko w sekcjach USER CODE BEGIN/END albo w osobnych plikach dopisanych
do target_sources w CMakeLists.txt. Po każdej regeneracji z CubeMX sprawdzić git diff -w.
