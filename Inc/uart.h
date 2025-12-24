/**
  ******************************************************************************
  * @file           : uart.h
  * @brief          : Header for uart.c file - UART communication module
  ******************************************************************************
  */

#ifndef __UART_H
#define __UART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define UART_CMD_PREFIX "FILE: "
#define UART_CMD_PREFIX_LEN 6

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Send a string via UART
  * @param  str: Pointer to null-terminated string to send
  * @retval None
  */
void UART_SendString(const char* str);

/**
  * @brief  Receive a command from UART with timeout
  * @param  buffer: Buffer to store received data
  * @param  max_len: Maximum length of buffer
  * @param  timeout_ms: Timeout in milliseconds
  * @retval 1 if command received successfully, 0 if timeout or error
  */
int UART_ReceiveCommand(char* buffer, uint32_t max_len, uint32_t timeout_ms);

/**
  * @brief  Send a response code via UART
  * @param  code: Response code string (from config.h definitions)
  * @retval None
  */
void UART_SendResponse(const char* code);

/**
  * @brief  Extract file path from received command
  * @param  command: Full command string ("FILE: <path>\r\n")
  * @param  filepath: Buffer to store extracted file path
  * @param  max_len: Maximum length of filepath buffer
  * @retval 1 if extraction successful, 0 if invalid format
  */
int UART_ExtractFilePath(const char* command, char* filepath, uint32_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* __UART_H */
