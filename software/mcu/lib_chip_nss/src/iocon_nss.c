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

/* Initialize IOCON block */
#pragma GCC diagnostic ignored "-Wunused-parameter"
void Chip_IOCON_Init(NSS_IOCON_T *pIOCON)
{
    Chip_Clock_Peripheral_EnableClock(CLOCK_PERIPHERAL_IOCON);
}

/* De-initialize IOCON block */
#pragma GCC diagnostic ignored "-Wunused-parameter"
void Chip_IOCON_DeInit(NSS_IOCON_T *pIOCON)
{
    Chip_Clock_Peripheral_DisableClock(CLOCK_PERIPHERAL_IOCON);
}

/* Sets I/O pin configuration */
void Chip_IOCON_SetPinConfig(NSS_IOCON_T *pIOCON, IOCON_PIN_T pin, int config)
{
    if (pin >= IOCON_ANA0_0) /* Analog Pins */{
        if (config == IOCON_FUNC_1) /* Indicates connection to analog bus */{
            Chip_IOCON_UngroundAnabus(pIOCON, (1 << (pin - IOCON_ANA0_0)));
        }
    }
    pIOCON->REG[pin] = config & 0xFFFFFF;
}

/* Gets I/O pin configuration */
int Chip_IOCON_GetPinConfig(NSS_IOCON_T *pIOCON, IOCON_PIN_T pin)
{
    return pIOCON->REG[pin] & 0xFFFFFF;
}

/* Grounds/UnGrounds the analog bus(s) described in #IOCON_ANABUS_T */
void Chip_IOCON_SetAnabusGrounded(NSS_IOCON_T *pIOCON, IOCON_ANABUS_T bitvector)
{
    pIOCON->ANABUSGROUND = bitvector & 0x0FFFFFFF;
}

/* Retrieves a bitvector stating the grounded/ungrounded analog buses described in #IOCON_ANABUS_T */
IOCON_ANABUS_T Chip_IOCON_GetAnabusGrounded(NSS_IOCON_T *pIOCON)
{
    return (IOCON_ANABUS_T)(pIOCON->ANABUSGROUND & 0x0FFFFFFF);
}

/* Grounds the required analog bus(s). */
void Chip_IOCON_GroundAnabus(NSS_IOCON_T *pIOCON, IOCON_ANABUS_T bitvector)
{
    pIOCON->ANABUSGROUND |= bitvector & 0x0FFFFFFF;
}

/* Ungrounds the required analog bus(s). */
void Chip_IOCON_UngroundAnabus(NSS_IOCON_T *pIOCON, IOCON_ANABUS_T bitvector)
{
    pIOCON->ANABUSGROUND &= ~(bitvector & 0x0FFFFFFF);
}
