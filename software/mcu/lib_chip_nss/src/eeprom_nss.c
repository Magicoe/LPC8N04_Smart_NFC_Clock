/*
 * Copyright (c), NXP Semiconductors
 * (C)NXP B.V. 2014-2017
 * All rights are reserved. Reproduction in whole or in part is prohibited without
 * the written consent of the copyright owner. NXP reserves the right to make
 * changes without notice at any time. NXP makes no warranty, expressed, implied or
 * statutory, including but not limited to any implied warranty of merchantability
 * or fitness for any particular purpose, or that the use will not infringe any
 * third party patent, copyright or trademark. NXP must not be liable for any loss
 * or damage arising from its use.
 */


#include "chip.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Private types/enumerations/variables
 * ------------------------------------------------------------------------- */

/**
 * EEPROM clock frequency (in Hz)
 * @note   Hardware spec: between 200kHz and 400kHz
 *         We want it to be as close as possible to BUT never higher then 400kHz.
 *         The EEPROM memory is using this as a reference clock for erasing/programming.
 *         If this clock freq. gets higher then 400 kHz, the program time is reduced and
 *         the erasing/programming cycle is not ensured.
 */
#define EEPROM_CLOCK_FREQUENCY_HZ      (375*1000)

/** Time EEPROM needs to get ready after power-up */
#define EEPROM_ACTIVATION_TIME_US 100

/** Erase/program bits in EEPROM command register */
#define EEPROM_START_ERASE_PROGRAM     6

/** Program done status bit in EEPROM interrupt registers*/
#define EEPROM_PROG_DONE_STATUS_BIT    (1 << 2)

/** Macro to convert an EEPROM offset to its row number */
#define EEPROM_OFFSET_TO_ROW(x) ( (x) / EEPROM_ROW_SIZE )

/**  Value for EEPROM_last_written_row to indicate there is no last written row (no pending flush) */
#define EEPROM_NO_LAST_WRITTEN_ROW  -1

/** SW flag indicating whether the EEPROM is being flushed or not */
static bool EEPROM_flushing;

/** Static variable holding the last (non flushed) row */
static int EEPROM_lastWrittenRow = EEPROM_NO_LAST_WRITTEN_ROW;

#define EEPROM_IS_FLUSH_PENDING() !(EEPROM_lastWrittenRow == EEPROM_NO_LAST_WRITTEN_ROW)

/* -------------------------------------------------------------------------
 * Private functions
 * ------------------------------------------------------------------------- */

/**
 * Waits till EEPROM is ready to be read/written
 * This function checks if EEPROM is being flushed, and busy waits till ready
 * @param pEEPROM The base address of the EEPROM block registers on the chip.
 */
static void WaitUntilReady(NSS_EEPROM_T *pEEPROM)
{
    while (EEPROM_flushing) {
        EEPROM_flushing = !(pEEPROM->INT_STATUS & EEPROM_PROG_DONE_STATUS_BIT);
    }
}

/** Generalized Write function supporting 2 operations:
 *      - Copy data (buffer) into EEPROM
 *      - Memset function (1 byte pattern)
 *      .
 */
static void EEPROM_Write(NSS_EEPROM_T *pEEPROM, int offset, void * pBuf, int size, bool memset)
{
    uint8_t *src;
    uint16_t *dst;
    bool unalignedStart;

    /* Offset and size must be positive */
    ASSERT(offset >= 0);
    ASSERT(size > 0);

    /* All bytes must be written to a valid EEPROM address */
    ASSERT((offset + size) <= (EEPROM_ROW_SIZE * EEPROM_NR_OF_RW_ROWS));

    src = pBuf;
    dst = &((uint16_t*) EEPROM_START)[offset / 2];
    unalignedStart = (offset % 2 != 0);

    /* If a flush is busy, wait for it to finish */
    WaitUntilReady(pEEPROM);

    /* Following cases need a prior flush:
     * - there is a pending flush for another row than first one in targeted area
     * - there is a pending flush and current offset is not 16 bit aligned
     * - there is a pending flush and current size is not 16 bit aligned
     */
    if (EEPROM_IS_FLUSH_PENDING() &&
       ((EEPROM_lastWrittenRow != EEPROM_OFFSET_TO_ROW(offset)) ||
       ((uint32_t)offset % sizeof(uint16_t) != 0) ||
       ((uint32_t)size % sizeof(uint16_t) != 0))) {
            Chip_EEPROM_Flush(pEEPROM, true);
    }

    while (size > 0) {
        /*If first byte is to be copied to non aligned EEPROM byte*/
        if (unalignedStart) {
            /* The first byte from the buffer is not 16-bits aligned.
             * Read the LSB from EEPROM to complete it.
             */
            *dst = (uint16_t)(((uint16_t)src[0] << 8) | (*dst & 0x00ff));
            unalignedStart = false;
        }
        /* Every iteration we write one 16-bit unit to the EEPROM memory.*/
        else if (size >= 2) {
            /* Combine two bytes from the buffer to a 16-bits word */
            *dst = (uint16_t)(((uint16_t)src[1] << 8) | src[0]);
            /* If normal 16 bit aligned write, we need src resp. size to be incremented/decremented by 2
             * First one performed here, second one at end of while (common for other cases)*/
            src ++;
            size --;
        }
        else{
            /* The last byte from the buffer is not 16-bits aligned.
             * Read the MSB from EEPROM to complete it.
             * With our strategy, we never need a flush here
             */
            *dst = (uint16_t)((*dst & 0xff00) | src[0]);
        }
        src ++;
        size --;
        dst++;

        /* If memset equals true, we don't want src to be increased */
        if (memset){
            src = pBuf;
        }
        /* If we just wrote to the last word in a row and there is still data to write, we have to flush */
        if ((((uint32_t)dst % EEPROM_ROW_SIZE) == 0) && size > 0) {
            /*Changing EEPROM_lastWrittenRow to force a flush, we do not need to remember the last written
             * row in this case because we are flushing immediately*/
            EEPROM_lastWrittenRow = -2;
            Chip_EEPROM_Flush(pEEPROM, true);
        }
    }

    /* Save the last written row, so next time we know if we need to flush */
    EEPROM_lastWrittenRow = EEPROM_OFFSET_TO_ROW( (int)(dst - 1) - EEPROM_START );
}

/* -------------------------------------------------------------------------
 * Public functions
 * ------------------------------------------------------------------------- */

/* Initialize and configure the EEPROM peripheral */
void Chip_EEPROM_Init(NSS_EEPROM_T *pEEPROM)
{
    int div;

    EEPROM_flushing = false;
    EEPROM_lastWrittenRow = EEPROM_NO_LAST_WRITTEN_ROW;

    Chip_Clock_Peripheral_EnableClock(CLOCK_PERIPHERAL_EEPROM);
    Chip_SysCon_Peripheral_AssertReset(SYSCON_PERIPHERAL_RESET_EEPROM);
    Chip_SysCon_Peripheral_EnablePower(SYSCON_PERIPHERAL_POWER_EEPROM);

    /* Wait for the EEPROM to get ready for content access */
    Chip_Clock_System_BusyWait_us(EEPROM_ACTIVATION_TIME_US);

    Chip_SysCon_Peripheral_DeassertReset(SYSCON_PERIPHERAL_RESET_EEPROM);

    /* Set clock division factor, making it 'ceiling' by adding (EEPROM_CLOCK_FREQUENCY_HZ - 1).
     * This ensures the resulting ref. clock will not exceed the specified maximum  */
    div = ( ( Chip_Clock_System_GetClockFreq() + (EEPROM_CLOCK_FREQUENCY_HZ - 1) ) / EEPROM_CLOCK_FREQUENCY_HZ) - 1;

    /*If divisor is set to 0, the EEPROM ref. clock is disabled. So ensure it to be at least 1*/
    if (div < 1) {
        div = 1;
    }
    pEEPROM->CLKDIV = (uint32_t)div;
}

/* Shutdown the EEPROM peripheral */
void Chip_EEPROM_DeInit(NSS_EEPROM_T *pEEPROM)
{
    Chip_EEPROM_Flush(pEEPROM, true);
    Chip_SysCon_Peripheral_AssertReset(SYSCON_PERIPHERAL_RESET_EEPROM);
    Chip_SysCon_Peripheral_DisablePower(SYSCON_PERIPHERAL_POWER_EEPROM);
    Chip_Clock_Peripheral_DisableClock(CLOCK_PERIPHERAL_EEPROM);
}

/* Flush last written row  */
void Chip_EEPROM_Flush(NSS_EEPROM_T *pEEPROM, bool wait)
{
    if (EEPROM_IS_FLUSH_PENDING()) {
        /* Write all prepared words */
        pEEPROM->INT_CLR_STATUS = EEPROM_PROG_DONE_STATUS_BIT;
        pEEPROM->CMD = EEPROM_START_ERASE_PROGRAM;

        EEPROM_flushing = true;
        EEPROM_lastWrittenRow = EEPROM_NO_LAST_WRITTEN_ROW;
    }
    if (wait) {
        WaitUntilReady(pEEPROM);
    }
}

/* read data from the EEPROM peripheral */
void Chip_EEPROM_Read(NSS_EEPROM_T *pEEPROM, int offset, void * pBuf, int size)
{
    int start_row;
    int end_row;

    /* Offset and size must be positive */
    ASSERT(offset >= 0);
    ASSERT(size > 0);

    /* All bytes must be read from a valid EEPROM address */
    ASSERT((offset + size) <= (EEPROM_ROW_SIZE * EEPROM_NR_OF_R_ROWS));

    start_row = EEPROM_OFFSET_TO_ROW(offset);
    end_row = EEPROM_OFFSET_TO_ROW((offset + size - 1));

    /* If a flush is busy, wait for it to finish */
    WaitUntilReady(pEEPROM);

    /* If there is a pending flush for a row in target section, flush now */
    if ((start_row <= EEPROM_lastWrittenRow) && (EEPROM_lastWrittenRow <= end_row)) {
        Chip_EEPROM_Flush(pEEPROM, true);
    }

    /* Read EEPROM */
    memcpy(pBuf, (uint8_t *)(EEPROM_START + offset), (uint32_t)size);
}

void Chip_EEPROM_Write(NSS_EEPROM_T *pEEPROM, int offset, void * pBuf, int size)
{
    EEPROM_Write(pEEPROM, offset, pBuf, size, false);
}

void Chip_EEPROM_Memset(NSS_EEPROM_T *pEEPROM, int offset, uint8_t pattern, int size)
{
    uint16_t temp_pattern = (uint16_t)((pattern << 8) | pattern);
    EEPROM_Write(pEEPROM, offset, &temp_pattern, size, true);
}
