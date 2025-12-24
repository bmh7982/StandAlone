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
  SPI_Init();

  /* Send READY message via UART */
  const char ready_msg[] = "READY\r\n";
  HAL_UART_Transmit(&huart3, (uint8_t*)ready_msg, sizeof(ready_msg)-1, HAL_MAX_DELAY);

  /* Initialize SD card */
  UART_SendString("Initializing SD card...\r\n");
  if (SD_Init() != 0) {
    UART_SendString("SD Init failed!\r\n");
  } else {
    UART_SendString("SD Init OK\r\n");

    /* Mount file system */
    if (SD_Mount() != 0) {
      UART_SendString("SD Mount failed!\r\n");
    } else {
      UART_SendString("SD Mount OK\r\n");
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

        /* Try to open file from SD card */
        FIL file;
        if (SD_OpenFile(filepath, &file) != 0)
        {
          UART_SendString("File not found!\r\n");
          UART_SendResponse(RESP_ERR_FILE_NOT_FOUND);
        }
        else
        {
          /* File opened successfully */
          UART_SendString("File opened, size: ");
          char size_str[16];
          sprintf(size_str, "%lu bytes\r\n", file.fsize);
          UART_SendString(size_str);

          /* Read first sector as test */
          uint8_t sector_buffer[SECTOR_SIZE];
          uint32_t bytes_read = 0;

          if (SD_ReadSector(&file, sector_buffer, SECTOR_SIZE, &bytes_read) == 0)
          {
            UART_SendString("First sector read: ");
            char bytes_str[16];
            sprintf(bytes_str, "%lu bytes\r\n", bytes_read);
            UART_SendString(bytes_str);

            /* Display first 64 bytes in hex */
            UART_SendString("First 64 bytes:\r\n");
            for (uint16_t i = 0; i < 64 && i < bytes_read; i++)
            {
              char hex_str[4];
              sprintf(hex_str, "%02X ", sector_buffer[i]);
              UART_SendString(hex_str);
              if ((i + 1) % 16 == 0)
                UART_SendString("\r\n");
            }
            UART_SendString("\r\n");
          }
          else
          {
            UART_SendString("Failed to read sector!\r\n");
          }

          SD_CloseFile(&file);

          /* TODO: In next steps, implement:
           * 1. Parse HEX file
           * 2. Program target MCU
           * 3. Verify programming
           */

          /* For now, send OK response */
          UART_SendResponse(RESP_OK);
        }
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
