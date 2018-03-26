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
#include "storage/storage.h"
#include "text.h"

/* ------------------------------------------------------------------------- */

#define EMPTY_STRING "Logging has not yet started.                  " /**< Retain the spaces, so we only need to copy. */
#define ALERT_STRING "ALERT: out-of-bounds detected."

#define STATUS_POS 0 /**< Start position in #gAppStrText where default value or #EMPTY_STRING resides. */
#define STATUS_COUNT_POS 4 /**< Position of least significant digit of total sample count */
#define STATUS_MINIMUM_POS 28 /**< Position of least significant digit of the minimal attained temperature. */
#define STATUS_MAXIMUM_POS 39 /**< Position of least significant digit of the maximal attained temperature. */
#define STATUS_LENGTH 46 /**< Length in bytes of default value and of #EMPTY_STRING, excluding the NUL byte */

#define ALERT_POS (STATUS_POS + STATUS_LENGTH + 1) /**< Start position in #gAppStrText if #ALERT_STRING must be placed. */
#define ALERT_LENGTH 30 /**< Excluding the NUL byte */

/**
 * Required buffer size to give to #Temperature2String.
 * This excludes a possible trailing @c NUL byte, but includes the unit (C).
 * @note Example: -12.3C
 */
#define TEMPERATURE_STRING_BUFFER_SIZE 6

#define CURRENT_POS (ALERT_POS + ALERT_LENGTH + 1 + 1) /**< Start position in #gAppStrText for the temperature value. */
#define CURRENT_TEMPERATURE_POS (ALERT_POS + ALERT_LENGTH + 1 + 1 + 21) /**< Start position in #gAppStrText for the temperature value. */
#define CURRENT_TEMPERATURE_STRING_DECIMAL_POINT_POS 3
#define CURRENT_LENGTH 27 /**< Excluding the NUL byte */

#if TEXT_STATUS_LENGTH != (STATUS_LENGTH + 1 + ALERT_LENGTH + 1 + 1 + CURRENT_LENGTH + 1)
    #error TEXT_STATUS_LENGTH is not defined correctly.
#endif

static char sStatus[TEXT_STATUS_LENGTH] =
    "    0 values logged between        and       .\n                              \n\nCurrent temperature:       ";
/*  "12345 values logged between -12.3C and -45.6C.\nALERT: out-of-bounds detected.\n\nCurrent temperature: -78.9C"
 *  "Logging has not yet started.                  \n                              \n\nCurrent temperature: -78.9C"
 *   0123456789012345678901234567890123456789012345 6789012345678901234567890123456 7 89012345678901234567890123456
 *   0         1         2         3         4         5          6         7         8         9           0
 */

static void Temperature2String(int temperature, char * string);

/* ------------------------------------------------------------------------- */

/**
 * Writes a human-readable version of a temperature value to the NFC buffer
 * @param value Temperature value, in deci-degrees celsius
 * @param string May not be @c NULL. Must have storage for at least @ref TEMPERATURE_STRING_BUFFER_SIZE bytes.
 */
static void Temperature2String(int temperature, char * string)
{
    int pos;
    bool positive = true;

    memset(string, ' ', TEMPERATURE_STRING_BUFFER_SIZE);
    string[TEMPERATURE_STRING_BUFFER_SIZE - 1] = 'C';

    if (temperature < 0) {
        positive = false;
        temperature *= -1;
    }
    pos = CURRENT_TEMPERATURE_STRING_DECIMAL_POINT_POS + 1;
    while (temperature || (pos >= CURRENT_TEMPERATURE_STRING_DECIMAL_POINT_POS - 1)) {
        if (pos == CURRENT_TEMPERATURE_STRING_DECIMAL_POINT_POS) {
            string[pos] = '.';
        }
        else {
            string[pos] = (uint8_t)('0' + (temperature % 10));
            temperature /= 10;
        }
        pos--;
    }
    if (!positive) {
        string[pos] = '-';
    }
}

/* ------------------------------------------------------------------------- */

const char * Text_GetState(void)
{
    return sStatus;
}

void Text_SetState(int temperature, bool valid, int minimum, int maximum)
{
    Temperature2String(temperature, &sStatus[CURRENT_TEMPERATURE_POS]);

    int count = Storage_GetCount();
    if (count) {
        int pos = STATUS_COUNT_POS;
        int x = count;
        while (x) {
            sStatus[pos] = (uint8_t)('0' + (x % 10));
            pos--;
            x /= 10;
        }

        Temperature2String(minimum, &sStatus[STATUS_MINIMUM_POS]);
        Temperature2String(maximum, &sStatus[STATUS_MAXIMUM_POS]);

        if (!valid) {
            const char alert[] = ALERT_STRING;
            strncpy(sStatus + ALERT_POS, alert, ALERT_LENGTH);
        }
    }
    else {
        const char empty[] = EMPTY_STRING;
        strncpy(sStatus + STATUS_POS, empty, STATUS_LENGTH);
    }
}
