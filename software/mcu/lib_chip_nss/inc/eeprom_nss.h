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


#ifndef __EEPROM_NSS_H_
#define __EEPROM_NSS_H_

/** @defgroup EEPROM_NSS eeprom: EEPROM driver
 * @ingroup DRV_NSS
 * The EEPROM contains two parts: a controller and a memory core.
 *
 * @par EEPROM controller
 *  The controller is the interface between the AHB and the EEPROM memory core.
 *  All interaction on and off the memory core is done via the controller (using #NSS_EEPROM_T).
 *
 * @par EEPROM memory core
 *  The EEPROM memory for a specific chip starts at address defined by #EEPROM_START. It is organized in rows
 *  with size #EEPROM_ROW_SIZE (bytes). The total amount of EEPROM rows is defined by #EEPROM_NR_OF_R_ROWS which are
 *  all readable. The total amount of user-writable rows is defined by #EEPROM_NR_OF_RW_ROWS.
 *
 * @par How to use EEPROM
 *  Before the EEPROM memory can be accessed, it must be initialized (#Chip_EEPROM_Init).
 *  Initializing means providing clock and power to the EEPROM controller and EEPROM memory. @n
 *  Once initialized, the EEPROM memory can be read (#Chip_EEPROM_Read) or written
 *  (#Chip_EEPROM_Write). Both functions need an offset in the EEPROM memory and a buffer with size.
 *  The @c offset for the read and write functions are relative to #EEPROM_START. @n
 *  Function #Chip_EEPROM_Flush makes sure that written data is effectively retained in the EEPROM memory.
 *
 * @warning Due to the HW specification concerning the ref. clock for the EEPROM block, proper working of EEPROM is only
 *  guaranteed while running a system clock of 500 kHz or higher. For a full list of clock restrictions in effect, see
 *  @ref NSS_CLOCK_RESTRICTIONS.
 *
 * @warning This driver is using the EEPROM page register as a cache hence, a flush (#Chip_EEPROM_Flush) can be postponed
 *  as long as we issue write orders (#Chip_EEPROM_Write) within the same page. One HW limitation has to be noted:
 *  "Do not write more than once on the same offset with different data. Data loss may occur." The user of the driver
 *  must, then, make sure that the same EEPROM offset is not written multiple times without a flush order in between.
 *
 * @par Example - Read, modify and write EEPROM content
 *  @snippet eeprom_nss_example_1.c eeprom_nss_example_1
 *
 * @{
 */

/** NSS EEPROM register block structure */
typedef struct NSS_EEPROM_S {
    __IO uint32_t CMD; /*!< EEPROM command register */
    __I uint32_t RESERVED1; /*   next field at offset 0x08 */
    __IO uint32_t RWSTATE; /*!< EEPROM read wait state register */
    __IO uint32_t PAUTOPROG; /*!< EEPROM auto programming register */
    __IO uint32_t WSTATE; /*!< EEPROM wait state register */
    __IO uint32_t CLKDIV; /*!< EEPROM clock divider register */
    __IO uint32_t PWRDWN; /*!< EEPROM power down register */
    __I uint32_t RESERVED2; /*   next field at offset 0x20 */
    __IO uint32_t MSSTART; /*!< EEPROM checksum start address register */
    __IO uint32_t MSSTOP; /*!< EEPROM checksum stop address register */
    __I uint32_t MSDATASIG; /*!< EEPROM Data signature register */
    __I uint32_t MSPARSIG; /*!< EEPROM parity signature register */
    __I uint32_t RESERVED3; /*   next field at offset 0x34 */
    __I uint32_t STATUS; /*!< EEPROM device status register */
    __I uint32_t RESERVED4[998]; /*   next field at offset 0xFD0 */
    __I uint32_t MODULE_CONFIG; /*!< Controller configuration options */
    __I uint32_t RESERVED5; /*   next field at offset 0xFD8 */
    __IO uint32_t INT_CLR_ENABLE; /*!< Clear interrupt enable bits */
    __IO uint32_t INT_SET_ENABLE; /*!< Set interrupt enable bits */
    __I uint32_t INT_STATUS; /*!< Interrupt status bits */
    __I uint32_t INT_ENABLE; /*!< Interrupt enable bits */
    __IO uint32_t INT_CLR_STATUS; /*!< Clear interrupt status bits */
    __IO uint32_t INT_SET_STATUS; /*!< Set interrupt status bits */
    __I uint32_t RESERVED6[3]; /*   next field at offset 0xFFC */
    __IO uint32_t MODULE_ID; /*!< Controller memory module identification */
} NSS_EEPROM_T;

/**
 * Initializes EEPROM peripheral.
 * Power and Clock are enabled for EEPROM and for EEPROM controller block.
 * Based upon the configured system clock, the right ref. clock divider is selected.
 * @param pEEPROM : The base address of the EEPROM peripheral on the chip
 * @warning EEPROM driver needs to be re-initialized, if System Clock frequency is changed after initialization.
 * @note EEPROM hardware needs a waiting time after enabling it, #Chip_EEPROM_Init will busy wait for this time,
 *  please refer to the user manual for the specific waiting time.
 * @note Before #Chip_EEPROM_Init is called, other API are not usable
 */
void Chip_EEPROM_Init(NSS_EEPROM_T *pEEPROM);

/**
 * Disables EEPROM peripheral.
 * Power and Clock are disabled for EEPROM and for EEPROM controller block.
 * @param pEEPROM : The base address of the EEPROM peripheral on the chip
 * @note After #Chip_EEPROM_DeInit, other API are not usable
 */
void Chip_EEPROM_DeInit(NSS_EEPROM_T *pEEPROM);

/**
 * Reads data from the EEPROM memory into a user allocated buffer
 * @param pEEPROM : The base address of the EEPROM peripheral on the chip
 * @param offset : EEPROM Offset, in bytes, from where to start reading
 * @param pBuf : Pointer to the user allocated buffer in ram
 * @param size : Number of bytes to copy
 * @note The offset is relative to #EEPROM_START as defined in chip.h
 * @note offset + size must not exceed ( #EEPROM_ROW_SIZE * #EEPROM_NR_OF_R_ROWS ) as defined in chip.h
 * @note There are no alignment requirements.
 * @note This function's timing is non deterministic; if, at function entrance, there is a pending flush for a row
 *  in targeted area [@c offset, @c offset + @c size], a flush including a busy wait is invoked.
 */
void Chip_EEPROM_Read(NSS_EEPROM_T *pEEPROM, int offset, void *pBuf, int size);

/**
 * Writes data from a user allocated buffer into the EEPROM memory
 * @param pEEPROM : The base address of the EEPROM peripheral on the chip
 * @param offset : EEPROM Offset, in bytes, from where to start reading
 * @param pBuf : Pointer to the data to be copied into EEPROM
 * @param size : Number of bytes to copy
 * @note The offset is relative to #EEPROM_START as defined in chip.h
 * @note offset + size must not exceed ( #EEPROM_ROW_SIZE * #EEPROM_NR_OF_RW_ROWS ) as defined in chip.h
 * @note There are no alignment requirements.
 * @note To be sure written data is effectively retained in the EEPROM memory, user needs to call #Chip_EEPROM_Flush
 * @note This funtion's timing is non deterministic, following cases will invoke a flush, including a busy wait:
 *  - At entrance, there is a pending flush for another row than first one in [@c offset, @c offset + @c size].
 *  - At entrance, there is a pending flush and current @c offset and/or @c size are not 16-bit aligned.
 *  - The targeted destination area [@c offset, @c offset + @c size] is spread over one or more EEPROM rows.
 *  .
 */
void Chip_EEPROM_Write(NSS_EEPROM_T *pEEPROM, int offset, void *pBuf, int size);

/**
 * If needed, this function flushes pending data into the EEPROM.
 * To be used only if the user wants to make sure written data is retained, for example before going to sleep.
 * @param pEEPROM : The base address of the EEPROM peripheral on the chip
 * @param wait : Indicates if the function needs to busy wait till flush operation is finished
 *  - @c true if driver needs to busy wait until flush action is completed
 *  - @c false if driver only needs to invoke the flush action whereupon it can return immediately
 *  .
 * @note Please refer to the user manual for the specific program time.
 * @note In case @c wait is @c false, there is no need to check if flushing is finished before calling
 *  #Chip_EEPROM_Read/#Chip_EEPROM_Write again.
 */
void Chip_EEPROM_Flush(NSS_EEPROM_T *pEEPROM, bool wait);

/**
 * Memsets a pattern into the EEPROM memory
 * @param pEEPROM : The base address of the EEPROM peripheral on the chip
 * @param offset : EEPROM Offset, in bytes, from where to start writing. The offset is relative to #EEPROM_START as
 *  defined in chip.h
 * @param pattern: Byte pattern to be set into EEPROM
 * @param size : Number of bytes to set
 * @note offset + size must not exceed ( #EEPROM_ROW_SIZE * #EEPROM_NR_OF_RW_ROWS ) as defined in chip.h
 * @note There are no alignment requirements.
 * @note To be sure written data is effectively retained in the EEPROM memory, user needs to call #Chip_EEPROM_Flush
 * @note This funtion's timing is non deterministic, following cases will invoke a flush, including a busy wait:
 *  - At entrance, there is a pending flush for another row than first one in [@c offset, @c offset + @c size].
 *  - At entrance, there is a pending flush and current @c offset and/or @c size are not 16-bit aligned.
 *  - The targeted destination area [@c offset, @c offset + @c size] is spread over one or more EEPROM rows.
 *  .
 */
void Chip_EEPROM_Memset(NSS_EEPROM_T *pEEPROM, int offset, uint8_t pattern, int size);
/**
 * @}
 */

#endif /* __EEPROM_NSS_H_ */

