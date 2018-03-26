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

void Chip_Flash_SetHighPowerMode(bool highPower)
{
    /* If disabling high power mode, make sure that either system clock divisor is not 1 or
     * wait states are configured. Else core might end up in hard fault (flash access error).
     * See "SW Clock Restrictions" */
    ASSERT(highPower || (Chip_Clock_System_GetClockDiv() > 1) || (Chip_Flash_GetNumWaitStates() > 0));

    /* Control bit 18 (Low Power Mode) as well as bit 19 (Bandgap). Both bits need to be set or cleared together. */
    uint32_t mask = (1 << 18) | (1 << 19);

    if (highPower) {
        NSS_FLASH->FCTR &= ~mask;
    }
    else {
        NSS_FLASH->FCTR |= mask;
    }
}

bool Chip_Flash_GetHighPowerMode(void)
{
    /* Control bit 18 (Low Power Mode) and bit 19 (Bandgap) are set and cleared together. */
    uint32_t mask = (1 << 18) | (1 << 19);

    return (NSS_FLASH->FCTR & mask) != mask;
}

void Chip_Flash_SetNumWaitStates(int waitStates)
{
    ASSERT((waitStates > 0) || (Chip_Clock_System_GetClockDiv() > 1) || Chip_Flash_GetHighPowerMode());

    NSS_FLASH->FBWST = (NSS_FLASH->FBWST & ~0x00FFu) | (uint32_t)(waitStates & 0xFF);
}

int Chip_Flash_GetNumWaitStates(void)
{
    return NSS_FLASH->FBWST & 0xFF;
}
