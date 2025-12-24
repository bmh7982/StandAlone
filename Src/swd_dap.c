/**
  ******************************************************************************
  * @file           : swd_dap.c
  * @brief          : CMSIS-DAP SWD protocol implementation (bit-banging)
  ******************************************************************************
  * @description
  * This module implements the Serial Wire Debug (SWD) protocol used for
  * programming and debugging ARM Cortex-M microcontrollers.
  *
  * SWD Protocol Overview:
  * - 2-wire protocol: SWDIO (bidirectional data) and SWCLK (clock)
  * - Replaces JTAG for ARM Cortex-M devices
  * - Provides access to Debug Port (DP) and Access Port (AP)
  *
  * Bit-banging Implementation:
  * - GPIO manipulation for clock and data
  * - Timing-critical sections marked with comments
  * - Software delays for clock generation
  ******************************************************************************
  */

#include "swd_dap.h"
#include "main.h"

/* Private defines -----------------------------------------------------------*/
#define SWD_CLOCK_DELAY()  /* Adjust for desired clock speed */

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void SWD_SetDirOutput(void);
static void SWD_SetDirInput(void);
static uint8_t SWD_TransferPacket(uint8_t request, uint32_t* data);
static uint8_t CalcParity(uint32_t value);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Set SWDIO direction to output
  */
static void SWD_SetDirOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SWDIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SWDIO_PORT, &GPIO_InitStruct);
}

/**
  * @brief  Set SWDIO direction to input
  */
static void SWD_SetDirInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SWDIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SWDIO_PORT, &GPIO_InitStruct);
}

/**
  * @brief  Calculate even parity of 32-bit value
  * @param  value: Value to calculate parity for
  * @retval Parity bit (0 or 1)
  */
static uint8_t CalcParity(uint32_t value)
{
    uint8_t parity = 0;
    for (int i = 0; i < 32; i++) {
        if (value & (1 << i))
            parity ^= 1;
    }
    return parity;
}

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  Initialize SWD interface
  * @retval None
  */
void SWD_Init(void)
{
    SWD_GPIO_Config();
}

/**
  * @brief  Configure SWD GPIO pins
  * @retval None
  */
void SWD_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIO clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Configure SWCLK as output */
    GPIO_InitStruct.Pin = SWCLK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SWCLK_PORT, &GPIO_InitStruct);

    /* Configure SWDIO as output (initially) */
    SWD_SetDirOutput();

    /* Configure RESET as output */
    GPIO_InitStruct.Pin = SWRST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SWRST_PORT, &GPIO_InitStruct);

    /* Set initial states */
    HAL_GPIO_WritePin(SWCLK_PORT, SWCLK_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SWDIO_PORT, SWDIO_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SWRST_PORT, SWRST_PIN, GPIO_PIN_SET);  /* Reset inactive (high) */
}

/**
  * @brief  Write a single bit to SWD
  * @param  bit: Bit value (0 or 1)
  * @retval None
  * @note   TIMING CRITICAL: Generates SWD clock cycle
  */
void SWD_WriteBit(uint8_t bit)
{
    /* Set data on falling edge of clock */
    HAL_GPIO_WritePin(SWCLK_PORT, SWCLK_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SWDIO_PORT, SWDIO_PIN, bit ? GPIO_PIN_SET : GPIO_PIN_RESET);
    SWD_CLOCK_DELAY();

    /* Clock high */
    HAL_GPIO_WritePin(SWCLK_PORT, SWCLK_PIN, GPIO_PIN_SET);
    SWD_CLOCK_DELAY();
}

/**
  * @brief  Read a single bit from SWD
  * @retval Bit value (0 or 1)
  * @note   TIMING CRITICAL: Generates SWD clock cycle
  */
uint8_t SWD_ReadBit(void)
{
    uint8_t bit;

    /* Clock low */
    HAL_GPIO_WritePin(SWCLK_PORT, SWCLK_PIN, GPIO_PIN_RESET);
    SWD_CLOCK_DELAY();

    /* Clock high and read data */
    HAL_GPIO_WritePin(SWCLK_PORT, SWCLK_PIN, GPIO_PIN_SET);
    bit = HAL_GPIO_ReadPin(SWDIO_PORT, SWDIO_PIN);
    SWD_CLOCK_DELAY();

    return bit;
}

/**
  * @brief  Write a byte to SWD (LSB first)
  * @param  data: Byte to write
  * @retval None
  */
void SWD_WriteByte(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        SWD_WriteBit(data & 0x01);
        data >>= 1;
    }
}

/**
  * @brief  Read a byte from SWD (LSB first)
  * @retval Byte value
  */
uint8_t SWD_ReadByte(void)
{
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        data >>= 1;
        if (SWD_ReadBit())
            data |= 0x80;
    }
    return data;
}

/**
  * @brief  Perform SWD line reset sequence
  * @retval 0 if success, -1 if error
  * @note   Sequence: 50+ clocks high, 0x00, 50+ clocks high, 0x00
  */
int SWD_LineReset(void)
{
    SWD_SetDirOutput();

    /* Send at least 50 cycles with SWDIO high */
    for (int i = 0; i < 56; i++) {
        SWD_WriteBit(1);
    }

    /* Send 0x00 (switch to SWD) */
    SWD_WriteByte(0x00);

    /* Send at least 50 more cycles with SWDIO high */
    for (int i = 0; i < 56; i++) {
        SWD_WriteBit(1);
    }

    /* Send 0x00 */
    SWD_WriteByte(0x00);

    return 0;
}

/**
  * @brief  Transfer SWD packet (read or write)
  * @param  request: Request byte
  * @param  data: Pointer to data (read or write)
  * @retval ACK response
  * @note   TIMING CRITICAL: Implements complete SWD transaction
  */
static uint8_t SWD_TransferPacket(uint8_t request, uint32_t* data)
{
    uint8_t ack;
    uint32_t value = 0;
    uint8_t parity;

    /* Send request (8 bits) */
    SWD_SetDirOutput();
    SWD_WriteByte(request);

    /* Turnaround: 1 clock cycle */
    SWD_SetDirInput();
    SWD_ReadBit();

    /* Read ACK (3 bits) */
    ack = 0;
    for (int i = 0; i < 3; i++) {
        ack >>= 1;
        if (SWD_ReadBit())
            ack |= 0x04;
    }

    if (ack == SWD_ACK_OK) {
        /* Check if read or write */
        if (request & 0x04) {
            /* Read operation */
            /* Read data (32 bits, LSB first) */
            value = 0;
            for (int i = 0; i < 32; i++) {
                value >>= 1;
                if (SWD_ReadBit())
                    value |= 0x80000000;
            }

            /* Read parity */
            parity = SWD_ReadBit();

            /* Verify parity */
            if (parity != CalcParity(value)) {
                ack = SWD_ACK_ERROR;
            } else {
                *data = value;
            }

            /* Turnaround */
            SWD_ReadBit();
        } else {
            /* Write operation */
            /* Turnaround */
            SWD_ReadBit();

            /* Write data (32 bits, LSB first) */
            SWD_SetDirOutput();
            value = *data;
            for (int i = 0; i < 32; i++) {
                SWD_WriteBit(value & 0x01);
                value >>= 1;
            }

            /* Write parity */
            SWD_WriteBit(CalcParity(*data));
        }
    }

    /* Idle cycles */
    SWD_SetDirOutput();
    SWD_WriteBit(0);

    return ack;
}

/**
  * @brief  Read Debug Port register
  * @param  addr: Register address
  * @param  data: Pointer to store read data
  * @retval 0 if success, -1 if error
  */
int SWD_ReadDP(uint8_t addr, uint32_t* data)
{
    uint8_t request;
    uint8_t ack;

    /* Build request: Start + APnDP(0) + RnW(1) + Addr[2:3] + Parity + Stop + Park */
    request = 0x81;  /* Start bit + APnDP=0 + RnW=1 */
    request |= (addr & 0x0C);  /* Address bits [2:3] */
    request |= (CalcParity(request) << 5);  /* Parity */

    ack = SWD_TransferPacket(request, data);

    return (ack == SWD_ACK_OK) ? 0 : -1;
}

/**
  * @brief  Write Debug Port register
  * @param  addr: Register address
  * @param  data: Data to write
  * @retval 0 if success, -1 if error
  */
int SWD_WriteDP(uint8_t addr, uint32_t data)
{
    uint8_t request;
    uint8_t ack;

    /* Build request: Start + APnDP(0) + RnW(0) + Addr[2:3] + Parity + Stop + Park */
    request = 0x81;  /* Start bit + APnDP=0 */
    request &= ~0x04;  /* RnW=0 (write) */
    request |= (addr & 0x0C);  /* Address bits [2:3] */
    request |= (CalcParity(request) << 5);  /* Parity */

    ack = SWD_TransferPacket(request, &data);

    return (ack == SWD_ACK_OK) ? 0 : -1;
}

/**
  * @brief  Read Access Port register
  * @param  addr: Register address
  * @param  data: Pointer to store read data
  * @retval 0 if success, -1 if error
  */
int SWD_ReadAP(uint8_t addr, uint32_t* data)
{
    uint8_t request;
    uint8_t ack;

    /* Build request: Start + APnDP(1) + RnW(1) + Addr[2:3] + Parity + Stop + Park */
    request = 0x85;  /* Start bit + APnDP=1 + RnW=1 */
    request |= (addr & 0x0C);  /* Address bits [2:3] */
    request |= (CalcParity(request) << 5);  /* Parity */

    ack = SWD_TransferPacket(request, data);

    /* For AP reads, need to read RDBUFF to get actual data */
    if (ack == SWD_ACK_OK) {
        return SWD_ReadDP(DP_RDBUFF, data);
    }

    return -1;
}

/**
  * @brief  Write Access Port register
  * @param  addr: Register address
  * @param  data: Data to write
  * @retval 0 if success, -1 if error
  */
int SWD_WriteAP(uint8_t addr, uint32_t data)
{
    uint8_t request;
    uint8_t ack;

    /* Build request: Start + APnDP(1) + RnW(0) + Addr[2:3] + Parity + Stop + Park */
    request = 0x85;  /* Start bit + APnDP=1 */
    request &= ~0x04;  /* RnW=0 (write) */
    request |= (addr & 0x0C);  /* Address bits [2:3] */
    request |= (CalcParity(request) << 5);  /* Parity */

    ack = SWD_TransferPacket(request, &data);

    return (ack == SWD_ACK_OK) ? 0 : -1;
}

/**
  * @brief  Connect to target MCU via SWD
  * @retval 0 if success, -1 if error
  */
int Target_Connect(void)
{
    uint32_t idcode = 0;

    /* Perform line reset */
    if (SWD_LineReset() != 0)
        return -1;

    /* Small delay after reset */
    HAL_Delay(10);

    /* Read IDCODE */
    if (Target_Detect(&idcode) != 0)
        return -1;

    /* Check if valid IDCODE */
    if (idcode == 0x00000000 || idcode == 0xFFFFFFFF)
        return -1;

    return 0;
}

/**
  * @brief  Detect target MCU and read IDCODE
  * @param  idcode: Pointer to store IDCODE
  * @retval 0 if success, -1 if error
  */
int Target_Detect(uint32_t* idcode)
{
    if (idcode == NULL)
        return -1;

    /* Read IDCODE register */
    return SWD_ReadDP(DP_IDCODE, idcode);
}

/**
  * @brief  Identify MCU type from IDCODE
  * @param  idcode: IDCODE value
  * @retval MCU type
  */
MCU_Type_t Target_IdentifyMCU(uint32_t idcode)
{
    switch (idcode) {
        case IDCODE_CORTEX_M0:
            return MCU_TYPE_CORTEX_M0;
        case IDCODE_CORTEX_M3:
            return MCU_TYPE_CORTEX_M3;
        case IDCODE_CORTEX_M4:
            return MCU_TYPE_CORTEX_M4;
        default:
            return MCU_TYPE_UNKNOWN;
    }
}

/**
  * @brief  Halt target core
  * @retval 0 if success, -1 if error
  */
int Target_HaltCore(void)
{
    uint32_t dhcsr;

    /* Write to Debug Halting Control and Status Register */
    /* Address: 0xE000EDF0 */
    /* Set C_DEBUGEN and C_HALT bits */
    dhcsr = 0xA05F0003;  /* Debug key + DEBUGEN + HALT */

    /* Need to implement memory write via AP */
    /* This is a simplified version */
    return 0;
}

/**
  * @brief  Reset target MCU
  * @retval 0 if success, -1 if error
  */
int Target_Reset(void)
{
    /* Assert hardware reset */
    HAL_GPIO_WritePin(SWRST_PORT, SWRST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);

    /* Release reset */
    HAL_GPIO_WritePin(SWRST_PORT, SWRST_PIN, GPIO_PIN_SET);
    HAL_Delay(100);

    return 0;
}

/* Flash Programming Functions -----------------------------------------------*/

/**
  * @brief  Read memory from target (32-bit aligned)
  * @param  address: Memory address to read from
  * @param  data: Buffer to store read data
  * @param  size: Number of bytes to read
  * @retval 0 if success, -1 if error
  */
int Target_ReadMemory(uint32_t address, uint8_t* data, uint32_t size)
{
    uint32_t value;
    uint32_t i;

    if (data == NULL || size == 0)
        return -1;

    /* Configure AP for auto-increment */
    if (SWD_WriteAP(AP_CSW, 0x23000002) != 0)  /* 32-bit access, auto-increment */
        return -1;

    /* Set transfer address */
    if (SWD_WriteAP(AP_TAR, address) != 0)
        return -1;

    /* Read data */
    for (i = 0; i < size; i += 4) {
        if (SWD_ReadAP(AP_DRW, &value) != 0)
            return -1;

        /* Copy to buffer (handle non-aligned sizes) */
        for (uint8_t j = 0; j < 4 && (i + j) < size; j++) {
            data[i + j] = (value >> (j * 8)) & 0xFF;
        }
    }

    return 0;
}

/**
  * @brief  Write memory to target (32-bit aligned)
  * @param  address: Memory address to write to
  * @param  data: Data to write
  * @param  size: Number of bytes to write
  * @retval 0 if success, -1 if error
  */
int Target_WriteMemory(uint32_t address, uint8_t* data, uint32_t size)
{
    uint32_t value;
    uint32_t i;

    if (data == NULL || size == 0)
        return -1;

    /* Configure AP for auto-increment */
    if (SWD_WriteAP(AP_CSW, 0x23000002) != 0)  /* 32-bit access, auto-increment */
        return -1;

    /* Set transfer address */
    if (SWD_WriteAP(AP_TAR, address) != 0)
        return -1;

    /* Write data */
    for (i = 0; i < size; i += 4) {
        value = 0;
        /* Assemble 32-bit word from bytes */
        for (uint8_t j = 0; j < 4 && (i + j) < size; j++) {
            value |= ((uint32_t)data[i + j]) << (j * 8);
        }

        if (SWD_WriteAP(AP_DRW, value) != 0)
            return -1;
    }

    return 0;
}

/**
  * @brief  Wait for flash operation to complete
  * @param  sr_addr: Flash status register address
  * @retval 0 if success, -1 if timeout/error
  */
static int Flash_WaitBusy(uint32_t sr_addr)
{
    uint32_t status;
    uint32_t timeout = 1000;  /* Timeout counter */

    while (timeout--) {
        if (Target_ReadMemory(sr_addr, (uint8_t*)&status, 4) != 0)
            return -1;

        /* Check busy flag */
        if ((status & FLASH_SR_BSY) == 0)
            break;

        HAL_Delay(1);
    }

    if (timeout == 0)
        return -1;  /* Timeout */

    /* Check for errors */
    if (status & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR))
        return -1;

    return 0;
}

/**
  * @brief  Unlock flash for programming
  * @retval 0 if success, -1 if error
  */
int Flash_Unlock(void)
{
    uint32_t keyr_addr = FLASH_KEYR_M3;  /* Assume M3 for now */

    /* Write unlock keys */
    if (Target_WriteMemory(keyr_addr, (uint8_t*)&(uint32_t){FLASH_KEY1}, 4) != 0)
        return -1;

    if (Target_WriteMemory(keyr_addr, (uint8_t*)&(uint32_t){FLASH_KEY2}, 4) != 0)
        return -1;

    return 0;
}

/**
  * @brief  Lock flash after programming
  * @retval 0 if success, -1 if error
  */
int Flash_Lock(void)
{
    uint32_t cr_addr = FLASH_CR_M3;
    uint32_t cr_value = FLASH_CR_LOCK;

    return Target_WriteMemory(cr_addr, (uint8_t*)&cr_value, 4);
}

/**
  * @brief  Erase flash page (STM32F1 - 1KB page)
  * @param  address: Address in the page to erase
  * @retval 0 if success, -1 if error
  */
int Flash_Erase(uint32_t address)
{
    uint32_t cr_value;

    /* Set PER bit */
    cr_value = FLASH_CR_PER;
    if (Target_WriteMemory(FLASH_CR_M3, (uint8_t*)&cr_value, 4) != 0)
        return -1;

    /* Write page address */
    if (Target_WriteMemory(FLASH_AR_M3, (uint8_t*)&address, 4) != 0)
        return -1;

    /* Start erase */
    cr_value = FLASH_CR_PER | FLASH_CR_STRT;
    if (Target_WriteMemory(FLASH_CR_M3, (uint8_t*)&cr_value, 4) != 0)
        return -1;

    /* Wait for completion */
    if (Flash_WaitBusy(FLASH_SR_M3) != 0)
        return -1;

    /* Clear PER bit */
    cr_value = 0;
    return Target_WriteMemory(FLASH_CR_M3, (uint8_t*)&cr_value, 4);
}

/**
  * @brief  Erase entire flash (mass erase)
  * @retval 0 if success, -1 if error
  */
int Flash_EraseFull(void)
{
    uint32_t cr_value;

    /* Set MER bit */
    cr_value = FLASH_CR_MER;
    if (Target_WriteMemory(FLASH_CR_M3, (uint8_t*)&cr_value, 4) != 0)
        return -1;

    /* Start erase */
    cr_value = FLASH_CR_MER | FLASH_CR_STRT;
    if (Target_WriteMemory(FLASH_CR_M3, (uint8_t*)&cr_value, 4) != 0)
        return -1;

    /* Wait for completion (this can take a while) */
    if (Flash_WaitBusy(FLASH_SR_M3) != 0)
        return -1;

    /* Clear MER bit */
    cr_value = 0;
    return Target_WriteMemory(FLASH_CR_M3, (uint8_t*)&cr_value, 4);
}

/**
  * @brief  Program flash memory (half-word programming for STM32F1)
  * @param  address: Flash address to program (must be half-word aligned)
  * @param  data: Data to program
  * @param  size: Number of bytes to program
  * @retval 0 if success, -1 if error
  */
int Flash_Program(uint32_t address, uint8_t* data, uint32_t size)
{
    uint32_t cr_value;
    uint16_t half_word;
    uint32_t i;

    if (data == NULL || size == 0)
        return -1;

    /* Set PG bit */
    cr_value = FLASH_CR_PG;
    if (Target_WriteMemory(FLASH_CR_M3, (uint8_t*)&cr_value, 4) != 0)
        return -1;

    /* Program half-word by half-word (STM32F1 requires 16-bit writes) */
    for (i = 0; i < size; i += 2) {
        /* Assemble half-word */
        half_word = data[i];
        if (i + 1 < size)
            half_word |= ((uint16_t)data[i + 1]) << 8;

        /* Write half-word to flash */
        if (Target_WriteMemory(address + i, (uint8_t*)&half_word, 2) != 0) {
            /* Clear PG bit on error */
            cr_value = 0;
            Target_WriteMemory(FLASH_CR_M3, (uint8_t*)&cr_value, 4);
            return -1;
        }

        /* Wait for completion */
        if (Flash_WaitBusy(FLASH_SR_M3) != 0) {
            /* Clear PG bit on error */
            cr_value = 0;
            Target_WriteMemory(FLASH_CR_M3, (uint8_t*)&cr_value, 4);
            return -1;
        }
    }

    /* Clear PG bit */
    cr_value = 0;
    return Target_WriteMemory(FLASH_CR_M3, (uint8_t*)&cr_value, 4);
}

/**
  * @brief  Verify programmed flash memory
  * @param  address: Flash address to verify
  * @param  data: Expected data
  * @param  size: Number of bytes to verify
  * @retval 0 if success, -1 if mismatch
  */
int Flash_Verify(uint32_t address, uint8_t* data, uint32_t size)
{
    uint8_t read_buffer[256];
    uint32_t i, chunk_size;

    if (data == NULL || size == 0)
        return -1;

    /* Verify in chunks */
    for (i = 0; i < size; i += sizeof(read_buffer)) {
        chunk_size = (size - i) < sizeof(read_buffer) ? (size - i) : sizeof(read_buffer);

        /* Read from flash */
        if (Target_ReadMemory(address + i, read_buffer, chunk_size) != 0)
            return -1;

        /* Compare */
        for (uint32_t j = 0; j < chunk_size; j++) {
            if (read_buffer[j] != data[i + j]) {
                /* Mismatch found */
                return -1;
            }
        }
    }

    return 0;
}
