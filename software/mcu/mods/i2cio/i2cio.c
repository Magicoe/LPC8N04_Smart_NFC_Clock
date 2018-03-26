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


/* -------------------------------------------------------------------------
 * Include files
 * ------------------------------------------------------------------------- */

#include <string.h>
#include "chip.h"
#include "i2cio.h"

/* -------------------------------------------------------------------------
 * Overriding WEAK alias
 * ------------------------------------------------------------------------- */

void I2C0_IRQHandler(void)
{
    if (Chip_I2C_IsMasterActive(I2C0)) {
        Chip_I2C_MasterStateHandler(I2C0);
    }
    else {
        Chip_I2C_SlaveStateHandler(I2C0);
    }
}

#if I2CIO_ENABLE_RX

/* -------------------------------------------------------------------------
 * types and defines
 * ------------------------------------------------------------------------- */

/**
 * - An I2C transfer starts with one byte address + data payload. To receive 1 byte, rxSz must be set to at
 *  least 2: rxSz indicates the transfer size, _not_ the size of rxBuff.
 * - Another quirk (due to the I2C HW block) is that the CONSET and CONCLR registers (prepared via cclr) are
 *  used for the next to be received byte, _not_ the just received byte.
 * .
 * - When a transfer is ongoing, a NACK is prepared when _after_ receiving one data byte (handleSlaveXferState,
 *  case SLA: Data received) less than 2 bytes remain available. Since we never NACK due to lack of storage,
 *  rxSz must be at least 3.
 */
#define RXSZ 3

/**
 * Storage for received data via any communication means.
 * - When @c head equals @c tail, the buffer is empty
 * - When @c head is one less than @c tail, the buffer is full.
 * - Thus: the buffer can at most contain size-of-buffer - 1 elements.
 * .
 */
typedef struct RX_BUFFER_S {
    uint8_t data[I2CIO_RX_BUFFER_SIZE + 1]; /**< Used as circular buffer with @a head and @a tail. */
    volatile int head; /**< Points to the first free place in the @a data buffer. */
    volatile int tail; /**< Points to the oldest written not freed byte in the @a data buffer. */
} RX_BUFFER_T;

static RX_BUFFER_T sRxBuffer;

static I2C_XFER_T sI2cXfer;

static int GetRxFreeSpace(void);
static int GetRxUsedSpace(void);
static void I2cSlaveEventHandler(I2C_ID_T id, I2C_EVENT_T event);

/* -------------------------------------------------------------------------
 * static functions
 * ------------------------------------------------------------------------- */

static int GetRxUsedSpace(void)
{
    /* copy head as it might change during a context switch */
    int head = sRxBuffer.head;
    int used;

    if (head >= sRxBuffer.tail) {
        used = head - sRxBuffer.tail;
    }
    else {
        used = (int)sizeof(sRxBuffer.data) + head - sRxBuffer.tail;
    }
    return used;
}

/* This function is either called in an ISR or protected with a critical section - no need to protect against preemption */
static int GetRxFreeSpace(void)
{
    return (int)sizeof(sRxBuffer.data) - GetRxUsedSpace() - 1;
}

static void I2cSlaveEventHandler(I2C_ID_T id, I2C_EVENT_T event)
{
    (void)id;/* gracefully do nothing and avoid compiler warnings */

    /* We are assuming we're fast enough to receive and copy byte per byte.
     * We can thus assign rxBuff to the next free spot in the rx buffer, and
     * when this function is called - under interrupt - one byte has been received.
     */
    int space = GetRxFreeSpace();
    if (event == I2C_EVENT_SLAVE_RX) {
        if (space > 0) {
            sRxBuffer.head = (sRxBuffer.head == sizeof(sRxBuffer.data) - 1) ? 0 : sRxBuffer.head + 1;
            sI2cXfer.rxBuff = &sRxBuffer.data[sRxBuffer.head];
            space--;
        }
    }
    if (space > 0) {
        sI2cXfer.rxSz = RXSZ;
    }
    else {
        /* Block the I2C reception of more bytes. */
        sI2cXfer.rxSz = 0;
    }
}
#endif

/* -------------------------------------------------------------------------------- */

void I2CIO_Init(void)
{
#if I2CIO_ENABLE_RX
    sRxBuffer.head = 0;
    sRxBuffer.tail = 0;
#endif
    /* The order of the calls below is not important. It just seems more logical to first set the correct PIO function,
     * then to get the i2C block out of reset.
     */
    Chip_IOCON_SetPinConfig(NSS_IOCON, IOCON_PIO0_4, IOCON_FUNC_1);
    Chip_IOCON_SetPinConfig(NSS_IOCON, IOCON_PIO0_5, IOCON_FUNC_1);
    Chip_SysCon_Peripheral_DeassertReset(SYSCON_PERIPHERAL_RESET_I2C0);

    Chip_I2C_Init(I2C0);
#if I2CIO_ENABLE_TX
    Chip_I2C_SetClockRate(I2C0, I2CIO_CLK_RATE);
#endif

#if I2CIO_ENABLE_RX
    sI2cXfer.slaveAddr = I2CIO_OWN_ADDRESS << 1;
    sI2cXfer.rxBuff = &sRxBuffer.data[sRxBuffer.head];
    sI2cXfer.rxSz = RXSZ;
    sI2cXfer.txBuff = NULL;
    sI2cXfer.txSz = 0; /* No transmission in slave mode. Sending always happens as I2C master. */
#endif

#if I2CIO_ENABLE_TX
    Chip_I2C_SetMasterEventHandler(I2C0, Chip_I2C_EventHandler);
#endif
#if I2CIO_ENABLE_RX
    Chip_I2C_SlaveSetup(I2C0, I2C_SLAVE_0, &sI2cXfer, I2cSlaveEventHandler, 0);
#endif
    NVIC_EnableIRQ(I2C0_IRQn);
}

void I2CIO_DeInit(void)
{
#if I2CIO_ENABLE_RX
    sRxBuffer.head = sRxBuffer.tail;
#endif
    Chip_IOCON_SetPinConfig(NSS_IOCON, IOCON_PIO0_4, IOCON_FUNC_0 | IOCON_RMODE_INACT);
    Chip_SysCon_Peripheral_AssertReset(SYSCON_PERIPHERAL_RESET_I2C0);
    Chip_IOCON_SetPinConfig(NSS_IOCON, IOCON_PIO0_5, IOCON_FUNC_0 | IOCON_RMODE_INACT);
    Chip_I2C_DeInit(I2C0);
    NVIC_DisableIRQ(I2C0_IRQn);
}

bool I2CIO_Tx(const uint8_t* data, int length)
{
#if I2CIO_ENABLE_TX
    int sent;
    ASSERT(length > 0);
    sent = Chip_I2C_MasterSend(I2C0, (I2CIO_INTERFACE_ADDRESS), data, length);
    return (sent == length);
#else
    /* gracefully do nothing and avoid compiler warnings */
    (void)data;
    (void)length;
    return false;
#endif
}

bool I2CIO_Rx(uint8_t* pOutBuffer, int length, bool clear)
{
#if I2CIO_ENABLE_RX
    int used;
    bool success;
    ASSERT(length > 0);
    used = GetRxUsedSpace();
    success = length <= used;
    if (success && (pOutBuffer != NULL)) {
        if (sRxBuffer.tail + length >= (int)sizeof(sRxBuffer.data)) {
            /* Data to copy is wrapped. */
            uint32_t n = sizeof(sRxBuffer.data) - (uint32_t)sRxBuffer.tail;
            memcpy(pOutBuffer, &sRxBuffer.data[sRxBuffer.tail], n);
            memcpy(pOutBuffer + n, &sRxBuffer.data[0], (uint32_t)length - n);
        }
        else {
            memcpy(pOutBuffer, &sRxBuffer.data[sRxBuffer.tail], (uint32_t)length);
        }
        if (clear) {
            sRxBuffer.tail = (sRxBuffer.tail + length) % (int)sizeof(sRxBuffer.data);

            /* Critical section - ISR may preempt between the test and the assignment (and buffer might become full) */
            __disable_irq();
            if(GetRxFreeSpace() > 0){
                sI2cXfer.rxSz = RXSZ;
            }
            __enable_irq();
            /* End of critical section */
        }
    }
    return success;
#else
    /* gracefully do nothing and avoid compiler warnings */
    (void)pOutBuffer;
    (void)length;
    (void)clear;
    return false;
#endif
}

int I2CIO_GetChar(void)
{
#if I2CIO_ENABLE_RX
    int rx = 0;
    if (!I2CIO_Rx((uint8_t *)&rx, 1, true)) {
        rx = EOF;
    }
    return rx;
#else
    return EOF;
#endif

}
