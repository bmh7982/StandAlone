/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */

#include "main.h"
#include <stdio.h>

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart3;
extern SPI_HandleTypeDef hspi1;

/* Private function prototypes -----------------------------------------------*/
void System_Clock_Config(void);
void GPIO_Init(void);
void UART_Init(void);
void LED_Init(void);
void SPI_Init(void);
int Test_ProgramCallback(uint32_t addr, uint8_t* data, uint32_t size);

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
  LED_Control_Init();
  SPI_Init();

  /* Send READY message via UART */
  const char ready_msg[] = "READY\r\n";
  HAL_UART_Transmit(&huart3, (uint8_t*)ready_msg, sizeof(ready_msg)-1, HAL_MAX_DELAY);

  /* LED Test: Show progress during initialization */
  LED_Progress();
  UART_SendString("LED Test: Progress pattern\r\n");
  HAL_Delay(2000);

  /* Initialize SD card */
  UART_SendString("Initializing SD card...\r\n");
  if (SD_Init() != 0) {
    UART_SendString("SD Init failed!\r\n");
    LED_Error();
    HAL_Delay(2000);
  } else {
    UART_SendString("SD Init OK\r\n");

    /* Mount file system */
    if (SD_Mount() != 0) {
      UART_SendString("SD Mount failed!\r\n");
      LED_Error();
      HAL_Delay(2000);
    } else {
      UART_SendString("SD Mount OK\r\n");
      LED_Success();
      UART_SendString("LED Test: Success pattern\r\n");
      HAL_Delay(2000);
      LED_Idle();
    }
  }

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
        /* File path extracted successfully */
        UART_SendString("Opening file: ");
        UART_SendString(filepath);
        UART_SendString("\r\n");

        /* Show progress LED */
        LED_Progress();

        /* Try to open file from SD card */
        FIL file;
        if (SD_OpenFile(filepath, &file) != 0)
        {
          UART_SendString("File not found!\r\n");
          LED_Error();
          UART_SendResponse(RESP_ERR_FILE_NOT_FOUND);
        }
        else
        {
          /* File opened successfully */
          UART_SendString("File opened, size: ");
          char size_str[16];
          sprintf(size_str, "%lu bytes\r\n", file.fsize);
          UART_SendString(size_str);

          /* Parse HEX file */
          UART_SendString("Parsing HEX file...\r\n");

          if (HEX_ProcessFile(&file, Test_ProgramCallback) == 0)
          {
            UART_SendString("HEX parsing completed!\r\n");
            LED_Success();
            UART_SendResponse(RESP_OK);
          }
          else
          {
            UART_SendString("HEX parsing failed!\r\n");
            LED_Error();
            UART_SendResponse(RESP_ERR_HEX_PARSE);
          }

          SD_CloseFile(&file);

          /* TODO: In next steps, implement:
           * 1. Connect to target MCU via SWD
           * 2. Program target MCU with parsed data
           * 3. Verify programming
           */
        }
      }
      else
      {
        /* Invalid command format */
        UART_SendResponse(RESP_NG);
      }
    }

    /* Update LED states (non-blocking) */
    LED_Update();

    /* Small delay to prevent tight loop */
    HAL_Delay(10);
  }
}

/**
  * @brief  Test callback function for HEX file processing
  * @param  addr: Base address of data
  * @param  data: Pointer to data buffer
  * @param  size: Size of data in bytes
  * @retval 0 if success, -1 if error
  */
int Test_ProgramCallback(uint32_t addr, uint8_t* data, uint32_t size)
{
  char msg[64];

  /* Display sector information */
  sprintf(msg, "Sector @ 0x%08lX, size: %lu bytes\r\n", addr, size);
  UART_SendString(msg);

  /* Display first 32 bytes of each sector */
  UART_SendString("First 32 bytes: ");
  for (uint16_t i = 0; i < 32 && i < size; i++)
  {
    char hex_str[4];
    sprintf(hex_str, "%02X ", data[i]);
    UART_SendString(hex_str);
  }
  UART_SendString("\r\n");

  /* In actual implementation, this callback would:
   * 1. Write data to target MCU flash via SWD
   * 2. Verify the write was successful
   * 3. Return 0 on success, -1 on error
   */

  return 0;  /* Success */
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
