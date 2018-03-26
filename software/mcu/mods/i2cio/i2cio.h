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


#ifndef __I2CIO_NSS_H_
#define __I2CIO_NSS_H_

/** @defgroup MODS_NSS_I2CIO i2cio: I2C multi-master communication module
 * @ingroup MODS_NSS
 * The I2CIO module provides a way to asynchronously transmit and receive binary data over the I2C interface.
 *
 * The I2CIO module will hide all details of the I2C driver: it will initialize the driver, configure it and ensure all
 * interrupts are enabled and captured by the module itself. The user of the I2CIO module only needs to use the API
 * functions below to have full I2C communication.
 * On the other hand, lower level access to the I2C driver still remains possible.
 *
 * The I2CIO module will always set up the I2C driver in multi-master mode: transmission is only done as
 * master, while reception is only done as slave.
 *
 * @par Diversity
 *  This module supports diversity, like the I2C clock rate, I2C addresses and RX buffer size.
 *  Check @ref MODS_NSS_I2CIO_DFT for all diversity parameters.
 *
 * @par Caveats
 *  - The ARM system clock must be set 'high enough' for I2C to work. See #I2CIO_CLK_RATE for a full explanation.
 *  - The implicit assumption is made that the ARM system clock is set fast enough to receive and copy all data byte per
 *      byte. This assumption can only hold if the I2C IRQ vector handler is not preempted for a longer period of time.
 *      In particular, calls to #I2CIO_Tx and #I2CIO_Rx must be done on a lower priority.
 *  - The I2C driver will be configured to @b always acknowledge the reception of data bytes, even if no more room is
 *      available to store the byte.
 *  - During initialization of the I2C driver, the desired I2C clock rate is determined using the ARM system clock
 *      frequency by setting a divider.Changing the ARM system clock while using the I2 driver will thus result in a
 *      corresponding change in the I2C clock rate. There is no code foreseen to prevent this, or to correct the I2C
 *      clock rate when this happens.
 *  .
 *
 * @par Connections with other HW blocks
 *  This module will
 *  - make sure that all HW prerequisites are met for I2C
 *      This includes a check on the correct system clock frequency.
 *  - initialize and configure the I2C driver
 *      This implies that all previous configurations to the I2C driver are lost.
 *  - configures the correct PIO lines for I2C operation
 *      This mandates the initialization of the IOCON driver before using this module - which is already taken care of
 *      by the board library initialization function #Board_Init.
 *  - enable and implement the corresponding interrupt vector handler #I2C0_IRQHandler.
 *  .
 *  Simultaneous direct access to the I2C driver is still possible, but only @b after initializing this module.
 *
 * @par Multiple use of the I2C driver.
 *  It is still possible to directly access the I2C driver for both transmission and reception, even when receiving
 *  and/or transmitting through this module is enabled via the diversity settings.
 *  - To configure additional receive addresses:
 *      first initialize the module - #I2CIO_Init - and only after that to configure an additional slave address to
 *      react to - #Chip_I2C_SlaveSetup
 *  - To transmit data to additional addresses:
 *      do not setup your master device as it has already been done and just call the I2C transmit function with the
 *      I2C interface address of your choosing - #Chip_I2C_MasterSend.
 *  .
 *
 * @par Example Transmit / receive sequence
 *  Demonstrates the transmission of a 4-byte sequence ['1', '4', '1', '5'], followed by the reception of a 4-byte
 *  sequence, which is then sent out unaltered before stopping I2C.
 *
 *  @snippet i2cio_mod_example_1.c i2cio_mod_example_1
 *
 * @{
 */

#include "stdio.h"
#include "i2cio/i2cio_dft.h"

/**
 * Initializes the module.
 * @pre This must be the first function to be called from this module after startup or after #I2CIO_DeInit has been
 *  called.
 * @pre The IOCON driver must have been initialized beforehand. This is already taken care of by the board library
 *  initialization function #Board_Init.
 * @see Chip_IOCON_Init
 */
void I2CIO_Init(void);

/**
 * De-initializes the module.
 * Ensures interrupts are disabled, internal storage is cleared and the I2C block is disabled.
 * @post De-initialize the IOCON driver when it is not used elsewhere.
 */
void I2CIO_DeInit(void);

/**
 * Transmits the given data over I2C, as I2C master.
 * This call is blocking until all data has been sent, or until transmission has been given up.
 * @param pData : Must point to a contiguous linear array containing the bytes to transmit.
 * @param length : The number of bytes to transmit.
 *  @pre @c length must be a strict positive value.
 * @note This call will always fail if #I2CIO_ENABLE_TX is not set to 1.
 * @return
 *  - true if all the bytes have been transmitted successfully.
 *  - false otherwise. This can indicate a NACK from the RX Slave, arbitration loss or a (likely electrical) bus error.
 *  .
 * @note This call has no effect on the received but not yet cleared bytes. These remain present in the internal RX
 *  buffer (whose size is determined by #I2CIO_RX_BUFFER_SIZE) and can be fetched at any convenient time using
 *  #I2CIO_Rx.
 * @note
 */
bool I2CIO_Tx(const uint8_t * pData, int length);

/**
 * Retrieves a number of already received data bytes.
 * Reception over the I2C bus is done without the need to call this function. Instead, this function retrieves the bytes
 * that have already been received and are stored in the internal RX buffer - whose size you can control using
 * #I2CIO_RX_BUFFER_SIZE. This function can thus be called at irregular intervals or at very slow intervals without
 * risking to loose bytes. Polling this function slowly does not impede I2C data reception. On the other hand, if not
 * called at all, the internal RX buffer will completely fill up, and no data can be received until the oldest bytes are
 * explicitly cleared by calling this function with @c clear set to @c true.
 * @param pOutBuffer : Points to a contiguous linear array where the data may be copied to.
 *  May be @c NULL. Even if @c NULL, the other arguments are checked and used. This allows the caller to empty (part of)
 *  the FIFO RX buffer without the need to copy the data, and to check the returned value - @c true or @c false.
 * @param length : The number of received bytes to copy. Only if this number of bytes is available, a copy action is
 *  started. If this value is greater than #I2CIO_RX_BUFFER_SIZE, this call will always fail.
 * @param clear : If @c false, a subsequent call to this function will retrieve the same bytes.
 *  If @c true and @c length bytes are available, the oldest that many bytes are discarded from the RX buffer at the end
 *  of this function, even if @c pOutBuffer equals @c NULL.
 * @note This call is non-blocking.
 * @note This call will always fail if #I2CIO_ENABLE_RX is not set to 1.
 * @return
 *  - @c true if @c length bytes were available
 *  - @c false otherwise.
 *  .
 *  @post @c pOutBuffer will only be changed when @c true is returned.
 */
bool I2CIO_Rx(uint8_t * pOutBuffer, int length, bool clear);

/**
 * Convenience function to quickly get and consume 1 byte.
 * @return @c EOF if character was not received; else the character value.
 * @note This call will always return @c EOF if #I2CIO_ENABLE_RX is not set to 1.
 * @note The returned char will be cleared (consumed) from the receiver buffer.
 */
int I2CIO_GetChar(void);

/** @} */

#endif /* __I2CIO_NSS_H_ */
