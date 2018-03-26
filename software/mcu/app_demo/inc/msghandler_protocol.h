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


#ifndef __MSGHANDLER_PROTOCOL_H_
#define __MSGHANDLER_PROTOCOL_H_

/** @defgroup APP_DEMO_TLOGGER_MSGHANDLER_PROTOCOL Protocol Extension
 * @ingroup APP_DEMO_TLOGGER_MSGHANDLER 
 *  This file describes the application specific commands and responses used in the Temperature Logger demo application.
 * @{
 */

#include "msg/msg.h"

/* -------------------------------------------------------------------------------- */

/**
 * The temperature sensor was already in use. Wait, then try again.
 * The time to wait is dependent on the resolution of the temperature conversion currently in progress - see
 * #TSEN_RESOLUTION_T - and is thus at most 100 ms.
 */
#define APP_MSG_ERR_TSEN 0x1000E

/**
 * The maximum temperature the application can handle. This is a result of the limitations of the IC (-40:+85C), the
 * limitations of the battery (say, -30:+50C), and the requirements of the use case.
 * Since this is a @b demo, the maximum temperature the IC validated for is chosen.
 * - Its negative value is used as an absolute minimum value.
 * - Measured temperatures are clamped in the range [MIN-MAX].
 * - The number of bits used to store the values in NVM is derived from this value: @c log_2(MAX): see #STORAGE_BITSIZE.
 * - Expressed in deci-degrees.
 */
#define APP_MSG_MAX_TEMPERATURE 850

/**
 * A value used to indicate either
 * - a value above #APP_MSG_MAX_TEMPERATURE or below -#APP_MSG_MAX_TEMPERATURE was measured. The measurement value was
 *  replaced with this value. Or
 * - a measurement was due while an NFC field was present. The measurement did not take place and this value was stored
 * instead.
 */
#define APP_MSG_TEMPERATURE_PLACEHOLDER_VALUE (APP_MSG_MAX_TEMPERATURE + 1)

/**
 * Supported messages
 */
typedef enum APP_MSG_ID {
    /**
     * @c 0x46 @n
     * Retrieves (part of) the stored measurements, that were taken after the last #APP_MSG_ID_SETCONFIG command.
     * @param APP_MSG_CMD_GETMEASUREMENTS_T
     * @return #MSG_RESPONSE_RESULTONLY_T if the command could not be handled;
     *  #APP_MSG_RESPONSE_GETMEASUREMENTS_T otherwise.
     * @note synchronous command
     * @note All temperatures retrieved are within the range [-#APP_MSG_MAX_TEMPERATURE; +#APP_MSG_MAX_TEMPERATURE].
     *  There is one special value, #APP_MSG_TEMPERATURE_PLACEHOLDER_VALUE, that is used to indicate anomalies.
     */
    APP_MSG_ID_GETMEASUREMENTS = 0x46,

    /**
     * @c 0x48 @n
     * Retrieves all configuration parameters, and the number of measurements available.
     * @param none
     * @return #APP_MSG_RESPONSE_GETCONFIG_T.
     * @note synchronous command
     * @note The response of this command will be buffered by the application in addition to being sent out immediately.
     *   It will then also be placed in the same NFC message that contains the response for the
     *   #MSG_ID_GETVERSION command.
     */
    APP_MSG_ID_GETCONFIG = 0x48,

    /**
     * @c 0x49 @n
     * Sets all configuration parameters, and clears the buffer holding the measurements.
     * @param APP_MSG_CMD_SETCONFIG_T
     * @return #MSG_RESPONSE_RESULTONLY_T.
     * @note synchronous command
     */
    APP_MSG_ID_SETCONFIG = 0x49,

    /**
     * @c 0x50 @n
     * Measures the temperature using the built-in temperature sensor.
     * @param APP_MSG_CMD_MEASURETEMPERATURE_T
     * @return #MSG_RESPONSE_RESULTONLY_T immediately; @n
     *  If @c result was equal to #MSG_OK, #APP_MSG_RESPONSE_MEASURETEMPERATURE_T thereafter. This may take up to
     *  100 ms. This second response must be fetched by issuing a command with #MSG_ID_GETRESPONSE.
     * @note asynchronous command
     */
    APP_MSG_ID_MEASURETEMPERATURE = 0x50
} APP_MSG_ID_T;

/* -------------------------------------------------------------------------------- */

#pragma pack(push, 1)

/** @see APP_MSG_ID_GETMEASUREMENTS */
typedef struct APP_MSG_CMD_GETMEASUREMENTS_S {
    /**
     * Unit: number of samples.
     * @li A value of 0 returns the oldest samples
     * @li Any other value denotes the number of old samples to skip.
     * @note This command has to be issued multiple times using different offsets, each time retrieving part of the
     *   stored samples.
     */
    uint16_t offset;
} APP_MSG_CMD_GETMEASUREMENTS_T;

/** @see APP_MSG_ID_SETCONFIG */
typedef struct APP_MSG_CMD_SETCONFIG_S {
    /**
     * The absolute current time in epoch seconds.
     */
    uint32_t currentTime;

    /**
     * The time between two measurements, set in seconds.
     * @note: A value of 0 disables taking measurements: after entering a power save mode,
     *   the chip can then only wake up when an NFC field is present.
     */
    uint16_t interval;

    /**
     * A validity constraint. The minimum value in deci-Celsius degrees for each measure temperature value.
     * - The entire batch of stored samples is considered valid if all samples comply with this constraint.
     * - The entire batch of stored samples is considered invalid if one or more samples do not comply with this
     *  constraint.
     * .
     * @note If @c validMinimum is not less than @c validMaximum, all samples are considered valid.
     */
    int16_t validMinimum;

    /**
     * A validity constraint. The minimum value in deci-Celsius degrees for each measure temperature value.
     * - The entire batch of stored samples is considered valid if all samples comply with this constraint.
     * - The entire batch of stored samples is considered invalid if one or more samples do not comply with this
     *  constraint.
     * .
     * If @c validMinimum is not less than @c validMaximum, all samples are considered valid.
     */
    int16_t validMaximum;

    /**
     * The number of samples that may be taken between the presence of two NFC fields can be limited.
     * - If the value equals 0, the number of samples taken will not be limited.
     * - If the value equals 1, the number of samples taken will be limited.
     *  The precise number is calculated based on the given interval. A minimum number of samples will be taken.
     *  When that minimum is reached, is is further clamped to at most half an hour or a large number of samples,
     *  whichever is reached first.
     * .
     * When the calculated number of samples are stored, the device will go to deep power down indefinitely, not waking
     * up any more due to an RTC timeout.
     * - When an NFC field is detected again before this number of samples are taken, this number of counts are taken
     *  again when the nfc field disappears.
     * - When an NFC field is detected again after this number of samples are taken, a new #APP_MSG_ID_SETCONFIG
     *  command is necessary to take new measurements.
     * .
     */
    uint8_t limitCount;
} APP_MSG_CMD_SETCONFIG_T;

/** @see APP_MSG_ID_MEASURETEMPERATURE */
typedef struct APP_MSG_CMD_MEASURETEMPERATURE_S {
    uint8_t resolution; /**< Type: #TSEN_RESOLUTION_T */
} APP_MSG_CMD_MEASURETEMPERATURE_T;

/* ------------------------------------------------------------------------- */

/** @see APP_MSG_ID_GETMEASUREMENTS */
typedef struct APP_MSG_RESPONSE_GETMEASUREMENTS_S {
    /**
     * The command result.
     * Only when @c result equals #MSG_OK, the contents of @c data is valid.
     */
    uint32_t result;

    /**
     * Unit: number of samples.
     * Defines the sequence number of the first data value that follows.
     * @note This may differ from the offset value given in the corresponding command: #APP_MSG_CMD_GETMEASUREMENTS_T.offset
     */
    uint16_t offset;

    /**
     * The number of values that follow. This number can be @c 0.
     * Immediately following this structure - no implicit padding bytes added - is an array of @c count elements, each
     * element 16 bits wide. Each element holds one measurement value in deci-Celsius degrees.
     * The total size of the response is thus variable and equals
     * @code sizeof(APP_MSG_RESPONSE_GETMEASUREMENTS_T) + sizeof(int16_t) * count @endcode.
     */
    uint8_t count;

    /**
     * Padding bytes. Must be @c 0.
     * @note Added to ensure size is a multiple of 2. Added solely to ease ARM SW development, and left at 3 for
     *  backwards compatibility.
     */
    uint8_t zero[3];

    //int16_t data[count];
} APP_MSG_RESPONSE_GETMEASUREMENTS_T;

/** @see APP_MSG_ID_GETCONFIG */
typedef struct APP_MSG_RESPONSE_GETCONFIG_S {
    /**
     * The command result.
     * Only when @c result equals #MSG_OK, the contents below this field are valid.
     */
    uint32_t result;

    /** The value as set by a previous command #APP_MSG_ID_SETCONFIG, with @c 0 as default value. */
    uint32_t configTime;

    /** The value as set by a previous command #APP_MSG_ID_SETCONFIG, with @c 0 as default value. */
    uint16_t interval;

    /**
     * The number of samples that will be taken in total before measurements will stop.
     * There are two special values:
     * - @c 0: The minimum value @c 0 indicates no limitation was in effect.
     * - @c 0xFFFF: The maximum value @c 0xFFFF indicates an error occurred: either the IC was reset unexpectedly,
     *  or storing a temperature value failed - with as most likely cause that the assigned storage space is completely
     *  full. This indicates logging has stopped and the device went to power-off mode (see also @c currentTime below).
     * .
     * @note Default value: 0
     */
    uint16_t maxCount;

    /** The value as set by a previous command #APP_MSG_ID_SETCONFIG, with @c 0 as default value. */
    int16_t validMinimum;

    /** The value as set by a previous command #APP_MSG_ID_SETCONFIG, with @c 0 as default value. */
    int16_t validMaximum;

    /**
     * The number of measurements samples available.
     */
    uint16_t count;

    /**
     * Indicates the validity of the batch of samples:
     * @li It will be 0 when at least one sample violated the constraints.
     * @li It will be 1 when all samples comply with the constraints.
     */
    uint8_t valid;

    /**
     * The current time. This is the time as set with the last command with id #APP_MSG_ID_SETCONFIG, incremented
     * with the number of seconds that have elapsed since as reported by the RTC block. Comparing this value with the
     * actual current time gives you the exact time deviation in seconds.
     * @note This value may be zero, or have any values less than @c configTime. This indicates logging has stopped and
     *  the device went to power-off mode or has experienced a power loss.
     */
    uint32_t currentTime;

    /** @} */
} APP_MSG_RESPONSE_GETCONFIG_T;

/** @see APP_MSG_ID_MEASURETEMPERATURE */
typedef struct APP_MSG_RESPONSE_MEASURETEMPERATURE_S {
    /**
     * The command result.
     * Only when @c result equals #MSG_OK, the contents of @c data is valid.
     */
    uint32_t result;

    int16_t temperature; /**< The measured temperature in deci-Celsius degrees. */
} APP_MSG_RESPONSE_MEASURETEMPERATURE_T;

#pragma pack(pop)

#endif /** @} */
