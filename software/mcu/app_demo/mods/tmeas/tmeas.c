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


/* -------------------------------------------------------------------------
 * Include files
 * ------------------------------------------------------------------------- */

#include <string.h>
#include "tmeas.h"

/* -------------------------------------------------------------------------
 * Private function prototypes
 * ------------------------------------------------------------------------- */

static int Convert(TMEAS_FORMAT_T format, int input);

/* -------------------------------------------------------------------------
 * Private variables
 * ------------------------------------------------------------------------- */

static volatile bool sMeasurementInProgress = false;
#if defined(TMEAS_CB)
static volatile TMEAS_FORMAT_T sFormat;
static volatile uint32_t sContext;
#endif

/* -------------------------------------------------------------------------
 * Private functions
 * ------------------------------------------------------------------------- */

#if defined(TMEAS_CB)
void TSEN_IRQHandler(void)
{
    /* If interrupt is reached, we can safely deduct that the RDY bit was set and therefore the
     * TSEN_STATUS_MEASUREMENT_SUCCESS status bit is set. The remaining (RANGE) status bits, even when set, should not
     * invalidate the temperature measurement,
     * hence we can always assume that, at this moment, the value present in the TSEN Value register is always valid.
     */
    /* Measurement ready. Read the data (thereby also clearing the interrupt). */
    int value = Chip_TSen_GetValue(NSS_TSEN);
    int output = Convert(sFormat, value);
    NVIC_DisableIRQ(TSEN_IRQn);
    Chip_TSen_DeInit(NSS_TSEN);
    {
        extern void TMEAS_CB(TSEN_RESOLUTION_T resolution, TMEAS_FORMAT_T format, int value, uint32_t context);
        TMEAS_CB(Chip_TSen_GetResolution(NSS_TSEN), sFormat, output, sContext);
    }
    sMeasurementInProgress = false;
}
#endif

/* ------------------------------------------------------------------------- */

static int Convert(TMEAS_FORMAT_T format, int input)
{
    int output;

    /* Temperature sensor correction is applied in the Native value here.
     * Regardless of the sample, a correction needs to be applied to fix a deviation with the sensor.
     * For a value C in degrees Celsius:
     *  C -> C * (1 - 0.6/85), with
     *  C -> C - C/128 as a good approximation
     * Which translates to a correction for a value N in native value (returned by the TSEN HW block):
     *  N -> N - N/128 + (273.15 * 64)/128, with
     *  N -> N - N/128 + 137 as a good approximation
     * The maximum error due to the approximations w.r.t. the full correction is
     * at most 0.06 degrees Celsius in the range [-40, +85], and
     * at most 0.03 degrees Celsius in the range [0, +40].
     */
#if TMEAS_SENSOR_CORRECTION
    input = input - (input/128) + 137;
#endif

    switch (format) {
#if TMEAS_KELVIN
        case TMEAS_FORMAT_KELVIN:
        output = Chip_TSen_NativeToKelvin(input, 10);
        break;
#endif
#if TMEAS_CELSIUS
        case TMEAS_FORMAT_CELSIUS:
            output = Chip_TSen_NativeToCelsius(input, 10);
            break;
#endif
#if TMEAS_FAHRENHEIT
            case TMEAS_FORMAT_FAHRENHEIT:
            output = Chip_TSen_NativeToFahrenheit(input, 10);
            break;
#endif
        default:
        case TMEAS_FORMAT_NATIVE:
            output = input; /* Measurement was taken in calibrated mode with the MSBit 0. */
            break;
    }
    return output;
}

/* -------------------------------------------------------------------------
 * Exported functions
 * ------------------------------------------------------------------------- */

int TMeas_Measure(TSEN_RESOLUTION_T resolution, TMEAS_FORMAT_T format, bool synchronous, uint32_t context)
{
#if !defined(TMEAS_CB)
    /* gracefully do nothing and avoid compiler warnings */
    (void)synchronous;
    (void)context;
#endif
    int output = TMEAS_ERROR;
    if (!sMeasurementInProgress) {
        sMeasurementInProgress = true;

        Chip_TSen_Init(NSS_TSEN);
        Chip_TSen_SetResolution(NSS_TSEN, resolution);
#if defined(TMEAS_CB)
        if (!synchronous) {
            sFormat = format;
            sContext = context;
            Chip_TSen_Int_SetEnabledMask(NSS_TSEN, TSEN_INT_MEASUREMENT_RDY);
            NVIC_EnableIRQ(TSEN_IRQn);
        }
#endif
        output = TMEAS_ERROR;

        Chip_TSen_Start(NSS_TSEN);
#if defined(TMEAS_CB)
        if (synchronous)
#endif
        {
            while (!(Chip_TSen_ReadStatus(NSS_TSEN, NULL) & TSEN_STATUS_MEASUREMENT_DONE)) {
                ; /* wait */
            }
            /* The remaining (RANGE) status bits, even when set, should not invalidate the temperature measurement,
             * hence we can always assume that, at this moment, the value present in the TSEN Value register is always valid. */
            /* Measurement ready. Read the data (thereby also clearing the DONE status bit). */
            output = Convert(format, Chip_TSen_GetValue(NSS_TSEN));
            NVIC_DisableIRQ(TSEN_IRQn);
            Chip_TSen_DeInit(NSS_TSEN);
            sMeasurementInProgress = false;
        }
#if defined(TMEAS_CB)
        else {
            output = 0;
            /* sMeasurementInProgress is set to false in TSEN_IRQHandler */
        }
#endif
    }

    return output;
}
