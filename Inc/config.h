/**
  ******************************************************************************
  * @file           : config.h
  * @brief          : Configuration macros for CMSIS-DAP programmer
  ******************************************************************************
  */

#ifndef __CONFIG_H
#define __CONFIG_H

#include "stm32f1xx_hal_conf.h"

// UART Pin Definitions (USART3)
#define UART_TX_PIN             GPIO_PIN_10
#define UART_TX_PORT            GPIOB
#define UART_RX_PIN             GPIO_PIN_11
#define UART_RX_PORT            GPIOB

/* SWD Pin Definitions */
#define SWDIO_PIN               GPIO_PIN_2
#define SWDIO_PORT              GPIOA
#define SWCLK_PIN               GPIO_PIN_4
#define SWCLK_PORT              GPIOA
#define SWRST_PIN               GPIO_PIN_6
#define SWRST_PORT              GPIOA

// LED Pin Definitions
#define LED1_PIN                GPIO_PIN_12
#define LED1_PORT               GPIOB
#define LED2_PIN                GPIO_PIN_13
#define LED2_PORT               GPIOB

// SD Card and File System
#define SECTOR_SIZE             512

// File System Configuration
#define MAX_FILENAME_LEN        128

// UART Configuration
#define UART_BAUDRATE           9600
#define UART_INSTANCE           USART3

// SPI Configuration for SD Card
#define SD_SPI_INSTANCE         SPI1
#define SD_SPI_SCK_PIN          GPIO_PIN_3
#define SD_SPI_SCK_PORT         GPIOB
#define SD_SPI_MISO_PIN         GPIO_PIN_4
#define SD_SPI_MISO_PORT        GPIOB
#define SD_SPI_MOSI_PIN         GPIO_PIN_5
#define SD_SPI_MOSI_PORT        GPIOB
#define SD_SPI_CS_PIN           GPIO_PIN_15
#define SD_SPI_CS_PORT          GPIOA

// SD Card SPI Speed
#define SD_SPI_SPEED_INIT       400000    // 400kHz for initialization
#define SD_SPI_SPEED_NORMAL     18000000  // 18MHz for normal operation

/* UART Response Codes */
#define RESP_OK                 "OK\r\n"
#define RESP_NG                 "NG\r\n"
#define RESP_ERR_SD_MOUNT       "ERR_SD_MOUNT\r\n"
#define RESP_ERR_FILE_NOT_FOUND "ERR_FILE_NOT_FOUND\r\n"
#define RESP_ERR_HEX_PARSE      "ERR_HEX_PARSE\r\n"
#define RESP_ERR_TARGET_CONNECT "ERR_TARGET_CONNECT\r\n"
#define RESP_ERR_PROGRAM_FAIL   "ERR_PROGRAM_FAIL\r\n"
#define RESP_ERR_VERIFY_FAIL    "ERR_VERIFY_FAIL\r\n"

#endif /* __CONFIG_H */
