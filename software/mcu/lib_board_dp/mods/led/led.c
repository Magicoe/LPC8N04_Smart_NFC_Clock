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


#include "chip.h"
#include "led/led.h"

#if LED_COUNT
static LED_PROPERTIES_T sLeds[LED_COUNT] = LED_PROPERTIES;
#endif

/* -------------------------------------------------------------------------
 * Public functions
 * ------------------------------------------------------------------------- */

void LED_Init(void)
{
#if LED_COUNT
    for (int n = 0; n < LED_COUNT; n++) {
        Chip_IOCON_SetPinConfig(NSS_IOCON, sLeds[n].pio, IOCON_FUNC_0 | IOCON_RMODE_INACT);
        Chip_GPIO_SetPinDIROutput(NSS_GPIO, sLeds[n].port, sLeds[n].pin);
    }
    LED_Off(LED_ALL);
#endif
}

void LED_SetState(int leds, int states)
{
#if LED_COUNT
    for (int n = 0; n < LED_COUNT; n++) {
        if (leds & LED_(n)) {
            Chip_GPIO_SetPinState(NSS_GPIO, sLeds[n].port, sLeds[n].pin, ((states >> n) & 1) == sLeds[n].polarity);
        }
    }
#endif
}

int LED_GetState(int leds)
{
#if LED_COUNT
    int result = 0;
    for (int n = 0; n < LED_COUNT; n++) {
        if (leds & LED_(n)) {
            result |= (Chip_GPIO_GetPinState(NSS_GPIO, sLeds[n].port, sLeds[n].pin) == sLeds[n].polarity) << n;
        }
    }
    return result;
#else
    return 0;
#endif
}

void LED_On(int leds)
{
    LED_SetState(leds, leds);
}

void LED_Off(int leds)
{
    LED_SetState(leds, 0);
}

void LED_Toggle(int leds)
{
    LED_SetState(leds, ~LED_GetState(leds));
}
