/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file
  ******************************************************************************
  */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_uart.h"
#include "stm32f1xx_hal_spi.h"
#include "config.h"
#include "uart.h"
#include "sd_card.h"
#include "hex_parser.h"
#include "led_control.h"
#include "swd_dap.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
