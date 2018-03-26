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


#ifndef VALIDATE_H_
#define VALIDATE_H_

#include <stdint.h>
#include <stdbool.h>

/* -------------------------------------------------------------------------------- */

/**
 * Initializes file scoped variables.
 * @pre Must be the first function called in this file.
 * @pre Must be called before changes to state and/or configuration are to be made.
 */
void Validate_Init(void);

/**
 * To be called once @b per active lifetime: when monitoring - of whatever - starts, this call ensures the validation
 * starts with a good known state, using initial values.
 * @note Do @b not call this function each time after starting up or waking up from a low power state, as it will reset
 *  all intermediate validation data and conclusions.
 * @warning This call may take a long time to complete. See the specific implementation for details.
 */
void Validate_Reset(void);

/**
 * Checks the data from #Memory_GetConfig and takes proper actions based on the updated data.
 * @note May be called multiple times. The implementation is robust against superfluous calls.
 * @param temperature The last temperature that was measured.
 * @warning This call may take a long time to complete. See the specific implementation for details.
 */
void Validate_Temperature(int16_t temperature);

/**
 * When the BOD has been fired, call this function to ensure the algorithm takes this event properly into account.
 * @note May be called multiple times. The implementation is robust against superfluous calls.
 * @param bod @c true when the Brown-out detector indicated a low voltage; @c false otherwise.
 * @warning This call may take a long time to complete. See the specific implementation for details.
 */
void Validate_Bod(bool bod);

#endif
