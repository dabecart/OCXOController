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
#define OCXO_PPS_H_Pin GPIO_PIN_2
#define OCXO_PPS_H_GPIO_Port GPIOA
#define OCXO_PPS_L_Pin GPIO_PIN_3
#define OCXO_PPS_L_GPIO_Port GPIOA
#define OCXO_VCO_Pin GPIO_PIN_4
#define OCXO_VCO_GPIO_Port GPIOA
#define OCXO_OUT_Pin GPIO_PIN_4
#define OCXO_OUT_GPIO_Port GPIOC
#define PPS_OUT_Pin GPIO_PIN_8
#define PPS_OUT_GPIO_Port GPIOA
#define PPS_REF_H_Pin GPIO_PIN_15
#define PPS_REF_H_GPIO_Port GPIOA
#define PPS_REF_L_Pin GPIO_PIN_3
#define PPS_REF_L_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
