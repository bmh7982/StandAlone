/**
  ******************************************************************************
  * @file           : uart.c
  * @brief          : UART communication module implementation
  ******************************************************************************
  */

#include "uart.h"
#include "main.h"
#include <string.h>

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart3;

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/**
  * @brief  Send a string via UART
  * @param  str: Pointer to null-terminated string to send
  * @retval None
  */
void UART_SendString(const char* str)
{
  if (str == NULL)
    return;

  uint32_t len = strlen(str);
  HAL_UART_Transmit(&huart3, (uint8_t*)str, len, HAL_MAX_DELAY);
}

/**
  * @brief  Receive a command from UART with timeout
  * @param  buffer: Buffer to store received data
  * @param  max_len: Maximum length of buffer
  * @param  timeout_ms: Timeout in milliseconds
  * @retval 1 if command received successfully, 0 if timeout or error
  */
int UART_ReceiveCommand(char* buffer, uint32_t max_len, uint32_t timeout_ms)
{
  if (buffer == NULL || max_len == 0)
    return 0;

  uint32_t start_tick = HAL_GetTick();
  uint32_t idx = 0;
  uint8_t rx_char;
  int found_cr = 0;

  /* Clear buffer */
  memset(buffer, 0, max_len);

  while (1)
  {
    /* Check timeout */
    if ((HAL_GetTick() - start_tick) > timeout_ms)
    {
      return 0;  /* Timeout */
    }

    /* Try to receive one character (with 10ms timeout per character) */
    if (HAL_UART_Receive(&huart3, &rx_char, 1, 10) == HAL_OK)
    {
      /* Reset timeout after receiving data */
      start_tick = HAL_GetTick();

      /* Check for \r\n terminator */
      if (rx_char == '\r')
      {
        found_cr = 1;
        continue;
      }
      else if (rx_char == '\n' && found_cr)
      {
        /* Command complete */
        buffer[idx] = '\0';
        return 1;
      }
      else
      {
        found_cr = 0;

        /* Store character if buffer has space */
        if (idx < max_len - 1)
        {
          buffer[idx++] = rx_char;
        }
        else
        {
          /* Buffer overflow */
          return 0;
        }
      }
    }
  }
}

/**
  * @brief  Send a response code via UART
  * @param  code: Response code string (from config.h definitions)
  * @retval None
  */
void UART_SendResponse(const char* code)
{
  UART_SendString(code);
}

/**
  * @brief  Extract file path from received command
  * @param  command: Full command string ("FILE: <path>\r\n")
  * @param  filepath: Buffer to store extracted file path
  * @param  max_len: Maximum length of filepath buffer
  * @retval 1 if extraction successful, 0 if invalid format
  */
int UART_ExtractFilePath(const char* command, char* filepath, uint32_t max_len)
{
  if (command == NULL || filepath == NULL || max_len == 0)
    return 0;

  /* Check if command starts with "FILE: " */
  if (strncmp(command, UART_CMD_PREFIX, UART_CMD_PREFIX_LEN) != 0)
    return 0;

  /* Extract path (skip "FILE: " prefix) */
  const char* path_start = command + UART_CMD_PREFIX_LEN;
  uint32_t path_len = strlen(path_start);

  /* Check buffer size */
  if (path_len >= max_len)
    return 0;

  /* Copy path to output buffer */
  strcpy(filepath, path_start);

  return 1;
}
