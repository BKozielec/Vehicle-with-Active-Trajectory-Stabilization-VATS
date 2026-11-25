/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "vats_config.h"
#include "Devices/motor.h"
#include "Devices/enkoder.h"
#include "Devices/servo.h"
#include "Devices/esp.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static const char *reset_cause = "?";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* Przyczyna resetu (kolejność: najbardziej znacząca pierwsza). BOR = spadek
   * zasilania; przy zwykłym włączeniu zasilania ustawione są BOR i PIN. */
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != 0U)      reset_cause = "IWDG";
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != 0U) reset_cause = "WWDG";
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST) != 0U) reset_cause = "LPWR";
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != 0U)  reset_cause = "SFT";
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST) != 0U)  reset_cause = "BOR";
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != 0U)  reset_cause = "PIN";
  __HAL_RCC_CLEAR_RESET_FLAGS();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_TIM8_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  /* Najpierw silniki w stan bezpieczny, potem reszta. */
  if (motor_init() != HAL_OK)
  {
    Error_Handler();
  }
  if (encoder_init() != HAL_OK)
  {
    Error_Handler();
  }
  if (servo_init() != HAL_OK)
  {
    Error_Handler();
  }
  if (ESP_Init() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t last_control_tick = HAL_GetTick();
  uint32_t last_telemetry_tick = last_control_tick;
  uint32_t last_cmd_seq = 0U;
  EspCommand cmd;
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    ESP_Poll();

    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - last_control_tick;
    if (elapsed >= CONTROL_PERIOD_MS)
    {
      last_control_tick = now;

      ESP_GetCommand(&cmd);
      if (cmd.seq != last_cmd_seq)
      {
        last_cmd_seq = cmd.seq;
        if (cmd.open_loop)
        {
          motor_set_open_loop(cmd.dir, cmd.open_loop_duty);
        }
        else
        {
          motor_set_target(cmd.dir, cmd.speed_rpm);
        }
        /* Błąd możliwy tylko przy niezainicjalizowanym serwie (wtedy Error_Handler wyżej). */
        (void)servo_set_angle(cmd.servo_angle);
      }

      encoder_update(elapsed);
      motor_control_update(elapsed);
    }

    if ((now - last_telemetry_tick) >= TELEMETRY_PERIOD_MS)
    {
      last_telemetry_tick = now;
      EspTelemetry t = {
        .rpm_l = encoder_get_rpm(WHEEL_LEFT),
        .rpm_r = encoder_get_rpm(WHEEL_RIGHT),
        .duty_l = motor_get_duty(WHEEL_LEFT),
        .duty_r = motor_get_duty(WHEEL_RIGHT),
        .enc_l = encoder_get_position(WHEEL_LEFT),
        .enc_r = encoder_get_position(WHEEL_RIGHT),
        .servo_angle = servo_get_angle(),
        .uptime_ms = now,
        .reset_cause = reset_cause,
      };
      /* HAL_BUSY = poprzednia ramka jeszcze leci; pomijamy tę próbkę. */
      (void)ESP_SendTelemetry(&t);
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV2;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the programming delay
  */
  __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_0);
}

/* USER CODE BEGIN 4 */


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  motor_emergency_stop();
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
