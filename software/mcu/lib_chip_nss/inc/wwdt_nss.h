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


#ifndef __WWDT_NSS_H_
#define __WWDT_NSS_H_

/** @defgroup WWDT_NSS wwdt: Windowed Watchdog Timer driver
 * @ingroup DRV_NSS
 * This driver provides APIs for the configuration and operation of the WWDT hardware block. The WWDT driver
 * provides the following functionalities:
 *  -# Setting up operation mode (Interrupt mode or Reset mode) and WWDT expiry timeout
 *  -# Starting and Feeding the WWDT
 *  -# Checking and clearing timeout status flag
 *  .
 *
 * The WWDT driver provides APIs to control a special timer which can perform the configured action on expiry. This can
 * be used to recover from an anomaly in software: for example an infinite loop or an endless wait for an event or
 * interrupt.
 * The WWDT can be configured to work in 2 modes as given below.
 *  - Reset mode: This mode is the normal intended operation mode and causes an unpreventable chip reset on WWDT expiry.
 *  .
 *      @warning The watchdog reset status is cleared on system reset. It is thus not possible to distinguish between a
 *          hard reset after toggling the RESETN pin and a WWDT timeout.
 *
 *  - Interrupt mode: This mode can be used for debugging purposes without actually resetting the device on WWDT expiry.
 *      The WWDT interrupt flag cannot be cleared by SW after a WWDT expiry: the firmware must disable the WWDT
 *      interrupt inside the interrupt handler using #NVIC_DisableIRQ with #WDT_IRQn as argument.
 *  .
 *      @warning Using the interrupt mode will not protect against hangs inside an interrupt handler which has a higher
 *          priority. See #IRQn_Type for a full list of interrupts and their priority. 
 *
 * @anchor WWDTClockingAndTimeout_anchor
 * @par Deriving WWDT timeout value:
 *  The WWDT has a fixed pre-scaler of 4 which is driven by the WWDT clock (WDCLK). Thus, there will be one tick every
 *  4 WWDT clock cycles. The WWDT clock is derived from the SFRO. Refer to the Clock driver documentation @ref CLOCK_NSS
 *  for more details on how to configure the WWDT clock frequency. The minimum and maximum timeout values are @c 0xFF
 *  and @c 0x00FFFFFF respectively and the timeout granularity or duration of one tick is 4/WDCLK seconds.
 *  @note The WWDT clock frequency shall be selected such that the restrictions for the clock divider as required by
 *      #Chip_Clock_Watchdog_SetClockFreq API are met. Refer to @ref CLOCK_NSS for more details.
 *  @note Setting the frequency to 0 (WWDT clock divider = 0) using #Chip_Clock_Watchdog_SetClockFreq or changing the
 *      clock source can stop the WWDT. 
 *
 * @par To configure and start the WWDT:
 *  -# Set the WWDT clock source using #Chip_Clock_Watchdog_SetClockSource to #CLOCK_WATCHDOGSOURCE_SFRO
 *  -# Set WWDT clock frequency using #Chip_Clock_Watchdog_SetClockFreq
 *  -# Initialise WWDT using #Chip_WWDT_Init
 *  -# Set timeout value using #Chip_WWDT_SetTimeOut
 *  -# If using reset mode, set option to reset the chip on timeout using #Chip_WWDT_SetOption
 *  -# If using interrupt mode, enable the WWDT interrupt in NVIC using #NVIC_EnableIRQ
 *  -# Start the WWDT activity using #Chip_WWDT_Start
 *  .
 *
 * @par To prevent the WWDT from timing out:
 *  -# Call #Chip_WWDT_Feed periodically within the configured timeout period
 *  .
 *
 * @par Example - WWDT in interrupt mode
 *  - Timeout: 0.8 seconds
 *  - WWDT Clock: 20 kHz
 *  .
 *
 *  Setup code:
 *  @snippet wwdt_nss_example_1.c wwdt_nss_example_1_setup
 *
 *  Main context:
 *  @snippet wwdt_nss_example_1.c wwdt_nss_example_1_main
 *
 *  WWDT Interrupt Handler:
 *  @snippet wwdt_nss_example_1.c wwdt_nss_example_1_irq
 *
 * @{
 */

/** WWDT register block structure */
typedef struct NSS_WWDT_S {
    __IO uint32_t MOD; /*!< WWDT mode register. This register contains the basic mode and status of the WWDT Timer. */
    __IO uint32_t TC; /*!< WWDT timer constant register. This register determines the time-out value. */
    __O uint32_t FEED; /*!< WWDT feed sequence register. Writing 0xAA followed by 0x55 to this register reloads the
                            WWDT timer with the value contained in WDTC. */
    __I uint32_t TV; /*!< WWDT timer value register. This register reads out the current value of the WWDT timer. */
} NSS_WWDT_T;


#define WWDT_WDMOD_BITMASK  ((uint32_t) 0x0FUL) /**< WWDT Mode Bitmask */
#define WWDT_WDMOD_WDEN     ((uint32_t) (1 << 0)) /**< WWDT enable bit */
#define WWDT_WDMOD_WDRESET  ((uint32_t) (1 << 1)) /**< WWDT reset enable bit */
#define WWDT_WDMOD_WDTOF    ((uint32_t) (1 << 2)) /**< WWDT time out flag bit */
#define WWDT_WDMOD_WDINT    ((uint32_t) (1 << 3)) /**< WWDT interrupt flag bit */

/**
 * Enables the WWDT clock and initialise the WWDT
 * @param pWWDT : The base address of the WWDT peripheral on the chip
 * @note Calling this function while the WWDT is running will set the timeout to default.
 */
void Chip_WWDT_Init(NSS_WWDT_T *pWWDT);

/**
 * Disables the WWDT clock and de-initialise the WWDT
 * @param pWWDT : The base address of the WWDT peripheral on the chip
 * @note This function can be called after WWDT has started running to disable the WDT clock and save power.
 */
void Chip_WWDT_DeInit(NSS_WWDT_T *pWWDT);

/**
 * Set WWDT timeout constant value used for feed
 * @param pWWDT : The base address of the WWDT peripheral on the chip
 * @param timeout : WWDT timeout in ticks, between 0xFF and 0xFFFFFF. Refer to @ref WWDTClockingAndTimeout_anchor for
 *                  more details.
 * @note: Passing timeout values below 0xFF will cause the timeout to be set to a default value of 0x0000 00FF.
 */
static inline void Chip_WWDT_SetTimeOut(NSS_WWDT_T *pWWDT, uint32_t timeout)
{
    pWWDT->TC = timeout;
}

/**
 * Feed WWDT timer.
 * @param pWWDT : The base address of the WWDT peripheral on the chip
 * @note This is a mandatory step after enabling the WWDT using #Chip_WWDT_SetOption
 * @note If this function isn't called after enabling the WWDT, WWDT will ignore timeout errors and will not generate
 *  a WWDT interrupt or reset the chip.
 */
static inline void Chip_WWDT_Feed(NSS_WWDT_T *pWWDT)
{
    __disable_irq();
    pWWDT->FEED = 0xAA;
    pWWDT->FEED = 0x55;
    __enable_irq();
}

/**
 * Enable WWDT timer options
 * @param pWWDT : The base address of the WWDT peripheral on the chip
 * @param options : Supply #WWDT_WDMOD_WDRESET here.
 * @note This function is applicable only for reset mode.
 */
static inline void Chip_WWDT_SetOption(NSS_WWDT_T *pWWDT, uint32_t options)
{
    pWWDT->MOD |= options;
}

/**
 * Enable WWDT activity
 * @param pWWDT : The base address of the WWDT peripheral on the chip
 */
static inline void Chip_WWDT_Start(NSS_WWDT_T *pWWDT)
{
    Chip_WWDT_SetOption(pWWDT, WWDT_WDMOD_WDEN);
    Chip_WWDT_Feed(pWWDT);
}

/**
 * Read WWDT status flag
 * @param pWWDT : The base address of the WWDT peripheral on the chip
 * @return WWDT status, an Or'ed value of WWDT_WDMOD_*
 * @note If #WWDT_WDMOD_WDEN or #WWDT_WDMOD_WDRESET are set, they cannot be unset by SW. They are cleared by reset or
 *  WWDT expiry.
 */
static inline uint32_t Chip_WWDT_GetStatus(NSS_WWDT_T *pWWDT)
{
    return pWWDT->MOD;
}

/**
 * Clear WWDT interrupt status flags
 * @param pWWDT : The base address of the WWDT peripheral on the chip
 * @param status : Or'ed value of status flag(s) that you want to clear, should be:
 *  - #WWDT_WDMOD_WDTOF
 *  .
 */
void Chip_WWDT_ClearStatusFlag(NSS_WWDT_T *pWWDT, uint32_t status);

/**
 * Get the current value of WWDT
 * @param pWWDT : The base address of the WWDT peripheral on the chip
 * @return Current value of WWDT
 * @note This function is useful only for debug purpose.
 * @note Due to the fact that the watchdog HW block is in a separate clock domain, the value reported by this function
 *  is older than the actual one. Retrieving this value takes up to 6 watchdog clock cycles + 6 system clock cycles
 *  due to the lock and synchronization procedure.
 */
static inline uint32_t Chip_WWDT_GetCurrentCount(NSS_WWDT_T *pWWDT)
{
    return pWWDT->TV;
}

/**
 * @}
 */

#endif /* __WWDT_NSS_H_ */
