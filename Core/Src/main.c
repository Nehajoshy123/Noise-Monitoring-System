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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include <stdio.h>
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
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
typedef enum
{
    NOISE_NORMAL,
    NOISE_WARNING,
    NOISE_CRITICAL
} NoiseState;

NoiseState noise_state = NOISE_NORMAL;

uint32_t warning_start = 0;
uint32_t critical_start = 0;
uint32_t quiet_start = 0;

uint8_t warning_tracking = 0;
uint8_t critical_tracking = 0;
uint8_t quiet_tracking = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t Get_Noise_PeakToPeak(void)
{
    uint32_t start_tick = HAL_GetTick();
    uint16_t min_val = 4095;
    uint16_t max_val = 0;

    /* Sample microphone signal for 50 milliseconds */
    while ((HAL_GetTick() - start_tick) < 50)
    {
        HAL_ADC_Start(&hadc1);

        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
        {
            uint16_t sample = HAL_ADC_GetValue(&hadc1);

            if (sample < min_val)
                min_val = sample;

            if (sample > max_val)
                max_val = sample;
        }

        HAL_ADC_Stop(&hadc1);
    }

    return (max_val - min_val);
}

void UpdateNoiseState(uint16_t noise_level)
{
    uint32_t now = HAL_GetTick();

    /* Track sustained warning-level noise */
    if (noise_level >= 15)
    {
        if (!warning_tracking)
        {
            warning_start = now;
            warning_tracking = 1;
        }
    }
    else
    {
        warning_tracking = 0;
    }

    /* Track sustained critical-level noise */
    if (noise_level >= 80)
    {
        if (!critical_tracking)
        {
            critical_start = now;
            critical_tracking = 1;
        }
    }
    else
    {
        critical_tracking = 0;
    }

    /* Track continuous quiet conditions for recovery */
    if (noise_level <= 10)
    {
        if (!quiet_tracking)
        {
            quiet_start = now;
            quiet_tracking = 1;
        }
    }
    else
    {
        quiet_tracking = 0;
    }

    /* Change state based on duration */
    if (critical_tracking &&
        (now - critical_start >= 3000))
    {
        noise_state = NOISE_CRITICAL;
    }
    else if (noise_state == NOISE_NORMAL &&
             warning_tracking &&
             (now - warning_start >= 1500))
    {
        noise_state = NOISE_WARNING;
    }

    if ((noise_state == NOISE_WARNING ||
         noise_state == NOISE_CRITICAL) &&
        quiet_tracking &&
        (now - quiet_start >= 1500))
    {
        noise_state = NOISE_NORMAL;
    }
}


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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */

  HAL_ADCEx_Calibration_Start(&hadc1);

  OLED_Init(&hi2c1);
  char disp_buf[20];

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  uint16_t noise_level = Get_Noise_PeakToPeak();

	      UpdateNoiseState(noise_level);

	      /* Check for Push Button Reset (Stage 9) */
	      if (HAL_GPIO_ReadPin(Push_button_GPIO_Port, Push_button_Pin) == GPIO_PIN_RESET)
	      {
	          noise_state = NOISE_NORMAL;
	          critical_tracking = 0;
	          warning_tracking = 0;
	          quiet_tracking = 0;
	      }

	      uint32_t now = HAL_GetTick();

	      /* Turn off all status LEDs first */
	      HAL_GPIO_WritePin(GPIOB,
	                        Green_Pin | Yellow_Pin | Red_Pin,
	                        GPIO_PIN_RESET);

	      /* Blink the LED for the current state */
	      if ((now / 500) % 2 == 0)
	      {
	          if (noise_state == NOISE_NORMAL)
	          {
	              HAL_GPIO_WritePin(GPIOB, Green_Pin, GPIO_PIN_SET);
	          }
	          else if (noise_state == NOISE_WARNING)
	          {
	              HAL_GPIO_WritePin(GPIOB, Yellow_Pin, GPIO_PIN_SET);
	          }
	          else
	          {
	              HAL_GPIO_WritePin(GPIOB, Red_Pin, GPIO_PIN_SET);
	          }
	      }

	      /* --- BUZZER CONTROL --- */
	      if (noise_state == NOISE_CRITICAL)
	      {
	          HAL_GPIO_WritePin(GPIOB, Buzzer_Pin, GPIO_PIN_SET);
	      }
	      else
	      {
	          HAL_GPIO_WritePin(GPIOB, Buzzer_Pin, GPIO_PIN_RESET);
	      }

	      /* Update OLED */
	      OLED_Clear();

	      OLED_SetCursor(0, 0);
	      OLED_WriteString("CLASSROOM MONITOR");

	      OLED_SetCursor(0, 1);
	      snprintf(disp_buf, sizeof(disp_buf),
	               "NOISE LEVEL: %u", noise_level);
	      OLED_WriteString(disp_buf);

	      OLED_SetCursor(0, 3);

	      if (noise_state == NOISE_NORMAL)
	      {
	          OLED_WriteString("STATUS: NORMAL");
	      }
	      else if (noise_state == NOISE_WARNING)
	      {
	          OLED_WriteString("STATUS: WARNING");
	      }
	      else
	      {
	          OLED_WriteString("STATUS: CRITICAL");
	      }

	      OLED_Update();

	      HAL_Delay(50);
    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Buzzer_Pin|Green_Pin|Yellow_Pin|Red_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : Buzzer_Pin Green_Pin Yellow_Pin Red_Pin */
  GPIO_InitStruct.Pin = Buzzer_Pin|Green_Pin|Yellow_Pin|Red_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : Push_button_Pin */
  GPIO_InitStruct.Pin = Push_button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Push_button_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
