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
#define KAF_INIT_P 0.f    // 初始的估计误差
#define KAF_INIT_OUT 25.f // 默认初值
#define KAF_MAX_INC 15.0f

#define KAF_Q_MIN 0.004f
#define KAF_Q_MAX 0.3f
#define KAF_R_MIN 0.08f
#define KAF_R_MAX 0.8f

#define KAF_INIT_Q KAF_Q_MIN  // 过程噪声，推荐取0.001~0.01
#define KAF_INIT_R KAF_R_MAX   // 测量噪声，推荐取 0.1~1
#define KAF_DIFF_FULL 4.5f
#define KAF_ADAPT_START 0.20f
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
  26.3f,23.9f,25.8f,24.7f,27.4f,25.2f,23.5f,26.1f,24.3f,25.0f
};
KaF temp_filter_setup;
float init_temp;
/* USER CODE END PV */

void SystemClock_Config(void);
/* USER CODE BEGIN PFP */


float Kalman_out(KaF *Kal, float in, float max_inc)
{
  float diff;
  diff = in - Kal->out;
  float abs_diff = fabsf(diff);
  if (abs_diff > max_inc) // 跳变过大，说明可能电路扰动等
  {
    in = Kal->out;
    diff = in - Kal->out;
    abs_diff = fabsf(diff);
  }

  // Q、R随温度差值自动调整，为函数映射关系。
  float alpha = 0.0f;
  if (abs_diff > KAF_ADAPT_START)
  {
    alpha = (abs_diff - KAF_ADAPT_START) / (KAF_DIFF_FULL - KAF_ADAPT_START);
    if (alpha > 1.0f)
    {
      alpha = 1.0f;
    }
    alpha *= alpha;
    Kal->Q = KAF_Q_MIN + (KAF_Q_MAX - KAF_Q_MIN) * alpha;
    Kal->R = KAF_R_MAX - (KAF_R_MAX - KAF_R_MIN) * alpha;
  }

  // k时刻估计误差协方差 = k-1时刻的估计误差协方差 + 过程噪声协方差
  Kal->Now_P = Kal->Last_P + Kal->Q; // 估计误差随时间流逝而增加
  // 卡尔曼增益 = 估计误差/(估计误差+测量噪声) k时刻
  Kal->K = Kal->Now_P / (Kal->Now_P + Kal->R);
  // 当前估计值，测量值与估计值的加权
  // k时刻状态变量最优值 = 预测值 + 卡尔曼增益*(测量值-状态变量)
  Kal->out = Kal->out + Kal->K * (in - Kal->out);
  // 更新下一次的估计误差，调整对测量值的相信程度
  Kal->Last_P = (1.f - Kal->K) * Kal->Now_P; // 修正估计值后，根据卡尔曼增益，降低认为的误差。
  return Kal->out;
}
void Kalman_init(KaF *kf, float init_p, float init_out, float init_q, float init_r)
{
  kf->Last_P = init_p;
  kf->Now_P = 0.0f;
  kf->out = init_out;
  kf->K = 0.0f;
  kf->Q = init_q;
  kf->R = init_r;
}
void temp_filter_init(float init_value)
{
  Kalman_init(&temp_filter_setup, KAF_INIT_P, init_value, KAF_INIT_Q, KAF_INIT_R);
}
float Get_Init_Temperature(void)
{
  return test_temp[0];
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();
  MX_GPIO_Init();
  MX_LPUART1_UART_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);

  init_temp = Get_Init_Temperature();
  temp_filter_init(init_temp);
  /* USER CODE END 2 */
 while (1)
{
  int i;
  char msg[100];

  // 每次重新输出一轮数据前，重新初始化滤波器
  temp_filter_init(test_temp[0]);

  HAL_UART_Transmit(&hlpuart1,
                    (uint8_t *)"Index,Raw,Kalman\r\n",
                    strlen("Index,Raw,Kalman\r\n"),
                    100);

  for (i = 0; i < TEST_NUM; i++)
  {
    float raw = test_temp[i];
    float temperature = Kalman_out(&temp_filter_setup, raw, KAF_MAX_INC);

    snprintf(msg, sizeof(msg),
             "%d,%.2f,%.2f\r\n",
             i,
             raw,
             temperature);

    HAL_UART_Transmit(&hlpuart1, (uint8_t *)msg, strlen(msg), 100);
    HAL_Delay(100);
  }

  while (1)
  {
    HAL_Delay(1000);
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