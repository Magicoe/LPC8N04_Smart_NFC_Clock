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


#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>
#include <stdbool.h>


/**
 * Initialize the timers so that the other function calls become available.
 * @pre This must be the first call to this block of code.
 */
void Timer_Init(void);

/* -------------------------------------------------------------------------------- */

/**
 * Starts or restarts a timer.
 * @note The 16-bit timer is used. It will run as slow as possible.
 * @param seconds The timeout interval. After this many seconds an interrupt will be fired and handled internally.
 *  Use #Timer_StopHostTimeout to check the status.
 * @pre @c seconds must be a struct positive number
 */
void Timer_StartHostTimeout(int seconds);

/**
 * Stops the 16-bit timer.
 * @post A call to #Timer_CheckHostTimeout will now return @c false.
 */
void Timer_StopHostTimeout(void);

/**
 * Check if the interrupt has been fired or not.
 * @return @c True when the timer was started and the interrupt was fired. @c false otherwise.
 * @note After the interrupt was fired, an explicit call to #Timer_StartHostTimeout is required to restart the
 *  16-bit timer.
 */
bool Timer_CheckHostTimeout(void);

/* -------------------------------------------------------------------------------- */

 /**
 * Starts or restarts a timer.
 * @note The RTC timer is used, which will continue running when going to the Deep Power Down mode, and wake up the IC
 *  when it expires.
 * @param seconds The timeout interval. After this many seconds an interrupt will be fired and handled internally.
 *  Use #Timer_CheckMeasurementTimeout to check the status.
 * @pre @c seconds must be a positive number. If equal to 0, the previously set value is used. If this is the first
 *  call, @c 42 is used.
 * @post The internal status is reset: an immediate call to #Timer_CheckMeasurementTimeout will now return @c false.
 */
void Timer_StartMeasurementTimeout(int seconds);

/**
 * Stops the RTC timer.
 * @post An immediate or later call to #Timer_CheckMeasurementTimeout will now return @c false.
 */
void Timer_StopMeasurementTimeout(void);

/**
 * Check if the interrupt has been fired or not.
 * @return @c True when the timer was started and the interrupt was fired. @c false otherwise.
 * @note AFter the interrupt was fired, an explicit call to #Timer_StartMeasurementTimeout is required to restart the
 *  RTC timer.
 */
bool Timer_CheckMeasurementTimeout(void);

/* -------------------------------------------------------------------------------- */

/**
 * Starts a timer.
 * @note The 32-bit timer is used, without setting any interrupts. It will run as fast as possible.
 */
void Timer_StartFreeRunning(void);

/**
 * Stops the 32-bit timer.
 * @post A call to #Timer_GetFreeRunning will now return ??
 */
void Timer_StopFreeRunning(void);

/**
 * Retrieve the current timer value.
 * @return A positive number. When the moment of calling this function is not determined, the outcome can be used as a
 *  source of entropy.
 */
uint32_t Timer_GetFreeRunning(void);


#endif
