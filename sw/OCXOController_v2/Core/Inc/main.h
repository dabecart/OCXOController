/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "MainMCU.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TEST_LED_Pin GPIO_PIN_13
#define TEST_LED_GPIO_Port GPIOC
#define OCXO_DIVIDED_Pin GPIO_PIN_1
#define OCXO_DIVIDED_GPIO_Port GPIOA
#define TFT_A0_Pin GPIO_PIN_0
#define TFT_A0_GPIO_Port GPIOB
#define TFT_RESET_Pin GPIO_PIN_1
#define TFT_RESET_GPIO_Port GPIOB
#define TFT_CS_Pin GPIO_PIN_2
#define TFT_CS_GPIO_Port GPIOB
#define USB_ALERT_Pin GPIO_PIN_11
#define USB_ALERT_GPIO_Port GPIOB
#define PG_1V8_Pin GPIO_PIN_12
#define PG_1V8_GPIO_Port GPIOB
#define PG_3V3_Pin GPIO_PIN_13
#define PG_3V3_GPIO_Port GPIOB
#define CH_OUT3_Pin GPIO_PIN_5
#define CH_OUT3_GPIO_Port GPIOB
#define CH_OUT2_Pin GPIO_PIN_6
#define CH_OUT2_GPIO_Port GPIOB
#define CH_OUT1_Pin GPIO_PIN_7
#define CH_OUT1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
