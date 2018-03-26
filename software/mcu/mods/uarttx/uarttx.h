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


#ifndef __UARTTX_NSS_H_
#define __UARTTX_NSS_H_

/** @defgroup MODS_NSS_UARTTX uarttx: Uart Tx-only module
 * @ingroup MODS_NSS
 * The Uart Tx module provides a way to transmit byte sequences over the SSP interface using the UART protocol.
 * This module will hide all details of the SPI driver: it will initialize and configure the driver. The user of
 * the Uart module only needs to use the API functions below to have trace functionality over a UART interface.
 *
 * @par Diversity
 *  This module supports diversity, to ease the transmission of data according to each application's needs.
 *  Check @ref MODS_NSS_UARTTX_DFT for all diversity parameters.
 *
 * @par Setup
 *  - Connect the MOSI pin (PIO9) to the RX pin of e.g. an FTDI FT232R.
 *  .
 *
 * @par How to link Uart Tx module with Trace module?
 *  This module can be with the Trace module @ref MODS_NSS_TRACE using the #TRACE_DATAPIPE_CUSTOM. The following
 *  steps can be used to configure the Trace module @ref MODS_NSS_TRACE.
 *  - Use the trace custom data pipe by setting #TRACE_DATAPIPE_CUSTOM to 1.
 *  - Define TRACE_DATAPIPE_CUSTOM_INIT to use #UartTx_Init
 *  - Define TRACE_DATAPIPE_CUSTOM_WRITE to use #UartTx_Tx
 *  - Define TRACE_DATAPIPE_CUSTOM_DEINIT to use #UartTx_DeInit
 *  .
 *  Once the above diversity settings of Trace module (@ref MODS_NSS_TRACE_DFT) are set, Trace module @ref MODS_NSS_TRACE
 *  functions can be used to transmit data using UART protocol.
 *
 * @par Warning
 *  - With this module is in use, it is not possible to directly access the SPI driver for either transmission or
 *      reception, doing so will result in unforeseen behavior, possibly generating errors in the communication or a
 *      hard fault.
 *  - When including this module, the unused SSP pins (PIOs 2, 6 and 8) can still be used as GPIO.
 *  .
 *
 * @par Example 1
 *  @snippet uarttx_mod_example_1.c uarttx_mod_example_1
 *
 * @{
 */

#include "uarttx/uarttx_dft.h"

/**
 * Initializes the module.
 * @pre This is the module's first function to be called after startup or after #UartTx_DeInit has been called.
 * @pre The IOCON driver must have been initialized beforehand. This is already taken care of by the board library
 *  initialization function #Board_Init.
 * @pre The PIO9 pin is configured for the SSP MOSI functional mode (FUNC_1) using #Chip_IOCON_SetPinConfig.
 * @see Chip_IOCON_Init
 * @see pTrace_CustomInit_t
 */
void UartTx_Init(void);

/**
 * De-initializes the module.
 * @post De-initialize the IOCON driver if it is not used elsewhere.
 * @see pTrace_CustomDeInit_t
 */
void UartTx_DeInit(void);

/**
 * Transmits the given data over the MOSI line, mimicking the UART protocol.
 * This call is blocking until all data has been sent.
 * @param pData : Must point to a contiguous linear array containing the bytes to transmit.
 * @param length : The number of bytes to transmit.
 * @see MODS_NSS_UART_DFT
 * @see pTrace_CustomWrite_t
 */
void UartTx_Tx(const uint8_t * pData, int length);

/** @} */

#endif /* __UARTTX_NSS_H_ */
