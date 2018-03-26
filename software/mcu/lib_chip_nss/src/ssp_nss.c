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

static void SSP_Write2BFifo(NSS_SSP_T *pSSP, Chip_SSP_DATA_SETUP_T *xf_setup)
{
    if (xf_setup->tx_data) {
        Chip_SSP_SendFrame(pSSP, (*(uint16_t *) ((uint32_t) xf_setup->tx_data +
                                                 xf_setup->tx_cnt)));
    }
    else {
        Chip_SSP_SendFrame(pSSP, 0xFFFF);
    }

    xf_setup->tx_cnt += 2;
}

/** SSP macro: write 1 bytes to FIFO buffer */
static void SSP_Write1BFifo(NSS_SSP_T *pSSP, Chip_SSP_DATA_SETUP_T *xf_setup)
{
    if (xf_setup->tx_data) {
        Chip_SSP_SendFrame(pSSP, (*(uint8_t *) ((uint32_t) xf_setup->tx_data + xf_setup->tx_cnt)));
    }
    else {
        Chip_SSP_SendFrame(pSSP, 0xFF);
    }

    xf_setup->tx_cnt++;
}

/** SSP macro: read 1 bytes from FIFO buffer */
static void SSP_Read2BFifo(NSS_SSP_T *pSSP, Chip_SSP_DATA_SETUP_T *xf_setup)
{
    uint16_t rDat;

    while ((Chip_SSP_GetStatus(pSSP, SSP_STAT_RNE) == SET) &&
           (xf_setup->rx_cnt < xf_setup->length)) {
        rDat = Chip_SSP_ReceiveFrame(pSSP);
        if (xf_setup->rx_data) {
            *(uint16_t *) ((uint32_t) xf_setup->rx_data + xf_setup->rx_cnt) = rDat;
        }

        xf_setup->rx_cnt += 2;
    }
}

/** SSP macro: read 2 bytes from FIFO buffer */
static void SSP_Read1BFifo(NSS_SSP_T *pSSP, Chip_SSP_DATA_SETUP_T *xf_setup)
{
    uint16_t rDat;

    while ((Chip_SSP_GetStatus(pSSP, SSP_STAT_RNE) == SET) &&
           (xf_setup->rx_cnt < xf_setup->length)) {
        rDat = Chip_SSP_ReceiveFrame(pSSP);
        if (xf_setup->rx_data) {
            *(uint8_t *) ((uint32_t) xf_setup->rx_data + xf_setup->rx_cnt) = (uint8_t)rDat;
        }

        xf_setup->rx_cnt++;
    }
}

/* Returns clock for the peripheral block */
static CLOCK_PERIPHERAL_T Chip_SSP_GetClockIndex(NSS_SSP_T *pSSP)
{
    CLOCK_PERIPHERAL_T clkSSP;

    if (pSSP == NSS_SSP0) {
        clkSSP = CLOCK_PERIPHERAL_SPI0;
    }

    return clkSSP;
}

/* Returns reset ID for the peripheral block */
static SYSCON_PERIPHERAL_RESET_T Chip_SSP_GetResetIndex(NSS_SSP_T *pSSP)
{
    SYSCON_PERIPHERAL_RESET_T resetSSP;

    if (pSSP == NSS_SSP0) {
        resetSSP = SYSCON_PERIPHERAL_RESET_SPI0;
    }

    return resetSSP;
}

/* Returns reset ID for the peripheral block */
static void Chip_SSP_SetSSPClkDivider(NSS_SSP_T *pSSP, uint32_t div)
{
    if (pSSP == NSS_SSP0) {
        Chip_Clock_SPI0_SetClockDiv((int)div);
    }
}

/* Returns SSP peripheral clock for the peripheral block */
static uint32_t Chip_SSP_GetPCLKkRate(NSS_SSP_T *pSSP)
{
    uint32_t sspCLK = 0;

    if (pSSP == NSS_SSP0) {
        sspCLK = (uint32_t)Chip_Clock_SPI0_GetClockFreq();
    }

    return sspCLK;
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/*Set up output clocks per bit for SSP bus*/
void Chip_SSP_SetClockRate(NSS_SSP_T *pSSP, uint32_t clk_rate, uint32_t prescale)
{
    uint32_t temp;
    temp = pSSP->CR0 & (~(SSP_CR0_SCR(0xFF)));
    pSSP->CR0 = temp | (SSP_CR0_SCR(clk_rate));
    pSSP->CPSR = prescale;
}

/* SSP Polling Read/Write in blocking mode */
uint32_t Chip_SSP_RWFrames_Blocking(NSS_SSP_T *pSSP, Chip_SSP_DATA_SETUP_T *xf_setup)
{
    /* Clear all remaining frames in RX FIFO */
    while (Chip_SSP_GetStatus(pSSP, SSP_STAT_RNE)) {
        Chip_SSP_ReceiveFrame(pSSP);
    }

    /* Clear status */
    Chip_SSP_ClearIntPending(pSSP, SSP_INT_CLEAR_BITMASK);

    if (Chip_SSP_GetDataSize(pSSP) > SSP_BITS_8) {
        while (xf_setup->rx_cnt < xf_setup->length || xf_setup->tx_cnt < xf_setup->length) {
            /* write data to buffer */
            if (( Chip_SSP_GetStatus(pSSP, SSP_STAT_TNF) == SET) && ( xf_setup->tx_cnt < xf_setup->length) ) {
                SSP_Write2BFifo(pSSP, xf_setup);
            }

            /* Check overrun error */
            if (Chip_SSP_GetRawIntStatus(pSSP, SSP_RORRIS) == SET) {
                return ERROR;
            }

            /* Check for any data available in RX FIFO */
            SSP_Read2BFifo(pSSP, xf_setup);
        }
    }
    else {
        while (xf_setup->rx_cnt < xf_setup->length || xf_setup->tx_cnt < xf_setup->length) {
            /* write data to buffer */
            if (( Chip_SSP_GetStatus(pSSP, SSP_STAT_TNF) == SET) && ( xf_setup->tx_cnt < xf_setup->length) ) {
                SSP_Write1BFifo(pSSP, xf_setup);
            }

            /* Check overrun error */
            if (Chip_SSP_GetRawIntStatus(pSSP, SSP_RORRIS) == SET) {
                return ERROR;
            }

            /* Check for any data available in RX FIFO */
            SSP_Read1BFifo(pSSP, xf_setup);
        }
    }
    if (xf_setup->tx_data) {
        return xf_setup->tx_cnt;
    }
    else if (xf_setup->rx_data) {
        return xf_setup->rx_cnt;
    }

    return 0;
}

/* SSP Polling Write in blocking mode */
uint32_t Chip_SSP_WriteFrames_Blocking(NSS_SSP_T *pSSP, uint8_t *buffer, uint32_t buffer_len)
{
    uint32_t tx_cnt = 0, rx_cnt = 0;

    /* Clear all remaining frames in RX FIFO */
    while (Chip_SSP_GetStatus(pSSP, SSP_STAT_RNE)) {
        Chip_SSP_ReceiveFrame(pSSP);
    }

    /* Clear status */
    Chip_SSP_ClearIntPending(pSSP, SSP_INT_CLEAR_BITMASK);

    if (Chip_SSP_GetDataSize(pSSP) > SSP_BITS_8) {
        uint16_t *wdata16;

        wdata16 = (uint16_t *) buffer;

        while (tx_cnt < buffer_len || rx_cnt < buffer_len) {
            /* write data to buffer */
            if ((Chip_SSP_GetStatus(pSSP, SSP_STAT_TNF) == SET) && (tx_cnt < buffer_len)) {
                Chip_SSP_SendFrame(pSSP, *wdata16);
                wdata16++;
                tx_cnt += 2;
            }

            /* Check overrun error */
            if (Chip_SSP_GetRawIntStatus(pSSP, SSP_RORRIS) == SET) {
                return ERROR;
            }

            /* Check for any data available in RX FIFO */
            while (Chip_SSP_GetStatus(pSSP, SSP_STAT_RNE) == SET) {
                Chip_SSP_ReceiveFrame(pSSP);    /* read dummy data */
                rx_cnt += 2;
            }
        }
    }
    else {
        uint8_t *wdata8;

        wdata8 = buffer;

        while (tx_cnt < buffer_len || rx_cnt < buffer_len) {
            /* write data to buffer */
            if ((Chip_SSP_GetStatus(pSSP, SSP_STAT_TNF) == SET) && (tx_cnt < buffer_len)) {
                Chip_SSP_SendFrame(pSSP, *wdata8);
                wdata8++;
                tx_cnt++;
            }

            /* Check overrun error */
            if (Chip_SSP_GetRawIntStatus(pSSP, SSP_RORRIS) == SET) {
                return ERROR;
            }

            /* Check for any data available in RX FIFO */
            while (Chip_SSP_GetStatus(pSSP, SSP_STAT_RNE) == SET && rx_cnt < buffer_len) {
                Chip_SSP_ReceiveFrame(pSSP);    /* read dummy data */
                rx_cnt++;
            }
        }
    }

    return tx_cnt;

}

/* SSP Polling Read in blocking mode */
uint32_t Chip_SSP_ReadFrames_Blocking(NSS_SSP_T *pSSP, uint8_t *buffer, uint32_t buffer_len)
{
    uint32_t rx_cnt = 0, tx_cnt = 0;

    /* Clear all remaining frames in RX FIFO */
    while (Chip_SSP_GetStatus(pSSP, SSP_STAT_RNE)) {
        Chip_SSP_ReceiveFrame(pSSP);
    }

    /* Clear status */
    Chip_SSP_ClearIntPending(pSSP, SSP_INT_CLEAR_BITMASK);

    if (Chip_SSP_GetDataSize(pSSP) > SSP_BITS_8) {
        uint16_t *rdata16;

        rdata16 = (uint16_t *) buffer;

        while (tx_cnt < buffer_len || rx_cnt < buffer_len) {
            /* write data to buffer */
            if ((Chip_SSP_GetStatus(pSSP, SSP_STAT_TNF) == SET) && (tx_cnt < buffer_len)) {
                Chip_SSP_SendFrame(pSSP, 0xFFFF);   /* just send dummy data */
                tx_cnt += 2;
            }

            /* Check overrun error */
            if (Chip_SSP_GetRawIntStatus(pSSP, SSP_RORRIS) == SET) {
                return ERROR;
            }

            /* Check for any data available in RX FIFO */
            while (Chip_SSP_GetStatus(pSSP, SSP_STAT_RNE) == SET && rx_cnt < buffer_len) {
                *rdata16 = Chip_SSP_ReceiveFrame(pSSP);
                rdata16++;
                rx_cnt += 2;
            }
        }
    }
    else {
        uint8_t *rdata8;

        rdata8 = buffer;

        while (tx_cnt < buffer_len || rx_cnt < buffer_len) {
            /* write data to buffer */
            if ((Chip_SSP_GetStatus(pSSP, SSP_STAT_TNF) == SET) && (tx_cnt < buffer_len)) {
                Chip_SSP_SendFrame(pSSP, 0xFF); /* just send dummy data      */
                tx_cnt++;
            }

            /* Check overrun error */
            if (Chip_SSP_GetRawIntStatus(pSSP, SSP_RORRIS) == SET) {
                return ERROR;
            }

            /* Check for any data available in RX FIFO */
            while (Chip_SSP_GetStatus(pSSP, SSP_STAT_RNE) == SET && rx_cnt < buffer_len) {
                *rdata8 = (uint8_t)Chip_SSP_ReceiveFrame(pSSP);
                rdata8++;
                rx_cnt++;
            }
        }
    }

    return rx_cnt;

}

/* Clean all data in RX FIFO of SSP */
void Chip_SSP_Int_FlushData(NSS_SSP_T *pSSP)
{
    if (Chip_SSP_GetStatus(pSSP, SSP_STAT_BSY)) {
        while (Chip_SSP_GetStatus(pSSP, SSP_STAT_BSY)) {}
    }

    /* Clear all remaining frames in RX FIFO */
    while (Chip_SSP_GetStatus(pSSP, SSP_STAT_RNE)) {
        Chip_SSP_ReceiveFrame(pSSP);
    }

    /* Clear status */
    Chip_SSP_ClearIntPending(pSSP, SSP_INT_CLEAR_BITMASK);
}

/* SSP Interrupt Read/Write with 8-bit frame width */
Status Chip_SSP_Int_RWFrames8Bits(NSS_SSP_T *pSSP, Chip_SSP_DATA_SETUP_T *xf_setup)
{
    /* Check overrun error in RIS register */
    if (Chip_SSP_GetRawIntStatus(pSSP, SSP_RORRIS) == SET) {
        return ERROR;
    }

    if ((xf_setup->tx_cnt != xf_setup->length) || (xf_setup->rx_cnt != xf_setup->length)) {
        /* check if RX FIFO contains data */
        SSP_Read1BFifo(pSSP, xf_setup);

        while ((Chip_SSP_GetStatus(pSSP, SSP_STAT_TNF)) && (xf_setup->tx_cnt != xf_setup->length)) {
            /* Write data to buffer */
            SSP_Write1BFifo(pSSP, xf_setup);

            /* Check overrun error in RIS register */
            if (Chip_SSP_GetRawIntStatus(pSSP, SSP_RORRIS) == SET) {
                return ERROR;
            }

            /*  Check for any data available in RX FIFO */
            SSP_Read1BFifo(pSSP, xf_setup);
        }

        return SUCCESS;
    }

    return ERROR;
}

/* SSP Interrupt Read/Write with 16-bit frame width */
Status Chip_SSP_Int_RWFrames16Bits(NSS_SSP_T *pSSP, Chip_SSP_DATA_SETUP_T *xf_setup)
{
    /* Check overrun error in RIS register */
    if (Chip_SSP_GetRawIntStatus(pSSP, SSP_RORRIS) == SET) {
        return ERROR;
    }

    if ((xf_setup->tx_cnt != xf_setup->length) || (xf_setup->rx_cnt != xf_setup->length)) {
        /* check if RX FIFO contains data */
        SSP_Read2BFifo(pSSP, xf_setup);

        while ((Chip_SSP_GetStatus(pSSP, SSP_STAT_TNF)) && (xf_setup->tx_cnt != xf_setup->length)) {
            /* Write data to buffer */
            SSP_Write2BFifo(pSSP, xf_setup);

            /* Check overrun error in RIS register */
            if (Chip_SSP_GetRawIntStatus(pSSP, SSP_RORRIS) == SET) {
                return ERROR;
            }

            /*  Check for any data available in RX FIFO          */
            SSP_Read2BFifo(pSSP, xf_setup);
        }

        return SUCCESS;
    }

    return ERROR;
}

/* Set the SSP operating modes, master or slave */
void Chip_SSP_SetMaster(NSS_SSP_T *pSSP, bool master)
{
    if (master) {
        Chip_SSP_Set_Mode(pSSP, SSP_MODE_MASTER);
    }
    else {
        Chip_SSP_Set_Mode(pSSP, SSP_MODE_SLAVE);
    }
}

/* Set the clock frequency for SSP interface */
void Chip_SSP_SetBitRate(NSS_SSP_T *pSSP, uint32_t bitRate)
{
    uint32_t ssp_clk, cr0_div, cmp_clk, prescale;

    ssp_clk = Chip_SSP_GetPCLKkRate(pSSP);

    cr0_div = 0;
    cmp_clk = 0xFFFFFFFF;
    prescale = 2;

    while (cmp_clk > bitRate) {
        cmp_clk = ssp_clk / ((cr0_div + 1) * prescale);
        if (cmp_clk > bitRate) {
            cr0_div++;
            if (cr0_div > 0xFF) {
                cr0_div = 0;
                prescale += 2;
            }
        }
    }

    Chip_SSP_SetClockRate(pSSP, cr0_div, prescale);
}

/* Get the clock frequency for SSP interface */
uint32_t Chip_SSP_GetBitRate(NSS_SSP_T *pSSP)
{
    uint32_t serialClockRate = (pSSP->CR0 >> 8) & 0xFF;
    uint32_t prescaler = pSSP->CPSR;
    uint32_t sysClk = (uint32_t)Chip_Clock_System_GetClockFreq();
    uint32_t bitrate = (uint32_t)(sysClk / (prescaler * (serialClockRate + 1.0)));
    return bitrate;
}

/* Initialize the SSP */
void Chip_SSP_Init(NSS_SSP_T *pSSP)
{
    Chip_Clock_Peripheral_EnableClock(Chip_SSP_GetClockIndex(pSSP));

    /* The SPI0 Clock divisor needs to be set to the same value as the System Clock divisor to prevent synchronization issues */
    Chip_SSP_SetSSPClkDivider(pSSP, (uint32_t)Chip_Clock_System_GetClockDiv());

    Chip_SysCon_Peripheral_DeassertReset(Chip_SSP_GetResetIndex(pSSP));

    Chip_SSP_Set_Mode(pSSP, SSP_MODE_MASTER);
    Chip_SSP_SetFormat(pSSP, SSP_BITS_8, SSP_FRAME_FORMAT_SPI, SSP_CLOCK_CPHA0_CPOL0);
    Chip_SSP_SetBitRate(pSSP, 100000);
}

/* De-initializes the SSP peripheral */
void Chip_SSP_DeInit(NSS_SSP_T *pSSP)
{
    Chip_SSP_Disable(pSSP);

    Chip_Clock_Peripheral_DisableClock(Chip_SSP_GetClockIndex(pSSP));
    Chip_SSP_SetSSPClkDivider(pSSP, 0);
}
