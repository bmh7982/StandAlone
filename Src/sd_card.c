/**
  ******************************************************************************
  * @file           : sd_card.c
  * @brief          : SD card module with basic FAT file system support
  ******************************************************************************
  */

#include "sd_card.h"
#include "main.h"
#include <string.h>

/* External variables --------------------------------------------------------*/
extern SPI_HandleTypeDef hspi1;

/* Private defines -----------------------------------------------------------*/
#define SD_CMD0     0    /* GO_IDLE_STATE */
#define SD_CMD8     8    /* SEND_IF_COND */
#define SD_CMD17    17   /* READ_SINGLE_BLOCK */
#define SD_CMD55    55   /* APP_CMD */
#define SD_ACMD41   41   /* SD_SEND_OP_COND */
#define SD_CMD58    58   /* READ_OCR */

#define SD_RESPONSE_TIMEOUT 1000
#define SD_INIT_TIMEOUT     2000

#define FAT_BOOT_SECTOR     0
#define FAT_DIR_ENTRY_SIZE  32
#define FAT_ATTR_DIRECTORY  0x10

/* Private variables ---------------------------------------------------------*/
static uint8_t sd_initialized = 0;
static uint32_t fat_start_sector = 0;
static uint32_t root_dir_sector = 0;
static uint32_t data_start_sector = 0;
static uint8_t sectors_per_cluster = 0;

/* Private function prototypes -----------------------------------------------*/
static void SD_CS_Low(void);
static void SD_CS_High(void);
static uint8_t SD_SPI_Transfer(uint8_t data);
static void SD_SPI_SendByte(uint8_t data);
static uint8_t SD_SPI_ReceiveByte(void);
static uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg);
static int SD_ReadResponse(void);
static int SD_WaitReady(void);
static int SD_ParseFAT(void);
static int SD_FindFile(const char* filename, uint32_t* start_cluster, uint32_t* file_size);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Assert CS line (low)
  */
static void SD_CS_Low(void)
{
    HAL_GPIO_WritePin(SD_SPI_CS_PORT, SD_SPI_CS_PIN, GPIO_PIN_RESET);
}

/**
  * @brief  Deassert CS line (high)
  */
static void SD_CS_High(void)
{
    HAL_GPIO_WritePin(SD_SPI_CS_PORT, SD_SPI_CS_PIN, GPIO_PIN_SET);
}

/**
  * @brief  Transfer one byte via SPI
  */
static uint8_t SD_SPI_Transfer(uint8_t data)
{
    uint8_t rx_data;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx_data, 1, HAL_MAX_DELAY);
    return rx_data;
}

/**
  * @brief  Send one byte via SPI
  */
static void SD_SPI_SendByte(uint8_t data)
{
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
}

/**
  * @brief  Receive one byte via SPI
  */
static uint8_t SD_SPI_ReceiveByte(void)
{
    return SD_SPI_Transfer(0xFF);
}

/**
  * @brief  Wait for SD card to be ready
  */
static int SD_WaitReady(void)
{
    uint32_t timeout = HAL_GetTick() + SD_RESPONSE_TIMEOUT;
    uint8_t response;

    do {
        response = SD_SPI_ReceiveByte();
        if (response == 0xFF)
            return 0;  /* Ready */
    } while (HAL_GetTick() < timeout);

    return -1;  /* Timeout */
}

/**
  * @brief  Send SD card command
  */
static uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg)
{
    uint8_t response;
    uint32_t timeout;

    /* Wait for card ready */
    SD_WaitReady();

    /* Send command packet */
    SD_SPI_SendByte(0x40 | cmd);              /* Command byte */
    SD_SPI_SendByte((uint8_t)(arg >> 24));    /* Argument[31..24] */
    SD_SPI_SendByte((uint8_t)(arg >> 16));    /* Argument[23..16] */
    SD_SPI_SendByte((uint8_t)(arg >> 8));     /* Argument[15..8] */
    SD_SPI_SendByte((uint8_t)arg);            /* Argument[7..0] */

    /* CRC (only matters for CMD0 and CMD8) */
    if (cmd == SD_CMD0)
        SD_SPI_SendByte(0x95);
    else if (cmd == SD_CMD8)
        SD_SPI_SendByte(0x87);
    else
        SD_SPI_SendByte(0xFF);

    /* Wait for response */
    timeout = HAL_GetTick() + SD_RESPONSE_TIMEOUT;
    do {
        response = SD_SPI_ReceiveByte();
        if ((response & 0x80) == 0)
            return response;
    } while (HAL_GetTick() < timeout);

    return 0xFF;  /* Timeout */
}

/**
  * @brief  Initialize SD card
  */
int SD_Init(void)
{
    uint8_t response;
    uint32_t timeout;
    uint8_t i;

    /* Set CS high initially */
    SD_CS_High();

    /* Send 80 dummy clocks */
    for (i = 0; i < 10; i++)
        SD_SPI_SendByte(0xFF);

    /* Enter SPI mode - CMD0 */
    SD_CS_Low();
    response = SD_SendCommand(SD_CMD0, 0);
    SD_CS_High();

    if (response != 0x01)
        return -1;  /* Failed to enter idle state */

    /* Send CMD8 (check voltage range) */
    SD_CS_Low();
    response = SD_SendCommand(SD_CMD8, 0x1AA);

    if (response == 0x01) {
        /* Read R7 response (4 bytes) */
        for (i = 0; i < 4; i++)
            SD_SPI_ReceiveByte();
    }
    SD_CS_High();

    /* Initialize card - ACMD41 */
    timeout = HAL_GetTick() + SD_INIT_TIMEOUT;
    do {
        /* Send CMD55 (APP_CMD) */
        SD_CS_Low();
        response = SD_SendCommand(SD_CMD55, 0);
        SD_CS_High();

        /* Send ACMD41 (SEND_OP_COND) */
        SD_CS_Low();
        response = SD_SendCommand(SD_ACMD41, 0x40000000);
        SD_CS_High();

        if (response == 0x00)
            break;  /* Initialization complete */

        HAL_Delay(10);
    } while (HAL_GetTick() < timeout);

    if (response != 0x00)
        return -1;  /* Initialization failed */

    sd_initialized = 1;
    return 0;
}

/**
  * @brief  Parse FAT file system
  */
static int SD_ParseFAT(void)
{
    uint8_t buffer[SECTOR_SIZE];
    uint32_t fat_size;
    uint32_t total_sectors;
    uint32_t root_dir_sectors;

    /* Read boot sector */
    if (SD_ReadRawSector(0, buffer) != 0)
        return -1;

    /* Check for valid boot sector */
    if (buffer[510] != 0x55 || buffer[511] != 0xAA)
        return -1;

    /* Parse FAT16/32 boot sector */
    uint16_t bytes_per_sector = buffer[11] | (buffer[12] << 8);
    sectors_per_cluster = buffer[13];
    uint16_t reserved_sectors = buffer[14] | (buffer[15] << 8);
    uint8_t num_fats = buffer[16];
    uint16_t root_entries = buffer[17] | (buffer[18] << 8);

    /* Get FAT size */
    fat_size = buffer[22] | (buffer[23] << 8);
    if (fat_size == 0) {
        /* FAT32 */
        fat_size = buffer[36] | (buffer[37] << 8) | (buffer[38] << 16) | (buffer[39] << 24);
    }

    /* Calculate sector positions */
    fat_start_sector = reserved_sectors;
    root_dir_sectors = ((root_entries * 32) + (bytes_per_sector - 1)) / bytes_per_sector;
    root_dir_sector = fat_start_sector + (num_fats * fat_size);
    data_start_sector = root_dir_sector + root_dir_sectors;

    return 0;
}

/**
  * @brief  Mount SD card file system
  */
int SD_Mount(void)
{
    if (!sd_initialized)
        return -1;

    return SD_ParseFAT();
}

/**
  * @brief  Read a raw sector from SD card
  */
int SD_ReadRawSector(uint32_t sector, uint8_t* buffer)
{
    uint8_t response;
    uint16_t i;
    uint32_t timeout;

    if (!sd_initialized || buffer == NULL)
        return -1;

    /* Send CMD17 (READ_SINGLE_BLOCK) */
    SD_CS_Low();
    response = SD_SendCommand(SD_CMD17, sector);

    if (response != 0x00) {
        SD_CS_High();
        return -1;
    }

    /* Wait for data token (0xFE) */
    timeout = HAL_GetTick() + SD_RESPONSE_TIMEOUT;
    do {
        response = SD_SPI_ReceiveByte();
        if (response == 0xFE)
            break;
    } while (HAL_GetTick() < timeout);

    if (response != 0xFE) {
        SD_CS_High();
        return -1;
    }

    /* Read 512 bytes of data */
    for (i = 0; i < SECTOR_SIZE; i++)
        buffer[i] = SD_SPI_ReceiveByte();

    /* Read CRC (2 bytes, but ignore) */
    SD_SPI_ReceiveByte();
    SD_SPI_ReceiveByte();

    SD_CS_High();
    SD_SPI_SendByte(0xFF);  /* Extra clock */

    return 0;
}

/**
  * @brief  Find file in root directory
  */
static int SD_FindFile(const char* filename, uint32_t* start_cluster, uint32_t* file_size)
{
    uint8_t buffer[SECTOR_SIZE];
    uint32_t sector = root_dir_sector;
    uint16_t i, j;
    char name[12];

    /* Convert filename to FAT format (8.3) */
    memset(name, ' ', 11);
    name[11] = '\0';

    /* Parse filename */
    const char* dot = strchr(filename, '.');
    if (dot) {
        uint8_t name_len = (dot - filename) > 8 ? 8 : (dot - filename);
        memcpy(name, filename, name_len);

        uint8_t ext_len = strlen(dot + 1) > 3 ? 3 : strlen(dot + 1);
        memcpy(name + 8, dot + 1, ext_len);
    } else {
        uint8_t name_len = strlen(filename) > 8 ? 8 : strlen(filename);
        memcpy(name, filename, name_len);
    }

    /* Convert to uppercase */
    for (i = 0; i < 11; i++) {
        if (name[i] >= 'a' && name[i] <= 'z')
            name[i] = name[i] - 'a' + 'A';
    }

    /* Search root directory (up to 16 sectors) */
    for (j = 0; j < 16; j++) {
        if (SD_ReadRawSector(sector + j, buffer) != 0)
            return -1;

        /* Check each directory entry */
        for (i = 0; i < SECTOR_SIZE; i += FAT_DIR_ENTRY_SIZE) {
            /* End of directory */
            if (buffer[i] == 0x00)
                return -1;

            /* Deleted entry */
            if (buffer[i] == 0xE5)
                continue;

            /* Skip directories and volume labels */
            if (buffer[i + 11] & (FAT_ATTR_DIRECTORY | 0x08))
                continue;

            /* Compare filename */
            if (memcmp(&buffer[i], name, 11) == 0) {
                /* Found file */
                *start_cluster = buffer[i + 26] | (buffer[i + 27] << 8) |
                                 (buffer[i + 20] << 16) | (buffer[i + 21] << 24);
                *file_size = buffer[i + 28] | (buffer[i + 29] << 8) |
                            (buffer[i + 30] << 16) | (buffer[i + 31] << 24);
                return 0;
            }
        }
    }

    return -1;  /* File not found */
}

/**
  * @brief  Open a file
  */
int SD_OpenFile(const char* filepath, FIL* file)
{
    uint32_t start_cluster, file_size;

    if (file == NULL || filepath == NULL)
        return -1;

    /* Find file in directory */
    if (SD_FindFile(filepath, &start_cluster, &file_size) != 0)
        return -1;

    /* Initialize file object */
    file->fsize = file_size;
    file->fptr = 0;
    file->start_cluster = start_cluster;
    file->current_sector = data_start_sector + ((start_cluster - 2) * sectors_per_cluster);
    file->flag = 1;  /* File opened */

    return 0;
}

/**
  * @brief  Close a file
  */
void SD_CloseFile(FIL* file)
{
    if (file != NULL)
        file->flag = 0;
}

/**
  * @brief  Rewind file to beginning
  */
void SD_Rewind(FIL* file)
{
    if (file != NULL) {
        file->fptr = 0;
        file->current_sector = 0;  /* Will be recalculated on next read */
    }
}

/**
  * @brief  Read sector from file
  */
int SD_ReadSector(FIL* file, uint8_t* buffer, uint32_t sector_size, uint32_t* bytes_read)
{
    uint32_t to_read;

    if (file == NULL || buffer == NULL || bytes_read == NULL)
        return -1;

    if (file->flag == 0)
        return -1;  /* File not open */

    if (file->fptr >= file->fsize) {
        *bytes_read = 0;
        return 0;  /* EOF */
    }

    /* Calculate bytes to read */
    to_read = file->fsize - file->fptr;
    if (to_read > sector_size)
        to_read = sector_size;

    /* Read sector */
    if (SD_ReadRawSector(file->current_sector, buffer) != 0)
        return -1;

    /* Update file pointer */
    file->fptr += to_read;
    file->current_sector++;
    *bytes_read = to_read;

    return 0;
}
