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


#ifndef __TRACE_H_
#define __TRACE_H_

/** @defgroup MODS_NSS_TRACE trace: Trace module
 * @ingroup MODS_NSS
 * The Trace module allows sending formatted data and receiving plain data to/from the outside world.
 * The most common example of it are, respectively, the 'printf' and 'fread' standard functions commonly
 * used for debugging purposes.
 *
 * The module API is data pipe independent, which means that the pipe through which the data is
 * sent/received (I2C, semi-hosting, etc) is implemented by a separate module and configured
 * at compile time.
 *
 * It is expected that each application that requires this module includes it and configures the
 * diversity settings of the module according to its specific needs.
 * Depending on the diversity settings, this module might depend on other modules which, in turn,
 * are also expected to be included and possibly configured by the application.
 * The goal is to have a normalized API for sending formatted data and receiving plain data to/from the
 * outside world across all the applications, regardless of their specific requirements (data pipe, memory
 * usage, string formatting capabilities, etc.).
 *
 * @par Diversity
 *  This module supports diversity, like the data pipe, buffer size or string formatting options.
 *  Check @ref MODS_NSS_TRACE_DFT for all diversity parameters and, consequently,
 *  respective dependency on other modules.
 *
 * @par How to use the module
 *  - The first step is to initialize the module (via #Trace_Init). The initialization will make sure
 *      that the respective data pipe module is also initialized.
 *      None of the modules will initialize the chip level drivers required for their operation.
 *      This means that the user must ensure, at application level, that the chip level drivers required by this
 *      module or the modules it depends on are properly initialized (e.g. if i2c is configured as the data pipe,
 *      the Trace module will depend on the I2CIO module, hence the user must ensure that if that module requires chip
 *      level drivers to be initialized at application level, this is done prior to initialization of the Trace module).
 *      The Trace module itself does not directly use any chip level driver.
 *
 *  - The next step is to print formatted strings (via #Trace_Printf) and/or read plain data (via #Trace_Read) to/from
 *      the outside world.
 *
 *  - The final step is to de-initialize the module (via #Trace_DeInit) which will make sure that the
 *      modules it depends on are properly de-initialized. Again, if a chip level driver is no longer
 *      required by the application, the user must ensure that they are properly de-initialized at
 *      application level.
 *  .
 *
 * @par Example 1 - Print "Hello world"
 *  @snippet trace_mod_example_1.c trace_mod_example_1
 *
 * @par Example 2 - Echo received data
 *  @snippet trace_mod_example_2.c trace_mod_example_2
 * @{
 */

#include "trace/trace_dft.h"

#define TRACE_TIMEOUT_INDEFINITE -1 /*!< This 'timeout' value specifies that the operation will never timeout */
#define TRACE_TIMEOUT_NOWAIT    0  /*!< This 'timeout' value specifies that the operation will not block */

#define TRACE_CHAROUT_NONE  -1 /*!< This 'charout' value specifies that the operation will not break in any specific character */

/**
 * Defines the prototype for the custom datapipe 'init' function.
 * This function shall initialize the custom datapipe.
 * See #TRACE_DATAPIPE_CUSTOM for more information on this diversity setting, namely the TRACE_DATAPIPE_CUSTOM_INIT macro.
 */
typedef void (*pTrace_CustomInit_t)(void);

/**
 * Defines the prototype for the custom datapipe 'read' function.
 * This function shall read one character from the custom datapipe.
 * See #TRACE_DATAPIPE_CUSTOM for more information on this diversity setting, namely the TRACE_DATAPIPE_CUSTOM_READ macro.
 * @return The character retrieved from the custom datapipe. Returns -1 if datapipe is empty.
 */
typedef int (*pTrace_CustomRead_t)(void);

/**
 * Defines the prototype for the custom datapipe 'write' function.
 * This function shall write the required number of characters to the custom datapipe.
 * See #TRACE_DATAPIPE_CUSTOM for more information on this diversity setting, namely the TRACE_DATAPIPE_CUSTOM_WRITE macro.
 * @param pData : The pointer to the data to write to the custom datapipe
 * @param length : The number of bytes to write
 */
typedef void (*pTrace_CustomWrite_t)(const char * pData, int length);

/**
 * Defines the prototype for the custom datapipe 'deInit' function.
 * This function shall de-initialize the custom datapipe.
 * See #TRACE_DATAPIPE_CUSTOM for more information on this diversity setting, namely the TRACE_DATAPIPE_CUSTOM_DEINIT macro.
 */
typedef void (*pTrace_CustomDeInit_t)(void);

/**
 * Initializes the Trace module, including the respective underlying data pipe module
 * @pre Depending on the configured data pipe, chip level drivers might be used.
 *  The user must ensure that they are properly initialized at application level, beforehand
 */
void Trace_Init(void);

/**
 * Writes the desired output to the outside world according to the provided formatted string
 * @param format : pointer to the formatted string to output
 * @pre The format string must be NULL ('\0') terminated
 * @note The output will be written to the configured underlying data pipe
 * @note Depending on the diversity settings, the format string will be processed either by the standard C library 'sprintf'
 *  function or by a dedicated module ('format') with its own diversity settings. Check the respective documentation
 *  (standard 'sprintf' or 'format' module) for details on the supported format flags
 */
void Trace_Printf(const char *format, ...);

/**
 * Reads characters from the outside world into the provided buffer until one of the 'out' conditions is met.
 * Function will return when one of the 'out' conditions is met.
 * @param [out] buf : The buffer where the read characters will be placed
 * @param lenout : The maximum number of bytes to read from the outside world
 * @param timeout : The maximum time (in RTC ticks) to wait for characters. @n
 *  Use #TRACE_TIMEOUT_NOWAIT for a non-blocking read or #TRACE_TIMEOUT_INDEFINITE to eliminate the 'timeout' condition.
 *  Time timeout value is only relevant when waiting for data to be received. In other words, as long as data is present
 *  in the read buffer the function will not timeout.
 * @param charout : The character that ends the read procedure (this character is also copied to the buffer). @n
 *  Use #TRACE_CHAROUT_NONE to eliminate the 'charout' condition
 * @return The actual number of characters read from the outside world
 * @pre Ensure that 'buf' has at least 'lenout' bytes of size
 * @note The characters will be read from the configured underlying data pipe
 * @note 'timeout' has a precision of +1 RTC tick (which is, by default, 1 second). Error is ensured to be
 *  within [timeout, timeout+1 RTC tick]. Refer to @ref RTC_NSS for more information on "RTC tick"
 */
int Trace_Read(char *buf, int lenout, int timeout, int charout);

/**
 * De-initializes the Trace module, including the respective underlying data pipe module
 * @post Depending on the configured data pipe, chip level drivers might be used.
 *  The user must ensure that they are properly de-initialized at application level in case they are not
 *  required by the application anymore
 */
void Trace_DeInit(void);

/**
 * @}
 */

#endif /* __TRACE_H_ */

