/*
 * buzzer.c
 *
 *  Created on: 2018-3-7
 *      Author: nxp58695
 */
#include <string.h>
#include "board.h"
#include "timer.h"
#include "buzzer.h"

/**
 * Buzzer enable
 */
void buzzer_start(void)
{
    Chip_IOCON_SetPinConfig(NSS_IOCON, 3, IOCON_FUNC_1 | IOCON_RMODE_INACT);
    Chip_TIMER16_0_Init();

    Chip_TIMER_Disable(NSS_TIMER16_0);
    Chip_TIMER_Reset(NSS_TIMER16_0);
    Chip_TIMER_SetMatch(NSS_TIMER16_0, 0, 250);
    Chip_TIMER_ResetOnMatchEnable(NSS_TIMER16_0, 0);
    Chip_TIMER_StopOnMatchDisable(NSS_TIMER16_0, 0);
    Chip_TIMER_MatchDisableInt(NSS_TIMER16_0, 0);
    Chip_TIMER_SetMatchOutputMode(NSS_TIMER16_0, 0, TIMER_MATCH_OUTPUT_EMC);
    Chip_TIMER_ExtMatchControlSet(NSS_TIMER16_0, 0, TIMER_EXTMATCH_TOGGLE, 0);
    Chip_TIMER_Enable(NSS_TIMER16_0);
}

/**
 * Buzzer disable
 */
void buzzer_stop(void)
{
    Chip_IOCON_SetPinConfig(NSS_IOCON, 3, IOCON_FUNC_0 | IOCON_RMODE_INACT);
    Chip_GPIO_SetPinDIROutput(NSS_GPIO, 0, 3);
    Chip_GPIO_SetPinState(NSS_GPIO, 0, 3, 0);

    Chip_TIMER_Disable(NSS_TIMER16_0);
    Chip_TIMER_Reset(NSS_TIMER16_0);
    Chip_TIMER_ExtMatchControlSet(NSS_TIMER16_0, 0, TIMER_EXTMATCH_TOGGLE, 0);
    Chip_TIMER_MatchDisableInt(NSS_TIMER32_0, 1);
}

// end file

