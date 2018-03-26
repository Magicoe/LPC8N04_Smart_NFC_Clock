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
 * types/enumerations/variables
 ****************************************************************************/

static volatile int Chip_RTC_AccessCounter;

/** Macro to read an RTC register */
#define RTC_READ(pReg)         Chip_BusSync_ReadReg(&NSS_RTC->ACCSTAT, &Chip_RTC_AccessCounter, (pReg))

/** Macro to write an RTC register */
#define RTC_WRITE(pReg, value) Chip_BusSync_WriteReg(&NSS_RTC->ACCSTAT, &Chip_RTC_AccessCounter, (pReg), (value))

/*****************************************************************************
 * Public functions
 ****************************************************************************/

// Enables ARM access to RTC block via APB clock
#pragma GCC diagnostic ignored "-Wunused-parameter"
void Chip_RTC_Init(NSS_RTC_T * pRTC)
{
    // Enable APB access to RTC domain
    Chip_Clock_Peripheral_EnableClock(CLOCK_PERIPHERAL_RTC);
}

// Halts the counting of "wake-up down-counter" and disables ARM access to RTC block via APB bus.
void Chip_RTC_DeInit(NSS_RTC_T *pRTC)
{
    // 1. Stop RTC "wake-up down-counter"
    RTC_WRITE(&pRTC->CR, RTC_WAKEUPCTRL_DISABLE);

    // 2. Disable 'wake-up' interrupt 
    RTC_WRITE(&pRTC->IMSC, RTC_INT_NONE);

    // 3. Disable RTC block register access from APB
    Chip_Clock_Peripheral_DisableClock(CLOCK_PERIPHERAL_RTC);
}


// Sets the number of TFRO clock pulses in one RTC 'tick'
void Chip_RTC_SetCalibration(NSS_RTC_T *pRTC, int calibValue)
{
    RTC_WRITE(&pRTC->CAL, calibValue & 0x0000FFFF);
}


 // Returns the number of TFRO clock pulses in one RTC 'tick'
int Chip_RTC_GetCalibration(NSS_RTC_T *pRTC)
{   // return 16bit masked result
    return 0x0000FFFF & RTC_READ(&pRTC->CAL);
}

// Controls the operation of "wake-up down-counter"
void Chip_RTC_Wakeup_SetControl(NSS_RTC_T *pRTC, RTC_WAKEUPCTRL_T control)
{
    RTC_WRITE(&pRTC->CR, (int) control & 0x07);
}

// Returns the control register of the "wake-up down-counter" operation
RTC_WAKEUPCTRL_T Chip_RTC_Wakeup_GetControl(NSS_RTC_T *pRTC)
{
    return (RTC_WAKEUPCTRL_T) (RTC_READ(&pRTC->CR) & 0x7);
}

// Sets the "wake-up down-counter" ticks.  
void Chip_RTC_Wakeup_SetReload(NSS_RTC_T *pRTC, int ticks)
{
    RTC_WRITE(&pRTC->SLEEPT, 0xFFFFFF & ticks);
}

// Returns the number of "wake-up down-counter" ticks (seconds) previously set by #Chip_RTC_Wakeup_SetReload
int Chip_RTC_Wakeup_GetReload(NSS_RTC_T *pRTC)
{
    return 0x00FFFFFF & RTC_READ(&pRTC->SLEEPT);
}

// Returns the remaining ("wake-up down-counter") ticks until 'wake-up' event occurs
int Chip_RTC_Wakeup_GetRemaining(NSS_RTC_T *pRTC)
{
    return 0x00FFFFFF & RTC_READ(&pRTC->VAL);
}

// Indicates whether or not the "wake-up-down-counter" is in fact running
bool Chip_RTC_Wakeup_IsRunning(NSS_RTC_T *pRTC)
{
    /* Due to an artifact with this HW block, we need to first trigger a "clear" of the register by writing to it */
    RTC_WRITE(&pRTC->SR, 0xFF);

    return (RTC_READ(&pRTC->SR) & (0x1u << 3)) != 0;
}

// Returns the current 'tick' value of the "time up-counter"
int Chip_RTC_Time_GetValue(NSS_RTC_T *pRTC)
{
    return (int)RTC_READ(&pRTC->TIME);
}

// Sets a new 'tick' value to the "time up-counter"
void Chip_RTC_Time_SetValue(NSS_RTC_T *pRTC, int tickValue)
{
    RTC_WRITE(&pRTC->TIME, (uint32_t)tickValue);
}

// Enables/Disables RTC interrupt event
void Chip_RTC_Int_SetEnabledMask(NSS_RTC_T *pRTC, RTC_INT_T mask)
{
    RTC_WRITE(&pRTC->IMSC, RTC_INT_ALL & mask);
}

// Retrieves the RTC interrupt enable bitfield
RTC_INT_T Chip_RTC_Int_GetEnabledMask(NSS_RTC_T *pRTC)
{
    return (RTC_INT_T) (RTC_INT_ALL & RTC_READ(&pRTC->IMSC));
}

// Retrieves the reason(s) of RTC interrupt event
RTC_INT_T Chip_RTC_Int_GetRawStatus(NSS_RTC_T *pRTC)
{
    return (RTC_INT_T) (RTC_INT_ALL & RTC_READ(&pRTC->RIS));
}

// Clears the indicated RTC interrupt flag(s). 
void Chip_RTC_Int_ClearRawStatus(NSS_RTC_T *pRTC, RTC_INT_T flags)
{
    RTC_WRITE(&pRTC->ICR, RTC_INT_ALL & flags);
}


