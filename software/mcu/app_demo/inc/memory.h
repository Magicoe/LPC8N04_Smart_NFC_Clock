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


#ifndef __MEMORY_H_
#define __MEMORY_H_

/**
 * @addtogroup APP_DEMO_TLOGGER_MEMORY Memory Manager
 * @ingroup APP_DEMO_TLOGGER
 *  The Temperature Logger Demo Memory Manager is responsible for managing data stored to and fetched from the ALON
 *  domain registers, the EEPROM and the FLASH memory.
 *  This component takes ownership of all three memory types and provides an API to store/read data.
 *
 * @par Assumptions:
 *  - The Memory component assumes that it has the exclusive use of all three memory types, but for reading and writing.
 *  .
 *
 *  @{
 */

#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------------- */

/**
 * The offset in EEPROM memory to the first unclaimed byte by this storage component.
 * The EEPROM region from the very first byte up to (not including) this offset is under full control of this component
 * and no other code is assumed to use it.
 */
#define MEMORY_FIRSTUNUSEDEEPROMOFFSET 28

/**
 * Defines the constant configuration under which to operate;
 * Also stores some data of the ongoing monitoring and logging session
 * @warning be sure to only list fields here that are updated very sparingly: each time these contents are changed,
 *  they will be written to EEPROM in #Memory_DeInit. EEPROM has a guaranteed endurance of 10000 writes (per page) only.
 */
typedef struct MEMORY_CONFIG_S {
    /** The absolute time as set by handling the command #APP_MSG_ID_SETCONFIG, and updated once the NFC field is gone. */
    uint32_t time;

    /** The time in seconds between two measurements in seconds as set by handling the command #APP_MSG_ID_SETCONFIG */
    uint16_t sleepTime;

    /** As set by handling the command #APP_MSG_ID_SETCONFIG. */
    int16_t validMinimum;

    /** As set by handling the command #APP_MSG_ID_SETCONFIG. */
    int16_t validMaximum;

    /**
     * The number of measurements to take before stopping automatically. (Re-)calculated each time an NFC field is
     * detected.
     * @note A value of 0 indicates no limit is in effect.
     */
    uint16_t maxSampleCount;

    /**
     * Indicates the validity of the batch of samples. May be set to @c false after each measurement taken.
     * Returned while handling the command #APP_MSG_ID_GETCONFIG.
     */
    bool valid;

    /**
     * Indicates whether the battery is nearly depleted - detected by the Brown-out detector.
     */
    bool bod;

    /** The absolute minimum value recorded in deci-Celsius degrees. */
    int attainedMinimum;

    /** The absolute minimum value recorded in deci-Celsius degrees. */
    int attainedMaximum;
} MEMORY_CONFIG_T;

/* ------------------------------------------------------------------------- */

/**
 * Initialization function. Must be called first in this component.
 * @return Whether the contents available in NVM were accepted. After flashing another image the contents are no longer
 *  accepted and a blank slate is used. In that case, @c false is returned. When @c true is returned, the contents
 *  that were set in NVM were read out and used.
 */
bool Memory_Init(void);

/**
 * De-Initializes the component.
 * - Must be called last in this component.
 * - Must be called before going to Power-off or Deep Power Down mode.
 * .
 */
void Memory_DeInit(void);

/* ------------------------------------------------------------------------- */

/**
 * Retrieve configuration.
 * @return A pointer to the filled-in MEMORY_CONFIG_T structure.
 * @note Multiple calls return the same pointer.
 */
const MEMORY_CONFIG_T * Memory_GetConfig(void);

/**
 * Resets all parameters to default values.
 * As a result, there are no limits, and no interval will be configured (#MEMORY_CONFIG_T.sleepTime).
 */
void Memory_ResetConfig(void);

/**
 * Updates the time in the configuration structure.
 * @param time The absolute current time in epoch seconds.
 * @note The change is made to the memory pointed to by #Memory_GetConfig.
 * @note Changes made are saved in NVM during #Memory_DeInit.
 */
void Memory_SetConfigTime(uint32_t time);

/**
 * Updates the attained extremities of the measured values. Does @b not check the validity of the batch of samples.
 * @param value The new value to compare against the current extremities.
 * @note The change is made to the memory pointed to by #Memory_GetConfig.
 * @note Changes made are saved in NVM during #Memory_DeInit.
 */
void Memory_SetConfigAttainedValue(int value);

/**
 * Updates the validity flag.
 * @param valid validity flag
 * @note The change is made to the memory pointed to by #Memory_GetConfig.
 * @note Changes made are saved in NVM during #Memory_DeInit.
 */
void Memory_SetValid(bool valid);

/**
 * Updates the Brown-out detector flag.
 * @param bod flag
 * @note The change is made to the memory pointed to by #Memory_GetConfig.
 * @note Changes made are saved in NVM during #Memory_DeInit.
 */
void Memory_SetBod(bool bod);

/**
 * Updates the minimum and maximum value all measurements should be in between.
 * @param validMinimum min
 * @param validMaximum max
 * @note The change is made to the memory pointed to by #Memory_GetConfig.
 * @note Changes made are saved in NVM during #Memory_DeInit.
 */
void Memory_SetConfigValidInterval(int16_t validMinimum, int16_t validMaximum);

/**
 * Updates the wake-up interval.
 * @param sleepTime The time between two measurements, set in seconds. A value of @c 0 disables logging.
 * @param limitCount If @c true, limit the number of samples that are to be taken. If that count is reached, no more
 *  samples need to be taken. The count is calculated using this rule: sample for half an hour, but ensure minimally
 *  42 samples and maximally 1024 samples are taken.
 * @note The change is made to the memory pointed to by #Memory_GetConfig.
 * @note Changes made are saved in NVM during #Memory_DeInit.
 */
void Memory_SetConfigSleepTime(uint16_t sleepTime, bool limitCount);

/* ------------------------------------------------------------------------- */

/**
 * @param logging
 *  - @c false Mark in the configuration the start of a new logging session.
 *  - @c true Mark in the configuration the end of an existing logging session.
 *  .
 * @param markFull When logging stopped due to an error, the special value @c 0xFFFF is assigned to @c maxSampleCount
 *  to indicate this.
 * @note The special value for @c markFull value is taken from the application specific response of
 *  #APP_MSG_ID_GETCONFIG in the communication protocol and is meant to be passed on unaltered to
 *  #APP_MSG_RESPONSE_GETCONFIG_T.maxCount
 */
void Memory_SetLogging(bool logging, bool markFull);

/**
 * Once started, logging continues until explicitly stopped, or until a reset condition occurs (Brown-out, reset pulse).
 * @return @c True if logging has not yet been interrupted since the start of the last logging session.
 * @see Memory_SetLogging
 */
bool Memory_IsLogging(void);

/** @} */
#endif
