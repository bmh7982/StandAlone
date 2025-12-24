/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */

#include "main.h"

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart3;

/* Private function prototypes -----------------------------------------------*/
void System_Clock_Config(void);
void GPIO_Init(void);
void UART_Init(void);
void LED_Init(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  System_Clock_Config();

  /* Initialize all configured peripherals */
  GPIO_Init();    // MX_GPIO_Init();
  UART_Init();
  LED_Init();

  /* Send READY message via UART */
  const char ready_msg[] = "READY\r\n";
  HAL_UART_Transmit(&huart3, (uint8_t*)ready_msg, sizeof(ready_msg)-1, HAL_MAX_DELAY);

  /* Command buffers */
  char cmd_buffer[MAX_FILENAME_LEN];
  char filepath[MAX_FILENAME_LEN];

  /* Infinite loop */
  while (1)
  {
    /* Wait for UART command with 60 second timeout */
    if (UART_ReceiveCommand(cmd_buffer, MAX_FILENAME_LEN, 60000))
    {
      /* Command received - try to extract file path */
      if (UART_ExtractFilePath(cmd_buffer, filepath, MAX_FILENAME_LEN))
      {
        /* File path extracted successfully - echo it back for testing */
        UART_SendString("Received file: ");
        UART_SendString(filepath);
        UART_SendString("\r\n");

        /* TODO: In next steps, implement:
         * 1. Open file from SD card
         * 2. Parse HEX file
         * 3. Program target MCU
         * 4. Send appropriate response code
         */

        /* For now, send OK response */
        UART_SendResponse(RESP_OK);
      }
      else
      {
        /* Invalid command format */
        UART_SendResponse(RESP_NG);
      }
    }

    /* Small delay to prevent tight loop */
    HAL_Delay(10);
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
