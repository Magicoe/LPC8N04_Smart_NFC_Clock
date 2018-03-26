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

#include <string.h>
#include "board.h"
#include "storage/storage.h"
#include "memory.h"

/* ------------------------------------------------------------------------- */

#define ALON_WORD_SIZE ((sizeof(ALON_T) + (sizeof(uint32_t) - 1)) / sizeof(uint32_t))

#define EEPROM_OFFSET_BUILDTIMESTAMP 0
#define EEPROM_OFFSET_CONFIG (EEPROM_OFFSET_BUILDTIMESTAMP + sizeof(uint32_t))

/* ------------------------------------------------------------------------- */

/** Defines what is stored in the GP registers that reside in the PMU always-on domain. */
typedef struct ALON_S {
    /**
     * Flag that indicates measurements can still continue.
     * New measurements cannot be taken when either the battery died, or when storage space is full.
     * - battery:
     *  During normal lifetime, this flag is set when configuring the IC and checked to decide to power-off - and make
     *  no (new) measurements or to go to Deep Power Down - and make a new measurements after the timer expired. When it
     *  is zero while checking, it indicates we lost track of time, and cannot but go back to the power-off mode - a
     *  reconfiguration and a full read-out is still possible, but storing new samples is not allowed any more.
     * - storage:
     *  When EEPROM is completely filled, but moving data from EEPROM to FLASH is no longer possible, new measurements
     *  can be taken, but cannot be stored anymore. For this demo application, it is chosen to stop measurements
     *  altogether. A different choice might be to continue measurements, not store them, but still use them for
     *  decision taking on validity of the attached product.
     */
    bool uninterrupted;
} ALON_T;

/* ------------------------------------------------------------------------- */

#if !defined(APP_BUILD_TIMESTAMP)
    #error APP_BUILD_TIMESTAMP not defined. Define APP_BUILD_TIMESTAMP in your Project settings.
    #error Under LPCXPresso: Project > Properties > C/C++ Build > Settings > Tool Settings > MCU C Compiler > Symbols
    #error Add for example the define "APP_BUILD_TIMESTAMP=$(shell date --utc +%s)" (including quotes)
    #error Add this define to all build configurations.
#endif
/**
 * The define @c APP_BUILD_TIMESTAMP is re-calculated for each C file being compiled, and LPCXPresso caches
 * this value. Building from scratch can give different values per C file, and re-building without cleaning will give
 * the same value (still possibly different per C file).
 * @warning Do not access @c APP_BUILD_TIMESTAMP from any other location. Use @c sArmBuidTimestamp instead.
 */
static uint32_t sArmBuildTimestamp = APP_BUILD_TIMESTAMP;

__attribute__ ((section(".noinit"))) __attribute__((aligned (4)))
static ALON_T sAlon;

__attribute__ ((section(".noinit"))) __attribute__((aligned (4)))
static volatile MEMORY_CONFIG_T sConfig;

/**
 * Dummy variable to test the value of #MEMORY_FIRSTUNUSEDEEPROMOFFSET.
 * If the macro is not correct, the dummy variable will have a negative array size and the compiler will raise an error
 * similar to:
 *   ../src/memory.c:71:13: error: size of array 'sTestLastusedoffset' is negative
 */
static char sTestLastusedoffset[(EEPROM_OFFSET_CONFIG + sizeof(MEMORY_CONFIG_T) == MEMORY_FIRSTUNUSEDEEPROMOFFSET) - 1] __attribute__((unused));

/* ------------------------------------------------------------------------- */

bool Memory_Init(void)
{
    bool accepted;

    Chip_PMU_GetRetainedData((uint32_t *)&sAlon, 0, ALON_WORD_SIZE);
    Chip_EEPROM_Init(NSS_EEPROM);
    {
        uint32_t eepromBuildTimestamp;
        Chip_EEPROM_Read(NSS_EEPROM, EEPROM_OFFSET_BUILDTIMESTAMP, &eepromBuildTimestamp, sizeof(uint32_t));
        if (eepromBuildTimestamp != sArmBuildTimestamp) {
            Chip_EEPROM_Write(NSS_EEPROM, EEPROM_OFFSET_BUILDTIMESTAMP, &sArmBuildTimestamp, sizeof(uint32_t));
            Chip_EEPROM_Memset(NSS_EEPROM, EEPROM_OFFSET_CONFIG, 0, sizeof(MEMORY_CONFIG_T));
            Memory_ResetConfig();
            Storage_Init();
            /* When no build timestamp was found (reading out only zeros) the assumption is made the IC was still fresh,
             * and this is the first run after the first time the IC is flashed.
             * In that case, the NVM memory is still empty, so there is no need to check each and every word in FLASH.
             */
            Storage_Reset(eepromBuildTimestamp != 0);
            accepted = false;
        }
        else {
            Chip_EEPROM_Read(NSS_EEPROM, EEPROM_OFFSET_CONFIG, (void *)&sConfig, sizeof(MEMORY_CONFIG_T));
            Storage_Init();
            accepted = true;
        }
    }

    return accepted;
}

void Memory_DeInit(void)
{
    Storage_DeInit();
    MEMORY_CONFIG_T storedConfig;
    Chip_EEPROM_Read(NSS_EEPROM, EEPROM_OFFSET_CONFIG, &storedConfig, sizeof(MEMORY_CONFIG_T));
    if (memcmp((void *)&sConfig, &storedConfig, sizeof(MEMORY_CONFIG_T))) {
        Chip_EEPROM_Write(NSS_EEPROM, EEPROM_OFFSET_CONFIG, (void *)&sConfig, sizeof(MEMORY_CONFIG_T));
    }
    Chip_EEPROM_DeInit(NSS_EEPROM);
    Chip_PMU_SetRetainedData((uint32_t *)&sAlon, 0, ALON_WORD_SIZE);
}

const MEMORY_CONFIG_T * Memory_GetConfig(void)
{
    return (const MEMORY_CONFIG_T *)&sConfig;
}

void Memory_ResetConfig(void)
{
    memset((void *)&sConfig, 0, sizeof(MEMORY_CONFIG_T));
    sConfig.attainedMinimum = 1000;
    sConfig.attainedMaximum = -1000;
    sConfig.valid = true;
}

void Memory_SetConfigTime(uint32_t time)
{
    sConfig.time = time;
}

void Memory_SetConfigAttainedValue(int value)
{
    if (value < sConfig.attainedMinimum) {
        sConfig.attainedMinimum = value;
    }
    if (value > sConfig.attainedMaximum) {
        sConfig.attainedMaximum = value;
    }
}

void Memory_SetValid(bool valid)
{
    sConfig.valid = valid;
}

void Memory_SetBod(bool bod)
{
    sConfig.bod = bod;
}

void Memory_SetConfigValidInterval(int16_t validMinimum, int16_t validMaximum)
{
    sConfig.validMinimum = validMinimum;
    sConfig.validMaximum = validMaximum;
}

void Memory_SetConfigSleepTime(uint16_t sleepTime, bool limitCount)
{
    sConfig.sleepTime = sleepTime;
    if (sConfig.sleepTime == 0) {
        /* When no longer measuring, also reset maxSampleCount */
        sConfig.maxSampleCount = 0;
    }
    else if (limitCount) {
        sConfig.maxSampleCount = (uint16_t)(30 * 60 / sConfig.sleepTime);
        if (sConfig.maxSampleCount > 1024) {
            sConfig.maxSampleCount = 1024;
        }
        else if (sConfig.maxSampleCount < 42) {
            sConfig.maxSampleCount = 42;
        }
    }
}

void Memory_SetLogging(bool logging, bool markFull)
{
    sAlon.uninterrupted = logging;
    if (markFull) {
        sConfig.maxSampleCount = 0xFFFF;
    }
}

bool Memory_IsLogging(void)
{
    return sAlon.uninterrupted && (sConfig.sleepTime > 0);
}
