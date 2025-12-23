/**
  ******************************************************************************
  * @file           : config.h
  * @brief          : Configuration macros for CMSIS-DAP programmer
  ******************************************************************************
  */

#ifndef __CONFIG_H
#define __CONFIG_H

/* UART Configuration */
#define UART_BAUDRATE           9600

/* File System Configuration */
#define SECTOR_SIZE             512
#define MAX_FILENAME_LEN        128

/* LED Pin Definitions */
#define LED1_PIN                GPIO_PIN_12
#define LED1_PORT               GPIOB
#define LED2_PIN                GPIO_PIN_13
#define LED2_PORT               GPIOB

/* SWD Pin Definitions */
#define SWDIO_PIN               GPIO_PIN_2
#define SWDIO_PORT              GPIOA
#define SWCLK_PIN               GPIO_PIN_4
#define SWCLK_PORT              GPIOA
#define SWRST_PIN               GPIO_PIN_6
#define SWRST_PORT              GPIOA

/* UART Pin Definitions (USART3) */
#define UART_TX_PIN             GPIO_PIN_10
#define UART_TX_PORT            GPIOB
#define UART_RX_PIN             GPIO_PIN_11
#define UART_RX_PORT            GPIOB

#endif /* __CONFIG_H */
