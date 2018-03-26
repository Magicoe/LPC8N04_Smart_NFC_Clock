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


#ifndef __STORAGE_H_
#define __STORAGE_H_

/** @defgroup MODS_NSS_STORAGE storage: NVM Storage module
 * @ingroup MODS_NSS
 * The storage module allows an application to store a large amount of samples of identical size in non-volatile
 * memories EEPROM and FLASH. @n
 * It will:
 * - abstract away where and how the samples are stored,
 * - provide an easy interface via which samples can be written to and read from,
 * - ensure no padding bits are added, packing all bits to maximize storage capacity,
 * - prefer EEPROM over FLASH for storage to minimize time and current consumption,
 * - move data from EEPROM to FLASH automatically whenever necessary to make sure the newest data is always in EEPROM,
 * - allow application-specific compression of the data just before moving data from EEPROM to FLASH,
 * - allow application-specific decompression of the data just before they are read out again,
 * - survive intermediate power-off states: it can recover its full state without losing any stored sample based on the
 *  contents on the non-volatile memory alone.
 *
 * The Storage module will use both EEPROM and FLASH to store bits of data. The newest samples are always stored in
 * EEPROM; when the reserved space in EEPROM is full, all the data is moved to FLASH, and the EEPROM is reused.
 * Writing samples always means appending them to the already written samples; it is not possible to edit the already
 * written stream of bits.
 * When reading back again, the user can control the starting read position using a sequence number. It is automatically
 * deduced where the corresponding sample is written, whether it is compressed, and what needs to be done to be able to
 * return the requested sample(s).
 *
 * It is expected that each application that requires this module includes it and configures the diversity settings of
 * the module according to its specific needs.
 *
 * @par Diversity
 *  This module supports diversity: some settings define the type and size of the sample, others define the
 *  EEPROM and FLASH regions placed under control of this module, the rest control the behavior of the module.
 *  Check @ref MODS_NSS_STORAGE_DFT for all diversity parameters.
 *
 * @par Memory Requirements
 *  The Storage module requires a large chunk of SRAM, called its workarea - see #STORAGE_WORKAREA and
 *  #STORAGE_WORKAREA_SIZE. This is used for two purposes:
 *  - When storing samples, and a move from EEPROM to FLASH is required, the assigned compress callback - see
 *      #STORAGE_COMPRESS_CB - is given a pointer inside this SRAM memory. The output is then stored in FLASH.
 *  - When reading samples from FLASH, the assigned decompress callback - see #STORAGE_DECOMPRESS_CB - is called as
 *      little as possible: its output is cached in the work area to speed up subsequent reads.
 *  .
 *  If two operations in your code require such a big chunk of memory, you can overlap them if they don't have to
 *  operate concurrently. Diversity setting #STORAGE_WORKAREA can be used for this.
 *
 * @par How to use the module
 *  -# Define the best diversity settings for your application, or accept the default ones. Keep in mind that
 *      the setting #STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BYTES influences the required work area
 *      size #STORAGE_WORKAREA_SIZE.
 *  -# Initialize the EEPROM driver and the Storage module, in that order.
 *  -# Read and write samples as necessary, in any order or quantity that is required for your use case.
 *  -# De-initialize the Storage module and the EEPROM driver, in that order.
 *  .
 *
 * @par Example
 *  @snippet storage_mod_example_1.c storage_mod_example_1
 *
 * @warning These functions are not re-entrant. Calling these functions from multiple threads or in an interrupt is
 *  highly discouraged.
 * @warning The Storage module requires the exclusive use of one register in the always-on domain - see
 *  #STORAGE_CONFIG_ALON_REGISTER. Under no circumstance may this register be touched from outside the Storage module.
 *
 * @{
 */

#include "board.h"
#include "storage_dft.h"

/* ------------------------------------------------------------------------- */

/**
 * Whenever data is about to be moved from EEPROM to FLASH, the application is notified via a callback of this
 * prototype. It then has a chance to compress the data before it is written to FLASH.
 * The application is in charge of:
 * - reading the data from EEPROM using @c eepromByteOffset as starting point,
 * - compressing exactly @c bitCount bits,
 * - storing the end result in @c out and
 * - returning the size of the data written in @c pOut, expressed in @b bits.
 * @param eepromByteOffset The absolute offset in bytes to the EEPROM where the first sample is stored.
 * @param bitCount An exact total of #STORAGE_BLOCK_SIZE_IN_SAMPLES samples are stored. They are
 *  packed together, i.e. without any padding bits. This argument will @b always equal
 *  @c #STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS.
 * @param pOut A pointer to SRAM where the compressed data must be stored in. The buffer has a size of
 *  @c #STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BYTES bytes.
 * @return The size of the compressed data @b in @b bits. When @c 0 is returned, or a value bigger than or equal to
 *  @c #STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS, the uncompressed data will be stored instead, and the corresponding
 *  decompression callback of type #pStorage_DecompressCb_t will not be called when reading out the data later.
 * @warning It is @b not allowed to call any function of this module during the lifetime of the callback.
 * @see STORAGE_COMPRESS_CB
 */
typedef int (*pStorage_CompressCb_t)(int eepromByteOffset, int bitCount, void * pOut);

/**
 * Whenever data is read from FLASH, the application is notified via a callback of this prototype. It then has a chance
 * to decompress the data before it is used to fulfill the read request #Storage_Read.
 * The application is in charge of:
 * - reading the data from FLASH using @c data as starting point,
 * - decompressing one block of compressed data stored from that point with size @c bitCount,
 * - storing the end result - #STORAGE_BLOCK_SIZE_IN_SAMPLES samples - in @c out. The samples @b must be written
 *  packed together, i.e. without any padding bits.
 * @param pData The absolute byte address to FLASH memory where the start of the (compressed) data block is found.
 * @param bitCount The size in bits of the (compressed) data block.
 * @param pOut A pointer to SRAM where the compressed data must be stored in. The buffer has a size of
 *  @c #STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BYTES bytes.
 * @return The number of bits written to in @c pOut.
 *  If @c #STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS is returned, the operation is assumed to be
 *  successful and the decompressed samples are now available. @c Any other value indicates a decompression failure:
 *  the samples stored in that block can @b not be retrieved any more, and the call to #Storage_Read which initiated
 *  this callback will fail.
 * @warning It is @b not allowed to call any function of this module during the lifetime of the callback.
 * @see STORAGE_DECOMPRESS_CB
 */
typedef int (*pStorage_DecompressCb_t)(const uint8_t * pData, int bitCount, void * pOut);

/* ------------------------------------------------------------------------- */

/**
 * This function must be the first function to call in this module after going to deep power down or power-off power
 * save mode. Not calling this function will result at best in random data being written and read, and possibly generate
 * hard faults.
 * @pre The value written in #STORAGE_CONFIG_ALON_REGISTER is either exactly what was stored after leaving
 *  #Storage_DeInit, or equal to @c 0.
 * @pre EEPROM is initialized and is ready to be used.
 * @post When this function returns, #Storage_Seek still needs to be called before being able to read samples. This is
 *  also required when reading from the start of the memory, i.e. when reading the oldest stored sample referred to as
 *  index 0.
 * @warning This function can run for a long time before completion, when it is forced to scan the assigned EEPROM
 *  region to recover the data that is stored in EEPROM and/or FLASH. The time spent is depending on both the assigned
 *  EEPROM region and the number of samples stored in EEPROM: the longer the region, the more time spent; the more
 *  samples stored in EEPROM, the less time spent recovering. Under worst case conditions using a system clock of
 *  0.5 MHz, this may last more than 10 msec.
 *  This penalty only occurs under these combined conditions:
 *  - the IC went to power-off, losing all information stored in the register #STORAGE_CONFIG_ALON_REGISTER
 *  - data was added to the storage module after leaving a previous power-off mode
 *  .
 */
void Storage_Init(void);

/**
 * This function must be the last function to call in this module before going to deep power down or power-off power
 * save mode.
 * @pre EEPROM is initialized and is ready to be used.
 * @post Possibly, an EEPROM flush was necessary, but that has finished when this function returns.
 * @warning Loss of power before this call will result in loss of the newly added samples. Loss of power during this
 *  call may result in loss of @b all stored samples. Only after completion of this function the retention of all
 *  samples and the metadata required to track them is guaranteed.
 */
void Storage_DeInit(void);

/**
 * @return The total number of samples currently stored, in EEPROM and FLASH combined.
 */
int Storage_GetCount(void);

/**
 * Resets the storage module to a pristine state.
 * @param checkAll The contents in FLASH must be erased, as they can only be written again after being erased.
 *  The only completely fail-safe way is to check all words. This is time consuming - albeit much less than
 *  unconditionally erasing. Since with normal usage - ignoring (inadvertent) memory overwrites and firmware updates -
 *  a check on the first word provides the same information, the choice is given to the caller.
 *  When in doubt the safest is to use @c true.
 *  - When @c true is given, @b all words of the FLASH memory assigned for sample storage are checked.
 *  - When @c false is given, @b only the very first word of the FLASH memory assigned for sample storage is checked.
 *  If @b one checked word does not contain the erased value (@c 0xFFFFFFFF), @b all FLASH memory is erased.
 * @warning This call can possibly - regardless of the value of the argument - erase all FLASH memory assigned for
 *  sample storage. This action can worst case take up to 31 erase cycles. Aligning #STORAGE_FLASH_FIRST_PAGE and
 *  #STORAGE_FLASH_LAST_PAGE to sector boundaries can reduce this to the minimum of 1 erase cycle.
 */
void Storage_Reset(bool checkAll);

/**
 * Stores @c n samples in persistent storage.
 * @pre EEPROM is initialized
 * @param pSamples Pointer to the start of the array where to copy the samples from. For each element of the array,
 *  only the #STORAGE_BITSIZE LSBits are copied.
 * @param n The size of the array @c samples points to, in number of #STORAGE_TYPE elements.
 * @return The number of samples written. This value may be @c 0 or any number of samples less than or equal to @c n.
 * @note When a value less than @c n is returned, at least one of these errors occurred:
 *  - There is insufficient storage capacity
 *  - Compressing of samples was necessary during the call, but that operation yielded an error.
 *  .
 * @note A prior call to #Storage_Seek is @b not required, as writing will always @b append the new samples.
 * @post An EEPROM flush is ongoing when at least one sample was written.
 */
int Storage_Write(STORAGE_TYPE * pSamples, int n);

/**
 * Determines which sample is read out next in a future call to #Storage_Read. This call is
 * required to be called once before calling #Storage_Read one or multiple times.
 * @param n Must be a positive number. A value of @c 0 indicates the oldest sample, which was written first.
 * @return @c true when the sought for sequence number was found; @c false otherwise.
 * @post the next call to #Storage_Read will either return at least one sample - the value
 *  which was stored as the @c n-th sample - or fail - when less than @c n samples are being stored at the time of
 *  calling this function.
 * @note After initialization, the default sequence number is @b not @c 0. A first call to this function is required
 *  before #Storage_Read can retrieve samples.
 */
bool Storage_Seek(int n);

/**
 * Reads @c n samples from persistent storage, starting from the sequence number set in #Storage_Seek.
 * @pre EEPROM is initialized
 * @pre A prior successful call to #Storage_Seek is required before this function can succeed.
 * @note Multiple reads can be issued after calling #Storage_Seek once, each time fetching samples in sequence.
 * @param [out] pSamples : Pointer to an array of @n elements, where the read samples are copied to. Upon successful
 *  completion, each element will contain one sample, where only the #STORAGE_BITSIZE LSBits are used per
 *  element; the remainder MSBits are set to 0.
 * @param n : The size of the array @c samples points to, in number of #STORAGE_TYPE elements.
 * @return The number of samples read. This value may be @c 0 or any number of samples less than @c n.
 *  The remainder of the elements with a higher index may have been written to, but must be ignored.
 * @note When a value less than @c n is returned, at least one of these errors occurred:
 *  - There was no prior successful call to #Storage_Seek
 *  - Decompressing of samples was necessary during the call, but that operation yielded an error.
 *  - There are no more samples stored.
 */
int Storage_Read(STORAGE_TYPE * pSamples, int n);

/** @} */
#endif
