/**
  ******************************************************************************
  * @file           : swd_dap.h
  * @brief          : Header for swd_dap.c file - CMSIS-DAP SWD protocol
  ******************************************************************************
  */

#ifndef __SWD_DAP_H
#define __SWD_DAP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  MCU type identification
  */
typedef enum {
    MCU_TYPE_UNKNOWN = 0,
    MCU_TYPE_CORTEX_M0,   /* Cortex-M0 (IDCODE: 0x0BB11477) */
    MCU_TYPE_CORTEX_M3,   /* Cortex-M3 (IDCODE: 0x4BA00477) */
    MCU_TYPE_CORTEX_M4    /* Cortex-M4 (IDCODE: 0x4BA01477) */
} MCU_Type_t;

/* Exported constants --------------------------------------------------------*/

/* Debug Port (DP) Registers */
#define DP_IDCODE       0x00  /* IDCODE register */
#define DP_ABORT        0x00  /* Abort register (write) */
#define DP_CTRL_STAT    0x04  /* Control/Status register */
#define DP_SELECT       0x08  /* Select register */
#define DP_RDBUFF       0x0C  /* Read buffer */

/* Access Port (AP) Registers */
#define AP_CSW          0x00  /* Control/Status Word */
#define AP_TAR          0x04  /* Transfer Address */
#define AP_DRW          0x0C  /* Data Read/Write */
#define AP_IDR          0xFC  /* Identification Register */

/* SWD ACK responses */
#define SWD_ACK_OK      0x01
#define SWD_ACK_WAIT    0x02
#define SWD_ACK_FAULT   0x04
#define SWD_ACK_ERROR   0x07

/* Known IDCODE values */
#define IDCODE_CORTEX_M0   0x0BB11477
#define IDCODE_CORTEX_M3   0x4BA00477
#define IDCODE_CORTEX_M4   0x4BA01477

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initialize SWD interface
  * @retval None
  */
void SWD_Init(void);

/**
  * @brief  Configure SWD GPIO pins
  * @retval None
  */
void SWD_GPIO_Config(void);

/**
  * @brief  Write a single bit to SWD
  * @param  bit: Bit value (0 or 1)
  * @retval None
  */
void SWD_WriteBit(uint8_t bit);

/**
  * @brief  Read a single bit from SWD
  * @retval Bit value (0 or 1)
  */
uint8_t SWD_ReadBit(void);

/**
  * @brief  Write a byte to SWD (LSB first)
  * @param  data: Byte to write
  * @retval None
  */
void SWD_WriteByte(uint8_t data);

/**
  * @brief  Read a byte from SWD (LSB first)
  * @retval Byte value
  */
uint8_t SWD_ReadByte(void);

/**
  * @brief  Perform SWD line reset sequence
  * @retval 0 if success, -1 if error
  */
int SWD_LineReset(void);

/**
  * @brief  Read Debug Port register
  * @param  addr: Register address (DP_IDCODE, DP_CTRL_STAT, etc.)
  * @param  data: Pointer to store read data
  * @retval 0 if success, -1 if error
  */
int SWD_ReadDP(uint8_t addr, uint32_t* data);

/**
  * @brief  Write Debug Port register
  * @param  addr: Register address
  * @param  data: Data to write
  * @retval 0 if success, -1 if error
  */
int SWD_WriteDP(uint8_t addr, uint32_t data);

/**
  * @brief  Read Access Port register
  * @param  addr: Register address (AP_CSW, AP_TAR, AP_DRW, etc.)
  * @param  data: Pointer to store read data
  * @retval 0 if success, -1 if error
  */
int SWD_ReadAP(uint8_t addr, uint32_t* data);

/**
  * @brief  Write Access Port register
  * @param  addr: Register address
  * @param  data: Data to write
  * @retval 0 if success, -1 if error
  */
int SWD_WriteAP(uint8_t addr, uint32_t data);

/**
  * @brief  Connect to target MCU via SWD
  * @retval 0 if success, -1 if error
  */
int Target_Connect(void);

/**
  * @brief  Detect target MCU and read IDCODE
  * @param  idcode: Pointer to store IDCODE
  * @retval 0 if success, -1 if error
  */
int Target_Detect(uint32_t* idcode);

/**
  * @brief  Identify MCU type from IDCODE
  * @param  idcode: IDCODE value
  * @retval MCU type
  */
MCU_Type_t Target_IdentifyMCU(uint32_t idcode);

/**
  * @brief  Halt target core
  * @retval 0 if success, -1 if error
  */
int Target_HaltCore(void);

/**
  * @brief  Reset target MCU
  * @retval 0 if success, -1 if error
  */
int Target_Reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __SWD_DAP_H */
