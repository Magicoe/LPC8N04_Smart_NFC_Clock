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


#ifndef __STORAGE_DFT_H_
#define __STORAGE_DFT_H_

/** @defgroup MODS_NSS_STORAGE_DFT Diversity Settings
 *  @ingroup MODS_NSS_STORAGE
 *
 * The application can adapt the Storage module to better fit the different application scenarios through the use of
 * diversity flags in the form of defines below. Sensible defaults are chosen; to override the default settings, place
 * the defines with their desired values in the application app_sel.h header file: the compiler will pick up your
 * defines before parsing this file.
 *
 * Additional notes regarding some flags:
 * - By default, the assigned EEPROM region takes up 1KB and is located just below the RO EEPROM rows. This storage
 *  space can be moved and resized by adapting #STORAGE_EEPROM_FIRST_ROW and #STORAGE_EEPROM_LAST_ROW.
 * - By default, the complete FLASH not used by your program binary file is used for data storage - the available free
 *  FLASH space is automatically determined. It can also be manually assigned: move and resize the storage space in
 *  FLASH by adapting #STORAGE_FLASH_FIRST_PAGE and #STORAGE_FLASH_LAST_PAGE.
 * - Data is stored decompressed in EEPROM; a chance is given to the application to compress the data before it is moved
 *  to FLASH. For this, define both #STORAGE_DECOMPRESS_CB and #STORAGE_DECOMPRESS_CB.
 * - Ensure matching sizes are chosen for EEPROM and FLASH: only full EEPROM regions are moved to FLASH. This is
 *  certainly necessary in case data is stored decompressed in FLASH: optimal storage results are then achieved when
 *  the assigned FLASH size is a multiple of the assigned EEPROM size.
 * .
 *
 * These flags may be overridden:
 * - #STORAGE_CONFIG_ALON_REGISTER
 * - #STORAGE_EEPROM_FIRST_ROW
 * - #STORAGE_EEPROM_LAST_ROW
 * - #STORAGE_FLASH_FIRST_PAGE
 * - #STORAGE_FLASH_LAST_PAGE
 * - #STORAGE_TYPE
 * - #STORAGE_BITSIZE
 * - #STORAGE_SIGNED
 * - #STORAGE_WORKAREA
 * - #STORAGE_BLOCK_SIZE_IN_SAMPLES
 * - #STORAGE_COMPRESS_CB
 * - #STORAGE_DECOMPRESS_CB
 * - #STORAGE_ALWAYS_TRY_FAST_RECOVERY
 * .
 *
 * These defines are fixed or derived from the above flags and may not be defined or redefined in an application:
 * - #STORAGE_EEPROM_ROW_COUNT
 * - #STORAGE_EEPROM_SIZE
 * - #STORAGE_WORKAREA_SELF_DEFINED
 * - #STORAGE_WORKAREA_SIZE
 * - #STORAGE_MAX_BLOCK_SIZE_IN_SAMPLES
 * - #STORAGE_BLOCK_HEADER_SIZE
 * - #STORAGE_MAX_UNCOMPRESSED_BLOCK_SIZE_IN_BITS
 * - #STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS
 * - #STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BYTES
 *
 * @{
 */

#ifndef STORAGE_CONFIG_ALON_REGISTER
    /**
     * The offset to one of the registers #NSS_PMU_T.GPREG in the ALON domain that may be used by the storage module
     * for his own housekeeping. This register must be guaranteed not to be touched outside the storage module.
     * @note More information about the ALON registers can be found in the @ref PMU_NSS. Accessing them is done via
     *  #Chip_PMU_GetRetainedData and #Chip_PMU_SetRetainedData.
     */
    #define STORAGE_CONFIG_ALON_REGISTER 4
#endif
#if !(STORAGE_CONFIG_ALON_REGISTER >= 0) || !(STORAGE_CONFIG_ALON_REGISTER <= 4)
    #error Invalid value for STORAGE_CONFIG_ALON_REGISTER
#endif

/* ------------------------------------------------------------------------- */

#ifndef STORAGE_EEPROM_FIRST_ROW
    /**
     * The first EEPROM row assigned for sample storage. Starting from the first byte in this row, until the last byte
     * in #STORAGE_EEPROM_LAST_ROW, the storage module has full control: no other code may touch this EEPROM region.
     * @note By default, the EEPROM row one kB below the locked EEPROM rows will be chosen.
     */
    #define STORAGE_EEPROM_FIRST_ROW (EEPROM_NR_OF_RW_ROWS - (1024 / EEPROM_ROW_SIZE))
#endif
#if !(STORAGE_EEPROM_FIRST_ROW >= 0) || !(STORAGE_EEPROM_FIRST_ROW < EEPROM_NR_OF_RW_ROWS)
    #error Invalid value for STORAGE_EEPROM_FIRST_ROW
#endif

#ifndef STORAGE_EEPROM_LAST_ROW
    /**
     * The last EEPROM row assigned for sample storage. Starting from the first byte in #STORAGE_EEPROM_FIRST_ROW,
     * until the last byte in this row, the storage module has full control: no other code may touch this EEPROM region.
     * @note the size of the EEPROM region is not required to be a multiple of the FLASH sector size.
     */
    #define STORAGE_EEPROM_LAST_ROW (EEPROM_NR_OF_RW_ROWS - 1)
#endif
#if !(STORAGE_EEPROM_LAST_ROW >= STORAGE_EEPROM_FIRST_ROW) || !(STORAGE_EEPROM_LAST_ROW < EEPROM_NR_OF_RW_ROWS)
    #error Invalid value for STORAGE_EEPROM_LAST_ROW
#endif

/** The number of EEPROM rows assigned for sample storage. */
#define STORAGE_EEPROM_ROW_COUNT (STORAGE_EEPROM_LAST_ROW - STORAGE_EEPROM_FIRST_ROW + 1)

/** The size of the assigned EEPROM storage in bytes */
#define STORAGE_EEPROM_SIZE (STORAGE_EEPROM_ROW_COUNT * EEPROM_ROW_SIZE)

/* ------------------------------------------------------------------------- */

#ifndef STORAGE_FLASH_FIRST_PAGE
    /**
     * The first FLASH page assigned for storage. Starting from the first byte in this page, until the last byte
     * in #STORAGE_FLASH_LAST_PAGE, the Storage module has full control: no other code will touch this FLASH region.
     * @note A special value is @c 0. This indicates that the first empty FLASH page @b after the @c .text and @c .data
     *  sections must be used. This page is automatically determined during link time by the Storage module's code.
     * @note The page is not required to be sector aligned.
     * @warning If a non-zero value is specified, it is the responsibility of the application programmer to ensure the
     *  FLASH location pointed to is outside the @c .text and @c .data sections.
     */
    #define STORAGE_FLASH_FIRST_PAGE 0
#endif
#if !(STORAGE_FLASH_FIRST_PAGE >= 0) || (STORAGE_FLASH_FIRST_PAGE >= FLASH_NR_OF_RW_SECTORS * FLASH_PAGES_PER_SECTOR)
    #error Invalid value for STORAGE_FLASH_FIRST_PAGE
#endif

#ifndef STORAGE_FLASH_LAST_PAGE
    /**
     * The last FLASH page assigned for storage. Starting from the first byte in #STORAGE_FLASH_FIRST_PAGE,
     * until the last byte in this page, the Storage module has full control: no other code will touch this FLASH
     * region.
     * @note the size of the region is not required to be a multiple of the FLASH sector size.
     */
    #define STORAGE_FLASH_LAST_PAGE (FLASH_NR_OF_RW_SECTORS * FLASH_PAGES_PER_SECTOR - 1)
#endif
#if (STORAGE_FLASH_LAST_PAGE < STORAGE_FLASH_FIRST_PAGE) || (STORAGE_FLASH_LAST_PAGE >= FLASH_NR_OF_RW_SECTORS * FLASH_PAGES_PER_SECTOR)
    #error Invalid value for STORAGE_FLASH_LAST_PAGE
#endif
#if (STORAGE_FLASH_FIRST_PAGE == 0) && (STORAGE_FLASH_LAST_PAGE != (FLASH_NR_OF_RW_SECTORS * FLASH_PAGES_PER_SECTOR - 1))
    #error STORAGE_FLASH_FIRST_PAGE is determined at link time; STORAGE_FLASH_LAST_PAGE can thus not be checked at precompilation time.
    #error STORAGE_FLASH_LAST_PAGE must be set to the highest possible value in this case.
#endif

/* ------------------------------------------------------------------------- */

#ifndef STORAGE_TYPE
    /**
     * The type that is used to store one decompressed sample. When writing, samples are to be delivered using this
     * type - see #Storage_Write; when reading, samples are returned again using this type -
     * see #Storage_Read.
     */
    #define STORAGE_TYPE uint8_t
#endif

#ifndef STORAGE_BITSIZE
    /**
     * The number of bits that are to be stored for each sample. For each sample given using #Storage_Write
     * this number of LSBits are written;
     */
    #define STORAGE_BITSIZE 8
#endif
#if (STORAGE_BITSIZE <= 0)
    #error Invalid value for STORAGE_BITSIZE
#endif

#ifndef STORAGE_SIGNED
    /**
     * Define or set to a non-zero value (e.g. @c 1) to indicate that #STORAGE_TYPE is a signed type.
     * - If defined, the bit at position @code (#STORAGE_BITSIZE - 1) @endcode will be treated as the sign bit:
     *  when reading out samples from EEPROM or FLASH, this bit will be propagated left up to the MSBit of
     *  #STORAGE_TYPE.
     * - If not defined, the MSBits at positions #STORAGE_BITSIZE and up will be set to @c 0.
     * .
     * @warning Setting this diversity flag to 1 while #STORAGE_TYPE is a structure, while raise compiler errors.
     * see #Storage_Read.
     */
    #define STORAGE_SIGNED 0
#endif

/** The size in bytes of the required memory for this module */
#define STORAGE_WORKAREA_SIZE ((FLASH_PAGE_SIZE * 2) + STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BYTES)

#ifdef STORAGE_WORKAREA
    #undef STORAGE_WORKAREA_SELF_DEFINED
#else
    /**
     * Used internally to know when to provide (static) memory for this.
     * @internal
     */
    #define STORAGE_WORKAREA_SELF_DEFINED 1
    /**
     * An array, or a pointer to an array of at least #STORAGE_WORKAREA_SIZE bytes.
     * During the lifetime of this module - starting from the start of the call to #Storage_Init until the end of
     * the call to #Storage_DeInit, this memory is under full control by this module.
     * @pre Must be word (32 bits) aligned.
     */
    #define STORAGE_WORKAREA sStorage_Workarea
#endif

/* ------------------------------------------------------------------------- */

/**
 * The maximum allowed value for #STORAGE_BLOCK_SIZE_IN_SAMPLES. After this many samples, the assigned EEPROM region is
 * completely filled and no new sample can be written to EEPROM before its contents are moved to FLASH first.
 * @hideinitializer
 */
#define STORAGE_MAX_BLOCK_SIZE_IN_SAMPLES (((STORAGE_EEPROM_SIZE * 8) - 128) / STORAGE_BITSIZE)
/* Magic value 128 is checked at compile time in storage.c */

/** Defines the number of bits required to store one block of samples. */
#define STORAGE_MAX_UNCOMPRESSED_BLOCK_SIZE_IN_BITS (STORAGE_MAX_BLOCK_SIZE_IN_SAMPLES * STORAGE_BITSIZE)

/**
 * The size in bytes of the meta data stored just in front of the (compressed) data block in FLASH.
 */
#define STORAGE_BLOCK_HEADER_SIZE 2

#ifndef STORAGE_BLOCK_SIZE_IN_SAMPLES
    /**
     * After writing this number of samples to EEPROM, the module will try to compress them all at once - see
     * #STORAGE_COMPRESS_CB - and move them to FLASH - defined by #STORAGE_FLASH_FIRST_PAGE and #STORAGE_FLASH_LAST_PAGE.
     * @note This size determines the value of #STORAGE_WORKAREA_SIZE - the larger this number, the more SRAM is required,
     * and the larger chunks the compression and decompression algorithms have to work with.
     * @note By default, the block size will be chosen such that an uncompressed block can fit in one FLASH sector,
     *  including the accompanying meta data.
     */
    #define STORAGE_BLOCK_SIZE_IN_SAMPLES (((1024 - STORAGE_BLOCK_HEADER_SIZE) * 8) / STORAGE_BITSIZE)
    #if STORAGE_BLOCK_SIZE_IN_SAMPLES > STORAGE_MAX_BLOCK_SIZE_IN_SAMPLES
        #undef STORAGE_BLOCK_SIZE_IN_SAMPLES
        #define STORAGE_BLOCK_SIZE_IN_SAMPLES STORAGE_MAX_BLOCK_SIZE_IN_SAMPLES
    #endif
#endif
#if ((STORAGE_BLOCK_SIZE_IN_SAMPLES <= 1) || (STORAGE_BLOCK_SIZE_IN_SAMPLES > STORAGE_MAX_BLOCK_SIZE_IN_SAMPLES))
    #error Invalid value for STORAGE_BLOCK_SIZE_IN_SAMPLES
#endif

/** Defines the number of bits required to store one block of samples. */
#define STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS (STORAGE_BLOCK_SIZE_IN_SAMPLES * STORAGE_BITSIZE)

/**
 * Helper. Performs integer division, rounding up.
 * @param n Must be a positive number.
 * @param d Must be a strict positive number
 */
#define STORAGE_IDIVUP(n, d) (((n)+(d)-1)/(d))

/** Defines the number of bytes required to store one block of samples. */
#define STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BYTES STORAGE_IDIVUP(STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS, 8)

/* ------------------------------------------------------------------------- */


#ifdef STORAGE_COMPRESS_CB
    #undef STORAGE_COMPRESS_CB_SELF_DEFINED
#else
    /**
     * The name of the function - @b not a function pointer - of type #pStorage_CompressCb_t that is able to
     * compress #STORAGE_BLOCK_SIZE_IN_SAMPLES packed samples of type #STORAGE_TYPE where #STORAGE_BITSIZE bits
     * are retained for each.
     */
    #define STORAGE_COMPRESS_CB Storage_DummyCompressCb
#endif

#ifdef STORAGE_DECOMPRESS_CB
    #undef STORAGE_DECOMPRESS_CB_SELF_DEFINED
#else
    /**
     * The name of the function - @b not a function pointer - of type #pStorage_DecompressCb_t that is able to
     * decompress #STORAGE_BLOCK_SIZE_IN_SAMPLES packed samples of type #STORAGE_TYPE where #STORAGE_BITSIZE bits
     * are retained for each.
     */
    #define STORAGE_DECOMPRESS_CB Storage_DummyDecompressCb
#endif

#ifndef STORAGE_ALWAYS_TRY_FAST_RECOVERY
    /**
     * Several recovery mechanisms are set in place to allow the Storage module to recover the exact state where it left
     * off under all circumstances. This includes the ability to recover after a complete power-off. Moreover, it tries
     * to do so as fast as possible, to allow the application a fast reaction time while using all the information the
     * Storage module can provide.
     * One such recovery mechanism writes status information to EEPROM at always the exact same location and may
     * therefore - under special conditions - hit the endurance limit: the maximum write times the EEPROM is guaranteed
     * to be able to cope with. This recovery mechanism is put in place to ensure that only one slow search after
     * leaving the power-off mode is required to recover the full contents: a second time power-off is left and
     * #Storage_Init is called, the recovery of the state will happen fast as it will not require a full inspection
     * of the full assigned EEPROM region. This information remains valid as long as no new samples are given to the
     * Storage module.
     * In the unlikely case that @b and the application is to leave power-off mode more than 10.000 times, @b and it
     * writes at least 1 sample each time after leaving power-off mode, the define below must be set to @c 0. The
     * penalty is a slow recovery each time after leaving power-off, irrespective whether samples are added or not.
     */
    #define STORAGE_ALWAYS_TRY_FAST_RECOVERY 1
#endif

/** @} */

#endif
