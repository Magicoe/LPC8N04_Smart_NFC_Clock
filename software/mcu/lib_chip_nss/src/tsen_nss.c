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

/**
 * Simplification macro, implementing integer division with simple rounding to closest number
 * It supports both positive and negative numbers, but ONLY positive divisors
 */
#define IDIV(n,d) ( ( (n)>0 ? (n)+(d)/2 : (n)-(d)/2 ) / (d) )

/* ------------------------------------------------------------------------- */

/* Initialize and configure the TSEN peripheral */
void Chip_TSen_Init(NSS_TSEN_T *pTSen)
{
    /* Enable power and clock to peripheral*/
    Chip_SysCon_Peripheral_EnablePower(SYSCON_PERIPHERAL_POWER_TSEN);
    Chip_Clock_Peripheral_EnableClock(CLOCK_PERIPHERAL_TSEN);

    /* Set the Control Register to its default value
     * This ensures that a measurement always starts, when the TSen_Start function is called,
     * regardless of the previous state of the HW block */
    pTSen->CR = 0;

    /* Disable all interrupts for TSEN and clear interrupt flag register */
    pTSen->IMSC = 0;
    pTSen->ICR = 0x7;

    /* Set mode to calibrated (ensure) and resolution to HW default */
    pTSen->SP0 = 0x1 | (TSEN_12BITS << 1);

    /* Set thresholds to their default values */
    Chip_TSen_Int_SetThresholdLow(pTSen, 0x8000);
    Chip_TSen_Int_SetThresholdHigh(pTSen, 0x7FFF);
}

/* Shutdown the TSEN peripheral */
void Chip_TSen_DeInit(NSS_TSEN_T *pTSen)
{
    /* Disable all interrupts for TSEN and clear interrupt flag register */
    pTSen->IMSC = 0;

    /* Disable clock and power to peripheral*/
    Chip_Clock_Peripheral_DisableClock(CLOCK_PERIPHERAL_TSEN);
    Chip_SysCon_Peripheral_DisablePower(SYSCON_PERIPHERAL_POWER_TSEN);
}

/* Set the resolution for the Temperature Sensor */
void Chip_TSen_SetResolution(NSS_TSEN_T *pTSen, TSEN_RESOLUTION_T resolution)
{
    pTSen->SP0 = (pTSen->SP0 & (~(0x7u << 1))) | ((resolution & 0x7u) << 1);
}

/* Get the resolution for the Temperature Sensor */
TSEN_RESOLUTION_T Chip_TSen_GetResolution(NSS_TSEN_T *pTSen)
{
    return (TSEN_RESOLUTION_T)((pTSen->SP0 >> 1) & 0x7);
}

/* Start a temperature measurement */
void Chip_TSen_Start(NSS_TSEN_T *pTSen)
{
    pTSen->CR |= 0x1 << 0;
}

TSEN_STATUS_T Chip_TSen_ReadStatus(NSS_TSEN_T *pTSen, TSEN_RESOLUTION_T *pResolution)
{
    /* TSEN_STATUS_SENSOR_IN_OPERATION flag derived from Start bit */
    uint32_t result = (pTSen->CR & 0x1) << 8;

    /* TSEN_STATUS_MEASUREMENT_DONE derived from RDY Interrupt Flag */
    result |= (pTSen->RIS & 0x1) << 9;

    /* HW status bits */
    result |= pTSen->SR & 0x1F;

    /* write to pResolution */
    if (pResolution != NULL) {
        *pResolution = (TSEN_RESOLUTION_T)((pTSen->SR >> 5) & 0x7);
    }

    return (TSEN_STATUS_T)result;
}

/* Get the temperature value */
int Chip_TSen_GetValue(NSS_TSEN_T *pTSen)
{
    /* read measured value into data - This clears RDY Interrupt Flag */
    return (int)((int16_t)pTSen->DR);

}

/* Convert a temperature value in the HW format to a human readable format in Kelvin */
int Chip_TSen_NativeToKelvin(int native, int multiplier)
{
    ASSERT(multiplier > 0);

    /* As native format is 1-(9,6), to obtain the temperature in Kelvin, it must be divided by 2^6 */
    return IDIV(native * multiplier, 64);
}

/* Convert a temperature value in a human readable format in Kelvin to the HW format */
int Chip_TSen_KelvinToNative(int kelvin, int multiplier)
{
    ASSERT(multiplier > 0);

    /* As native format is 1-(9,6), to convert the temperature in Kelvin to native, it must be multiplied by 2^6 */
    return IDIV(kelvin * 64, multiplier);
}

/* Convert a temperature value in the HW format to a human readable format in Celsius */
int Chip_TSen_NativeToCelsius(int native, int multiplier)
{
    ASSERT(multiplier > 0);

    /* The formula for converting Native directly to Celsius is: C = (N - 273.15*64) / 64
     * Note that native format is 1-(9,6) */
    return IDIV(((native * 10) - 174816) * multiplier, 640);
}

/* Convert a temperature value in a human readable format in Celsius to the HW format */
int Chip_TSen_CelsiusToNative(int celsius, int multiplier)
{
    ASSERT(multiplier > 0);

    /* The formula for converting Celsius directly to Native is: N = (64*C + 273.15*64)
     * Note that native format is 1-(9,6) */
    return IDIV((celsius * 640) + (multiplier * 174816), multiplier * 10);
}

/* Convert a temperature value in the HW format to a human readable format in Fahrenheit */
int Chip_TSen_NativeToFahrenheit(int native, int multiplier)
{
    ASSERT(multiplier > 0);

    /* The formula for converting Native directly to Fahrenheit is: F = (N * 9/320) - 459,67
     * Note that native format is 1-(9,6) */
    int m = 1024 / multiplier;
    return IDIV( IDIV(native*45*multiplier*m, 16) - (45967*multiplier*m), 100 * m);
}

/* Convert a temperature value in a human readable format in Fahrenheit to the HW format */
int Chip_TSen_FahrenheitToNative(int fahrenheit, int multiplier)
{
    ASSERT(multiplier > 0);

    /* The formula for converting Fahrenheit directly to Native is: N = (1600F + 735472) / 45
     * Note that native format is 1-(9,6) */
    return IDIV((fahrenheit * 1600) + (multiplier * 735472), multiplier * 45);
}

/* Set the low-temperature threshold value */
void Chip_TSen_Int_SetThresholdLow(NSS_TSEN_T *pTSen, int native)
{
    pTSen->TLO = 0xFFFF & native;
}

/* Get the low-temperature threshold value */
int Chip_TSen_Int_GetThresholdLow(NSS_TSEN_T *pTSen)
{
    return (int)((int16_t)pTSen->TLO);
}

/* Set the high-temperature threshold value */
void Chip_TSen_Int_SetThresholdHigh(NSS_TSEN_T *pTSen, int native)
{
    pTSen->THI = 0xFFFF & native;
}

/* Get the high-temperature threshold value */
int Chip_TSen_Int_GetThresholdHigh(NSS_TSEN_T *pTSen)
{
    return (int)((int16_t)pTSen->THI);
}

/* Enable/Disable Temperature Sensor interrupts */
void Chip_TSen_Int_SetEnabledMask(NSS_TSEN_T *pTSen, TSEN_INT_T mask)
{
    pTSen->IMSC = mask & 0x7;
}

/* Retrieve the Temperature Sensor interrupt enable mask */
TSEN_INT_T Chip_TSen_Int_GetEnabledMask(NSS_TSEN_T *pTSen)
{
    return (TSEN_INT_T)(pTSen->IMSC & 0x7);
}

/* Retrieves a bitVector with the RAW Temperature Sensor interrupt flags */
TSEN_INT_T Chip_TSen_Int_GetRawStatus(NSS_TSEN_T *pTSen)
{
    return (TSEN_INT_T)(pTSen->RIS & 0x7);
}

/* Clear the required Temperature Sensor interrupt flags */
void Chip_TSen_Int_ClearRawStatus(NSS_TSEN_T *pTSen, TSEN_INT_T flags)
{
    pTSen->ICR = flags & 0x7;
}
