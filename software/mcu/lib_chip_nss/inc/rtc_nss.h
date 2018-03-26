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


#ifndef __RTC_NSS_H_
#define __RTC_NSS_H_

#include <chip.h>

/** @defgroup RTC_NSS rtc: Real-Time Clock driver
 * @ingroup DRV_NSS
 * The Real-Time Clock (RTC) module has two main features:
 *  <dl><dt> Time up-counter </dt><dd>
 *      This records the time of day (e.g. seconds since epoch,
 *      which can be converted in year/month/day, hour/minute/second).</dd>
 *  <dt> Wake-up down-counter </dt><dd>
 *      This can 'wake-up' the chip from standby.
 *      Standby includes Sleep, Deep Sleep and Deep Power Down states.
 *      This counter counts down to zero.</dd></dl>
 *
 * Both the counters step on a "tick". A tick consists of a number of TFRO pulses.
 * Since the TFRO is calibrated to run at about 32768 Hz on reset, one tick is typically
 * about 32768 TFRO pulses and thus 1 second. The exact number of TFRO clocks to make 1 second 
 * is also checked during calibration and stored in the RTCCAL register.
 * The TFRO clock is enabled as the RTC block
 * clock source on a full system reset. The RTC block and the TFRO are in the
 * always-on power domain.
 *
 * @par Time up-counter
 *  As soon as the TFRO is connected, the "time up-counter"
 *  starts incrementing and does it even in the various standby modes.
 *  See @ref PMU_NSS for details on TFRO configuration.
 *
 * @par Wake-up down-counter
 *  The "wake-up down-counter" needs to be enabled and started to count. When it does,
 *  the counter counts even in the various standby modes. This allows the chip to
 *  'wake-up' from various modes of standby. A 'wake-up' event occurs when the
 *  "wake-up down-counter" reaches zero.
 * @par
 *  When the "wake-up down-counter" reaches zero, it generates an RTC interrupt that
 *  forks to two slots in the NVIC: the standard RTC peripheral interrupt #RTC_IRQn
 *  and the secondary interrupt #RTCPWREQ_IRQn via Start Logic. This can be used in
 *  active, sleep and deep sleep modes.
 *  See @ref CMSIS_Core_NVICFunctions "NVIC API" and @ref startLogic_anchor
 *  "System wake-up start logic" for more details.
 * @par
 *  In Deep Power Down mode, the IC will wake up unconditionally when the 
 *  "wake-up down-counter" is enabled and reaches zero. It is not required to 
 *  configure the NVIC and start logic for waking up from the Deep Power Down mode.
 *  When the chip wakes up from Deep Power Down on an
 *  RTC 'wake-up' event, the LDOs will be enabled again and the ARM will reboot.
 *  No interrupt function is called. The #RTC_INT_WAKEUP flag will be set.
 *  This indicates that RTC 'wake-up' event is the 'wake-up' reasons. That can be
 *  checked by the application program at startup.
 * @par
 *  Note that there can be other 'wake-up' reasons from various standby modes.
 *  See  @ref PMU_NSS for details.
 *
 * @par In order to operate the RTC module
 *  - Time Up-counter function:
 *      -# Use #Chip_RTC_Time_GetValue and #Chip_RTC_Time_SetValue to access "time up-counter" value.
 *      .
 *  - Wake-up down-counter function:
 *      -# Call #Chip_RTC_Wakeup_SetControl to operate "wake-up down-counter".
 *      -# Call #Chip_RTC_Wakeup_SetReload to set the "wake-up down-counter" ticks.
 *      -# Set #RTC_INT_WAKEUP with #Chip_RTC_Int_SetEnabledMask to enable
 *      -# Additional steps are necessary to operate the "wake-up down-counter" function with various states of standby.
 *      .
 *  .
 *  See the following examples, along with @ref PMU_NSS and @ref SYSCON_NSS drivers for more details.
 *
 * @anchor RTC_WARNING
 * @warning Each RTC block register read and write performs at least one @c wait period that may take up to 100us
 *  to complete due to hardware synchronization within the module. All the functions in this driver
 *  make at least one such register access unless stated otherwise. They impact performance and should be
 *  carefully considered especially when calling RTC driver functions in ISRs context and high-priority threads.
 *  If an RTC block register read/write preempts another RTC block register read/write, the procedure is repeated
 *  leading to another @c wait period, until no preemption is detected.
 *
 * @par Example 1 - RTC "wake-up down-counter" with Sleep Mode
 *  System Initialization (once on boot-up):
 *  @snippet rtc_nss_example_1.c rtc_nss_example_1
 *  Application enters Sleep mode:
 *  @snippet rtc_nss_example_1.c rtc_nss_example_1_b
 *  RTC interrupt handling:
 *  @snippet rtc_nss_example_1.c rtc_nss_example_1_irq
 *
 * @par Example 2 - RTC "wake-up down-counter" with Sleep Mode using alternative (without bus synchronization in) IRQ
 *  System Initialization (once on boot-up):
 *  @snippet rtc_nss_example_2.c rtc_nss_example_2
 *  Application enters Sleep mode:
 *  @snippet rtc_nss_example_2.c rtc_nss_example_2_b
 *  Alternative RTC interrupt handling:
 *  @snippet rtc_nss_example_2.c rtc_nss_example_2_irq
 *
 * @par Example 3: - RTC "wake-up down-counter" with Deep Sleep Mode, and manual RTC 'wake-up' start.
 *  This example is using @ref startLogic_anchor "System wake-up start logic" @n
 *  System Initialization (once on boot-up):
 *  @snippet rtc_nss_example_3.c rtc_nss_example_3
 *  Application enters Deep Sleep mode:
 *  @snippet rtc_nss_example_3.c rtc_nss_example_3_b
 *  RTC interrupt handling
 *  @snippet rtc_nss_example_3.c rtc_nss_example_3_irq
 *
 * @par Example 4: - RTC "wake-up down-counter" with Deep Power Down Mode
 *  System Initialization (once on boot-up):
 *  @snippet rtc_nss_example_4.c rtc_nss_example_4
 *  Application enters Deep Power Down mode:
 *  @snippet rtc_nss_example_4.c rtc_nss_example_4_b
 *  RTC interrupt service routine is not necessary with Deep Power Down mode as NVIC is not active.
 *  Instead, the application should check for #RTC_INT_WAKEUP signal as soon as the system starts up.
 * @{
 */

/** RTC block register block structure */
typedef struct NSS_RTC_S {
    __IO uint32_t   CR;         /*!< Control register */
    __IO uint32_t   SR;         /*!< Status register */
    __IO uint32_t   CAL;        /*!< TFRO counts per 'tick' calibration value */
    __IO uint32_t   SLEEPT;     /*!< "Wake-up down-counter" tick value - Amount of ticks until a 'wake-up' event */
    __I  uint32_t   VAL;        /*!< Current (remaining) "wake-up down-counter" tick value */
    __IO uint32_t   IMSC;       /*!< Interrupt mask set/clear register - Enable/disable interrupt */
    __I  uint32_t   RIS;        /*!< Raw Interrupt status register */
    __I  uint32_t   MIS;        /*!< Masked interrupt status register */
    __O  uint32_t   ICR;        /*!< Interrupt clear register */
    __I  uint32_t   ACCSTAT;    /*!< Access status register */
    __I  uint32_t   RESERVED[2];/* reserved */
    __IO uint32_t   TIME;       /*!< Real-time clock "time up-counter" value */
} NSS_RTC_T;


/**
 * RTC "wake-up down-counter" control. For use with #Chip_RTC_Wakeup_SetControl and #Chip_RTC_Wakeup_GetControl
 * @note
 *  - Counting of the "Wake-up down-counter" is enabled when control field is:
 *      - #RTC_WAKEUPCTRL_ENABLE | #RTC_WAKEUPCTRL_AUTO
 *      - #RTC_WAKEUPCTRL_ENABLE | #RTC_WAKEUPCTRL_START
 *      .
 *  - Clearing #RTC_WAKEUPCTRL_ENABLE halts the "wake-up down-counter"
 *  - When #RTC_WAKEUPCTRL_AUTO is set, #RTC_WAKEUPCTRL_START is ignored
 *  - When #RTC_WAKEUPCTRL_AUTO is unset, loading a new value with #Chip_RTC_Wakeup_SetReload clears
 *      #RTC_WAKEUPCTRL_START, and halts counting of the "Wake-up down-counter".
 *  .
 */
typedef enum RTC_WAKEUPCTRL {
    RTC_WAKEUPCTRL_ENABLE  = (1 << 0), /*!< Enables "wake-up down-counter"  */
    RTC_WAKEUPCTRL_AUTO    = (1 << 1), /*!< Enables the automatic start/restart of the "wake-up down-counter",
                                           each time a tick value is written in #NSS_RTC_T.SLEEPT with
                                           #Chip_RTC_Wakeup_SetReload(). */
    RTC_WAKEUPCTRL_START   = (1 << 2), /*!< Starts counting of "wake-up down-counter" with tick value loaded with
                                           #Chip_RTC_Wakeup_SetReload. */
    RTC_WAKEUPCTRL_DISABLE = 0, /*!< Disables "wake-up down-counter" and stops counting */
} RTC_WAKEUPCTRL_T;

/** RTC block interrupt bitfields */
typedef enum RTC_INT {
    RTC_INT_WAKEUP                          = (1 << 0), /*!< "wake-up down-counter" has reached 0 */
    RTC_INT_ALL                             = 0x01, /*!< All RTC interrupt bits */
    RTC_INT_NONE                            = 0, /*!< No RTC interrupt */
} RTC_INT_T;

/**
 * Enables ARM access to RTC block via APB bus
 * @param pRTC : The base address of the RTC peripheral on the chip
 * @note This RTC driver requires that TFRO clock connection to RTC is enabled.
 *  TFRO is connected to RTC by default after a full-system reset.
 */
void Chip_RTC_Init(NSS_RTC_T * pRTC);

/**
 * Halts the counting of "wake-up down-counter" and disables ARM access to RTC block via APB bus.\n
 * This function:
 *  -# Disables "wake-up down-counter" operation
 *  -# Disables "wake-up down-counter" interrupt
 *  -# Disables ARM access to RTC block via APB clock
 *  .
 * @param pRTC : The base address of the RTC peripheral on the chip
 * @warning This function performs two synchronized register accesses. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
void Chip_RTC_DeInit(NSS_RTC_T * pRTC);

/**
 * Sets the number of TFRO clock pulses in one RTC 'tick'
 * @param pRTC : The base address of the RTC peripheral on the chip
 * @param calibValue: 16-bit value indicating the number of TFRO clock pulses in one tick.
 * @warning This function performs a synchronized register access. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
void Chip_RTC_SetCalibration(NSS_RTC_T *pRTC, int calibValue);

/**
 * Returns the number of TFRO clock pulses in one RTC 'tick'
 * @param pRTC : The base address of the RTC peripheral on the chip
 * @return 16-bit value indicating the number of TFRO clock pulses in one tick.
 * @warning This function performs a synchronized register access. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
int Chip_RTC_GetCalibration(NSS_RTC_T *pRTC);


/**
 * Controls the operation of "wake-up down-counter"
 * @param pRTC : The base address of the RTC peripheral on the chip
 * @param control : Bitfield of Control Mode of type #RTC_WAKEUPCTRL_T
 * @see #RTC_WAKEUPCTRL_T for operation details
 * @note RTC interrupt MUST be enabled to wake up from various standby modes (Sleep/Deep Sleep/Deep Power Down).
 * @warning This function performs a synchronized register access. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
void Chip_RTC_Wakeup_SetControl(NSS_RTC_T * pRTC, RTC_WAKEUPCTRL_T control);

/**
 * Returns the control register of the "wake-up down-counter" operation
 * @param pRTC : The base address of the RTC peripheral on the chip
 * @return Bitfield of Control state.
 * @note #RTC_WAKEUPCTRL_START bit is cleared when "wake-up down-counter" reaches zero,
 *  or when a new tick is loaded with #Chip_RTC_Wakeup_SetReload without #RTC_WAKEUPCTRL_AUTO
 * @warning This function performs a synchronized register access. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
RTC_WAKEUPCTRL_T Chip_RTC_Wakeup_GetControl(NSS_RTC_T *pRTC);

/**
 * Sets the "wake-up down-counter" ticks.  This tick value is decremented until it reaches zero,
 * and the RTC interrupt is generated, thus a 'wake-up' event
 * @param pRTC : The base address of the RTC peripheral on the chip
 * @param ticks : 24bit unsigned number of ticks (seconds) to wake-up.
 *  Acceptable value from 0 to 16,777,215
 * @note RTC interrupt MUST be enabled to wake up from various standby modes (Sleep/Deep Sleep/Deep Power Down).
 * @note When #RTC_WAKEUPCTRL_AUTO bit is set, calling #Chip_RTC_Wakeup_SetControl starts the "wake-up down-counter".
 * @note Without #RTC_WAKEUPCTRL_AUTO bit, setting a "wake-up down-counter" value clears #RTC_WAKEUPCTRL_START.
 * @warning This function performs a synchronized register access. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
void Chip_RTC_Wakeup_SetReload(NSS_RTC_T *pRTC, int ticks);

/**
 * Returns the number of "wake-up down-counter" ticks (seconds) previously set by #Chip_RTC_Wakeup_SetReload
 * @param pRTC : The base address of the RTC peripheral on the chip
 * @return 24bit unsigned value of the number of ticks
 * @note This function does not return remaining ticks to a 'wake-up' event.
 * @warning This function performs a synchronized register access. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
int Chip_RTC_Wakeup_GetReload(NSS_RTC_T *pRTC);

/**
 * Returns the remaining ("wake-up down-counter") ticks until 'wake-up' event occurs
 * @param pRTC : The base address of the RTC peripheral on the chip
 * @return 24bit unsigned value of the remaining number of ticks to a 'wake-up' event
 * @warning This function performs a synchronized register access. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
int Chip_RTC_Wakeup_GetRemaining(NSS_RTC_T *pRTC);

/**
 * Indicates whether or not the "wake-up-down-counter" is in fact running
 * @param pRTC : The base address of the RTC peripheral on the chip
 * @return true if it is running or false otherwise.
 * @warning This function performs synchronized register accesses. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
bool Chip_RTC_Wakeup_IsRunning(NSS_RTC_T *pRTC);

/**
 * Returns the current 'tick' value of the "time up-counter"
 * @param pRTC : The base address of the RTC peripheral on the chip
 * @return Current ticks since the TFRO clock was enabled (unless updated via #Chip_RTC_Time_SetValue).
 * @note Note that HW returns 'tick' in 32bit of unsigned magnitude, type-casted and
 *  returned as signed 'int' in #Chip_RTC_Time_GetValue
 * @warning This function performs a synchronized register access. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
int Chip_RTC_Time_GetValue(NSS_RTC_T *pRTC);

/**
 * Sets a new 'tick' value to the "time up-counter". When using as an epoch, the number of seconds since
 * that epoch can be written to the "time up-counter", so that it maintains the current date/time.
 * @param pRTC : The base address of the RTC peripheral on the chip
 * @param tickValue: tick value to assign to "time up-counter". This value will be
 *  type-casted as unsigned 32-bit then applied to HW registers
 * @warning This function performs a synchronized register access. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
void Chip_RTC_Time_SetValue(NSS_RTC_T *pRTC, int tickValue);

/**
 * Enables/Disables the RTC interrupts.
 * @param pRTC : The base address of the RTC block on the chip
 * @param mask : Interrupt enabled mask to set
 * @note Interrupt MUST be enabled to wake up from Sleep/Deep Sleep states
 * @warning This function performs a synchronized register access. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
void Chip_RTC_Int_SetEnabledMask(NSS_RTC_T *pRTC, RTC_INT_T mask);

/**
 * Retrieves the RTC interrupt enabled mask.
 * @param pRTC : base address of the RTC block on chip.
 * @return Interrupt enabled mask
 * @warning This function performs a synchronized register access. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
RTC_INT_T Chip_RTC_Int_GetEnabledMask(NSS_RTC_T *pRTC);

/**
 * Retrieves a bitVector with the RAW RTC interrupt flags
 * @param pRTC : The base address of the RTC peripheral on the chip
 * @return BitVector with the RTC RAW interrupt flags
 * @note A bit set to 1 means that the correspondent interrupt flag is set.
 * @warning This function performs a synchronized register access. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
RTC_INT_T Chip_RTC_Int_GetRawStatus(NSS_RTC_T *pRTC);

/**
 * Clears the required RTC interrupt flags.
 * @param pRTC : The base address of the RTC peripheral on the chip
 * @param flags : Bitvector indicating which interrupt flags to clear
 * @warning This function performs a synchronized register access. Impact on runtime performance and
 *  other same and lower-priority contexts should be carefully considered. See @ref RTC_WARNING "warning section".
 */
void Chip_RTC_Int_ClearRawStatus(NSS_RTC_T *pRTC, RTC_INT_T flags);

/**
 * @}
 */

#endif /* __RTC_NSS_H_ */

