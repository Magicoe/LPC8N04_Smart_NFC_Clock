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


#ifndef __APP_SEL_H_
#define __APP_SEL_H_

//#define SW_MAJOR_VERSION redefined in project settings per each build
#define SW_MINOR_VERSION 11

#define MSG_APP_HANDLERS App_CmdHandler
#define MSG_APP_HANDLERS_COUNT 4U
#define MSG_RESPONSE_BUFFER_SIZE 20 /**< A value large enough to store #APP_MSG_RESPONSE_MEASURETEMPERATURE_T - nothing else is buffered. */
#define MSG_RESPONSE_BUFFER App_ResponseBuffer
#define MSG_ENABLE_PREPAREDEBUG 1
#define MSG_ENABLE_GETUID 1

#define TMEAS_CB App_TmeasCb

#define NDEFT2T_EEPROM_COPY_SUPPPORT 0
#define NDEFT2T_FIELD_STATUS_CB NDEFT2T_FieldStatus_Cb
#define NDEFT2T_MSG_AVAILABLE_CB NDEFT2T_MsgAvailable_Cb

//#define STORAGE_TYPE int16_t
//#define STORAGE_BITSIZE 11 /**< round_up(log_2(2 * APP_MSG_MAX_TEMPERATURE)) */
//#define STORAGE_SIGNED 1
//#define STORAGE_EEPROM_FIRST_ROW (EEPROM_NR_OF_RW_ROWS - 3*16)
//#define STORAGE_COMPRESS_CB App_CompressCb
//#define STORAGE_DECOMPRESS_CB App_DecompressCb

#endif
