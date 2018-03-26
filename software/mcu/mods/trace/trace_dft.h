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


#ifndef __TRACE_DFT_H_
#define __TRACE_DFT_H_

/** @defgroup MODS_NSS_TRACE_DFT Diversity Settings
 *  @ingroup MODS_NSS_TRACE
 * These 'defines' capture the diversity settings of the module. The displayed values refer to the default settings.
 * To override the default settings, create the corresponding 'cfg' file and change the values there.
 * @{
 */

#if !defined(TRACE_PRINTF_BUFSIZE)
    /**
     * The #Trace_Printf function needs a buffer to format its output.
     * The buffer is statically claimed, and has a size determined by the following macro.
     * A value of 0 is allowed, but in this case the #Trace_Printf should never be called with arguments other then 'format'
     */
    #define TRACE_PRINTF_BUFSIZE 128
#else
    #if (TRACE_PRINTF_BUFSIZE != 0) && (TRACE_PRINTF_BUFSIZE < 3)
        #error The Printf buffer needs to have at least 3 bytes
    #endif
#endif



/* The Trace module sends/receives its data through a physical data pipe.
 * The macros below must be defined and must have a value of 0 (not supported) or 1 (pipe supported).
 * Precisely one macro can have the value 1.
 */

#if (!TRACE_DATAPIPE_I2C0) && (!TRACE_DATAPIPE_SEMIHOSTING) && (!TRACE_DATAPIPE_CUSTOM)
    /**
     * The Trace module will send/receive data through the I2C0 data pipe if this is set to 1
     * @note I2C data pipe depends on I2CIO module.
     */
    #define TRACE_DATAPIPE_I2C0 1

    /**
     * The Trace module will send/receive data through the 'semihosting' data pipe if this is set to 1
     * @note Semihosting data pipe is provided by the IDE toolchain.
     */
    #define TRACE_DATAPIPE_SEMIHOSTING 0

    /**
     * The Trace module will use a custom implementation of the underlying data pipe to send/receive data if this is set to 1.
     * @note The custom implementation must be provided at application level.
     * @note The Trace module will hand over the "init", "read", "write" and "deInit" actions, if a valid function pointer with
     *  the expected signature is provided
     * @par TRACE_DATAPIPE_CUSTOM_INIT Macro
     *  If using #TRACE_DATAPIPE_CUSTOM, define with this macro name, the function call to your custom datapipe "init"
     *  implementation. This function will be called during #Trace_Init.
     *  The Trace module expects that the custom init function configures the custom datapipe so that it is ready
     *  for read/write actions.
     *  Leave undefined if your datapipe has no "init" implementation.
     * @par TRACE_DATAPIPE_CUSTOM_READ Macro
     *  If using #TRACE_DATAPIPE_CUSTOM, define with this macro name, the function call to your custom datapipe "read"
     *  implementation. This function will be called during #Trace_Read to retrieve the actual data.
     *  The Trace module expects that the custom read function retrieves and consumes one character from the custom datapipe.
     *  In case the datapipe is empty, -1 must be returned.
     *  Leave undefined if your datapipe has no 'read' implementation.
     * @par TRACE_DATAPIPE_CUSTOM_WRITE Macro
     *  If using #TRACE_DATAPIPE_CUSTOM, define with this macro name, the function call to your custom datapipe "write"
     *  implementation. This function will be called during #Trace_Printf with the respective data and size.
     *  The Trace module expects that the custom "write" function writes the required number of bytes to the custom datapipe.
     *  Leave undefined if your datapipe has no 'write' implementation.
     * @par TRACE_DATAPIPE_CUSTOM_DEINIT Macro
     *  If using #TRACE_DATAPIPE_CUSTOM, define with this macro name, the function call to your custom datapipe "deInit"
     *   implementation. This function will be called during #Trace_DeInit.
     *  The Trace module expects that the custom deInit function performs the de-initialization actions applicable to
     *  the custom datapipe.
     *  Leave undefined if your datapipe has no "deInit" implementation.
     */
    #define TRACE_DATAPIPE_CUSTOM 0

#else
    #if (TRACE_DATAPIPE_I2C0 + TRACE_DATAPIPE_SEMIHOSTING + TRACE_DATAPIPE_CUSTOM) > 1
        #error Precisely one TRACE_DATAPIPE_* macro can have the value 1
    #endif
#endif

#if !defined(TRACE_SPRINTF_STDLIB)
    /**
     * The #Trace_Printf function needs to format its output.
     * It can use 'sprintf' from the standard library (accompanying the c compiler) or the one from the 'format' module.
     * The former is configured via the IDE, the latter through format_cfg.h.
     * The macro must be defined, and must be 0 (for 'format' module) or 1 (for standard lib).
     */
    #define TRACE_SPRINTF_STDLIB 1
#else
    #if (TRACE_SPRINTF_STDLIB != 0) && (TRACE_SPRINTF_STDLIB != 1)
        #error It is mandatory to define TRACE_SPRINTF_STDLIB as 0 or 1
    #endif
#endif

/**
 * @}
 */

#endif
