/**
  ******************************************************************************
  * @file           : sd_card.h
  * @brief          : Header for sd_card.c file - SD card module with basic FAT support
  ******************************************************************************
  */

#ifndef __SD_CARD_H
#define __SD_CARD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  File object structure (simplified FatFS-style)
  */
typedef struct {
    uint32_t fsize;          /* File size in bytes */
    uint32_t fptr;           /* Current read/write pointer */
    uint32_t start_cluster;  /* Start cluster of the file */
    uint32_t current_sector; /* Current sector being accessed */
    uint8_t  flag;           /* File status flags */
} FIL;

/**
  * @brief  File system result codes
  */
typedef enum {
    FR_OK = 0,              /* Success */
    FR_DISK_ERR,            /* Disk error */
    FR_NOT_READY,           /* Not ready */
    FR_NO_FILE,             /* File not found */
    FR_INVALID_NAME,        /* Invalid file name */
    FR_INVALID_OBJECT,      /* Invalid object */
    FR_INVALID_DRIVE,       /* Invalid drive */
} FRESULT;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initialize SD card hardware and SPI interface
  * @retval 0 if success, -1 if error
  */
int SD_Init(void);

/**
  * @brief  Mount the SD card file system
  * @retval 0 if success, -1 if error
  */
int SD_Mount(void);

/**
  * @brief  Open a file on the SD card
  * @param  filepath: Path to the file
  * @param  file: Pointer to file object
  * @retval 0 if success, -1 if error
  */
int SD_OpenFile(const char* filepath, FIL* file);

/**
  * @brief  Close an opened file
  * @param  file: Pointer to file object
  * @retval None
  */
void SD_CloseFile(FIL* file);

/**
  * @brief  Read data from file in sector-sized chunks
  * @param  file: Pointer to file object
  * @param  buffer: Buffer to store read data
  * @param  sector_size: Size of sector to read (typically 512 bytes)
  * @param  bytes_read: Pointer to variable to store actual bytes read
  * @retval 0 if success, -1 if error
  */
int SD_ReadSector(FIL* file, uint8_t* buffer, uint32_t sector_size, uint32_t* bytes_read);

/**
  * @brief  Read a single sector from SD card
  * @param  sector: Sector number to read
  * @param  buffer: Buffer to store sector data (512 bytes)
  * @retval 0 if success, -1 if error
  */
int SD_ReadRawSector(uint32_t sector, uint8_t* buffer);

/**
  * @brief  Rewind file to beginning (reset file pointer)
  * @param  file: Pointer to file object
  * @retval None
  */
void SD_Rewind(FIL* file);

#ifdef __cplusplus
}
#endif

#endif /* __SD_CARD_H */
