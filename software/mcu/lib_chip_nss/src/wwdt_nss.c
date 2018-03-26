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


#include "chip.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
void Chip_WWDT_Init(NSS_WWDT_T *pWWDT)
{
    Chip_Clock_Peripheral_EnableClock(CLOCK_PERIPHERAL_WATCHDOG);

    /* Set timeout to default */
    pWWDT->TC = 0xFF;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void Chip_WWDT_DeInit(NSS_WWDT_T *pWWDT)
{
    Chip_Clock_Peripheral_DisableClock(CLOCK_PERIPHERAL_WATCHDOG);
}

void Chip_WWDT_ClearStatusFlag(NSS_WWDT_T *pWWDT, uint32_t status)
{
    if (status & WWDT_WDMOD_WDTOF) {
        pWWDT->MOD &= (~WWDT_WDMOD_WDTOF) & WWDT_WDMOD_BITMASK;
    }

    if (status & WWDT_WDMOD_WDINT) {
        pWWDT->MOD |= WWDT_WDMOD_WDINT;
    }
}

