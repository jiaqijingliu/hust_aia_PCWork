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
#include "adc.h"
#include "usart.h"
#include "gpio.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

typedef struct
{
  float Last_P;
  float Now_P;
  float out;
  float K;
  float Q;
  float R;
} KaF;
#define TEST_NUM 100
#define MEDIAN_WINDOW 5
float test_temp[TEST_NUM] =
    {
        25.0f,25.6f,24.3f,26.2f,23.8f,25.5f,24.7f,26.8f,23.5f,25.1f,
  24.2f,27.1f,25.7f,23.9f,26.5f,24.8f,25.3f,22.9f,27.4f,24.6f,

  55.0f,25.2f,24.4f,26.1f,23.6f,25.8f,24.9f,27.0f,23.4f,25.0f,
  24.1f,26.7f,25.5f,23.7f,27.2f,24.5f,-5.0f,25.4f,24.8f,26.0f,

  23.3f,25.7f,24.2f,27.6f,25.1f,23.9f,26.4f,24.6f,25.8f,23.5f,
  25.0f,24.4f,26.9f,23.8f,25.6f,24.7f,27.3f,25.2f,23.6f,26.1f,

  65.0f,24.9f,25.5f,23.7f,26.8f,24.3f,25.9f,23.2f,27.0f,24.8f,
  25.3f,26.5f,23.9f,25.7f,24.6f,27.2f,23.5f,25.1f,24.4f,26.0f,

  24.7f,25.6f,23.8f,26.6f,24.2f,25.4f,27.1f,23.6f,25.0f,24.5f,
  26.3f,23.9f,25.8f,24.7f,27.4f,25.2f,23.5f,26.1f,24.3f,25.0f};
KaF temp_filter_setup;
float init_temp;
/* USER CODE END PV */

void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
float Median_Filter(float *data, int index, int window)
{
    int i, j;
    int count = 0;
    float buffer[MEDIAN_WINDOW];

    for(i = index; i > index - window && i >= 0; i--)
    {
        buffer[count++] = data[i];
    }

    // 简单冒泡排序
    for(i = 0; i < count - 1; i++)
    {
        for(j = i + 1; j < count; j++)
        {
            if(buffer[i] > buffer[j])
            {
                float tmp = buffer[i];
                buffer[i] = buffer[j];
                buffer[j] = tmp;
            }
        }
    }

    if(count % 2 == 1)
        return buffer[count / 2];
    else
        return (buffer[count/2 - 1] + buffer[count/2]) / 2.0f;
}

/* USER CODE END 0 */

/* USER CODE BEGIN 2 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_LPUART1_UART_Init();

    int i;
    char msg[100];

    HAL_UART_Transmit(&hlpuart1,
                      (uint8_t *)"Index,Raw,Median\r\n",
                      strlen("Index,Raw,Median\r\n"),
                      100);

    while(1)
    {
        for(i = 0; i < TEST_NUM; i++)
        {
            float raw = test_temp[i];
            float median = Median_Filter(test_temp, i, MEDIAN_WINDOW);

            snprintf(msg, sizeof(msg),
                     "%d,%.2f,%.2f\r\n",
                     i,
                     raw,
                     median);

            HAL_UART_Transmit(&hlpuart1, (uint8_t *)msg, strlen(msg), 100);
            HAL_Delay(100); // 模拟采样间隔
        }

        while(1)
        {
            HAL_Delay(1000); // 输出完成后停止
        }
    }
}

  /* USER CODE END 3 */


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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

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
