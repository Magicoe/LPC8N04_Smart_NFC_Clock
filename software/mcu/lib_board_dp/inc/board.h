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


#ifndef __BOARD_H_
#define __BOARD_H_

#include "chip.h"

#include "led/led.h"

/** @defgroup DP_BOARD_NSS dp: board support for the Demo PCB
 * @ingroup BOARDS_NSS
 * The Demo PCB is the promoted HW board for the evaluation of NHS31xx family ICs along with
 * the respective SDK.
 *
 * The Demo PCB provides a couple of features, which are exposed at SW level:
 *  - one controllable LED: #LED1 / #LED_RED
 *  - one button readable via @ref GPIO_NSS "GPIO"
 *  .
 *
 * The initialization function only performs initialization of LED. Apart from those pins, it does
 * not change the HW default IO pin configuration or system clock configuration.
 *
 * @par Board support concept:
 *  Board support comes in the form of a statically-linked library which includes the bare minimum for an application
 *  to start the HW properly (IO pin configuration and/or clock configuration if required)
 *  as well as SW support for the features provided by the board.
 *  There is a single function that is part of the board support API: the board initialization function (#Board_Init).
 *  The board initialization has two distinct purposes. In one hand, to ensure that IO pin configuration and state are
 *  set to a harmless state for the respective board. On the other hand that the features provided by the board
 *  (LEDs, external memories, etc) are properly initialized in SW and ready to be used directly by the application.
 *  Whenever board support is needed for a different board (different pin layout, reduced/extended feature set, etc), a
 *  separate board library must be created, matching the requirements of the new board. As long as the initialization
 *  function prototype (#Board_Init) and the design is kept consistent between board APIs, applications may switch boards
 *  more easily during development or even be designed to run on different boards.
 *
 *  @par Diversity
 *  This board makes use of some higher level modules which support diversity: where possible, the default values have
 *  been kept. Necessary overrides have been made in @ref DP_BOARD_NSS_SEL.
 *
 * @{
 */

/**
 * A unique define for the Software Development Board.
 * Applications or modules can use this define to enable/disable code at compile time based on the board being used.
 */
#define BOARD_DP

/**
 * Sets up and initializes all required blocks and functions related to the board hardware.
 * This includes initializing all the required chip level drivers as well as the higher abstraction modules so that
 * SW support for the board features is available.
 * @note Initialization does not touch the default IO pin configuration or system clock configuration.
 * @post @ref MODS_NSS_LED "LED" module can be directly used.
 * @post @ref IOCON_NSS "IOCON" and @ref GPIO_NSS "GPIO" chip level drivers are initialized.
 */
void Board_Init(void);

/**
 * @}
 */

#endif /* __BOARD_H_ */
