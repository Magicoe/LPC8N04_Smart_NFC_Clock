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
 * @defgroup MODS_NSS_UARTTX_DFT Diversity settings
 * @ingroup MODS_NSS_UARTTX
 * These 'defines' capture the diversity settings of the module. The displayed values refer to the default settings.
 * To override the default settings, place the defines with their desired values in the application app_sel.h header
 * file: the compiler will pick up your defines before parsing this file.
 * @{
 */
#ifndef __UARTTX_DFT_H_
#define __UARTTX_DFT_H_

/**
 * By default, the uart module will use two stop bits.
 * Explicitly define @c UARTTX_STOPBITS to @c 1 to end each byte with only 1 stopbit.
 * @note If not defined, or if equal to any value different from @c 1, @c 2 stop bits will be used.
 */
#if !defined(UARTTX_STOPBITS)
    #define UARTTX_STOPBITS 2
#endif

/**
 * The uart module will use @c 9600 as default bit rate, assuming the default system clock of 500 kHz.
 * Explicitly define @c UARTTX_BITRATE to switch to a higher bit rate.
 * @warning
 *  Ensure valid bit rate/system clock combinations as per @ref NSS_CLOCK_RESTRICTIONS "SW Clock Restrictions" and
 *  @ref SSPClockRates_anchor "SSP clock rates" are followed. Uart module does not perform any check for this.
 */
#if !defined(UARTTX_BITRATE)
    #define UARTTX_BITRATE 9600
#endif

#endif /** @} */
