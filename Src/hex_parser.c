/**
  ******************************************************************************
  * @file           : hex_parser.c
  * @brief          : Intel HEX format parser implementation
  ******************************************************************************
  * @description
  * This module parses Intel HEX format files commonly used for MCU programming.
  *
  * Intel HEX Format:
  * Each line: :LLAAAATTDDDDCC
  *   : - Start code
  *   LL - Byte count (2 hex digits)
  *   AAAA - Address (4 hex digits)
  *   TT - Record type (2 hex digits)
  *   DD - Data bytes (variable length)
  *   CC - Checksum (2 hex digits)
  *
  * Supported Record Types:
  *   00 - Data record
  *   01 - End of file
  *   04 - Extended linear address (upper 16 bits of 32-bit address)
  *   05 - Start linear address (execution start address)
  ******************************************************************************
  */

#include "hex_parser.h"
#include "main.h"
#include <string.h>
#include <ctype.h>

/* Private defines -----------------------------------------------------------*/
#define HEX_LINE_MAX_LEN  256
#define HEX_START_CODE    ':'

/* Private variables ---------------------------------------------------------*/
static uint32_t current_extended_address = 0;  /* Current extended address */

/* Private function prototypes -----------------------------------------------*/
static int hex_char_to_int(char c);
static int hex_string_to_byte(const char* str);
static int hex_string_to_word(const char* str);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Convert hex character to integer
  * @param  c: Hex character ('0'-'9', 'A'-'F', 'a'-'f')
  * @retval Integer value (0-15), or -1 if invalid
  */
static int hex_char_to_int(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

/**
  * @brief  Convert 2-character hex string to byte
  * @param  str: Pointer to 2-character hex string
  * @retval Byte value (0-255), or -1 if invalid
  */
static int hex_string_to_byte(const char* str)
{
    int high, low;

    high = hex_char_to_int(str[0]);
    low = hex_char_to_int(str[1]);

    if (high < 0 || low < 0)
        return -1;

    return (high << 4) | low;
}

/**
  * @brief  Convert 4-character hex string to word (16-bit)
  * @param  str: Pointer to 4-character hex string
  * @retval Word value (0-65535), or -1 if invalid
  */
static int hex_string_to_word(const char* str)
{
    int high_byte, low_byte;

    high_byte = hex_string_to_byte(str);
    low_byte = hex_string_to_byte(str + 2);

    if (high_byte < 0 || low_byte < 0)
        return -1;

    return (high_byte << 8) | low_byte;
}

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  Verify checksum of Intel HEX line
  * @param  line: Null-terminated HEX line string
  * @retval 0 if checksum valid, -1 if invalid
  *
  * @note   Checksum calculation:
  *         1. Sum all bytes (byte count, address, type, data)
  *         2. Take two's complement (negate and add 1)
  *         3. Keep only lower 8 bits
  */
int HEX_VerifyChecksum(const char* line)
{
    uint32_t len;
    uint8_t sum = 0;
    uint8_t checksum;
    const char* ptr;

    if (line == NULL || line[0] != HEX_START_CODE)
        return -1;

    /* Calculate length (exclude ':' and checksum bytes) */
    len = strlen(line);
    if (len < 11)  /* Minimum: :LLAAAATTCC */
        return -1;

    /* Sum all bytes except checksum */
    ptr = line + 1;  /* Skip ':' */
    while (ptr < line + len - 2) {
        int byte_val = hex_string_to_byte(ptr);
        if (byte_val < 0)
            return -1;
        sum += (uint8_t)byte_val;
        ptr += 2;
    }

    /* Get checksum from line */
    checksum = (uint8_t)hex_string_to_byte(ptr);

    /* Verify: sum of all bytes including checksum should be 0 */
    sum += checksum;
    return (sum == 0) ? 0 : -1;
}

/**
  * @brief  Parse a single Intel HEX line
  * @param  line: Null-terminated HEX line string
  * @param  record: Pointer to HEX_Record_t structure to store parsed data
  * @retval 0 if success, -1 if error
  */
int HEX_ParseLine(const char* line, HEX_Record_t* record)
{
    const char* ptr;
    int i;

    if (line == NULL || record == NULL)
        return -1;

    /* Check start code */
    if (line[0] != HEX_START_CODE)
        return -1;

    /* Verify checksum first */
    if (HEX_VerifyChecksum(line) != 0)
        return -1;

    ptr = line + 1;  /* Skip ':' */

    /* Parse byte count */
    record->data_len = (uint8_t)hex_string_to_byte(ptr);
    ptr += 2;

    /* Parse address */
    record->address = (uint16_t)hex_string_to_word(ptr);
    ptr += 4;

    /* Parse record type */
    record->record_type = (uint8_t)hex_string_to_byte(ptr);
    ptr += 2;

    /* Parse data bytes */
    for (i = 0; i < record->data_len; i++) {
        int byte_val = hex_string_to_byte(ptr);
        if (byte_val < 0)
            return -1;
        record->data[i] = (uint8_t)byte_val;
        ptr += 2;
    }

    /* Store current extended address */
    record->extended_address = current_extended_address;

    /* Handle extended linear address record */
    if (record->record_type == HEX_RECORD_EXT_LINEAR_ADDR) {
        if (record->data_len == 2) {
            /* Extended address is upper 16 bits */
            current_extended_address = ((uint32_t)record->data[0] << 24) |
                                      ((uint32_t)record->data[1] << 16);
        }
    }

    return 0;
}

/**
  * @brief  Process HEX record into sector buffer
  * @param  record: Pointer to parsed HEX record
  * @param  sector: Pointer to program sector buffer
  * @retval 0 if success, -1 if error
  *
  * @note   This function accumulates data records into a sector buffer.
  *         When the buffer is full or address changes significantly,
  *         the caller should flush the buffer using the callback.
  */
int HEX_ProcessRecord(HEX_Record_t* record, Program_Sector_t* sector)
{
    uint32_t full_address;
    uint32_t offset;

    if (record == NULL || sector == NULL)
        return -1;

    /* Only process data records */
    if (record->record_type != HEX_RECORD_DATA)
        return 0;

    /* Calculate full 32-bit address */
    full_address = record->extended_address | record->address;

    /* Initialize sector if empty */
    if (sector->size == 0) {
        sector->base_address = full_address;
        memset(sector->data, 0xFF, SECTOR_SIZE);  /* Fill with 0xFF (erased flash) */
    }

    /* Check if data fits in current sector */
    if (full_address < sector->base_address ||
        full_address >= sector->base_address + SECTOR_SIZE) {
        /* Data doesn't fit - sector needs to be flushed first */
        return 1;  /* Signal caller to flush sector */
    }

    /* Calculate offset in sector */
    offset = full_address - sector->base_address;

    /* Copy data to sector buffer */
    if (offset + record->data_len > SECTOR_SIZE)
        return -1;  /* Data overflow */

    memcpy(&sector->data[offset], record->data, record->data_len);

    /* Update sector size */
    if (offset + record->data_len > sector->size)
        sector->size = offset + record->data_len;

    return 0;
}

/**
  * @brief  Process entire HEX file with callback
  * @param  file: Pointer to opened file object
  * @param  program_callback: Callback function called for each sector
  * @retval 0 if success, -1 if error
  *
  * @note   This function reads the HEX file line by line, parses each line,
  *         accumulates data into sectors, and calls the callback function
  *         for each complete sector. This allows streaming processing
  *         without loading the entire file into RAM.
  */
int HEX_ProcessFile(FIL* file,
                    int (*program_callback)(uint32_t addr, uint8_t* data, uint32_t size))
{
    uint8_t line_buffer[HEX_LINE_MAX_LEN];
    uint32_t bytes_read;
    uint16_t line_len;
    HEX_Record_t record;
    Program_Sector_t sector;
    int parse_result;

    if (file == NULL || program_callback == NULL)
        return -1;

    /* Initialize sector */
    memset(&sector, 0, sizeof(Program_Sector_t));
    current_extended_address = 0;

    /* Read file sector by sector */
    while (1) {
        /* Read one sector from file */
        if (SD_ReadSector(file, line_buffer, SECTOR_SIZE, &bytes_read) != 0)
            return -1;

        if (bytes_read == 0)
            break;  /* End of file */

        /* Process each line in the sector */
        line_len = 0;
        for (uint32_t i = 0; i < bytes_read; i++) {
            char c = (char)line_buffer[i];

            /* Check for line ending */
            if (c == '\n' || c == '\r') {
                if (line_len > 0) {
                    /* Null-terminate line */
                    line_buffer[line_len] = '\0';

                    /* Parse line */
                    parse_result = HEX_ParseLine((char*)line_buffer, &record);
                    if (parse_result != 0)
                        return -1;  /* Parse error */

                    /* Check for end of file */
                    if (record.record_type == HEX_RECORD_EOF) {
                        /* Flush last sector if not empty */
                        if (sector.size > 0) {
                            if (program_callback(sector.base_address,
                                               sector.data,
                                               sector.size) != 0)
                                return -1;
                        }
                        return 0;  /* Success */
                    }

                    /* Process record */
                    int process_result = HEX_ProcessRecord(&record, &sector);
                    if (process_result == 1) {
                        /* Sector full - flush it */
                        if (sector.size > 0) {
                            if (program_callback(sector.base_address,
                                               sector.data,
                                               sector.size) != 0)
                                return -1;
                        }

                        /* Reset sector and process record again */
                        memset(&sector, 0, sizeof(Program_Sector_t));
                        if (HEX_ProcessRecord(&record, &sector) != 0)
                            return -1;
                    } else if (process_result < 0) {
                        return -1;  /* Error */
                    }

                    line_len = 0;
                }
            } else if (line_len < HEX_LINE_MAX_LEN - 1) {
                /* Accumulate character */
                line_buffer[line_len++] = (uint8_t)c;
            }
        }
    }

    /* Flush last sector if not empty */
    if (sector.size > 0) {
        if (program_callback(sector.base_address, sector.data, sector.size) != 0)
            return -1;
    }

    return 0;
}
