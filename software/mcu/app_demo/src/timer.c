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
#include "timer.h"

/**
 * @c false when the timer is stopped or when the RTC_IRQn interrupt wasn't fired after being started (again).
 * @c true when the timer was started (again) and the interrupt was fired.
 */
static volatile bool sMeasurementTimeoutInterruptFired = false;

/* -------------------------------------------------------------------------------- */

void RTC_IRQHandler(void)
{
    RTC_INT_T status = Chip_RTC_Int_GetRawStatus(NSS_RTC);
    Chip_RTC_Int_ClearRawStatus(NSS_RTC, status);

    if (status & RTC_INT_WAKEUP) {
        /* The down counter of the RTC does not automatically reload. In this handler, only a flag is stored, that the
         * main thread has to check and act upon. When the flash is checked, it will then call ExecuteMeasurementMode
         * Inside that function, the RTC is reloaded with the correct seconds interval. This is not done here, since
         * the IC might 'wake up' from Deep Power Down due to an expiring RTC, when this interrupt handler is not
         * called.
         * When the main thread fails to check the flag before going to Deep Power Down, he will not wake up anymore
         * due to RTC. To prevent this, we unconditionally program a new short timeout now, to ensure the RTC is never
         * stopped: the next correct time to make a next measurement will be set in ExecuteMeasurementMode, overruling
         * this.
         */
        Chip_RTC_Wakeup_SetReload(NSS_RTC, 1); /* Any small value will do. */
        sMeasurementTimeoutInterruptFired = true;
    }
}

/* -------------------------------------------------------------------------------- */

void Timer_Init(void)
{
    /* RTC timer: down counter may be already running or have been expired just now. Ensure interrupts arrive. */
    Chip_RTC_Init(NSS_RTC);
    Chip_RTC_Int_SetEnabledMask(NSS_RTC, RTC_INT_WAKEUP);
    sMeasurementTimeoutInterruptFired = false;
    NVIC_EnableIRQ(RTC_IRQn);

    /* 16-bit timer: nothing to do. Started on request only. */
    /* 32-bit timer: nothing to do. Started on request only. */
}

/* -------------------------------------------------------------------------------- */

void Timer_StartMeasurementTimeout(int seconds)
{
    Chip_RTC_Wakeup_SetControl(NSS_RTC, RTC_WAKEUPCTRL_DISABLE);

    sMeasurementTimeoutInterruptFired = false;
    if (!seconds) {
        seconds = Chip_RTC_Wakeup_GetReload(NSS_RTC);
    }
    if (!seconds) {
        seconds = 42; /* 42 seems a sensible default value. */
    }

    Chip_RTC_Wakeup_SetControl(NSS_RTC, RTC_WAKEUPCTRL_ENABLE | RTC_WAKEUPCTRL_AUTO);
    Chip_RTC_Wakeup_SetReload(NSS_RTC, seconds);
    Chip_SysCon_StartLogic_SetEnabledMask(SYSCON_STARTSOURCE_RTC);
    Chip_SysCon_StartLogic_ClearStatus(SYSCON_STARTSOURCE_RTC);
    NVIC_EnableIRQ(RTC_IRQn);
    Chip_RTC_Int_SetEnabledMask(NSS_RTC, RTC_INT_WAKEUP);
}

void Timer_StopMeasurementTimeout(void)
{
    Chip_RTC_Wakeup_SetControl(NSS_RTC, RTC_WAKEUPCTRL_DISABLE);
    NVIC_DisableIRQ(RTC_IRQn);
    Chip_RTC_Int_SetEnabledMask(NSS_RTC, RTC_INT_NONE);
    sMeasurementTimeoutInterruptFired = false;
}

bool Timer_CheckMeasurementTimeout(void)
{
    return sMeasurementTimeoutInterruptFired;
}

// end file
