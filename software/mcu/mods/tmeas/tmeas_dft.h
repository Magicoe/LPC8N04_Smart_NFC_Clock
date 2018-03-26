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


#ifndef __TMEAS_DFT_H_
#define __TMEAS_DFT_H_

/** @defgroup MODS_NSS_TMEAS_DFT Diversity Settings
 *  @ingroup MODS_NSS_TMEAS
 * These 'defines' capture the diversity settings of the module. The displayed values refer to the default settings.
 * To override the default settings, place the defines with their desired values in the application app_sel.h header
 * file: the compiler will pick up your defines before parsing this file.
 * @{
 */

/**
 * Set this define to 1 to enable the format @ref TMEAS_FORMAT_NATIVE
 * @note This format is enabled by default.
 */
#if (!defined(TMEAS_CALIBRATED))
    #define TMEAS_CALIBRATED 1
#endif

/**
 * Set this define to 1 to enable the format @ref TMEAS_FORMAT_KELVIN
 */
#if (!defined(TMEAS_KELVIN))
    #define TMEAS_KELVIN 0
#endif

/**
 * Set this define to 0 to disable the format @ref TMEAS_FORMAT_CELSIUS
 * @note This format is enabled by default.
 */
#if (!defined(TMEAS_CELSIUS))
    #define TMEAS_CELSIUS 0
#endif

/**
 * Set this define to 1 to enable the format @ref TMEAS_FORMAT_FAHRENHEIT
 */
#if (!defined(TMEAS_FAHRENHEIT))
    #define TMEAS_FAHRENHEIT 1
#endif

/**
 * Set this define to 0 to disable the temperature sensor correction (enabled by default).
 * If, for your IC revision, the correction is applied straight in the TSEN calibration parameters, it must be set to 0.
 * Otherwise, it must be left at default value (1).
 */
#if (!defined(TMEAS_SENSOR_CORRECTION))
    #define TMEAS_SENSOR_CORRECTION 1
#endif

/**
 * By default, only synchronous measurements are enabled.
 * To enable asynchronous measurements, where the main thread execution continues and the measurement will be reported
 * later under interrupt by calling a callback function, define that callback function here.
 * Set this define to the function to be called.
 * @note The value set @b must have the same signature as @ref pTMeas_Cb_t
 * @note This must be set to the name of a function, not a pointer to a function: no dereference will be made!
 */
#ifndef TMEAS_CB
//    #define TMEAS_CB your_callback
#endif

/**
 * @}
 */

#endif
