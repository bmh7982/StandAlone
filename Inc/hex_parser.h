/**
  ******************************************************************************
  * @file           : hex_parser.h
  * @brief          : Header for hex_parser.c file - Intel HEX format parser
  ******************************************************************************
  */

#ifndef __HEX_PARSER_H
#define __HEX_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "config.h"
#include "sd_card.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  Intel HEX record structure
  */
typedef struct {
    uint8_t record_type;        /* Record type (00, 01, 04, 05) */
    uint16_t address;           /* 16-bit address field */
    uint8_t data_len;           /* Data length in bytes */
    uint8_t data[256];          /* Data bytes (max 255) */
    uint32_t extended_address;  /* Extended linear address (upper 16 bits) */
} HEX_Record_t;

/**
  * @brief  Program sector buffer structure
  */
typedef struct {
    uint32_t base_address;      /* Base address of this sector */
    uint8_t data[SECTOR_SIZE];  /* Sector data buffer (512 bytes) */
    uint16_t size;              /* Valid data size in buffer */
} Program_Sector_t;

/**
  * @brief  Intel HEX record types
  */
#define HEX_RECORD_DATA              0x00  /* Data record */
#define HEX_RECORD_EOF               0x01  /* End of file */
#define HEX_RECORD_EXT_SEG_ADDR      0x02  /* Extended segment address (not used) */
#define HEX_RECORD_START_SEG_ADDR    0x03  /* Start segment address (not used) */
#define HEX_RECORD_EXT_LINEAR_ADDR   0x04  /* Extended linear address */
#define HEX_RECORD_START_LINEAR_ADDR 0x05  /* Start linear address */

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Parse a single Intel HEX line
  * @param  line: Null-terminated HEX line string
  * @param  record: Pointer to HEX_Record_t structure to store parsed data
  * @retval 0 if success, -1 if error
  */
int HEX_ParseLine(const char* line, HEX_Record_t* record);

/**
  * @brief  Verify checksum of Intel HEX line
  * @param  line: Null-terminated HEX line string
  * @retval 0 if checksum valid, -1 if invalid
  */
int HEX_VerifyChecksum(const char* line);

/**
  * @brief  Process HEX record into sector buffer
  * @param  record: Pointer to parsed HEX record
  * @param  sector: Pointer to program sector buffer
  * @retval 0 if success, -1 if error
  */
int HEX_ProcessRecord(HEX_Record_t* record, Program_Sector_t* sector);

/**
  * @brief  Process entire HEX file with callback
  * @param  file: Pointer to opened file object
  * @param  program_callback: Callback function called for each sector
  *         Parameters: (address, data, size)
  *         Returns: 0 for success, -1 for error
  * @retval 0 if success, -1 if error
  */
int HEX_ProcessFile(FIL* file,
                    int (*program_callback)(uint32_t addr, uint8_t* data, uint32_t size));

#ifdef __cplusplus
}
#endif

#endif /* __HEX_PARSER_H */
