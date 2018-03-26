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


#ifndef __MSGHANDLER_H_
#define __MSGHANDLER_H_

/** @defgroup APP_DEMO_TLOGGER_MSGHANDLER Message Handler Module
 * @ingroup APP_DEMO_TLOGGER
 * The Temperature Logger Message Handler is responsible for handling the communication with the host
 * (tag reader/smartphone).
 *  -# It makes use of:
 *      - @ref NFC_NSS as its communication channel.
 *      - @ref TIMER_NSS (Timer 0) to implement a Host-Timeout detection mechanism.
 *      - @ref MODS_NSS_MSG module to implement a command/response mechanism.
 *      - @ref MODS_NSS_NDEFT2T to generate/parse NDEF formatted messages.
 *      .
 *  -# The supported command ID's are described by #APP_MSG_ID_T enum
 *  -# The specifics of the protocol are described in the documentation section of the @ref MODS_NSS_MSG module
 *  -# The content of the commands and responses defined by the Therapy Adherence demo application
 *      are described by @ref APP_DEMO_TLOGGER_MSGHANDLER_PROTOCOL.
 *  -# It is also responsible for sending and receiving the data to the underlying physical communication channel
 *      (NFC interface).
 *  .
 * @{
 */

#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------------- */

/**
 * Initializes the messaging part of the application; initializes the msg mod;
 * Ensures an initial ndef message is loaded in the NFC shared memory.
 * @param reuseKeys @c True when previously set keys - if any - are to be reused; @c false otherwise.
 */
void AppMsgInit(bool reuseKeys);

/**
 * Wrapper round #Msg_HandleCommand
 * @pre AppMsgInit must have been called beforehand
 * @param cmdLength : The size in bytes in @c cmdData
 * @param cmdData : Pointer to the array containing the raw command bytes.
 */
void AppMsgHandleCommand(int cmdLength, const uint8_t* cmdData);

/**
 * @note It is expected this function only to be called as the last step in the process initiated by sending a
 * @param success The command result. Only when @c success equals @c true, the contents of @c temperature must be valid.
 * @param temperature The measured temperature in deci-Celsius degrees
 */
void AppMsgHandlerSendMeasureTemperatureResponse(bool success, int16_t temperature);

#endif /** @} */
