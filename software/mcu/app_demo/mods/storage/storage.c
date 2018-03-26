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


#include <string.h>
#include "storage.h"

/**
 * @file
 *
 *  @par Maintaining state
 *      It is a crucial feature of this module to be able to resume operation at all times. Between de-initialization
 *      and re-initialization anything can happen, and no volatile memory can be trusted. Still, speed of operation is
 *      necessary. To be able to restore the exact state - done in #Storage_DeInit - three recovery mechanisms are
 *      used:
 *      - #RecoverInfo_t
 *      - #Hint_t
 *      - #Marker_t
 *      .
 *      These mechanisms are linked to each other as explained below. After recovery, state is maintained in SRAM using
 *      the structure #Storage_Instance_t, and in #Storage_DeInit, at least two of these three structures are updated.
 *
 *      - #RecoverInfo_t
 *          This data is stored in the designated ALON register #STORAGE_CONFIG_ALON_REGISTER and is by far the fastest
 *          and easiest way to recover the current state. It is always tried first, and if samples are found, the other
 *           structures are not looked at. If any value of this structure is non-zero, its data is trusted and used.
 *      - #Hint_t
 *           This data is stored in the last portion of the assigned EEPROM region, and also allows to recover the
 *           current state in a fast way. However, this structure is seldom updated and its contents may thus be wrong.
 *           Reason for not updating is to ensure the maximum number of writes (endurance) is never reached. Rather,
 *           this data will only accurately tell whether samples are stored or not. If samples are available, it will
 *           also provide a possible location to #Marker_t, which is allowed to be wrong. The possible location will
 *           only be correct after a full slow search to #Marker_t was carried out and before new data samples were
 *           added.
 *      - #Marker_t
 *           This data is always stored just after the last sample written in EEPROM. Its precise location is not known,
 *           as it is progressing together with the data. #RecoverInfo_t points to the start of this structure in the
 *           assigned EEPROM region; after a reset #Hint_t may point to it; if both are failing a full slow search is
 *           performed in the assigned EEPROM region, and if found, #Hint_t is updated to ensure a second reset is not
 *           slow anymore.
 *      .
 *
 *      When during recovery, a full backward search is performed, we rely on the length of the marker to eliminate
 *      false positives, i.e. to ensure no bit sequence exists that looks like a valid marker but are in reality one or
 *      more samples stored in EEPROM. See also the comments near the code in #FindMarker. Although highly improbable,
 *      there is still a tiny possibility that real-life data being stored results in such a false positive. Even then,
 *      most applications will never have the need to perform a full backward search, evading this problem. If your
 *      application is more likely to hit this, due to the use case being sufficiently different, and due to the samples
 *      being sufficiently similar, this module can be adapted to erase the full EEPROM after moving all data to FLASH -
 *      see #MoveSamplesFromEepromToFlash. During our continuous tests using randomized data of different lengths, we
 *      never encountered this problem and therefore decided not to suffer the extra cost (time and power consumption)
 *      of implementing this.
 *
 *  @par Recovery information update examples
 *      In #Storage_DeInit, at least the ALON register (#RecoverInfo_t) and the EEPROM marker (#Marker_t) is updated.
 *      Assuming only one sample was added, the EEPROM contents would then change as visually depicted below:
 *
 *      @note the pictures below use this legend:
 *          - @c aaa, @c bbb, ... @c fff: samples, with bitsize #STORAGE_BITSIZE
 *          - @c AA: ALON register, formatted according to #RecoverInfo_t
 *          - @c HH: hint, formatted according to #Hint_t
 *          - @c MMMMMMM: marker, formatted according to #Marker_t
 *          - @c BB: block info, of size #FLASH_DATA_HEADER_SIZE
 *          - @c xxxx: block bytes, the (compressed) data bytes moved from EEPROM and now stored in FLASH
 *          .
 *
 *      @code
 *          EEPROM                             EEPROM
 *         with samples a..d                  with samples a..e
 *        +---------+                        +---------+
 *        | aaabbbc |                        | aaabbbc |
 *        | ccdddMM |     After calling      | ccdddee |
 *        | MMMMM   |     Storage_Write      | eMMMMMM |
 *        |         |     with n=1:          | MM      |
 *        |         |  ------------------>   |         |
 *        |      HH |                        |      HH |
 *        +---------+                        +---------+
 *      @endcode
 *
 *      The ALON register will then point to the new location of the marker; while the hint will not be updated and will
 *      for sure contain faulty information.
 *
 *      @dot
 *          digraph "Alon, Marker OK" {
 *              node [shape=box];
 *              splines="ortho"
 *              rankdir="LR"
 *              subgraph a {
 *                  rank=same
 *                  AA              [label="ALON register", URL="\ref STORAGE_CONFIG_ALON_REGISTER"]
 *              }
 *              subgraph b {
 *                  rank=same
 *                  MMMMMMM         [label="Moving marker in EEPROM", URL="\ref Marker_t"]
 *                  HH              [label="Hint information on a\nfixed EEPROM location", URL="\ref HINT_ABSOLUTE_BYTE_OFFSET"]
 *              }
 *              subgraph c {
 *                  rank=same
 *                  flash           [label="flashByteCursor", URL="\ref Storage_Instance_t.flashByteCursor"]
 *                  random          [shape=point]
 *              }
 *
 *              AA -> MMMMMMM
 *              HH -> random
 *              MMMMMMM -> flash
 *          }
 *      @enddot
 *
 *      When power gets lost, the ALON register contents is reset to @c 0, and will then point to a faulty location as
 *      well.
 *
 *      @dot
 *          digraph "Marker OK" {
 *              node [shape=box];
 *              splines="ortho"
 *              rankdir="LR"
 *              subgraph a {
 *                  rank=same
 *                  AA              [label="ALON register", URL="\ref STORAGE_CONFIG_ALON_REGISTER"]
 *              }
 *              subgraph b {
 *                  rank=same
 *                  MMMMMMM         [label="Moving marker in EEPROM", URL="\ref Marker_t"]
 *                  HH              [label="Hint information on a\nfixed EEPROM location", URL="\ref HINT_ABSOLUTE_BYTE_OFFSET"]
 *                  NULL            [label="0"]
 *              }
 *              subgraph c {
 *                  rank=same
 *                  flash           [label="flashByteCursor", URL="\ref Storage_Instance_t.flashByteCursor"]
 *                  random          [shape=point]
 *              }
 *
 *              AA -> NULL
 *              HH -> random
 *              MMMMMMM -> flash
 *          }
 *      @enddot
 *
 *      Only the marker will then still be correct, which can be recovered only through a full backward search. When
 *      that is done, after leaving #Storage_DeInit, all three recovery structures will be correctly linked.
 *
 *      @dot
 *          digraph "Alon, Hint, Marker OK" {
 *              node [shape=box];
 *              splines="ortho"
 *              rankdir="LR"
 *              subgraph a {
 *                  rank=same
 *                  AA              [label="ALON register", URL="\ref STORAGE_CONFIG_ALON_REGISTER"]
 *              }
 *              subgraph b {
 *                  rank=same
 *                  MMMMMMM         [label="Moving marker in EEPROM", URL="\ref Marker_t"]
 *                  HH              [label="Hint information on a\nfixed EEPROM location", URL="\ref HINT_ABSOLUTE_BYTE_OFFSET"]
 *              }
 *              subgraph c {
 *                  rank=same
 *                  flash           [label="flashByteCursor", URL="\ref Storage_Instance_t.flashByteCursor"]
 *              }
 *
 *              AA -> MMMMMMM
 *              HH -> MMMMMMM
 *              MMMMMMM -> flash
 *          }
 *      @enddot
 *
 *  @par Using EEPROM @b and FLASH
 *      Both EEPROM and FLASH are used to store data - in the form of equisized samples. Since writing to EEPROM is
 *      cheaper, faster and less complicated than writing to FLASH, it is the preferred storage medium. Whenever the
 *      size of all the samples stored in EEPROM is large enough - see #STORAGE_BLOCK_SIZE - all that data is moved in
 *      one operation to FLASH. EEPROM is then completely empty, and new samples are then in EEPROM again.
 *
 *      The memory content changes are visually depicted below. If writing a new sample would increase the size to equal
 *      to or higher than #STORAGE_BLOCK_SIZE, first the EEPROM contents are moved, then the new sample is written.
 *
 *      @code
 *          EEPROM                                 EEPROM
 *         with samples a..e                      with sample f only
 *        +---------+                            +---------+
 *        | aaabbbc |                            | fffMMMM |
 *        | cc..... |                            | MMM     |
 *        | ....... |                            |         |
 *        | ....ddd |  ---------------------->   |         |
 *        | eeeMMMM |                            |         |
 *        | MMM  HH |                            |      HH |
 *        +---------+                            +---------+
 *
 *          FLASH                                  FLASH
 *         with one block moved from EEPROM       with two blocks moved from EEPROM
 *        +---------+                            +---------+
 *        | BBxxxxx |                            | BBxxxxx |
 *        | xxxxxxx |                            | xxxxxxx |
 *        | x       |  ---------------------->   | xBBxxxx |
 *        |         |                            | xxxxx   |
 *        |         |                            |         |
 *        |         |                            |         |
 *        +---------+                            +---------+
 *      @endcode
 */

/**
 * The absolute offset to the very first byte of the assigned EEPROM region.
 * @note Unless explicitly specified, all bit and byte offsets referring to the EEPROM region in the code are relative
 *  to this value.
 */
#define EEPROM_ABSOLUTE_FIRST_BYTE_OFFSET (STORAGE_EEPROM_FIRST_ROW * EEPROM_ROW_SIZE)

/** The absolute offset to the very last byte of the assigned EEPROM region. */
#define EEPROM_ABSOLUTE_LAST_BYTE_OFFSET (((STORAGE_EEPROM_LAST_ROW + 1) * EEPROM_ROW_SIZE) - 1)

/** Translates a flash byte cursor relative to assigned FLASH region to a byte address. */
#define FLASH_CURSOR_TO_BYTE_ADDRESS(flashByteCursor) \
    ((uint8_t *)(FLASH_START + (STORAGE_FLASH_FIRST_PAGE * FLASH_PAGE_SIZE) + (flashByteCursor)))

/** Translates a FLASH byte cursor relative to assigned FLASH region to the number of the page the cursor refers to. */
#define FLASH_CURSOR_TO_PAGE(flashByteCursor) (STORAGE_FLASH_FIRST_PAGE + ((flashByteCursor) / FLASH_PAGE_SIZE))

/** Translates a FLASH page number to the start address of that page. */
#define FLASH_PAGE_TO_ADDRESS(type, flashPage) ((type)(FLASH_START + ((flashPage) * FLASH_PAGE_SIZE)))

/** The very first byte address of the assigned FLASH region. */
#define FLASH_FIRST_BYTE_ADDRESS FLASH_PAGE_TO_ADDRESS(uint8_t *, STORAGE_FLASH_FIRST_PAGE)

/** The very last byte address of the assigned FLASH region. */
#define FLASH_LAST_BYTE_ADDRESS (FLASH_PAGE_TO_ADDRESS(uint8_t *, STORAGE_FLASH_LAST_PAGE + 1) - 1)

/** The very first 32-bit word address of the assigned FLASH region. */
#define FLASH_FIRST_WORD_ADDRESS FLASH_PAGE_TO_ADDRESS(uint32_t *, STORAGE_FLASH_FIRST_PAGE)

/** The very last WORD address of the assigned FLASH region. */
#define FLASH_LAST_WORD_ADDRESS (FLASH_PAGE_TO_ADDRESS(uint32_t *, STORAGE_FLASH_LAST_PAGE + 1) - 1)

/**
 * The size in bytes of the meta data stored just in front of the (compressed) data block in FLASH. The LSBit of the
 * first byte after these header size contains the start of the (compressed) data block.
 * This header is used to give the decompress callback the correct arguments, and to deduce where to find the next
 * block.
 * @note Either this size is a value greater than 0 but less than #STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS, which
 *  indicates the contents have been compressed; either this size is equal to #STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS
 *  which indicates no compression/decompression algorithm was set or compression yielded bad results on this block and
 *  therefore the block was stored uncompressed.
 * @see FLASH_BLOCK_SIZE
 */
#define FLASH_DATA_HEADER_SIZE STORAGE_BLOCK_HEADER_SIZE

/**
 *  Given the header value of a (compressed) data block in FLASH, calculates the size in bytes of the complete block,
 *  @b including #FLASH_DATA_HEADER_SIZE.
 *  The start of a new block @b must @c always be stored on a 32-bit word boundary: this allows to continue writing the
 *  same page without erasing. As a consequence, up to 31 padding bits after the end of previous (compressed) data
 *  block are lost.
 *  @param bitCount The size in bits of the (compressed) data block, @b excluding #FLASH_DATA_HEADER_SIZE.
 *  @note
 *  - From the FLASH specification:
 *      <em>It is possible to write only a sub-set of the page (Y words), but the maximum number of
 *      times that a page can be written to, before an erase must be performed, is 16.
 *      WARNING: Writing a page more than 16 times without erasing may result in corrupting
 *      the page’s contents, due to write disturb (an intrinsic mechanism to the programming
 *      mechanism of the C14EFLASH)</em>
 *  - From the NVMC datasheet:
 *      <em>Due to the presence of ECC, it is not allowed to modify additional bits inside a memory
 *      word where some bits have already been programmed: the resulting ECC code in
 *      memory would then be the AND of the codes for the previous and new value written, and
 *      will most likely be inconsistent with the resulting data, potentially resulting in unwanted or
 *      missing bit corrections, or spurious error conditions. [C-NODPG]</em>
 *  .
 */
#define FLASH_BLOCK_SIZE(bitCount) (4 * STORAGE_IDIVUP((bitCount) + FLASH_DATA_HEADER_SIZE * 8, 32))

/**
 * The first of two special values that are used in #Marker_t to be able to reconstruct the EEPROM and FLASH bit cursor
 * in case the battery has died - and thus the register value has been reset to zero.
 * @note These values are not chosen randomly: the code using them - #FindMarker - is depending on their specific bit
 * values.
 */
#define MARKER_HEADER ((int)0x0000FFFF)
#define MARKER_FOOTER ((int)0x7FFFFFFF) /**< The second special value. @see MARKER_HEADER */

/** The mask to use to zero out all possible 1 bits in #Marker_t.flashByteCursor */
#define MARKER_CURSOR_ZERO_MASK 0xFFFF8003

/* ------------------------------------------------------------------------- */

#if !STORAGE_FLASH_FIRST_PAGE
extern const int _etext; /**< Generated by the linker, used to calculate #STORAGE_FLASH_FIRST_PAGE */
extern const int _data; /**< Generated by the linker, used to calculate #STORAGE_FLASH_FIRST_PAGE */
extern const int _edata; /**< Generated by the linker, used to calculate #STORAGE_FLASH_FIRST_PAGE */
__attribute__ ((section(".noinit")))
static int sStorageFlashFirstPage; /* Initialized in #Storage_Init, RO afterwards. */
    #undef STORAGE_FLASH_FIRST_PAGE
    #define STORAGE_FLASH_FIRST_PAGE sStorageFlashFirstPage
#endif

/* ------------------------------------------------------------------------- */

/** @see Storage_Instance_t.readLocation */
typedef enum Location {
    Location_Flash, /**< Memory in the assigned FLASH region is targeted. */
    Location_Eeprom /**< Memory in the assigned EEPROM region is targeted. */
} Location_t;

/**
 * Lists all values that can be assigned to #Hint_t.samplesAvailable.
 */
typedef enum SamplesAvailable {
    SamplesAvailable_None = 0, /**< No data is available in EEPROM nor in FLASH. */
    SamplesAvailable_EepromOnly = 1, /**< Data is available in EEPROM, nothing is available in FLASH. */
    SamplesAvailable_EepromAndFlash = 3 /**< Data is available both in EEPROM and in FLASH. */
} SamplesAvailable_t;

/**
 * An instance of this structure is stored in the designated register of the ALON domain, and points to where to find
 * #Marker_t. This structure is stored in ALON and allows for the fastest initialization.
 * If all these values are 0, either no logging is ongoing, or the battery has gone empty. To discover which case
 * holds true, first #Hint_t is checked; if that fails too, the EEPROM is checked backwards for the location of a stored
 * instance of type #Marker_t (which is time consuming). Once found, the correct values for this structure can be
 * reconstructed.
 */
typedef struct RecoverInfo_s {
    int eepromBitCursor; /**< @see Storage_Instance_t.eepromBitCursor */
} RecoverInfo_t;

/** Byte size of #Hint_t. Checked at compile time using #checkSizeOfHint. */
#define SIZE_OF_HINT 4

/** The absolute offset to #Hint_t at the end of the assigned EEPROM region. */
#define HINT_ABSOLUTE_BYTE_OFFSET (EEPROM_ABSOLUTE_LAST_BYTE_OFFSET + 1 - SIZE_OF_HINT)

/**
 * An instance of this structure is stored at the fixed location #HINT_ABSOLUTE_BYTE_OFFSET, and points to where to
 * find #Marker_t. If all the information contained is correct (which may not be the case) it still allows for a fast
 * initialization.
 * - The value of the field @c samplesAvailable can always be trusted. If @c samplesAvailable is 0, no samples have
 *  been stored.
 * - Else, @c eepromBitCursor @b may contain correct information. If not, the EEPROM is checked backwards for the
 *  location of a stored instance of type #Marker_t (which is time consuming) and this hint structure is adapted (so a
 *  next initialization under the same circumstances is done quickly).
 * .
 */
typedef struct Hint_s {
    /**
     * @note This bit value is only set correct in #FindMarker.
     * @see Storage_Instance_t.eepromBitCursor
     */
    uint16_t eepromBitCursor;

    /**
     * Internally, samples are always first stored in EEPROM. Whenever a new call to #Storage_Write is made which
     * would otherwise cause the EEPROM storage to overflow, all samples are moved to Flash first, then the write of
     * the new samples is done - in EEPROM.
     * The EEPROM contains two special fixed bytes at the end of the assigned storage in EEPROM. The only allowed values
     * are:
     * - #NO_SAMPLES_AVAILABLE
     * - #SAMPLES_AVAILABLE_IN_EEPROM_ONLY
     * - #SAMPLES_AVAILABLE_IN_EEPROM_AND_FLASH
     * .
     * @note This value is always set correct in #Storage_DeInit.
     */
    uint16_t samplesAvailable;
} Hint_t;

/**
 * An instance of this structure is stored in EEPROM, right after the end of the last sample in EEPROM.
 * It is used to re-create #sInstance upon initialization.
 * Since a number of false candidates may be proposed, it contains a @c header and a @c footer to have a near 100%
 * chance of detecting erroneous information.
 */
typedef struct Marker_s {
    const int header; /**< Must equal #MARKER_HEADER, or @c flashByteCursor is not valid. */

    /**
     * @see Storage_Instance_t.flashByteCursor
     * @note Usage
     *  - RO in #Storage_Init,
     *  - WO to set the value equal to #Storage_Instance_t.flashByteCursor in #Storage_DeInit, or to clear the
     *      marker from EEPROM in #Storage_Reset,
     *  - not to be read, not to be written to at all other times.
     *  .
     */
    int flashByteCursor;

    const int footer; /**< Must equal #MARKER_FOOTER, or @c flashByteCursor is not valid. */
} Marker_t;

/**
 * The total number of bits in EEPROM consumed by meta-data, to be able to keep track of what is stored in FLASH and
 * EEPROM, even after a power-off.
 * Apart from #Marker_t, there is an additional bit that marks whether the storage in FLASH and EEPROM is empty or at
 * least one sample is stored in EEPROM - @see #Hint_t
 */
#define SIZE_OF_MARKER 12

/**
 * Stores all meta data to perform the requested reads and writes in FLASH and EEPROM. Upon de-initialization, an
 * updated instance of #Marker_t is updated and stored in EEPROM, and an updated instance of #RecoverInfo_t is stored
 * in the ALON domain, using the information in this structure.
 */
typedef struct Storage_Instance_s {
    /**
     * The position of the first bit where new data can be stored in EEPROM, relative to #STORAGE_EEPROM_FIRST_ROW.
     * This doubles up as the start position of a stored instance of type #Marker_t in EEPROM.
     * There are at most #EEPROM_NR_OF_RW_ROWS rows of EEPROM that can be used for storage, thus an EEPROM bit cursor
     * always has values less than 2**15: 15 bits are required to store this information.
     */
    int eepromBitCursor;

    /**
     * The position of the first byte where new data can be stored in FLASH, relative to #STORAGE_FLASH_FIRST_PAGE.
     * @pre Must be 32-bit word-aligned: multiple FLASH writes in the same page must occur at word boundaries. See
     *  #FLASH_BLOCK_SIZE
     * @note There are at most (#FLASH_NR_OF_RW_SECTORS * #FLASH_PAGES_PER_SECTOR - program size) pages of FLASH that
     *  can be used for storage, thus a FLASH byte cursor always has values less than 2**15; and due to the boundary
     *  requirement, the 2 LSBits must be 0 as well: 13 bits are thus required to store this information.
     *  Masking with #MARKER_CURSOR_ZERO_MASK must thus always yield a @c 0 value.
     */
    int flashByteCursor;

    /* ------------------------------------------------------------------------- */

    /**
     * Indicates whether @c readCursor is either a FLASH byte cursor or an EEPROM bit cursor.
     */
    Location_t readLocation;

    /**
     * - If @c readLocation equals #Location_t.LOCATION_FLASH: the offset in bytes relative to
     *  #FLASH_FIRST_BYTE_ADDRESS where to read next in FLASH. This points to the two-byte header that precedes
     *  a (compressed) data block, stored in FLASH.
     * - If @c readLocation equals #Location_t.LOCATION_EEPROM: the offset in bits relative to
     *  #EEPROM_ABSOLUTE_FIRST_BYTE_OFFSET where to read next in EEPROM. This points to the position of the LSBit of
     *  the next sample to read, stored in EEPROM.
     * @note A negative value indicates nothing can be read. A call to #Storage_Seek is then required.
     */
    int readCursor;

    /**
     * A sequence number to help identify where the sample which must be read out next is located.
     * - In case of reading from FLASH, where (compressed) blocks of samples are stored, this is the absolute sequence
     *  number of the first sample in that block after decompressing.
     * - In case of reading from EEPROM, this is the absolute sequence number of the sample @c readCursor points to.
     * The very first sample written via #Storage_Write has sequence number 0.
     */
    int readSequence;

    /**
     * The sequence number that was requested in the last call to #Storage_Seek. This value equals @c readSequence
     * in case @c readLocation equals #Location_t.LOCATION_EEPROM; this value may be equal - but is unlikely to  -
     * in case @c readLocation equals #Location_t.LOCATION_FLASH.
     */
    int targetSequence;

    /* ------------------------------------------------------------------------- */

    /**
     * Determines what contents are available in #STORAGE_WORKAREA.
     * - The flash byte offset relative to #FLASH_FIRST_BYTE_ADDRESS where the header preceding the (compressed) data
     *  block can be found; these contents are decompressed and the samples, packed without padding bits, are available
     *  from the start of #STORAGE_WORKAREA.
     * - @c -1 if #STORAGE_WORKAREA does not contain samples after decompressing a data block stored in FLASH, or
     *  if #STORAGE_WORKAREA is used to compress data.
     * .
     */
    int cachedBlockOffset;
} Storage_Instance_t;

/**
 * The assigned EEPROM region cannot be used fully: there must be always room for #Marker_t and the last couple of
 * bytes are used to #Hint_t. The total overhead is summed up here.
 */
#define EEPROM_OVERHEAD_IN_BITS ((SIZE_OF_MARKER + SIZE_OF_HINT) * 8)

#if STORAGE_MAX_BLOCK_SIZE_IN_SAMPLES != (((STORAGE_EEPROM_ROW_COUNT * EEPROM_ROW_SIZE * 8) - EEPROM_OVERHEAD_IN_BITS) / STORAGE_BITSIZE)
    #error Internal memory storage model has changed - likely Marker_t or Hint_t - and no longer matches the define STORAGE_MAX_BLOCK_SIZE_IN_SAMPLES
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/** If this construct doesn't compile, #STORAGE_TYPE can not hold #STORAGE_BITSIZE bits. */
int checkSizeOfSampleType[(sizeof(STORAGE_TYPE) * 8 < STORAGE_BITSIZE) ? -1 : 1]; /* Dummy variable since we can't use sizeof during precompilation. */

/** If this construct doesn't compile, the define #SIZE_OF_MARKER is no longer equal to @c sizeof(#Hint_t) */
int checkSizeOfHint[(SIZE_OF_HINT == sizeof(Hint_t)) ? 1 : -1]; /* Dummy variable since we can't use sizeof during precompilation. */

/** If this construct doesn't compile, the define #SIZE_OF_MARKER is no longer equal to @c sizeof(#Marker_t) */
int checkSizeOfMarker[(SIZE_OF_MARKER == sizeof(Marker_t)) ? 1 : -1]; /* Dummy variable since we can't use sizeof during precompilation. */

#pragma GCC diagnostic pop

/* ------------------------------------------------------------------------- */

/**
 * The module's instance information, where recovered information is copied to, and where all updates are stored.
 * In #Storage_DeInit, this information is copied to more permanent storage.
 */
static Storage_Instance_t sInstance;

/**
 * To be used whenever the current location to the marker in EEPROM must be cleared.
 * @see MoveSamplesFromEepromToFlash
 * @see Storage_Reset
 */
static const uint8_t sZeroMarker[sizeof(Marker_t)] = {0};

/* ------------------------------------------------------------------------- */

/**
 * Set to @c true after writing a new sample, used in #Storage_DeInit to know when to write the marker.
 * Not set when resetting the module; as there is no need for a marker in that case - #Hint_t.samplesAvailable is then
 * also cleared.
 */
bool sBitCursorChanged = false;

#if STORAGE_WORKAREA_SELF_DEFINED == 1
__attribute__ ((section(".noinit"))) __attribute__((aligned (4)))
uint8_t sStorage_Workarea[STORAGE_WORKAREA_SIZE];
#endif
extern uint8_t STORAGE_WORKAREA[STORAGE_WORKAREA_SIZE];

extern int STORAGE_COMPRESS_CB(int eepromByteOffset, int bitCount, void * pOut);
extern int STORAGE_DECOMPRESS_CB(const uint8_t * pData, int bitCount, void * pOut);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
/**
 * This is a dummy implementation to provoke fallback behavior: a plain copy from EEPROM to FLASH.
 * It is only used when #STORAGE_COMPRESS_CB is not overridden in an application specific @c app_sel.h header file
 * @see pStorage_CompressCb_t
 * @see STORAGE_COMPRESS_CB
 * @param eepromByteOffset unused
 * @param bitCount unused
 * @param pOut unused
 * @return @c 0
 */
int Storage_DummyCompressCb(int eepromByteOffset, int bitCount, void * pOut)
{
    return 0;
}
/**
 * This is a dummy implementation to provoke a failure; it should normally never be called, since the dummy compression
 * implementation will provoke the fallback behavior: a plain copy from EEPROM to FLASH, and vice-versa while reading.
 * It is only used when #STORAGE_COMPRESS_CB is not overridden in an application specific @c app_sel.h header file
 * @see pStorage_DecompressCb_t
 * @see STORAGE_DECOMPRESS_CB
 * @param pData unused
 * @param bitCount unused
 * @param pOut unused
 * @return @c 0
 */
int Storage_DummyDecompressCb(const uint8_t * pData, int bitCount, void * pOut)
{
    return 0;
}
#pragma GCC diagnostic pop

/* ------------------------------------------------------------------------- */

static void ResetInstance(void);
static void ShiftAlignedData(uint8_t * pTo, const uint8_t * pFrom, const int bitAlignment, const int bitCount);
static void ShiftUnalignedData(uint8_t * pTo, const uint8_t * pFrom, const int bitAlignment, const int bitCount);
static void WriteToEeprom(const int bitCursor, const void * pData, const int bitCount);
static void ReadFromEeprom(const int bitCursor, void * pData, const int bitCount);
static int FindMarker(Marker_t * pMarker);
static int GetEepromCount(void);
static int GetFlashCount(void);
static void WriteToFlash(const int pageCursor, const void * pData, const int pageCount);
static bool MoveSamplesFromEepromToFlash(void);
static int ReadAndCacheSamplesFromFlash(int readCursor);

/* ------------------------------------------------------------------------- */

/**
 * Re-initializes #sInstance with default values.
 * @note No EEPROM reads or writes, no FLASH reads or writes are done.
 * @post sBitCursorChanged is set when samples have become inaccessible.
 */
static void ResetInstance(void)
{
    sBitCursorChanged = (sInstance.eepromBitCursor != 0) || (sInstance.flashByteCursor != 0);

    sInstance.eepromBitCursor = 0;
    sInstance.flashByteCursor = 0;
    sInstance.readCursor = -1;
    sInstance.readLocation = Location_Flash;
    sInstance.readSequence = -1;
    sInstance.targetSequence = -1;
    sInstance.cachedBlockOffset = -1;
}

/**
 * Copies a number of bits of byte aligned data to a buffer, non-byte aligned.
 * @param pTo The location to copy to. A number of LSBits of the first byte are not touched.
 * @param pFrom The location to copy from. The first bit to copy will become the LSBit of the first byte in this buffer.
 * @param bitAlignment The number of bits to disregard in @c to. Must be less than @c 8.
 * @param bitCount The number of bits to copy.
 * @pre @code STORAGE_IDIVUP(bitCount, 8) @endcode bytes must be available in @c from.
 * @post @code STORAGE_IDIVUP(bitCount + bitAlignment, 8) @endcode bytes will be written to @c to.
 * @note The first @c bitAlignment bits of the first byte written to are not touched.
 * @note For example, use this function to move bits in this way:
 *  - With @c - indicating not touched or don't care,
 *  - @c bitAlignment equal to @c 5, and
 *  - @c bitCount equal to @c 14:
 *  .
 *  @code 8765 4321   --fe dcba is copied as 321- ----   cba8 7654   0000 0fed @endcode
 */
static void ShiftAlignedData(uint8_t * pTo, const uint8_t * pFrom, const int bitAlignment, const int bitCount)
{
    const uint8_t mask = (uint8_t)(0xFF & ((1 << bitAlignment) - 1)); /* Covering the existing bits in the last byte. */

    ASSERT((bitAlignment >= 0) && (bitAlignment < 8));
    ASSERT(bitCount > 0);

    /* 8 bits at a time, data is moved from @c from to @c to.
     *
     * 8 bits from @c from are copied to two consecutive bytes in @c to: 321- ----   ---8 7654 (-: don't care)
     * - Step 1: write the lsbits of 87654321 such that the ongoing byte is filled
     *   ... xxxxxxxx 321xxxxx -------- ...
     * - Step 2: write the msbits of 87654321 in the lsbit positions of the next byte
     *   ... xxxxxxxx 321xxxxx ---87654 ...
     * (in the above example, @c bitAlignment equals 5)
     */
    int n = 0;
    do {
        /* Step 1: copy x lsbits of the current byte, with x = 8-bitAlignment so left shift bitAlignment places */
        *pTo = (uint8_t)((pFrom[n] << bitAlignment) | ((*pTo) & mask));

        if ((n * 8) + 8 - bitAlignment < bitCount) {
            /* Advance to the next location to copy bits to. */
            pTo++;

            /* Step 2: copy x msbits of the new byte, with x = bitAlignment so right shift 8-bitAlignment places */
            *pTo = (uint8_t)(pFrom[n] >> (8 - bitAlignment));
        }

        n++;
    } while (n * 8 < bitCount);

    /* Clear the bits that were copied in excess. */
    uint8_t finalMask = (uint8_t)~(0xFF << ((bitCount + bitAlignment) % 8));
    if (finalMask != 0) { /* finalMask equals zero when the last bit to retain is the MSBit of the last byte written. */
        *pTo = *pTo & finalMask;
    }
}

/**
 * Copies a number of bits of non-byte aligned data to a buffer, byte aligned.
 * @param pTo The location to copy to. The first bit to copy will become the LSBit of the first byte in this buffer.
 * @param pFrom The location to copy from. A number of LSBits are disregarded.
 * @param bitAlignment The number of bits to disregard in @c from. Must be less than @c 8.
 * @param bitCount The number of bits to copy.
 * @pre @code STORAGE_IDIVUP(bitCount + bitAlignment, 8) @endcode bytes must be available in @c from.
 * @post @code STORAGE_IDIVUP(bitCount, 8) @endcode bytes will be written to @c to.
 * @note The remainder bits in @c to after @bitCount bits is set to @c 0.
 * @note For example, use this function to move bits in this way:
 *  - With @c - indicating not touched or don't care,
 *  - @c bitAlignment equal to @c 5, and
 *  - @c bitCount equal to @c 14:
 *  .
 *  @code 321- ----   cba8 7654   ---- -fed is copied as 8765 4321   00fe dcba @endcode
 */
static void ShiftUnalignedData(uint8_t * pTo, const uint8_t * pFrom, const int bitAlignment, const int bitCount)
{
    const uint8_t mask = 0xFF & (uint8_t)((1 << (8 - bitAlignment)) - 1); /* Covering the lsbits to retain in the copied byte. */

    ASSERT((bitAlignment >= 0) && (bitAlignment < 8));
    ASSERT(bitCount > 0);

    /* 8 bits at a time, data is moved from @c from to @c to.
     *
     * 8 bits from two consecutive bytes in @c from: 321- ----   ---8 7654 (-: don't care) are copied to one byte in
     * @c to:
     * - Step 1: read the msbits of 321- ----
     *   byte = 0b00000321
     * - Step 2: read the lsbits of ---8 7654
     *   byte = 0b87654321
     * (in the above example, @c bitAlignment equals 5)
     */
    int n = 0;
    do {
        /* Step 1: copy x msbits of the current byte, with x = 8-bitAlignment so right shift bitAlignment places. */
        *pTo = (uint8_t)((pFrom[n]) >> bitAlignment);

        /* Advance to the next location to copy bits from. */
        n++;

        if (((n - 1) * 8) + 8 - bitAlignment < bitCount) {

            /* Step 2: copy x lsbits of the next byte, with x = bitAlignment so left shift 8-bitAlignment places. */
            *pTo = (uint8_t)((pFrom[n] << (8 - bitAlignment)) | ((*pTo) & mask));

            if (n * 8 < bitCount) {
                pTo++; /* Do not increment @c to unconditionally: @c finalmask still may have to be applied. */
            }
        }
    } while (n * 8 < bitCount);

    /* Clear the bits that were copied in excess. */
    uint8_t finalMask = (uint8_t)~(0xFF << (bitCount % 8));
    if (finalMask != 0) { /* finalMask equals zero when the last bit to retain is the MSBit of the last byte written. */
        *pTo = *pTo & finalMask;
    }
}

/* ------------------------------------------------------------------------- */

/**
 * @pre EEPROM is initialized
 * @pre Enough free space must be available in EEPROM starting from @c bitCursor
 * @param bitCursor Must be positive. The first bit where to start writing.
 * @param pData May not be @c NULL.
 * @param bitCount Must be strict positive.
 * @post #sInstance is not touched.
 */
static void WriteToEeprom(const int bitCursor, const void * pData, const int bitCount)
{
    const int bitAlignment = bitCursor % 8; /* The number of lsbits written in the last byte. */
    int byteOffset = EEPROM_ABSOLUTE_FIRST_BYTE_OFFSET + bitCursor / 8; /* The index to the last byte written. */

    ASSERT(bitCursor >= 0);
    ASSERT(pData != NULL);
    ASSERT(bitCount > 0);

    int byteCount = STORAGE_IDIVUP(bitCount + bitAlignment, 8);
    uint8_t bytes[byteCount];

    /* Read the first byte we will write to, to preserve a number the LSBits in that byte which were written
     * previously.
     */
    Chip_EEPROM_Read(NSS_EEPROM, byteOffset, &bytes, 1);
    ShiftAlignedData(bytes, (uint8_t *)pData, bitAlignment, bitCount);
    Chip_EEPROM_Write(NSS_EEPROM, byteOffset, bytes, byteCount);
}

/**
 * @pre EEPROM is initialized
 * @pre Enough bits to read are available in EEPROM starting from @c bitCursor
 * @param bitCursor Must be strict positive. The bit position in EEPROM to start reading: @c 0 means to start reading
 *  from the very first EEPROM bit assigned for bit storage, and so on.
 * @param pData May not be @c NULL. All bits are copied in the array pointed to.
 * @param bitCount Must be strict positive. When not a multiple of 8, the remainder MSBits of the last byte are set to
 *  @c 0.
 * @post #sInstance is not touched.
 */
static void ReadFromEeprom(const int bitCursor, void * pData, const int bitCount)
{
    const int bitAlignment = bitCursor % 8; /* The number of lsbits written in the last byte. */
    int byteOffset = EEPROM_ABSOLUTE_FIRST_BYTE_OFFSET + bitCursor / 8; /* The index to the last byte written. */

    ASSERT(bitCursor >= 0);
    ASSERT(pData != NULL);
    ASSERT(bitCount > 0);

    int byteCount = STORAGE_IDIVUP(bitCount + bitAlignment, 8);
    uint8_t bytes[byteCount];

    Chip_EEPROM_Read(NSS_EEPROM, byteOffset, bytes, byteCount);
    ShiftUnalignedData((uint8_t *)pData, bytes, bitAlignment, bitCount);
}

/* ------------------------------------------------------------------------- */

/**
 * Search backwards in the assigned EEPROM region, looking for a valid marker.
 * @param [out] pMarker : Where to copy the found marker data to. If @c 0 is returned, this may have been written to but
 *  must be ignored.
 * @return The position of the first bit of a stored instance of type #Marker_t in EEPROM, relative to
 *  #STORAGE_EEPROM_FIRST_ROW. @c 0 if the marker could not be found.
 */
static int FindMarker(Marker_t * pMarker)
{
    bool found = false;
    int bitCursor = 0;
    int byteOffset = EEPROM_ABSOLUTE_LAST_BYTE_OFFSET; /* Start at the end, search backwards. */
    uint16_t word;
    Hint_t hint;
    Chip_EEPROM_Read(NSS_EEPROM, HINT_ABSOLUTE_BYTE_OFFSET, &hint, sizeof(Hint_t));
    if (hint.samplesAvailable != SamplesAvailable_None) { /* If nothing is stored, no need to search for a marker. */
#if STORAGE_ALWAYS_TRY_FAST_RECOVERY
        /* First try the position #Hint_t hints at. Else start a slow search, checking the full EEPROM contents. */
        bitCursor = hint.eepromBitCursor;
#endif
        do {
            if ((bitCursor > 0) && (bitCursor < (STORAGE_EEPROM_SIZE * 8)) && ((bitCursor % STORAGE_BITSIZE) == 0)) {
                ReadFromEeprom(bitCursor, pMarker, sizeof(Marker_t) * 8); /* step f */
                if ((pMarker->header == MARKER_HEADER)
                        && (pMarker->footer == MARKER_FOOTER)
                        && (((unsigned int)pMarker->flashByteCursor & MARKER_CURSOR_ZERO_MASK) == 0)
                        && (FLASH_CURSOR_TO_BYTE_ADDRESS(pMarker->flashByteCursor) <= FLASH_LAST_BYTE_ADDRESS)) { /* step g */
                    found = true;
                    break; /* Step out of the do..while loop */
                }
            }

            /* The marker consists of a header, a value, and a footer.
             * By reading 16-bit words one at a time (step a) from high offset to low, we must find a value equal to
             * @c 0xFFFF that is part of the FOOTER: see ---------------- below.
             * If found (step b), we can read the word value @c h with relative byte offset @c -8 (step c) that is part
             * of the HEADER: see ++++++++++++++++ below.
             * A validation check is to verify whether @c h+1 is a power of 2 (step d): if so, from the number of bits
             * set in @c h (step e) we know the extra bitoffset to subtract to find the complete marker:
             * -7 for Example 1, -13 for Example 2, -1 for Example 3.
             * The full 96 bits can then be read out (step f) and validated (step g): #MARKER_HEADER and #MARKER_FOOTER
             * values must be found, and the size of the value must be acceptable.
             *
             * Header                           Value                            Footer
             * 0x00    0x00    0xFF    0xFF     0x00    0x00    0x??    0x??     0x7F    0xFF    0xFF    0xFF
             * 00000000000000001111111111111111 00000000000000000vvvvvvvvvvvvv00 01111111111111111111111111111111
             *          ++++++++++++++++                                                  ----------------        Example 1
             *    ++++++++++++++++                                                  ----------------              Example 2
             *                ++++++++++++++++                                                  ----------------  Example 3
             * 0         1         2         3          4         5         6          7         8         9
             * 01234567890123456789012345678901 23456789012345678901234567890123 45678901234567890123456789012345
             *
             * Magic values used:
             *  2: (step a) the portion in number of bytes of the FOOTER value to search for.
             *  0xFFFF: (step b) part of the FOOTER value.
             *  8, -8: (while, step c) The distance in bytes between the start of the footer and the start of the header.
             */
            bitCursor = 0;
            Chip_EEPROM_Read(NSS_EEPROM, byteOffset, &word, 2); /* step a */
            if (word == 0xFFFF) { /* step b */
                Chip_EEPROM_Read(NSS_EEPROM, byteOffset - 8, &word, 2); /* step c */
                if (((word + 1) & word) == 0) { /* step d */
                    int bits;
                    for (bits = 0; word; bits++) { /* step e */
                        word &= (uint16_t)(word - 1); /* Counting bits set, Brian Kernighan's way */
                    }
                    bitCursor = (byteOffset - EEPROM_ABSOLUTE_FIRST_BYTE_OFFSET - 8) * 8 - (16 - bits);
                }
            }
            byteOffset -= 2;

        } while (byteOffset >= EEPROM_ABSOLUTE_FIRST_BYTE_OFFSET + 8);
    }
    if (!found) {
        bitCursor = 0;
    }
#if STORAGE_ALWAYS_TRY_FAST_RECOVERY
    if (hint.eepromBitCursor != bitCursor) {
        hint.eepromBitCursor = (uint16_t)(bitCursor & 0x7FFF);
        /* hint.samplesAvailable = updated in Storage_DeInit */
        Chip_EEPROM_Write(NSS_EEPROM, HINT_ABSOLUTE_BYTE_OFFSET, &hint, sizeof(Hint_t));
    }
#endif
    return bitCursor;
}

static int GetEepromCount(void)
{
    ASSERT((sInstance.eepromBitCursor % STORAGE_BITSIZE) == 0); /* The integer division has no remainder. */
    return sInstance.eepromBitCursor / STORAGE_BITSIZE;
}

static int GetFlashCount(void)
{
    int sequenceCount = 0;
    /* Loop over all the (compressed) data blocks in FLASH - each is storing the same amount of samples. */
    int readCursor = 0;
    while (readCursor < sInstance.flashByteCursor) {
        uint8_t * header = FLASH_CURSOR_TO_BYTE_ADDRESS(readCursor);
        int bitCount = (int)(header[0] | (header[1] << 8));
        sequenceCount += STORAGE_BLOCK_SIZE_IN_SAMPLES;
        readCursor += FLASH_BLOCK_SIZE(bitCount);
    }
    return sequenceCount;
}

/* ------------------------------------------------------------------------- */

/**
 * Writes a block of (compressed) samples to FLASH.
 * @param pageCursor Must be strict positive. The absolute page number: a value of @c 0 indicates the first page of
 *  sector 0. Defines the first byte of the page where to start writing.
 * @param pData May not be @c NULL. Must be word (32 bits) aligned.
 * @param pageCount The number of pages to write. @c data must provide this multiple of #FLASH_PAGE_SIZE bytes of data.
 * @warning All interrupt are disabled at the beginning of this function and restored before leaving this function.
 * @warning This function will take 100+ milliseconds to complete.
 * @pre FLASH pages do not need to be erased.
 * @post #sInstance is not touched.
 */
static void WriteToFlash(const int pageCursor, const void * pData, const int pageCount)
{
    uint8_t * pDest;
    uint32_t size;
    IAP_STATUS_T status;
    uint32_t sectorStart = (uint32_t)(pageCursor / FLASH_PAGES_PER_SECTOR);
    uint32_t sectorEnd = (uint32_t)((pageCursor + pageCount - 1) / FLASH_PAGES_PER_SECTOR);

    ASSERT(pageCursor > 0);
    ASSERT(pData != NULL);
    ASSERT(pageCount > 0);
    ASSERT(sectorEnd < FLASH_NR_OF_RW_SECTORS);

    pDest = (uint8_t *)FLASH_START + (pageCursor * FLASH_PAGE_SIZE);
    size = (uint32_t)pageCount * FLASH_PAGE_SIZE;

    status = Chip_IAP_Flash_PrepareSector(sectorStart, sectorEnd);
    if (status == IAP_STATUS_CMD_SUCCESS) {
        __disable_irq();
        status = Chip_IAP_Flash_Program(pData, pDest, size, 0);
        __enable_irq();
    }
    ASSERT(status == IAP_STATUS_CMD_SUCCESS);
}

/**
 * Clear the assigned EEPROM region, by moving all data to assigned FLASH region. Before moving, the application is
 * notified, giving a chance to compress the data. The (compressed) data block is then appended in FLASH.
 * @return @c true when the compression was successful and the data has been moved to FLASH, @c false when the
 *  compression callback function returned @c false or when the FLASH storage is full: nothing has been changed in
 *  FLASH or EEPROM in that case.
 * @post #sInstance is fully updated when this function returns.
 */
static bool MoveSamplesFromEepromToFlash(void)
{
    bool success;
    uint8_t * pOut = STORAGE_WORKAREA;

    sInstance.cachedBlockOffset = -1;

    /* Prepare a number of pages to FLASH:
     * Pages: |----------------|---...-------------|
     * Data:   oooooohhcccccccccccc...ccfffffffffff
     * with:
     * - o: the last portion of the previously written (compressed) data block.
     * - h: the two-byte header indicating the size in bits of the (compressed) data block that follows.
     * - c: the (compressed) data block to write.
     * - f: the yet-unused trailing bytes of the last page where the new (compressed) data block is written to.
     *  By adding 1-bits, we can later write without the need for a costly FLASH page erase cycle.
     */

    /* The first flash page is the first page to flash in this call. It is likely that during the last call to this
     * function the last page flashed was not completely filled; that last page in the previous call is now completely
     * filled and becomes the first page to flash in this call.
     */
    int firstFlashPage = FLASH_CURSOR_TO_PAGE(sInstance.flashByteCursor);
    int flashByteOffsetInPage = sInstance.flashByteCursor % FLASH_PAGE_SIZE;

    /* o: Ensure the part of the last page that was already written to in a previous move from EEPROM to FLASH remains untouched. */
    memset(pOut, 0xFF, (size_t)flashByteOffsetInPage);
    pOut += flashByteOffsetInPage;

    /* c: The compress algorithm is to store the new (compressed) data block output just after the just copied data.
     * Skip the meta data header for now: that is filled in when the compression completed successfully.
     */
    int bitCount = STORAGE_COMPRESS_CB(EEPROM_ABSOLUTE_FIRST_BYTE_OFFSET, STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS,
                                       pOut + FLASH_DATA_HEADER_SIZE);
    if ((bitCount <= 0) || (bitCount >= STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS)) {
        /* Compression failed, or resulted in a larger block size. In this case we still have a fallback:
         * copy the data from EEPROM to FLASH unaltered.
         */
        Chip_EEPROM_Read(NSS_EEPROM, EEPROM_ABSOLUTE_FIRST_BYTE_OFFSET, pOut + FLASH_DATA_HEADER_SIZE,
                         STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BYTES);
        bitCount = STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS;
    }
    int compressedDataSizeInBytes = STORAGE_IDIVUP(bitCount, 8);

    /* h: */
    pOut[0] = (uint8_t)(bitCount & 0xFF);
    pOut[1] = (uint8_t)((bitCount >> 8) & 0xFF);
    pOut += FLASH_DATA_HEADER_SIZE;

    /* c: */
    pOut += compressedDataSizeInBytes;

    /* f: */
    while (((int)(pOut - STORAGE_WORKAREA) % FLASH_PAGE_SIZE) != 0) {
        *pOut = 0xFF;
        pOut++;
    }

    int newFlashByteCursor = sInstance.flashByteCursor + FLASH_BLOCK_SIZE(bitCount);
    ASSERT((newFlashByteCursor & 0x3) == 0); /* Must be 32-bit word-aligned. */
    if (FLASH_CURSOR_TO_BYTE_ADDRESS(newFlashByteCursor) > FLASH_LAST_BYTE_ADDRESS) {
        /* There is not enough space left in the assigned FLASH region to store the (compressed) data block. */
        success = false;
    }
    else {
        /* Only now we can write the full oooooohhcccccccccccc...ccfffffffffff sequence to FLASH. */
        int pageCountToFlash = (pOut - STORAGE_WORKAREA + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
        ASSERT(pageCountToFlash > 0); /* Must be at least 1. */
        WriteToFlash(firstFlashPage, STORAGE_WORKAREA, pageCountToFlash);

        success = true;
        /* All that is left now is to do some housekeeping: in EEPROM and in sInstance. */

        /* Clear the marker in EEPROM - after a power-off we don't want to find this information any more.
         * sBitCursorChanged is set in Storage_Write() - which is the sole caller of this function - and the new correct
         * marker on the new correct location will be written in Storage_DeInit().
         */
        WriteToEeprom(sInstance.eepromBitCursor, sZeroMarker, sizeof(Marker_t) * 8);

        sInstance.eepromBitCursor = 0;
        if (sInstance.readLocation == Location_Eeprom) {
            if (sInstance.readCursor < STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS) {
                sInstance.readLocation = Location_Flash;
                sInstance.readCursor = sInstance.flashByteCursor;
                sInstance.readSequence = GetFlashCount() - STORAGE_BLOCK_SIZE_IN_SAMPLES;
            }
            else {
                ASSERT(sInstance.readCursor == STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS);
                //sInstance.readLocation remains the same
                sInstance.readCursor = 0;
                //sInstance.readSequence remains the same
            }
            //sInstance.targetSequence remains the same
        }
        /* Only update flashByteCursor after updating readCursor & readSequence */
        sInstance.flashByteCursor = newFlashByteCursor;
    }

    return success;
}

/**
 * Reads a block of compressed data containing #STORAGE_BLOCK_SIZE_IN_SAMPLES samples, decompresses the data
 * and stores the result in the workspace given by the application.
 * @param readCursor The offset in bytes relative to FLASH_FIRST_BYTE_ADDRESS to the header preceding the
 *  (compressed) data block that must be read.
 * @return
 *  - When the previous decompression was still valid - and the callback was not called - or when decompression was
 *      successful as indicated by the returned value of the callback; thus when the samples are available in
 *      #STORAGE_WORKAREA: the size of the (compressed) data block including the header - i.e. the number of bytes
 *      to advance the read cursor to the header of the next (compressed) data block,
 *  - @c 0 when the FLASH contents were invalid, or when the decompression callback function returned @c false:
 *      nothing has been changed in that case.
 *  .
 * @post Only #sInstance.cachedBlockOffset is fully updated when this function returns.
 */
static int ReadAndCacheSamplesFromFlash(int readCursor)
{
    uint8_t * pHeader = FLASH_CURSOR_TO_BYTE_ADDRESS(readCursor);
    int bitCount = (int)(pHeader[0] | (pHeader[1] << 8));
    int blockSize;
    /* if header[0:1] == 0xFFFF, the flash was emptied.
     * This indicates a discrepancy between the instance information and the FLASH contents.
     * This may happen during development when re-flashing with the same image and erasing the non-used pages.
     */
    if (bitCount == 0x0000FFFF) {
        blockSize = 0;
    }
    else if (sInstance.cachedBlockOffset == readCursor) {
        /* The output from the previous call to STORAGE_DECOMPRESS_CB is still valid. */
        blockSize = FLASH_BLOCK_SIZE(bitCount);
    }
    else if (bitCount == STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS) {
        memcpy(STORAGE_WORKAREA, pHeader + FLASH_DATA_HEADER_SIZE, (size_t)STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BYTES);
        blockSize = FLASH_BLOCK_SIZE(STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS);
    } else {
        int decompressedBitCount = STORAGE_DECOMPRESS_CB(pHeader + FLASH_DATA_HEADER_SIZE, bitCount, STORAGE_WORKAREA);
        if (decompressedBitCount == STORAGE_UNCOMPRESSED_BLOCK_SIZE_IN_BITS) {
            sInstance.cachedBlockOffset = readCursor;
            blockSize = FLASH_BLOCK_SIZE(bitCount);
        }
        else {
            blockSize = 0;
        }
    }
    return blockSize;
}

/* ------------------------------------------------------------------------- */

void Storage_Init(void)
{
    Marker_t marker;
    RecoverInfo_t recoverInfo;
#if !STORAGE_FLASH_FIRST_PAGE
    sStorageFlashFirstPage = ((int)&_etext + (int)&_edata - (int)&_data + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
#endif
    ResetInstance();

    Chip_PMU_GetRetainedData((uint32_t *)&recoverInfo, STORAGE_CONFIG_ALON_REGISTER, 1);
    if ((recoverInfo.eepromBitCursor > 0) /* Situation after Storage_Reset or power-off: 0 */
            && ((recoverInfo.eepromBitCursor % STORAGE_BITSIZE) == 0) /* Validity check */
            && (recoverInfo.eepromBitCursor <= (STORAGE_MAX_UNCOMPRESSED_BLOCK_SIZE_IN_BITS))) { /* Validity check */
        /* Situation after Storage_DeInit or deep power down.
         * Immediately read out the stored marker.
         */
        ReadFromEeprom(recoverInfo.eepromBitCursor, &marker, sizeof(marker) * 8);
    }
    else {
        /* Search the EEPROM for a stored marker. */
        recoverInfo.eepromBitCursor = FindMarker(&marker);
    }
    /* Validity checks - redundant when just leaving FindMarker */
    if ((marker.header == MARKER_HEADER)
            && (marker.footer == MARKER_FOOTER)
            && (((unsigned int) marker.flashByteCursor & MARKER_CURSOR_ZERO_MASK) == 0)
            && (FLASH_CURSOR_TO_BYTE_ADDRESS(marker.flashByteCursor) <= FLASH_LAST_BYTE_ADDRESS)) {
        sInstance.eepromBitCursor = recoverInfo.eepromBitCursor;
        sInstance.flashByteCursor = marker.flashByteCursor;
    }
}

void Storage_DeInit(void)
{
    /* Write the marker, but only when samples have been added or when the module has been reset - to avoid hitting
     * the max write cycles of the EEPROM.
     */
    if (sBitCursorChanged) {
        Marker_t marker = {.header = MARKER_HEADER, .footer = MARKER_FOOTER};
        marker.flashByteCursor = sInstance.flashByteCursor;
        WriteToEeprom(sInstance.eepromBitCursor, &marker, sizeof(marker) * 8);
    }

    /* Write the hint, but only when @c samplesAvailable is different from the current situation - to avoid
     * hitting the max write cycles of the EEPROM.
     */
    uint16_t samplesAvailable = sInstance.flashByteCursor ? SamplesAvailable_EepromAndFlash :
                                sInstance.eepromBitCursor ? SamplesAvailable_EepromOnly : SamplesAvailable_None;
    Hint_t hint;
    Chip_EEPROM_Read(NSS_EEPROM, HINT_ABSOLUTE_BYTE_OFFSET, &hint, sizeof(Hint_t));
    if (hint.samplesAvailable != samplesAvailable) {
        hint.eepromBitCursor = (uint16_t)(sInstance.eepromBitCursor & 0x7FFF);
        hint.samplesAvailable = samplesAvailable;
        Chip_EEPROM_Write(NSS_EEPROM, HINT_ABSOLUTE_BYTE_OFFSET, &hint, sizeof(Hint_t));
    }

    Chip_EEPROM_Flush(NSS_EEPROM, true);

    RecoverInfo_t recoverInfo;
    recoverInfo.eepromBitCursor = sInstance.eepromBitCursor;
    Chip_PMU_SetRetainedData((uint32_t *)&recoverInfo, STORAGE_CONFIG_ALON_REGISTER, 1);
}

int Storage_GetCount(void)
{
    return GetEepromCount() + GetFlashCount();
}

void Storage_Reset(bool checkAll)
{
    /* The contents in EEPROM do not need to be erased, as they can be overwritten. Just ensure the EEPROM is marked
     * as 'empty'.
     * Clear the marker in EEPROM - after a power-off we don't want to find this information any more.
     */
    WriteToEeprom(sInstance.eepromBitCursor, sZeroMarker, sizeof(Marker_t) * 8);

    /* The contents in FLASH must be erased, as they can only be overwritten after erasing.
     * It is costly to erase the FLASH: it is taxing the battery and taking a long time. The FLASH is therefore
     * not erased unconditionally.
     * Alternative ways to avoid unnecessary FLASH erases:
     * - Read Hint_t at HINT_ABSOLUTE_BYTE_OFFSET and check if samplesAvailable == SamplesAvailable_EepromAndFlash
     * - check if sInstance.flashByteCursor is greater than 0
     * These are not used, as they can't catch memory corruption, or a direct EEPROM erase without a corresponding
     * direct FLASH erase. Also when the firmware image is updated - assuming this can be done it is possible the FLASH
     * contents and location is changed, containing unknown contents.
     * The only fail-safe way is to check the FLASH memory itself. The caller chooses the thoroughness via checkAll.
     * Check words from first to last, as the first words are more likely to be in use already. This is not done using
     * Chip_IAP_Flash_SectorBlankCheck, as this operates on full sectors only.
     */
    uint32_t * cursor = FLASH_FIRST_WORD_ADDRESS;
    uint32_t * last = FLASH_FIRST_WORD_ADDRESS;
    if (checkAll) {
        last = FLASH_LAST_WORD_ADDRESS;
        /* At 500 kHz, this adds +- 10 ms execution time per kb of assigned FLASH storage (worst case is
         * checking all words to find out an erase is not necessary.
         */
    }
    while ((cursor <= last) && (*cursor == 0xFFFFFFFF)) {
        cursor++;
    }
    if (cursor <= last) { /* If not above the last word to check, erase the FLASH memory. */
        IAP_STATUS_T status;
        const uint32_t sectorStart = (uint32_t)STORAGE_FLASH_FIRST_PAGE / FLASH_PAGES_PER_SECTOR;
        const uint32_t sectorEnd = STORAGE_FLASH_LAST_PAGE / FLASH_PAGES_PER_SECTOR;
        status = Chip_IAP_Flash_PrepareSector(sectorStart, sectorEnd);
        if (status == IAP_STATUS_CMD_SUCCESS) {
            /* Erasing pages/sectors is slow - try to avoid it as much as possible.
             * - First erase pages up to (not including) the first page @c b of a sector @c s.
             * - Next erase as many sectors as possible: from @c s till @c t.
             * - Finally erase pages from the first page of @c sectorEnd till #STORAGE_FLASH_LAST_PAGE.
             * Since each call involves different sectors, one prepare call (done above) is sufficient - only involved
             * sectors are locked again in each IAP call below.
             */
            uint32_t s = (uint32_t)(STORAGE_FLASH_FIRST_PAGE + FLASH_PAGES_PER_SECTOR - 1) / FLASH_PAGES_PER_SECTOR;
            uint32_t b = s * FLASH_PAGES_PER_SECTOR;
            uint32_t t = sectorEnd;
            if ((STORAGE_FLASH_LAST_PAGE % FLASH_PAGES_PER_SECTOR) < FLASH_PAGES_PER_SECTOR - 1) {
                t--; /* The last sector involved is not fully assigned for sample storage. */
            }
            if ((uint32_t)STORAGE_FLASH_FIRST_PAGE < b) {
                __disable_irq();
                status = Chip_IAP_Flash_ErasePage((uint32_t)STORAGE_FLASH_FIRST_PAGE, b - 1, 0);
                __enable_irq();
                ASSERT(status == IAP_STATUS_CMD_SUCCESS);
            }
            if (s <= t) {
                __disable_irq();
                status = Chip_IAP_Flash_EraseSector(s, t, 0);
                __enable_irq();
                ASSERT(status == IAP_STATUS_CMD_SUCCESS);
            }
            if (t < sectorEnd) {
                __disable_irq();
                status = Chip_IAP_Flash_ErasePage(sectorEnd * FLASH_PAGES_PER_SECTOR, STORAGE_FLASH_LAST_PAGE, 0);
                __enable_irq();
                ASSERT(status == IAP_STATUS_CMD_SUCCESS);
            }
        }
        ASSERT(status == IAP_STATUS_CMD_SUCCESS);
    }

    ResetInstance();
    /* A new marker will be written in EEPROM in a later call to Storage_DeInit() */
}

int Storage_Write(STORAGE_TYPE * samples, int n)
{
    int count = 0;
    ASSERT(samples != NULL);

    bool success = true;
    while (success && (count < n)) {
        if (GetEepromCount() == STORAGE_BLOCK_SIZE_IN_SAMPLES) {
            /* There are enough samples stored in EEPROM to warrant a compression and a move to FLASH.
             * Clear the EEPROM by moving them to FLASH. When that is done, the EEPROM is fully empty again.
             * We do this just before writing a new sample in EEPROM, to ensure at least one sample is present in
             * EEPROM whenever at least sample has been given to the storage module. There is no code relying on this:
             * it just feels more natural to only move when absolutely necessary.
             */
            (void)MoveSamplesFromEepromToFlash();
            /* Even if it fails, we can still continue and try writing in the rest of the EEPROM. */
        }
        if (GetEepromCount() < STORAGE_MAX_BLOCK_SIZE_IN_SAMPLES) {
            WriteToEeprom(sInstance.eepromBitCursor, samples + count, STORAGE_BITSIZE);
            sInstance.eepromBitCursor += STORAGE_BITSIZE;
            count++;
        }
        else {
            /* The EEPROM is fully filled with samples, and an earlier call to move data from EEPROM to FLASH failed
             * - if it succeeded, this branch would not be chosen.
             * Storage is full, both EEPROM and FLASH, and no data can be written any more.
             */
            success = false;
        }
    }
    if (count > 0) {
        Chip_EEPROM_Flush(NSS_EEPROM, false);
    }

    sBitCursorChanged |= (count > 0);
    return count;
}

bool Storage_Seek(int n)
{
    int currentSequence;
    int currentCursor;
    int nextSequence;
    int nextCursor;

    ASSERT(n >= 0);

    /* Step through the (compressed) blocks in FLASH, counting the number of samples stored in there.
     * If the count surpasses sequenceNumber we have found a block where the requested sequenceNumber is stored in.
     * - The cursor variables below indicate a byte offset in FLASH.
     * - The sequence variables below indicate a sequence count.
     */
    currentSequence = -1;
    currentCursor = -1;
    nextSequence = 0;
    nextCursor = 0;
    while (nextCursor < sInstance.flashByteCursor) {
        currentSequence = nextSequence;
        currentCursor = nextCursor;
        uint8_t * h = FLASH_CURSOR_TO_BYTE_ADDRESS(nextCursor);
        int bitCount = (int)(h[0] | (h[1] << 8));
        nextSequence += STORAGE_BLOCK_SIZE_IN_SAMPLES;
        nextCursor += FLASH_BLOCK_SIZE(bitCount);
        ASSERT((nextCursor & 0x3) == 0); /* Must be 32-bit word-aligned. */
        if ((currentSequence <= n) && (n < nextSequence)) {
            break;
        }
    }
    if ((currentSequence <= n) && (n < nextSequence)) {
        sInstance.readLocation = Location_Flash;
        sInstance.readSequence = currentSequence;
        sInstance.readCursor = currentCursor;
    }
    else {
        /* Jump to the sequence number sought for in EEPROM.
         * If the jump location surpasses sInstance.eepromBitCursor the sequence number sought for is too high.
         * - The cursor variable below indicates a bit offset in EEPROM.
         */
        nextCursor = (n - nextSequence) * STORAGE_BITSIZE;
        if (nextCursor < sInstance.eepromBitCursor) {
            sInstance.readLocation = Location_Eeprom;
            sInstance.readSequence = n;
            sInstance.readCursor = nextCursor;
        }
        else {
            /* Can not comply: : there are not yet this many samples stored in FLASH and EEPROM combined. */
            // sInstance.readLocation = don't care
            sInstance.readSequence = -1;
            sInstance.readCursor = -1;
        }
    }
    sInstance.targetSequence = n;

    return sInstance.readSequence >= 0;
}

int Storage_Read(STORAGE_TYPE * samples, int n)
{
    int count = 0;
    ASSERT(samples != NULL);

    if (sInstance.readSequence < 0) {
        /* A prior successful call to Storage_Seek is required. */
    }
    else {
        if (sInstance.readLocation == Location_Flash) {
            int blockSize = ReadAndCacheSamplesFromFlash(sInstance.readCursor);
            while (blockSize && (count < n) && (sInstance.readCursor < sInstance.flashByteCursor)) {
                while ((count < n) && (sInstance.readSequence + STORAGE_BLOCK_SIZE_IN_SAMPLES > sInstance.targetSequence)) {
                    /* Determine the offset in bytes and the initial number of LSBits to ignore. */
                    int bitOffset = (sInstance.targetSequence - sInstance.readSequence) * STORAGE_BITSIZE;
                    int byteOffset = bitOffset / 8;
                    int bitAlignment = bitOffset % 8;

                    ShiftUnalignedData((uint8_t *)(samples + count), STORAGE_WORKAREA + byteOffset, bitAlignment,
                                       STORAGE_BITSIZE);
                    bitOffset += STORAGE_BITSIZE;
                    count++;
                    sInstance.targetSequence++;
                }
                if (sInstance.readSequence + STORAGE_BLOCK_SIZE_IN_SAMPLES <= sInstance.targetSequence) {
                    /* A next sample is available in EEPROM or in the next (compressed) block of data in FLASH. */
                    sInstance.readCursor += blockSize;
                    ASSERT((sInstance.readCursor & 0x3) == 0); /* Must be 32-bit word-aligned. */
                    sInstance.readSequence += STORAGE_BLOCK_SIZE_IN_SAMPLES;
                }
                blockSize = ReadAndCacheSamplesFromFlash(sInstance.readCursor);
            }

            if (sInstance.readCursor >= sInstance.flashByteCursor) {
                /* All is read from FLASH, ensure the next read will pick the samples from EEPROM. */
                sInstance.readLocation = Location_Eeprom;
                sInstance.readCursor = 0;
            }
        }

        if (sInstance.readLocation == Location_Eeprom) {
            while ((count < n) && (sInstance.readCursor < sInstance.eepromBitCursor)) {
                ReadFromEeprom(sInstance.readCursor, samples + count, STORAGE_BITSIZE);
                sInstance.readCursor += STORAGE_BITSIZE;
                count++;
                sInstance.readSequence++;
            };
            sInstance.targetSequence = sInstance.readSequence;
        }
    }

#if STORAGE_SIGNED
    /* STORAGE_TYPE is signed: propagate the bit at position STORAGE_BITSIZE to the left */
    int msbits = sizeof(STORAGE_TYPE) * 8 - STORAGE_BITSIZE;
    for (int i = 0; i < count; i++) {
        samples[i] = (STORAGE_TYPE)((STORAGE_TYPE)(samples[i] << msbits) >> msbits);
    }
#endif
    return count;
}
