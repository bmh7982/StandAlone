/**
  ******************************************************************************
  * @file           : led_control.h
  * @brief          : Header for led_control.c file - LED status control module
  ******************************************************************************
  */

#ifndef __LED_CONTROL_H
#define __LED_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  LED identifier
  */
typedef enum {
    LED_1 = 1,  /* Progress LED (PB12) */
    LED_2 = 2   /* Result LED (PB13) */
} LED_ID_t;

/**
  * @brief  LED state patterns
  */
typedef enum {
    LED_STATE_IDLE = 0,     /* All LEDs off */
    LED_STATE_PROGRESS,     /* LED1 blinking (100ms), LED2 off */
    LED_STATE_SUCCESS,      /* LED1 off, LED2 on */
    LED_STATE_ERROR         /* LED1 off, LED2 fast blink (200ms) */
} LED_State_t;

/* Exported constants --------------------------------------------------------*/
#define LED_BLINK_SLOW      100   /* Slow blink period in ms */
#define LED_BLINK_FAST      200   /* Fast blink period in ms */

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initialize LED control module
  * @retval None
  */
void LED_Control_Init(void);

/**
  * @brief  Turn on specified LED
  * @param  led_num: LED identifier (LED_1 or LED_2)
  * @retval None
  */
void LED_On(uint8_t led_num);

/**
  * @brief  Turn off specified LED
  * @param  led_num: LED identifier (LED_1 or LED_2)
  * @retval None
  */
void LED_Off(uint8_t led_num);

/**
  * @brief  Toggle specified LED
  * @param  led_num: LED identifier (LED_1 or LED_2)
  * @retval None
  */
void LED_Toggle(uint8_t led_num);

/**
  * @brief  Set LED state to Progress (LED1 blinking, LED2 off)
  * @retval None
  */
void LED_Progress(void);

/**
  * @brief  Set LED state to Success (LED1 off, LED2 on)
  * @retval None
  */
void LED_Success(void);

/**
  * @brief  Set LED state to Error (LED1 off, LED2 fast blink)
  * @retval None
  */
void LED_Error(void);

/**
  * @brief  Set LED state to Idle (all LEDs off)
  * @retval None
  */
void LED_Idle(void);

/**
  * @brief  Update LED states (non-blocking)
  * @note   This function should be called periodically from main loop
  *         or from a timer interrupt
  * @retval None
  */
void LED_Update(void);

/**
  * @brief  SysTick callback for LED timing
  * @note   Called from SysTick_Handler every 1ms
  * @retval None
  */
void LED_SysTick_Callback(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_CONTROL_H */
