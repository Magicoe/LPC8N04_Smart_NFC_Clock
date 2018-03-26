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

/* -------------------------------------------------------------------------
 * Defines
 * ------------------------------------------------------------------------- */

/* Highest possible address for the IVT location in Flash (in ARM architecture Flash always start at address 0) */
#define SYSCON_IVT_FLASH_ADDRESS_END   0x00007400

/* Defines valid range for the IVT location in RAM */
#define SYSCON_IVT_RAM_ADDRESS_START   0x10000000
#define SYSCON_IVT_RAM_ADDRESS_END     0x10001C00

/* -------------------------------------------------------------------------
 * Public functions
 * ------------------------------------------------------------------------- */

/* Maps the Interrupt Vector Table (IVT) to any SRAM or Flash location */
void Chip_SysCon_IVT_SetAddress(uint32_t address)
{
    /* Address must be on a 1kB boundary */
    ASSERT((address & 0x3FF) == 0);

    if (address <= SYSCON_IVT_FLASH_ADDRESS_END) {
        /* setting Flash offset in number of 1kB segments to bits 1:5 (implicit >>10 followed by <<1 results in >>9) */
        NSS_SYSCON->SYSMEMREMAP = (address >> 9);
    }
    else if ((address >= SYSCON_IVT_RAM_ADDRESS_START) && (address <= SYSCON_IVT_RAM_ADDRESS_END)) {
        /* setting bit 0 to 1 means that IVT is mapped to SRAM
         * setting SRAM offset in number of 1kB segments to bits 1:5 (implicit >>10 followed by <<1 results in >>9) */
        NSS_SYSCON->SYSMEMREMAP = 1 | ((address - SYSCON_IVT_RAM_ADDRESS_START) >> 9);
    }
    else {
        /* Address not in valid range */
        ASSERT(false);
    }
}

/* Gets the absolute address (SRAM or Flash) on where the Interrupt Vector Table (IVT) is mapped */
uint32_t Chip_SysCon_IVT_GetAddress(void)
{
    /* Store register content to ensure information consistency
     * This variable will only contain the absolute address at the end of the function - for now it is just the register content*/
    uint32_t temp = NSS_SYSCON->SYSMEMREMAP & 0x3F;

    /* bits 1:5 of register contain the offset in number of 1kB segments (implicit <<10 followed by >>1 results in <<9 to
     * translate into an offset in bytes) */
    temp <<= 9;

    /* if bit 0 (now bit 9) is 1 means that IVT is mapped to SRAM */
    if ((temp & 0x200) != 0) {
        /* If IVT is mapped to SRAM, add the SRAM start address to translate into an absolute address */
        temp += SYSCON_IVT_RAM_ADDRESS_START;
    }

    /* returned address must always be on 1kB boundary
     * In case IVT is mapped to SRAM, this also ensures that bit 9 is cleared */
    return temp & (~0x3FFu);
}

/* Asserts the reset of the required peripheral(s) */
void Chip_SysCon_Peripheral_AssertReset(SYSCON_PERIPHERAL_RESET_T bitvector)
{
    /* The reset bits are active low - clear the bit to assert the respective reset */
    NSS_SYSCON->PRESETCTRL &= ~(bitvector & 0xFu);
}

/* De-asserts the reset of the required peripheral(s) */
void Chip_SysCon_Peripheral_DeassertReset(SYSCON_PERIPHERAL_RESET_T bitvector)
{
    /* The reset bits are active low - set the bit to deassert the respective reset */
    NSS_SYSCON->PRESETCTRL |= bitvector & 0xF;
}

/* Enables the power of the required peripheral(s) */
void Chip_SysCon_Peripheral_EnablePower(SYSCON_PERIPHERAL_POWER_T bitvector)
{
    /* The register actually selects the powered down peripherals - clear the bit to enable the peripheral power */
    NSS_SYSCON->PDRUNCFG &= ~(bitvector & 0x3Fu);
}

/* Disables the power of the required peripheral(s) */
void Chip_SysCon_Peripheral_DisablePower(SYSCON_PERIPHERAL_POWER_T bitvector)
{
    /* The register actually selects the powered down peripherals - set the bit to disable the peripheral power */
    NSS_SYSCON->PDRUNCFG |= bitvector & 0x3F;
}

/* Enables/Disables the power state of the peripheral(s) described in #SYSCON_PERIPHERAL_POWER_T */
void Chip_SysCon_Peripheral_SetPowerDisabled(SYSCON_PERIPHERAL_POWER_T bitvector)
{
    NSS_SYSCON->PDRUNCFG = bitvector & 0x3F;
}

/* Retrieves a bitvector stating the enabled/disabled peripheral(s) described in #SYSCON_PERIPHERAL_POWER_T */
SYSCON_PERIPHERAL_POWER_T Chip_SysCon_Peripheral_GetPowerDisabled(void)
{
    return (SYSCON_PERIPHERAL_POWER_T)(NSS_SYSCON->PDRUNCFG & 0x3F);
}

/* Gets the source of the last reset event(s) */
SYSCON_RESETSOURCE_T Chip_SysCon_Reset_GetSource(void)
{
    return (SYSCON_RESETSOURCE_T)(NSS_SYSCON->SYSRSTSTAT & 0xF);
}

/* Clears the reset source information */
void Chip_SysCon_Reset_ClearSource(void)
{
    NSS_SYSCON->SYSRSTSTAT = 0;
}

/* Enables/Disables the system wake-up start logic sources */
void Chip_SysCon_StartLogic_SetEnabledMask(SYSCON_STARTSOURCE_T mask)
{
    NSS_SYSCON->STARTERP0 = mask & 0x1FFF;
}

/* Gets the system wake-up start logic source enabled mask */
SYSCON_STARTSOURCE_T Chip_SysCon_StartLogic_GetEnabledMask(void)
{
    return (SYSCON_STARTSOURCE_T)(NSS_SYSCON->STARTERP0 & 0x1FFF);
}

/* Retrieves the status of the system wake-up start logic sources */
SYSCON_STARTSOURCE_T Chip_SysCon_StartLogic_GetStatus(void)
{
    return (SYSCON_STARTSOURCE_T)(NSS_SYSCON->STARTSRP0 & 0x1FFF);
}

/* Clears the status for the required system wake-up start logic source */
void Chip_SysCon_StartLogic_ClearStatus(SYSCON_STARTSOURCE_T flags)
{
    NSS_SYSCON->STARTRSRP0CLR = flags & 0x1FFF;
    /* Due to HW peculiarity, the flags have to be manually cleared by SW */
    NSS_SYSCON->STARTRSRP0CLR &= ~(flags & 0x1FFFu);
}

/* Selects a falling or rising edge as the trigger of the corresponding system wake-up PIO start source */
void Chip_SysCon_StartLogic_SetPIORisingEdge(SYSCON_STARTSOURCE_T bitvector)
{
    NSS_SYSCON->STARTAPRP0 = bitvector & 0x7FF;
}

/* Gets the selected edge (falling or rising) that is the trigger for the corresponding system wake-up PIO start source */
SYSCON_STARTSOURCE_T Chip_SysCon_StartLogic_GetPIORisingEdge(void)
{
    return (SYSCON_STARTSOURCE_T)(NSS_SYSCON->STARTAPRP0 & 0x7FF);
}

/* Gets the Device ID of the chip */
uint32_t Chip_SysCon_GetDeviceID(void)
{
    return NSS_SYSCON->DEVICEID;
}
