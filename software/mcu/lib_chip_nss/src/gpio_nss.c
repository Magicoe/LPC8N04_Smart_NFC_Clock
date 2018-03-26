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
void Chip_GPIO_Init(NSS_GPIO_T *pGPIO)
{
    Chip_Clock_Peripheral_EnableClock(CLOCK_PERIPHERAL_GPIO);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void Chip_GPIO_DeInit(NSS_GPIO_T *pGPIO)
{
    Chip_Clock_Peripheral_DisableClock(CLOCK_PERIPHERAL_GPIO);
}

void Chip_GPIO_SetPinDIR(NSS_GPIO_T *pGPIO, uint8_t port, uint8_t pin, bool output)
{
    if (output) {
        Chip_GPIO_SetPinDIROutput(pGPIO, port, pin);
    }
    else {
        Chip_GPIO_SetPinDIRInput(pGPIO, port, pin);
    }
}

void Chip_GPIO_SetPortDIR(NSS_GPIO_T *pGPIO, uint8_t port, uint32_t pinMask, bool outSet)
{
    if (outSet) {
        Chip_GPIO_SetPortDIROutput(pGPIO, port, pinMask);
    }
    else {
        Chip_GPIO_SetPortDIRInput(pGPIO, port, pinMask);
    }
}

void Chip_GPIO_SetupPinInt(NSS_GPIO_T *pGPIO, uint8_t port, uint8_t pin, GPIO_INT_MODE_T mode)
{
    uint32_t pinMask = (1u << pin);

    /* Edge mode selected? */
    if ((uint32_t) mode & 0x2) {
        Chip_GPIO_SetPinModeEdge(pGPIO, port, pinMask);

        /* Interrupt on both edges selected? */
        if ((uint32_t) mode & 0x4) {
            Chip_GPIO_SetEdgeModeBoth(pGPIO, port, pinMask);
        }
        else {
            Chip_GPIO_SetEdgeModeSingle(pGPIO, port, pinMask);
        }
    }
    else {
        /* Level mode */
        Chip_GPIO_SetPinModeLevel(pGPIO, port, pinMask);
    }

    /* Level selections will not alter 'dual edge' mode */
    if ((uint32_t) mode & 0x1) {
        /* High edge or level mode selected */
        Chip_GPIO_SetModeHigh(pGPIO, port, pinMask);
    }
    else {
        Chip_GPIO_SetModeLow(pGPIO, port, pinMask);
    }
}
