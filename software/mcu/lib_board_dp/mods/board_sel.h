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
 * @defgroup DP_BOARD_NSS_SEL Board diversity overrides
 * @ingroup DP_BOARD_NSS
 * @{
 */

#ifndef __BOARD_SEL_H_
#define __BOARD_SEL_H_

/**
 * The number of LEDs supported by the Demo PCB.
 * Matches the length of #LED_PROPERTIES.
 */
#define LED_COUNT 1

/**
 * The LED properties for the supported LEDs of the Demo PCB.
 * @see LED_PROPERTIES_T
 */
#define LED_PROPERTIES {/* LED1 LED_RED */ {0, 7, true, IOCON_PIO0_7}}

/**
 * Easier to remember macro name for the first LED
 * @{
 */
#define LED1 LED_(0)
#define LED_RED LED1
/** @} */

/** @} */

#endif /* __BOARD_SEL_H_ */
