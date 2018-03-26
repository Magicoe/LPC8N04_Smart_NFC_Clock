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


/**
 * @defgroup MODS_NSS_I2CIO_DFT Diversity settings
 * @ingroup MODS_NSS_I2CIO
 * These 'defines' capture the diversity settings of the module. The displayed values refer to the default settings.
 * To override the default settings, place the defines with their desired values in the application app_sel.h header
 * file: the compiler will pick up your defines before parsing this file.
 * @{
 */
#ifndef __I2CIO_DFT_H_
#define __I2CIO_DFT_H_

/**
 * I2C Clock Rate used for I2C communication. Expressed in Hz.
 * The maximum clock rate that can be chosen here is dependent on the ARM system clock. The table below lists the
 * minimum ARM system clock rate in MHz for each situation:
 *
 *  | I2C clock rate | I2CIO_CLK_RATE || Tx @n in master mode | RX @n in slave mode |
 *  | :--:           | :--:           || :--:                 | :--:                |
 *  | 100 kHz        | 100000         || 4 MHz                | 2 MHz               |
 *  | 400 kHz        | 400000         || 8 MHz                | 4 MHz               |
 *
 * @note The ARM system clock is @b not checked during initialization #Chip_I2C_Init. It is up to the caller to ensure
 *  a sufficiently high ARM system clock rate.
 * @note Changing the ARM system clock after initialization is for the same reasons not possible if a correct and
 *  continuous functioning of the I2 communication is required. This is also @b not checked.
 */
#if !I2CIO_CLK_RATE
    #define I2CIO_CLK_RATE 100000
#endif
#if (I2CIO_CLK_RATE < 1000) || (I2CIO_CLK_RATE > 400000)
    #error I2CIO_CLK_RATE must be in the range [1000, 400000].
#endif

/**
 * By default, receiving I2C data is enabled, making use of #I2CIO_RX_BUFFER_SIZE and #I2CIO_OWN_ADDRESS for proper
 * configuration. Define this value to 0 to disable I2C RX, reducing the code weight of this module.
 */
#ifndef I2CIO_ENABLE_RX
    #define I2CIO_ENABLE_RX 1
#endif

/**
 * By default, transmitting I2C data is enabled, making use of and #I2CIO_INTERFACE_ADDRESS for proper
 * configuration. Define this value to 0 to disable I2C TX, reducing the code weight of this module.
 */
#ifndef I2CIO_ENABLE_TX
    #define I2CIO_ENABLE_TX 1
#endif

#if !I2CIO_ENABLE_RX && !I2CIO_ENABLE_TX
    #error Both I2C RX and TX are disabled.
#endif

/**
 * The size in bytes of the receive buffer that must be allocated by this module. The buffer is used as a FIFO buffer,
 * blocking the reception of newer data whenever an overflow would otherwise occur. The value given here is also
 * the maximum size for which #I2CIO_Rx can return @c true.
 * The default value allows for storage of 32 bytes before data is discarded.
 */
#if !I2CIO_RX_BUFFER_SIZE
    #define I2CIO_RX_BUFFER_SIZE 32
#endif
#if I2CIO_RX_BUFFER_SIZE && I2CIO_RX_BUFFER_SIZE < 2
    #error I2CIO_RX_BUFFER_SIZE must be at least 2.
#endif
#if I2CIO_ENABLE_RX && !I2CIO_RX_BUFFER_SIZE
    #error I2C RX is enabled both no RX buffer has been defined.
#endif

/**
 * Own I2C Slave address: data sent by another IC to this I2C address is captured by NHS I2C HW block and stored in the
 * buffer with size #I2CIO_RX_BUFFER_SIZE.
 * If not defined, a default value is selected.
 * @see I2CIO_INTERFACE_ADDRESS
 */
#if !I2CIO_OWN_ADDRESS
    #define I2CIO_OWN_ADDRESS 0x7F
#endif

/**
 * I2C Slave address
 * The I2C Slave address of the INTERFACE chip MUST be lower than the OWN one
 * @note The default address given below - used if not defined by the application itself - can be the address of a
 *  debugger board that implements the I2C to USB Virtual Serial Port bridge. It is selected to have a lower value than
 *  #I2CIO_OWN_ADDRESS so that the Master write operation in the direction of the INTERFACE chip will always have
 *  priority, in case the two chips simultaneously try to perform an I2C Master write operation.
 * @see I2CIO_OWN_ADDRESS
 */
#if !I2CIO_INTERFACE_ADDRESS
    #define I2CIO_INTERFACE_ADDRESS 0x7E
#endif

#endif /** @} */
