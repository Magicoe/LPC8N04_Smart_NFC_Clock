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

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Sets the division factor that divides the SFRO for the clock that drives the System Clock and the Peripheral Clocks */
void Chip_Clock_System_SetClockDiv(int divisor)
{
    uint32_t hw_divisor = 0;

    /* Divide-by-0 NOT allowed */
    ASSERT(divisor > 0);

    /* If setting system clock divisor to 1, make sure that either flash wait states is set to 0 or
     * flash high power mode is configured Else core might end up in hard fault (flash access error).
     * See "SW Clock Restrictions" */
    ASSERT((divisor > 1) || Chip_Flash_GetHighPowerMode() || (Chip_Flash_GetNumWaitStates() > 0));

    /* Clip divisor to 128 beforehand to narrow down the maximum number of calculation cycles to 8 */
    if (divisor > 128) {
        divisor = 128;
    }

    /* Calculate floor(log2(divisor)) */
    while (divisor >>= 1) {
        hw_divisor++;
    }

    /* As the HW does not clear the SYSCLKUEN bit, we force it here to be cleared before setting the divisor,
     * otherwise it might happen that the clock frequency is immediately changed when the divisor is set and not
     * when the clock update sequence is done */
    NSS_SYSCON->SYSCLKUEN = 0;

    /* Set the new divisor */
    NSS_SYSCON->SYSCLKCTRL = ((NSS_SYSCON->SYSCLKCTRL & (~(0x7u << 1))) | ((hw_divisor & 0x7) << 1));

    /* This 0-1 sequence updates the System clock divisor */
    NSS_SYSCON->SYSCLKUEN = 0;
    NSS_SYSCON->SYSCLKUEN = 1;
}


/* Gets the division factor that divides the SFRO for the clock that drives the System Clock and the Peripheral Clocks */
int Chip_Clock_System_GetClockDiv(void)
{
    /* Calculate 2 ^ "hw_divisor" */
    return 1 << ((NSS_SYSCON->SYSCLKCTRL >> 1) & 0x7);
}


/* Sets the System Clock frequency in Hz */
void Chip_Clock_System_SetClockFreq(int frequency)
{
    /* Divide-by-0 NOT allowed */
    ASSERT(frequency > 0);

    /* Notes:
       Simple Integer division ensures the clipping to the closest supported value higher than or equal to 'frequency'.
       Chip_Clock_System_SetClockDiv function clips the divisor to the largest power of 2 lower than or equal to it */
    Chip_Clock_System_SetClockDiv(NSS_SFRO_FREQUENCY/frequency);
}


/* Gets the System Clock frequency in Hz */
int Chip_Clock_System_GetClockFreq(void)
{
    /* Rounding is not needed as the remaining of this division is always 0 */
    return NSS_SFRO_FREQUENCY / Chip_Clock_System_GetClockDiv();
}


/* Waits the specified amount of time (using instruction counting) */
void Chip_Clock_System_BusyWait_us(uint32_t us)
{
    uint32_t ticks_to_wait;
    uint32_t ns_per_tick;

    /* Do not wait if us is less than or equal to 0 */
    if (us == 0) return;

    /* Limit to 4 seconds, otherwise ticks_to_wait will overflow */
    ASSERT( us <= 4*1000*1000 );
    
    /* Calculate the time taken (in ns) per tick */
    ns_per_tick = (uint32_t)(1000*1000*1000/Chip_Clock_System_GetClockFreq());

    /* Calculate the number of ticks to wait */
    ticks_to_wait = 1000*us/ns_per_tick;

    /* Subtract the overhead of the function call (found by experiment) */
    ticks_to_wait -= 480;

    /* Backup r1 to the stack since it's used in the assembly code below */
    __asm ("push {r1}");

    /* Copy value from ticks_to_wait to r1 */
    __asm ("movs r1, %[ticks]" : : [ticks]"r"(ticks_to_wait));

    /* Busy wait loop in assembler so that it will never be optimized out */
    __asm (
           "_BUSYWAIT_LOOP:         \n"
           "sub  r1, #3             \n" /* instruction 1: 3 ticks per loop */
           "bgt _BUSYWAIT_LOOP      \n" /* instruction 2 */
    );

    /* Recover r1 from the stack */
    __asm ("pop {r1}"); 
}


/* Sets the division factor that divides the SFRO into the clock that drives the SPI0 HW block */
void Chip_Clock_SPI0_SetClockDiv(int divisor)
{
    ASSERT(divisor >= 0);

    /* Clip divisor to 255 */
    if (divisor > 255) {
        divisor = 255;
    }

    /* Floor to nearest even number if higher than 2  (and clip to 254) */
    if (divisor > 2) {
        divisor = divisor & 0xFE;
    }

    NSS_SYSCON->SSP0CLKDIV = (uint32_t)divisor;
}


/* Gets the division factor that divides the SFRO into the clock that drives the SPI0 HW block */
int Chip_Clock_SPI0_GetClockDiv(void)
{
    int divisor = NSS_SYSCON->SSP0CLKDIV & 0xFF;

    /* Floor to nearest even number if higher than 2 (and clip to 254) */
    if (divisor > 2) {
        divisor = divisor & 0xFE;
    }

    return divisor;
}


/* Sets the frequency in Hz of the clock that drives the SPI0 HW block */
void Chip_Clock_SPI0_SetClockFreq(int frequency)
{
    ASSERT((frequency >= 0) && (frequency <= NSS_SFRO_FREQUENCY));

    if (frequency == 0) {
        /* Frequency of 0 disables the SPI0 clock -> set divisor to 0 */
        Chip_Clock_SPI0_SetClockDiv(frequency);
    }
    else {
        /* Notes:
           Simple Integer division ensures the clipping to the closest supported value higher than or equal to 'frequency'.
           Chip_Clock_SPI0_SetClockDiv function clips the divisor to 254 (~31.496kHz) or floors it to nearest even number */
        Chip_Clock_SPI0_SetClockDiv(NSS_SFRO_FREQUENCY/frequency);
    }
}


/* Gets the frequency in Hz of the clock that drives the SPI0 HW block */
int Chip_Clock_SPI0_GetClockFreq(void)
{
    int clockDiv = Chip_Clock_SPI0_GetClockDiv();

    /* Simple Rounding trick is applied */
    return (NSS_SFRO_FREQUENCY + (clockDiv >> 1)) / clockDiv;
}


/* Sets the division factor that divides the SFRO into the clock that drives the Watchdog */
void Chip_Clock_Watchdog_SetClockDiv(int divisor)
{
    ASSERT(divisor >= 0);

    /* Clip divisor to 255 */
    if (divisor > 255) {
        divisor = 255;
    }

    /* Floor to nearest even number if higher than 2 (and clip to 254) */
    if (divisor > 2) {
        divisor = divisor & 0xFE;
    }

    NSS_SYSCON->WDTCLKDIV = (uint32_t)divisor;

    /* This 0-1-0 sequence updates the watchdog clock divisor */
    NSS_SYSCON->WDTCLKUEN = 0;
    NSS_SYSCON->WDTCLKUEN = 1;
    NSS_SYSCON->WDTCLKUEN = 0;
}


/* Gets the division factor that divides the SFRO into the clock that drives the Watchdog */
int Chip_Clock_Watchdog_GetClockDiv(void)
{
    int divisor = NSS_SYSCON->WDTCLKDIV & 0xFF;

    /* Floor to nearest even number if higher than 2 (and clip to 254) */
    if (divisor > 2) {
        divisor = divisor & 0xFE;
    }

    return divisor;
}


/* Sets the frequency in Hz of the clock that drives the Watchdog */
void Chip_Clock_Watchdog_SetClockFreq(int frequency)
{
    ASSERT((frequency >= 0) && (frequency <= NSS_SFRO_FREQUENCY));

    if (frequency == 0) {
        /* Frequency of 0 disables the Watchdog clock -> set divisor to 0 */
        Chip_Clock_Watchdog_SetClockDiv(frequency);
    }
    else {
        /* Notes:
           Simple Integer division ensures the clipping to the closest supported value higher than or equal to 'frequency'.
           Chip_Clock_Watchdog_SetClockDiv function clips the divisor to 254 (~31.496kHz) or floors it to nearest even number */
        Chip_Clock_Watchdog_SetClockDiv(NSS_SFRO_FREQUENCY/frequency);
    }
}


/* Gets the clock frequency in Hz of the clock that drives the Watchdog */
int Chip_Clock_Watchdog_GetClockFreq(void)
{
    int clockDiv = Chip_Clock_Watchdog_GetClockDiv();

    /* Simple Rounding trick is applied */
    return (NSS_SFRO_FREQUENCY + (clockDiv >> 1)) / clockDiv;
}


/* Enables the required peripheral clock(s) */
void Chip_Clock_Peripheral_EnableClock(CLOCK_PERIPHERAL_T bitvector)
{
    NSS_SYSCON->SYSAHBCLKCTRL |= bitvector & 0x1DBFFC;
}


/* Disable the required peripheral clock(s) */
void Chip_Clock_Peripheral_DisableClock(CLOCK_PERIPHERAL_T bitvector)
{
    NSS_SYSCON->SYSAHBCLKCTRL &= ~(bitvector & 0x1DBFFC);
}

/* Enables/Disables the clock of the peripheral(s) described in CLOCK_PERIPHERAL_T */
void Chip_Clock_Peripheral_SetClockEnabled(CLOCK_PERIPHERAL_T bitvector)
{
    NSS_SYSCON->SYSAHBCLKCTRL = bitvector & 0x1DBFFC;
}


/* Gets the clock enabled mask for the peripheral(s) described in #CLOCK_PERIPHERAL_T */
CLOCK_PERIPHERAL_T Chip_Clock_Peripheral_GetClockEnabled(void)
{
    return (CLOCK_PERIPHERAL_T)(NSS_SYSCON->SYSAHBCLKCTRL & 0x1DBFFC);
}

/* Sets the Watchdog clock source */
void Chip_Clock_Watchdog_SetClockSource(CLOCK_WATCHDOGSOURCE_T source)
{
    NSS_SYSCON->WDTCLKSEL = source & 0x3;

    /* This 0-1-0 sequence updates the watchdog clock source */
    NSS_SYSCON->WDTCLKUEN = 0;
    NSS_SYSCON->WDTCLKUEN = 1;
    NSS_SYSCON->WDTCLKUEN = 0;
}


/* Gets the Watchdog clock source */
CLOCK_WATCHDOGSOURCE_T Chip_Clock_Watchdog_GetClockSource(void)
{
    return (CLOCK_WATCHDOGSOURCE_T)(NSS_SYSCON->WDTCLKSEL & 0x3);
}


/* Sets the CLKOUT pin clock source */
void Chip_Clock_Clkout_SetClockSource(CLOCK_CLKOUTSOURCE_T source)
{
    NSS_SYSCON->CLKOUTEN = source & 0x7;
}


/* Gets the CLKOUT pin clock source */
CLOCK_CLKOUTSOURCE_T Chip_Clock_Clkout_GetClockSource(void)
{
    return (CLOCK_CLKOUTSOURCE_T)(NSS_SYSCON->CLKOUTEN & 0x7);
}
