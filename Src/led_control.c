/**
  ******************************************************************************
  * @file           : led_control.c
  * @brief          : LED status control module implementation
  ******************************************************************************
  * @description
  * This module provides non-blocking LED control with various patterns:
  * - Progress: LED1 blinks slowly (100ms period)
  * - Success: LED2 stays on
  * - Error: LED2 blinks fast (200ms period)
  * - Idle: All LEDs off
  *
  * Uses SysTick timer (1ms interrupt) for precise timing control.
  ******************************************************************************
  */

#include "led_control.h"
#include "main.h"

/* Private defines -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static LED_State_t current_state = LED_STATE_IDLE;
static uint32_t systick_counter = 0;       /* 1ms tick counter */
static uint8_t led1_blink_state = 0;       /* LED1 blink state */
static uint8_t led2_blink_state = 0;       /* LED2 blink state */

/* Private function prototypes -----------------------------------------------*/

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  Initialize LED control module
  * @retval None
  */
void LED_Control_Init(void)
{
    /* LEDs are already initialized in GPIO_Init() */
    /* Set initial state to idle */
    current_state = LED_STATE_IDLE;
    systick_counter = 0;
    led1_blink_state = 0;
    led2_blink_state = 0;

    /* Turn off both LEDs */
    LED_Off(LED_1);
    LED_Off(LED_2);
}

/**
  * @brief  Turn on specified LED
  * @param  led_num: LED identifier (LED_1 or LED_2)
  * @retval None
  */
void LED_On(uint8_t led_num)
{
    if (led_num == LED_1) {
        HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_SET);
    } else if (led_num == LED_2) {
        HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, GPIO_PIN_SET);
    }
}

/**
  * @brief  Turn off specified LED
  * @param  led_num: LED identifier (LED_1 or LED_2)
  * @retval None
  */
void LED_Off(uint8_t led_num)
{
    if (led_num == LED_1) {
        HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET);
    } else if (led_num == LED_2) {
        HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, GPIO_PIN_RESET);
    }
}

/**
  * @brief  Toggle specified LED
  * @param  led_num: LED identifier (LED_1 or LED_2)
  * @retval None
  */
void LED_Toggle(uint8_t led_num)
{
    if (led_num == LED_1) {
        HAL_GPIO_TogglePin(LED1_PORT, LED1_PIN);
    } else if (led_num == LED_2) {
        HAL_GPIO_TogglePin(LED2_PORT, LED2_PIN);
    }
}

/**
  * @brief  Set LED state to Progress (LED1 blinking, LED2 off)
  * @retval None
  */
void LED_Progress(void)
{
    current_state = LED_STATE_PROGRESS;
    systick_counter = 0;
    led1_blink_state = 0;
    led2_blink_state = 0;

    /* Turn off LED2 */
    LED_Off(LED_2);
}

/**
  * @brief  Set LED state to Success (LED1 off, LED2 on)
  * @retval None
  */
void LED_Success(void)
{
    current_state = LED_STATE_SUCCESS;

    /* Turn off LED1, turn on LED2 */
    LED_Off(LED_1);
    LED_On(LED_2);
}

/**
  * @brief  Set LED state to Error (LED1 off, LED2 fast blink)
  * @retval None
  */
void LED_Error(void)
{
    current_state = LED_STATE_ERROR;
    systick_counter = 0;
    led1_blink_state = 0;
    led2_blink_state = 0;

    /* Turn off LED1 */
    LED_Off(LED_1);
}

/**
  * @brief  Set LED state to Idle (all LEDs off)
  * @retval None
  */
void LED_Idle(void)
{
    current_state = LED_STATE_IDLE;

    /* Turn off both LEDs */
    LED_Off(LED_1);
    LED_Off(LED_2);
}

/**
  * @brief  Update LED states (non-blocking)
  * @note   This function should be called periodically from main loop
  * @retval None
  */
void LED_Update(void)
{
    switch (current_state) {
        case LED_STATE_IDLE:
            /* Nothing to do - LEDs already off */
            break;

        case LED_STATE_PROGRESS:
            /* LED1 blinks with 100ms period (50ms on, 50ms off) */
            if (systick_counter >= LED_BLINK_SLOW) {
                systick_counter = 0;
                led1_blink_state = !led1_blink_state;

                if (led1_blink_state)
                    LED_On(LED_1);
                else
                    LED_Off(LED_1);
            }
            break;

        case LED_STATE_SUCCESS:
            /* LED2 stays on - nothing to update */
            break;

        case LED_STATE_ERROR:
            /* LED2 blinks with 200ms period (100ms on, 100ms off) */
            if (systick_counter >= LED_BLINK_FAST) {
                systick_counter = 0;
                led2_blink_state = !led2_blink_state;

                if (led2_blink_state)
                    LED_On(LED_2);
                else
                    LED_Off(LED_2);
            }
            break;

        default:
            break;
    }
}

/**
  * @brief  SysTick callback for LED timing
  * @note   Called from SysTick_Handler every 1ms
  * @retval None
  */
void LED_SysTick_Callback(void)
{
    /* Increment 1ms counter */
    systick_counter++;
}
