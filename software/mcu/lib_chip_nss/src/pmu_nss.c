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

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* Bit definition for Power Control register (Power Mode related only) */
#define PMU_PCON_DPEN_POS               1
#define PMU_PCON_DPEN_MASK              (1u << PMU_PCON_DPEN_POS)
#define PMU_PCON_SLEEPFLAG_POS          8
#define PMU_PCON_SLEEPFLAG_MASK         (1u << PMU_PCON_SLEEPFLAG_POS)
#define PMU_PCON_DPDFLAG_POS            11
#define PMU_PCON_DPDFLAG_MASK           (1u << PMU_PCON_DPDFLAG_POS)
#define PMU_PCON_LPMFLAG_POS            13
#define PMU_PCON_LPMFLAG_MASK           (1u << PMU_PCON_LPMFLAG_POS)
#define PMU_PCON_POWER_REGS_POS         0

/** Retained data section size in words */
#define PMU_RETAINED_DATA_SIZE          5

/** Access counter for BusSync (since RTC is a slow HW block). Used in #PMU_READ and #PMU_WRITE */
static volatile int Chip_PMU_AccessCounter;

/** Macro to read a PMU register: must be done synchronized since PMU is part of the slow RTC HW block. */
#define PMU_READ(pReg) Chip_BusSync_ReadReg(&NSS_PMU->ACCSTAT, &Chip_PMU_AccessCounter, (pReg))

/** Macro to write a PMU register: must be done synchronized since PMU is part of the slow RTC HW block. */
#define PMU_WRITE(pReg, value) Chip_BusSync_WriteReg(&NSS_PMU->ACCSTAT, &Chip_PMU_AccessCounter, (pReg), (value))

/*****************************************************************************
 * Private functions
 ****************************************************************************/

static void EnterStandbyMode(uint32_t pconFlags, uint32_t scbFlags)
{
    uint32_t pcon = PMU_READ(&NSS_PMU->PCON);

    /* Modify PCON register - SLEEPFLAG and DPDFLAG cleared and PCON standby flags set as required
     * SLEEPFLAG and DPDFLAG are cleared before entering a standby mode so that when reading it after wakeup, it is always
     * correct */
    pcon = (pcon & (~0x1FFu)) | PMU_PCON_SLEEPFLAG_MASK | PMU_PCON_DPDFLAG_MASK | pconFlags;

    PMU_WRITE(&NSS_PMU->PCON, pcon);

    /* Set SCB register in ARM core as required*/
    SCB->SCR = (SCB->SCR & (~SCB_SCR_SLEEPDEEP_Msk)) | scbFlags;

    /* Execute WFI instruction - Execution stalls here */
    __WFI();

    /* ...and resumes here (function returns), after ISR execution, except for Deep Power Down and Power-off mode */
}

static void ModifyRegister(__IO uint32_t *pReg, uint32_t mask, uint32_t value)
{
    uint32_t content = PMU_READ(pReg);

    /* Modify register - clear bits selected with mask and set value */
    content = (content & (~mask)) | value;

    PMU_WRITE(pReg, content);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

void Chip_PMU_PowerMode_EnterSleep(void)
{
    EnterStandbyMode(0, 0);
}

void Chip_PMU_PowerMode_EnterDeepSleep(void)
{
    EnterStandbyMode(0, SCB_SCR_SLEEPDEEP_Msk);
}

void Chip_PMU_PowerMode_EnterDeepPowerDown(bool enableSwitching)
{
#if defined(DEBUG)
    if(Chip_PMU_GetStatus() & PMU_STATUS_VDD_NFC) {
        Chip_Clock_System_BusyWait_ms(500);
    }
#endif
    /* DPD flag indicates that Deep Power Down mode is to be entered
     * LPM flag makes sure no extra power is consumed by the automatic switching mechanism - only set if
     * enableSwitching is false */
    EnterStandbyMode(PMU_PCON_DPEN_MASK | (enableSwitching ? 0 : PMU_PCON_LPMFLAG_MASK), SCB_SCR_SLEEPDEEP_Msk);
}

PMU_DPD_WAKEUPREASON_T Chip_PMU_PowerMode_GetDPDWakeupReason(void)
{
    uint32_t pcon = PMU_READ(&NSS_PMU->PCON);

    /* If both DPDFLAG and SLEEPFLAG are set, something went wrong */
    ASSERT(!(((pcon & PMU_PCON_DPDFLAG_MASK) != 0) && ((pcon & PMU_PCON_SLEEPFLAG_MASK) != 0)));

    if (((pcon & PMU_PCON_DPDFLAG_MASK) != 0) && ((pcon & PMU_PCON_SLEEPFLAG_MASK) == 0)) {
        return (PMU_READ(&NSS_PMU->PSTAT) & (0x3 << 3)) >> 3;
    }
    else {
        return PMU_DPD_WAKEUPREASON_NONE;
    }
}

bool Chip_PMU_Switch_GetVDDBat(void)
{
    return (PMU_READ(&NSS_PMU->PSTAT) & (1 << 1)) != 0;
}

bool Chip_PMU_Switch_GetVNFC(void)
{
    return (PMU_READ(&NSS_PMU->PSTAT) & (1 << 0)) != 0;
}

bool Chip_PMU_Swtich_GetBOD(void)
{
    return (PMU_READ(&NSS_PMU->PSTAT) & (1 << 5)) != 0;
}

void Chip_PMU_Switch_OpenVDDBat(void)
{
    /* Modify PCON register - VBAT set to "force off" and back to "auto" */
    ModifyRegister(&NSS_PMU->PCON, 1 << 14, 1 << 14);
}

void Chip_PMU_SetBODEnabled(bool enabled)
{
    /* Modify PCON register - BODEN set to "enabled" */
    ModifyRegister(&NSS_PMU->PCON, 1 << 15, (uint32_t)(enabled != 0) << 15);
}

bool Chip_PMU_GetBODEnabled(void)
{
    return (PMU_READ(&NSS_PMU->PCON) & (1 << 15)) != 0;
}

void Chip_PMU_SetWakeupPinEnabled(bool enabled)
{
    ModifyRegister(&NSS_PMU->PCON, 1 << 19, (uint32_t)(enabled != 0) << 19);
}

bool Chip_PMU_GetWakeupPinEnabled(void)
{
    return (PMU_READ(&NSS_PMU->PCON) & (1 << 19)) != 0;
}

void Chip_PMU_SetRTCClockSource(PMU_RTC_CLOCKSOURCE_T source)
{
    ModifyRegister(&NSS_PMU->TMRCLKCTRL, 1 << 0, (source & 0x1) << 0);
}

PMU_RTC_CLOCKSOURCE_T Chip_PMU_GetRTCClockSource(void)
{
    return (PMU_RTC_CLOCKSOURCE_T)(PMU_READ(&NSS_PMU->TMRCLKCTRL) & (1 << 0));
}

void Chip_PMU_SetRetainedData(uint32_t *pData, int offset, int size)
{
    ASSERT((size > 0) && (offset >= 0) && (offset + size <= PMU_RETAINED_DATA_SIZE));
    for (int i = 0; i < size; i++) {
        PMU_WRITE(&NSS_PMU->GPREG[i+offset], pData[i]);
    }
}

void Chip_PMU_GetRetainedData(uint32_t *pData, int offset, int size)
{
    ASSERT((size > 0) && (offset >= 0) && (offset + size <= PMU_RETAINED_DATA_SIZE));
    for (int i = 0; i < size; i++) {
        pData[i] = PMU_READ(&NSS_PMU->GPREG[i+offset]);
    }
}

PMU_STATUS_T Chip_PMU_GetStatus(void)
{
    return (PMU_STATUS_T)(PMU_READ(&NSS_PMU->PSTAT) & (PMU_STATUS_BROWNOUT | PMU_STATUS_VDD_NFC));
}

void Chip_PMU_Int_SetEnabledMask(PMU_INT_T mask)
{
    PMU_WRITE(&NSS_PMU->IMSC, mask & 0x7);
}

PMU_INT_T Chip_PMU_Int_GetEnabledMask(void)
{
    return (PMU_INT_T)(PMU_READ(&NSS_PMU->IMSC) & 0x7);
}

PMU_INT_T Chip_PMU_Int_GetRawStatus(void)
{
    return (PMU_INT_T)(PMU_READ(&NSS_PMU->RIS) & 0x7);
}

void Chip_PMU_Int_ClearRawStatus(PMU_INT_T flags)
{
    PMU_WRITE(&NSS_PMU->ICR, flags & 0x7u);
}
