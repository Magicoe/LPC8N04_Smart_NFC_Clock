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


/**
 * @defgroup MODS_NSS_NDEFT2T_DFT Diversity Settings
 * @ingroup MODS_NSS_NDEFT2T
 * These 'defines' capture the diversity settings of the module. The displayed values refer to the default settings.
 * To override the default settings, place the defines with their desired values in the application app_sel.h header
 * file: the compiler will pick up your defines before parsing this file.
 *
 * @{
 */

#ifndef __NDEFT2T_DFT_H_
#define __NDEFT2T_DFT_H_

/**
 * Set this flag to '1' to enable support to copy record payload directly from EEPROM to message buffer and '0' to disable.
 */
#if !defined(NDEFT2T_EEPROM_COPY_SUPPPORT)
    #define NDEFT2T_EEPROM_COPY_SUPPPORT 1
#endif

/**
 * Set this flag to '1' to enable message header length correction and '0' to disable. When set to '1', it allows the
 * message to be created even if the argument 'shortMessage' in #NDEFT2T_CreateMessage API was set wrongly by the caller.
 * When set to '0' the message creation will fail and an error will be returned.
 */
#if !defined(NDEFT2T_MESSAGE_HEADER_LENGTH_CORRECTION)
    #define NDEFT2T_MESSAGE_HEADER_LENGTH_CORRECTION 1
#endif

/**
 * Set this flag to '1' to enable @ref colDetDesc_anchor "Shared memory access collision detection" and '0' to disable.
 */
#if !defined(NDEFT2T_COLLISION_DETECTION)
    #define NDEFT2T_COLLISION_DETECTION 0
#endif

/**
 * Number of tries for writing NDEF message into shared memory in case of
 * @ref colDetDesc_anchor "Shared memory access collision detection".
 */
#if !defined(NDEFT2T_WRITE_TRIES)
    #define NDEFT2T_WRITE_TRIES 1
#endif

/**
 * Number of tries for reading NDEF message from shared memory in case of
 * @ref colDetDesc_anchor "Shared memory access collision detection".
 */
#if !defined(NDEFT2T_READ_TRIES)
    #define NDEFT2T_READ_TRIES 1
#endif

/**
 * NDEFT2T MOD does interrupt handling by itself. So, the below callback shall be defined, to get notified on the NFC
 * field status. Refer @ref nfcIntHandling_anchor "NFC Interrupt Handling" for more details.
 * @note The value set @b must have the same signature as #pNdeft2t_FieldStatus_Cb_t.
 * @note This must be set to the name of a function, not a pointer to a function: no dereference will be made!
 */
#ifndef NDEFT2T_FIELD_STATUS_CB
    //#define NDEFT2T_FIELD_STATUS_CB your_callback
#endif

/**
 * The below callback shall be defined, for the application to get notified on the presence of a valid NDEF Message in
 * shared memory. Refer @ref nfcIntHandling_anchor "NFC Interrupt Handling" for more details.
 * @note The value set @b must have the same signatures as #pNdeft2t_MsgAvailable_Cb_t.
 * @note This must be set to the name of a function, not a pointer to a function: no dereference will be made!
 */
#ifndef NDEFT2T_MSG_AVAILABLE_CB
    //#define NDEFT2T_MSG_AVAILABLE_CB your_callback
#endif


#endif /** @} */
