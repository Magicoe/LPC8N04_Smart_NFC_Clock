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
#include "string.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
#define NFC_LAST_ACCESS_START_MASK (0xFF << 8) /*!< Start address mask for last RF access */
#define NFC_LAST_ACCESS_END_MASK (0xFF) /*!< End address mask for last RF access */
#define NFC_LAST_ACCESS_DIR_MASK (0x1 << 16) /*!< Direction mask for last RF access */
#define NFC_SHARED_MEM_PAGE_OFFSET 4 /*!< Page offset for shared memory */

static volatile uint32_t stickyMEM_WRITE = 0; /*!< Page offset for shared memory */

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/
/* Initializes the NFC HW block */
void Chip_NFC_Init(NSS_NFC_T *pNFC)
{
    pNFC->BUF[0] = 0x000000FE; /*Terminate shared RAM with a terminator TLV*/

    /* Disable NFC interrupt. */
    Chip_NFC_Int_SetEnabledMask(NSS_NFC, NFC_INT_NONE);

    /* Clear any pending stray NFC interrupts. */
    Chip_NFC_Int_ClearRawStatus(NSS_NFC, NFC_INT_ALL);

    /* Disabling bypass mode. */
    pNFC->CFG = 0x0;
}

/* De-initializes the NFC HW block */
void Chip_NFC_DeInit(NSS_NFC_T *pNFC)
{
    pNFC->BUF[0] = 0x000000FE; /*Terminate shared RAM with a terminator TLV*/

    /* Disable NFC interrupt. */
    Chip_NFC_Int_SetEnabledMask(NSS_NFC, NFC_INT_NONE);

    /* Clear any pending stray NFC interrupts. */
    Chip_NFC_Int_ClearRawStatus(NSS_NFC, NFC_INT_ALL);
}

/* Returns the current NFC HW block status information */
NFC_STATUS_T Chip_NFC_GetStatus(NSS_NFC_T *pNFC)
{
    return (NFC_STATUS_T)(pNFC->SR & 0xFF);
}

/* Enable/Disable the NFC interrupts */
void Chip_NFC_Int_SetEnabledMask(NSS_NFC_T *pNFC, NFC_INT_T mask)
{
    pNFC->IMSC = mask & NFC_INT_ALL;
}

/* Retrieves the NFC interrupt enable mask */
NFC_INT_T Chip_NFC_Int_GetEnabledMask(NSS_NFC_T *pNFC)
{
    return (NFC_INT_T)(pNFC->IMSC & NFC_INT_ALL);
}

/* Retrieves a bitVector with the RAW NFC interrupt flags */
NFC_INT_T Chip_NFC_Int_GetRawStatus(NSS_NFC_T *pNFC)
{
    return (NFC_INT_T)(pNFC->RIS & NFC_INT_ALL);
}

/* Clears the required NFC interrupt flags */
void Chip_NFC_Int_ClearRawStatus(NSS_NFC_T *pNFC, NFC_INT_T flags)
{
    stickyMEM_WRITE |= (pNFC->RIS & NFC_INT_MEMWRITE);

    pNFC->IC = flags & NFC_INT_ALL;

    /* To ensure at least one other APB access to the RFID/NFC shared memory interface before exiting the ISR.
     * Refer to NFC chapter of User Manual */
    pNFC->IC = flags & NFC_INT_ALL;
}

/* Sets the target address used for interrupt generation */
void Chip_NFC_SetTargetAddress(NSS_NFC_T *pNFC, uint32_t offset)
{
    pNFC->TARGET = offset + NFC_SHARED_MEM_PAGE_OFFSET;
}

/* Returns the target address used for interrupt generation */
uint32_t Chip_NFC_GetTargetAddress(NSS_NFC_T *pNFC)
{
    return (uint32_t)(pNFC->TARGET - NFC_SHARED_MEM_PAGE_OFFSET);
}

/* Returns the start and end 32-bit word offset from start of BUF and the direction of last RF access */
bool Chip_NFC_GetLastAccessInfo(NSS_NFC_T *pNFC, uint32_t *pStartOffset, uint32_t *pEndOffset)
{
    *pStartOffset = ((pNFC->LAST_ACCESS & NFC_LAST_ACCESS_START_MASK) >> 8) - NFC_SHARED_MEM_PAGE_OFFSET;
    *pEndOffset = (pNFC->LAST_ACCESS & NFC_LAST_ACCESS_END_MASK) - NFC_SHARED_MEM_PAGE_OFFSET;
    return (bool)((pNFC->LAST_ACCESS & NFC_LAST_ACCESS_DIR_MASK) == NFC_LAST_ACCESS_DIR_MASK);
}

/* Writes a block of words to the BUF, and returns success/failure of write operation */
bool Chip_NFC_WordWrite(NSS_NFC_T *pNFC, uint32_t * pDest, const uint32_t * pSrc, int n)
{
    stickyMEM_WRITE = 0;
    pNFC->IC = NFC_INT_MEMWRITE;

    /* To ensure at least one other APB access to the RFID/NFC shared memory interface before exiting the ISR.
     * Refer to NFC chapter of User Manual. */
    pNFC->IC = NFC_INT_MEMWRITE;

    memcpy(pDest, pSrc, (uint32_t)n*4);

    if (((pNFC->RIS & NFC_INT_MEMWRITE) == NFC_INT_MEMWRITE) || stickyMEM_WRITE) {
        return false;
    }
    return true;
}

/* Reads a block of bytes from the BUF, and returns success/failure of read operation */
bool Chip_NFC_ByteRead(NSS_NFC_T *pNFC, uint8_t * pDest, const uint8_t * pSrc, int n)
{
    stickyMEM_WRITE = 0;
    pNFC->IC = NFC_INT_MEMWRITE;

    /* To ensure at least one other APB access to the RFID/NFC shared memory interface before exiting the ISR.
     * Refer to NFC chapter of User Manual. */
    pNFC->IC = NFC_INT_MEMWRITE;

    memcpy(pDest, pSrc, (uint32_t)n);

    if (((pNFC->RIS & NFC_INT_MEMWRITE) == NFC_INT_MEMWRITE) || stickyMEM_WRITE) {
        return false;
    }
    return true;
}
