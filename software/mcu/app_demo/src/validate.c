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


#include "board.h"
#include "memory.h"
#include "msghandler_protocol.h"
#include "validate.h"

/* ------------------------------------------------------------------------- */

void Validate_Init(void)
{
    /* A full blown application will likely want to prepare some event logging or to resurrect the previous state of its
     * validation algorithm.
     *
     * This can be implemented here in this function or in this file.
     */
}

void Validate_Reset(void)
{
    /* A full blown application will likely want to implement some event logging or to initialize its validation
     * algorithm.
     *
     * This can be implemented here in this function or in this file.
     */
}

void Validate_Temperature(int16_t temperature)
{
    if (temperature < APP_MSG_TEMPERATURE_PLACEHOLDER_VALUE) {
        Memory_SetConfigAttainedValue(temperature);
        const MEMORY_CONFIG_T * config = Memory_GetConfig();
        if (config->validMinimum < config->validMaximum) { /* if we have a valid constraint set */
            Memory_SetValid((config->validMinimum <= config->attainedMinimum)
                    && (config->attainedMaximum <= config->validMaximum));
        }
    }

    /* Nothing else to be done in this demo application. */

    /* A full blown application will likely want to implement a more complicated algorithm tailored to the product it
     * is attached to, to decide when to mark a monitoring session as invalid.
     * In addition, it might want to store additional data when this happens, or it might have an external indicator
     * attached to the NHS31xx IC which needs to be updated.
     *
     * This can be implemented here in this function or in this file.
     */
}

void Validate_Bod(bool bod)
{
    /* A full blown application will likely want to implement some event logging or to prepare for a nascent battery
     * depletion.
     *
     * This can be implemented here in this function or in this file.
     */
    (void)bod; /* suppress [-Wunused-parameter]: function implementation is a stub. */
}
