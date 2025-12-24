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
int Program_Target(const char* filename);

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

  /* Initialize SWD interface */
  UART_SendString("\r\nInitializing SWD interface...\r\n");
  SWD_Init();
  UART_SendString("SWD Init OK\r\n");

  /* Test SWD connection */
  UART_SendString("Testing SWD connection...\r\n");
  LED_Progress();

  if (Target_Connect() == 0) {
    uint32_t idcode = 0;
    if (Target_Detect(&idcode) == 0) {
      char msg[64];
      sprintf(msg, "Target detected! IDCODE: 0x%08lX\r\n", idcode);
      UART_SendString(msg);

      /* Identify MCU type */
      MCU_Type_t mcu_type = Target_IdentifyMCU(idcode);
      switch (mcu_type) {
        case MCU_TYPE_CORTEX_M0:
          UART_SendString("MCU Type: Cortex-M0\r\n");
          break;
        case MCU_TYPE_CORTEX_M3:
          UART_SendString("MCU Type: Cortex-M3\r\n");
          break;
        case MCU_TYPE_CORTEX_M4:
          UART_SendString("MCU Type: Cortex-M4\r\n");
          break;
        default:
          UART_SendString("MCU Type: Unknown\r\n");
          break;
      }

      LED_Success();
      HAL_Delay(1000);
      LED_Idle();
    } else {
      UART_SendString("Failed to detect target!\r\n");
      LED_Error();
      HAL_Delay(2000);
      LED_Idle();
    }
  } else {
    UART_SendString("SWD connection failed!\r\n");
    LED_Error();
    HAL_Delay(2000);
    LED_Idle();
  }

  UART_SendString("\r\nCMSIS-DAP Programmer Ready\r\n");
  LED_Idle();

  /* Command buffer */
  char cmd_buffer[MAX_FILENAME_LEN];
  char filename[MAX_FILENAME_LEN];

  /* Infinite loop */
  while (1)
  {
    /* Wait for UART command with 60 second timeout */
    if (UART_ReceiveCommand(cmd_buffer, MAX_FILENAME_LEN, 60000))
    {
      /* Extract filename from command */
      if (UART_ExtractFilePath(cmd_buffer, filename, MAX_FILENAME_LEN))
      {
        /* Show progress LED */
        LED_Progress();

        /* Program target MCU */
        int result = Program_Target(filename);

        if (result == 0)
        {
          /* Programming successful */
          UART_SendResponse(RESP_OK);
          LED_Success();
        }
        else
        {
          /* Programming failed - error already reported */
          LED_Error();
        }

        /* Return to idle state after delay */
        HAL_Delay(2000);
        LED_Idle();
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
  * @brief  Main programming function - programs target MCU from HEX file
  * @param  filename: Path to HEX file on SD card
  * @retval 0 if success, negative if error
  */
int Program_Target(const char* filename)
{
  FIL file;
  int result;
  char msg[64];

  /* 1. Open HEX file from SD card */
  UART_SendString("Opening file: ");
  UART_SendString(filename);
  UART_SendString("\r\n");

  if (SD_OpenFile(filename, &file) != 0)
  {
    UART_SendString("ERROR: File not found!\r\n");
    UART_SendResponse(RESP_ERR_FILE_NOT_FOUND);
    return -1;
  }

  sprintf(msg, "File opened, size: %lu bytes\r\n", file.fsize);
  UART_SendString(msg);

  /* 2. Connect to target via SWD */
  UART_SendString("Connecting to target...\r\n");
  if (Target_Connect() != 0)
  {
    UART_SendString("ERROR: SWD connection failed!\r\n");
    UART_SendResponse(RESP_ERR_TARGET_CONNECT);
    SD_CloseFile(&file);
    return -2;
  }

  /* 3. Detect target MCU */
  uint32_t idcode;
  if (Target_Detect(&idcode) != 0)
  {
    UART_SendString("ERROR: Target detection failed!\r\n");
    UART_SendResponse(RESP_ERR_TARGET_CONNECT);
    SD_CloseFile(&file);
    return -3;
  }

  MCU_Type_t mcu_type = Target_IdentifyMCU(idcode);
  sprintf(msg, "Target detected! IDCODE: 0x%08lX\r\n", idcode);
  UART_SendString(msg);

  switch (mcu_type)
  {
    case MCU_TYPE_CORTEX_M0:
      UART_SendString("MCU Type: Cortex-M0\r\n");
      break;
    case MCU_TYPE_CORTEX_M3:
      UART_SendString("MCU Type: Cortex-M3\r\n");
      break;
    case MCU_TYPE_CORTEX_M4:
      UART_SendString("MCU Type: Cortex-M4\r\n");
      break;
    default:
      UART_SendString("MCU Type: Unknown\r\n");
      break;
  }

  /* 4. Unlock and erase flash */
  UART_SendString("Unlocking flash...\r\n");
  if (Flash_Unlock() != 0)
  {
    UART_SendString("ERROR: Flash unlock failed!\r\n");
    UART_SendResponse(RESP_ERR_PROGRAM_FAIL);
    SD_CloseFile(&file);
    return -4;
  }

  UART_SendString("Erasing flash...\r\n");
  if (Flash_EraseFull() != 0)
  {
    UART_SendString("ERROR: Flash erase failed!\r\n");
    UART_SendResponse(RESP_ERR_PROGRAM_FAIL);
    Flash_Lock();
    SD_CloseFile(&file);
    return -5;
  }

  /* 5. Program flash from HEX file */
  UART_SendString("Programming flash...\r\n");
  result = HEX_ProcessFile(&file, Flash_Program);
  if (result != 0)
  {
    UART_SendString("ERROR: Programming failed!\r\n");
    UART_SendResponse(RESP_ERR_PROGRAM_FAIL);
    Flash_Lock();
    SD_CloseFile(&file);
    return -6;
  }

  UART_SendString("Programming completed!\r\n");

  /* 6. Verify flash - rewind file and verify */
  UART_SendString("Verifying flash...\r\n");
  SD_Rewind(&file);

  result = HEX_ProcessFile(&file, Flash_Verify);
  if (result != 0)
  {
    UART_SendString("ERROR: Verification failed!\r\n");
    UART_SendResponse(RESP_ERR_VERIFY_FAIL);
    Flash_Lock();
    SD_CloseFile(&file);
    return -7;
  }

  UART_SendString("Verification passed!\r\n");

  /* 7. Lock flash and reset target */
  Flash_Lock();
  SD_CloseFile(&file);

  UART_SendString("Resetting target...\r\n");
  Target_Reset();

  UART_SendString("Programming complete!\r\n");

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
