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
#include "board.h"
#include "ndeft2t/ndeft2t.h"
#include "tmeas/tmeas.h"
#include "timer.h"

#include "validate.h"

#include "storage/storage.h"

#include "msghandler.h"
#include "msghandler_protocol.h"

#include "memory.h"

#include "main.h"

#include "ssd1306.h"
#include "buzzer.h"
#include "rtc.h"

/* -------------------------------------------------------------------------
 * function prototypes
 * ------------------------------------------------------------------------- */

static void Init(void);
static void DeInit(void);


/** Application's main entry point. Declared here since it is referenced in ResetISR. */
int main(void);

/* -------------------------------------------------------------------------
 * variables
 * ------------------------------------------------------------------------- */

__attribute__ ((section(".noinit"))) __attribute__((aligned (4)))
static      uint8_t     sData[MAX_COMMAND_MESSAGE_SIZE];

__attribute__ ((section(".noinit"))) __attribute__((aligned (4)))
static      uint8_t     sNdefInstance[NDEFT2T_INSTANCE_SIZE];

volatile    bool        sTargetWritten = false;               // nfc reader want to write or not flag
volatile    bool        g_nfcOn        = false;               // nfc reader touched or not flag

volatile    uint32_t    g_TemperatureValue	 = 0;             // Temperature value from LPC8N04 internal
volatile    uint32_t    g_TemperatureoFValue = 0;             // Temperature value from LPC8N04 internal convert to oF
volatile    uint32_t    g_TempRecord[5];                      // Temperature value record in every 5sec/1min/5mins

volatile    uint8_t     g_OLEDDispBuf[4][16];                 // OLED display Buffer

volatile    uint16_t    hostTimeout;                          // main time out value
volatile    uint16_t    hostTicks;                            // main timer count value

volatile    int         sQuickMeasurement;                    // Expressed in deci-Celsius, The current temperature, measured using a low resolution. Not to be used for validation; useful for quick status reports

volatile    uint32_t    g_LedStatus    = 0;                   // Led GPIO configed status.
volatile    uint32_t    g_TempSettings = 0;                   // Temperature sampling settings.

volatile    RTC_VALUE_T g_sRTCValue;                          // RTC Value
volatile    uint8_t     g_AlarmHour    = 0;                   // Alarm Hour
volatile    uint8_t     g_AlarmMin     = 0;                   // Alarm Min
volatile    uint32_t    g_AlarmCnt     = 0;                   // Alarm counter

volatile    uint8_t     g_MotorFlag    = 0;                   // 0-Disable Vibration Motor, 1-Enable Vibration Motor
volatile    uint32_t    g_MotorEnTime  = 0;                   // Decide moto vibration timing

const       char        g_taglang[2]   = "en";                // For NDEF header, english charactors

volatile    uint8_t     g_TextModeFlag = 0;                   // TODO, MGN, Display Text from MobilePhone, 0-display date, 1-display text

volatile    uint8_t     g_AlarmEnFlag  = 0;                   // Alarm enable flag, 0-disable, 1-enable
volatile    uint8_t     g_TempUnitType = 0;                   // Temperature Unit Type , 0-oC, 1-F
volatile    uint8_t     g_TempPeriod   = 2;                   // Temperature record period , 0-None, 1-1second, 2-5seconds, 3-1min
volatile    uint8_t     g_TempStep     = 0;                   // Temperature LED display Step
volatile    uint8_t     g_TempBase     = 0;                   // Temperature LED display Base

volatile    uint32_t    g_DispTimeFlag = 0;                   // OLED dislpay content need update flag
volatile    uint32_t    g_AppStatus    = 0;                   // Record sytem config informations
volatile    uint32_t    g_DispTimeCnt  = 0;

volatile    uint8_t    *g_sDispData    = NULL;
volatile    uint8_t    *nfcWriteMem    = NULL;
volatile    uint8_t     g_NFCDataUpdateFlag = 0;
volatile    uint8_t     g_TagDataBuf[128];

volatile    uint32_t    g_RTCTicks     = 0;
volatile    uint32_t    g_RTCTicksBak  = 0;
volatile    uint32_t    g_MainTickCnt  = 0;                 // Main Ticks, help record temperature history

volatile    uint8_t     g_OLEDInitFlag = 0;                 // 0 - not init, 1 - inited

volatile    uint32_t    g_LPC8N04PSTAT = 0;                 // Save LPC8N04 PSTAT register as temp

volatile	NDEFT2T_CREATE_RECORD_INFO_T g_recordInfo;

/* -------------------------------------------------------------------------
 * Private functions
 * ------------------------------------------------------------------------- */
void ResetISR(void)
{
    /* Increasing the system clock as soon as possible to reduce the startup time.
     * Setting the clock to 2MHz is the maximum: when
     * - running without a battery
     * - at 4MHz
     * - when the field is provided by some phones (e.g. S5)
     * a voltage drop to below 1.2V was observed - effectively resetting the chip.
     */
    Chip_Clock_System_SetClockFreq(2 * 1000 * 1000);
    Startup_VarInit();
    main();
}

/** Called under interrupt. Refer #NDEFT2T_FIELD_STATUS_CB. */
void NDEFT2T_FieldStatus_Cb(bool status)
{
    /* A PCD can do very strange things, a.o. power off the field and immediately power on the field again, or 10+ times
     * select the same device as part of its illogic procedure to select a PICC and start communicating with it.
     * Instead of deciding to Power-off or go to Deep Power Down immediately, it is more robust to additionally check
     * if during a small interval no NFC field is started again.
     * The loop in ExecuteNfcMode() may thus not look at the NFC interrupt status, but at the result of the timer
     * interrupt.
     */
    if (status) {
        hostTimeout = HOST_TIMEOUT;
        hostTicks = 0;
        g_nfcOn = true;
    }
    else {
        hostTimeout = LAST_HOST_TIMEOUT;
        hostTicks = 0;
        g_nfcOn = false;
    }
}

/** Called under interrupt. @see #NDEFT2T_MSG_AVAILABLE_CB. */
void NDEFT2T_MsgAvailable_Cb(void)
{
    sTargetWritten = true;
    hostTimeout = HOST_TIMEOUT;
    hostTicks = 0;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void TMEAS_CB(TSEN_RESOLUTION_T resolution, TMEAS_FORMAT_T format, int value, uint32_t context)
{
    ASSERT(format == TMEAS_FORMAT_CELSIUS);
    if (value < -APP_MSG_MAX_TEMPERATURE) {
        value = -APP_MSG_TEMPERATURE_PLACEHOLDER_VALUE;
    }
    if (APP_MSG_MAX_TEMPERATURE < value) {
        value = APP_MSG_TEMPERATURE_PLACEHOLDER_VALUE;
    }

    /* Based on the context, we deduce the reason of measuring the temperature. This context is given as last argument
     * in a call to TMeas_Measure()
     */
    switch (context) {
        case 0:
            /* A measurement was requested while not communicating. This value is to be stored internally, until a tag
             * reader makes an NFC connection and starts reading out all the stored samples.
             */
            /* resolution == TSEN_10BITS */
            g_TemperatureValue = value;
            break;

        case 1:
            /* A live measurement was requested while communicating. Wrap it in a response, and send it off. */
            /* resolution == can vary */
            AppMsgHandlerSendMeasureTemperatureResponse(value != TMEAS_ERROR, (int16_t)value);
            break;

        default:
            /* This value will be used in the initial response. We're still initializing everything at this point.
             * Unconditionally store the value for immediate use in the main thread.
             */
            /* resolution == TSEN_7BITS */
            sQuickMeasurement = value;
            break;
    }
}

// Calculate LED display numbers
uint8_t led_light_calc(uint8_t unit)
{
    uint32_t Step = 0;
    uint32_t g_TemperatureBase = 0;
    uint32_t g_TemperatureValueTmp = 0;

    /* unit = 0: oC */
    /* unit = 1: oF */
    if(unit == 0) {
        g_TemperatureValueTmp = g_TemperatureValue;
        g_TemperatureBase = (((g_TempSettings >> 16) & 0x00FF) +20) * 10;
        Step = (((g_TempSettings >> 8)  & 0x00FF) + 1) * 5;
    }
    else {
        g_TemperatureValueTmp = (g_TemperatureValue*18+3200)/10;
        g_TemperatureBase = (((g_TempSettings >> 16) & 0x00FF) +68) * 10;
        Step = (((g_TempSettings >> 8)  & 0x00FF) + 1) * 10;
    }
    /* turn on of off leds based on temperature values */
    if(g_TemperatureValueTmp  <= (g_TemperatureBase-2*Step)) {
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 0, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 1, 0);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 2, 0);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 6, 0);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 8, 0);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 9, 0);
        return 1;
    }
    if( (g_TemperatureValueTmp > (g_TemperatureBase-2*Step)) && (g_TemperatureValueTmp <= (g_TemperatureBase-1*Step))) {
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 0, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 1, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 2, 0);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 6, 0);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 8, 0);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 9, 0);
        return 2;
    }
    if( (g_TemperatureValueTmp > (g_TemperatureBase-1*Step)) && (g_TemperatureValueTmp <= (g_TemperatureBase-0*Step))) {
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 0, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 1, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 2, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 6, 0);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 8, 0);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 9, 0);
        return 3;
    }
    if( (g_TemperatureValueTmp > (g_TemperatureBase-0*Step)) && (g_TemperatureValueTmp <= (g_TemperatureBase+1*Step))) {
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 0, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 1, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 2, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 6, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 8, 0);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 9, 0);
        return 4;
    }
    if( (g_TemperatureValueTmp > (g_TemperatureBase+1*Step)) && (g_TemperatureValueTmp <= (g_TemperatureBase+2*Step))) {
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 0, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 1, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 2, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 6, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 8, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 9, 0);
        return 5;
    }
    if( (g_TemperatureValueTmp > (g_TemperatureBase+2*Step))) {
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 0, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 1, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 2, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 6, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 8, 1);
        Chip_GPIO_SetPinState(NSS_GPIO, 0, 9, 1);
        return 6;
    }

    /* Clear up LED, should never run */
    Chip_GPIO_SetPinState(NSS_GPIO, 0, 0, 1);
    Chip_GPIO_SetPinState(NSS_GPIO, 0, 1, 0);
    Chip_GPIO_SetPinState(NSS_GPIO, 0, 2, 0);
    Chip_GPIO_SetPinState(NSS_GPIO, 0, 6, 0);
    Chip_GPIO_SetPinState(NSS_GPIO, 0, 8, 0);
    Chip_GPIO_SetPinState(NSS_GPIO, 0, 9, 0);
    return 0;
}

/* Initialize System */
static void Init(void)
{
    /* First initialize the absolute minimum.
     * Avoid accessing the RTC and the PMU as these API calls are slow. Prepare an NFC message ASAP.
     * Only after that we can complete the initialization.
     */
    Board_Init();

    Chip_GPIO_SetPinState(NSS_GPIO, 0, 0, 0);
    Chip_GPIO_SetPinState(NSS_GPIO, 0, 1, 0);
    Chip_GPIO_SetPinState(NSS_GPIO, 0, 2, 0);
    Chip_GPIO_SetPinState(NSS_GPIO, 0, 6, 0);
    Chip_GPIO_SetPinState(NSS_GPIO, 0, 8, 0);
    Chip_GPIO_SetPinState(NSS_GPIO, 0, 9, 0);

    Chip_GPIO_SetPinState(NSS_GPIO, 0, 7, 0);

    // LED0
    Chip_IOCON_SetPinConfig(NSS_IOCON, 0, IOCON_FUNC_0 | IOCON_RMODE_PULLUP);
    Chip_GPIO_SetPinDIROutput(NSS_GPIO, 0, 0);
    // LED1
    Chip_IOCON_SetPinConfig(NSS_IOCON, 1, IOCON_FUNC_0 | IOCON_RMODE_PULLUP);
    Chip_GPIO_SetPinDIROutput(NSS_GPIO, 0, 1);
    // LED2
    Chip_IOCON_SetPinConfig(NSS_IOCON, 2, IOCON_FUNC_0 | IOCON_RMODE_PULLUP);
    Chip_GPIO_SetPinDIROutput(NSS_GPIO, 0, 2);
    // LED3
    Chip_IOCON_SetPinConfig(NSS_IOCON, 6, IOCON_FUNC_0 | IOCON_RMODE_PULLUP);
    Chip_GPIO_SetPinDIROutput(NSS_GPIO, 0, 6);
    // LED4
    Chip_IOCON_SetPinConfig(NSS_IOCON, 8, IOCON_FUNC_0 | IOCON_RMODE_PULLUP);
    Chip_GPIO_SetPinDIROutput(NSS_GPIO, 0, 8);
    // LED5
    Chip_IOCON_SetPinConfig(NSS_IOCON, 9, IOCON_FUNC_0 | IOCON_RMODE_PULLUP);
    Chip_GPIO_SetPinDIROutput(NSS_GPIO, 0, 9);

    // OLED PWR
    Chip_IOCON_SetPinConfig(NSS_IOCON, 7, IOCON_FUNC_0 | IOCON_RMODE_PULLUP);
    Chip_GPIO_SetPinDIROutput(NSS_GPIO, 0, 7);

    // OLED Power disable
    Chip_GPIO_SetPinState(NSS_GPIO, 0, 7, 0);

    // Measurement temperature at the beginning
    TMeas_Measure(TSEN_10BITS, TMEAS_FORMAT_CELSIUS, false, 0 /* Value used in App_TmeasCb */);
        while (Chip_TSen_ReadStatus(NSS_TSEN, NULL) & TSEN_STATUS_SENSOR_IN_OPERATION) {
        ; /* Wait until the temperature has become available in sQuickMeasurement. */
    }

    // Temperature LED display settings
    // bit 31:28   27:24   23:20   19:16   15:12   11:8    7:3     3:0
    //     HeaderH HeaderL Base_H  Base_L  Step_H  Step_L  TBD_H   TBD_L
    Chip_PMU_GetRetainedData(&g_TempSettings, 1, 1);
    if( (g_TempSettings&0xFF000000) != 0x5A000000) {
        g_TempSettings = 0x5A000000 | (0x00 << 16) | ( 0x00 << 8);
        Chip_PMU_SetRetainedData(&g_TempSettings, 1, 1);
    }

    Chip_PMU_GetRetainedData(&g_LedStatus, 3, 1);

    g_LedStatus = 0xAA550000;                       // added a header and this will let system know this is not the first reset.
    Chip_PMU_SetRetainedData(&g_LedStatus, 3, 1);   // Save Status in PUM_BUF[3]

    /* Reduce power consumption by adding a pull-down. The default register values after a reset do
     * not have enabled these pulls. The functionality of the SWD pins are kept.
     */
    /**/
    Chip_IOCON_SetPinConfig(NSS_IOCON, 3, IOCON_FUNC_1 | IOCON_RMODE_INACT);

    Chip_NFC_Init(NSS_NFC);         // NFC initilize
    NDEFT2T_Init();                 // NFC NDEF format

    Chip_EEPROM_Init(NSS_EEPROM);   // Initial System EEPROM
    Timer_Init();                   // Timer Initilize
    Validate_Init();

    g_nfcOn     = false;            // NFC reader touched flag initilize

    // Clear up NTAG RAM area
    memset(sData, 0x00, sizeof(sData));
    /* Enable, configure and start the watchdog timer in normal (reset) mode.
    * We let the clock run as slow as possible: thus using 254 in the SetClockDiv call below.
    */
    Chip_Clock_Watchdog_SetClockSource(CLOCK_WATCHDOGSOURCE_SFRO);
    Chip_Clock_Watchdog_SetClockDiv(254);
    Chip_WWDT_Init(NSS_WWDT);
    Chip_WWDT_SetTimeOut(NSS_WWDT, WATCHDOG_TICKS(254, WATCHDOG_TIMEOUT));
    Chip_WWDT_Start(NSS_WWDT);
    /* The watchdog will be fed - by calling Chip_WWDT_Feed - in ExecuteCommunicationMode in a while loop when the NFC
    * field is present. A reset will then occur when either in ExecuteMeasurementMode more than too many seconds are
    * spent, i.e. it is stuck somewhere.
    */
}



/**
 * Cleanly closes everything down, decides which low power state to go to - Deep Power Down or Power-off, and enters
 * that state using the correctly determined parameters.
 * @note Wake-up conditions other than a reset pulse or the presence of an NFC field - both of which cannot be disabled
 *  nor require configuration - must have been set beforehand.
 * @param waitBeforeDisconnect Present to aid the SW developer. If @c true, it will wait - not sleep - for a couple of
 *  seconds before disconnecting the battery. The argument has no effect if Deep Power Down mode is selected.
 *  The extra time window allows for easier breaking in using SWD, allowing time to halt the core and flash new
 *  firmware. However, it will @c not touch any PIO, or ensure that SWD functionality is offered.
 *  The default value should be @c false, i.e. go to Power-off state without delay: typical user behavior is to bring
 *  the IC in and out the NFC field quickly, before stabilizing on a correct position. Having a time penalty of several
 *  seconds - during which the host SW may already have made several decisions about the use and state of the IC -
 *  diminishes the user experience.
 */
static void DeInit(void)
{
    bool bod;

    NDEFT2T_DeInit();
    NVIC_DisableIRQ(CT32B0_IRQn);
    buzzer_stop();

    // Config GPIO as low for low power consumption
    NSS_GPIO->DATA[0xFFF] = 0x0000;

    Chip_PMU_SetBODEnabled(true);
    bod = ((Chip_PMU_GetStatus() & PMU_STATUS_BROWNOUT) != 0);
    Chip_PMU_SetBODEnabled(false);

    Timer_StartMeasurementTimeout(10);
    // Enter deep power down - low power mode
    Chip_PMU_PowerMode_EnterDeepPowerDown(bod);

    /* Normally, this function never returns. However, when providing power via SWD or any other PIO this will not
     * work - power is flowing through the bondpad ring via the SWD pin, still powering a small part of the VDD_ALON
     * domain.
     * This situation is not covered by HW design: we can't rely on anything being functional or even harmless.
     * Just ensure nothing happens, and wait until all power is gone.
     */
    for(;;);
}


/* -------------------------------------------------------------------------------- */
int main(void)
{
    PMU_DPD_WAKEUPREASON_T wakeupReason;

    Init();

    Timer_StartMeasurementTimeout(1);                // Set 1Seconds period and update lcd

    g_OLEDInitFlag = 0;

    Chip_PMU_GetRetainedData(&g_AppStatus, 0, 1);

    /* first power-up */
    if((g_AppStatus&0xFF000000) != 0x5A000000) {
        /* Initialize RTC and OLED panel */
        uint32_t iRTCSetTicks;
        RTC_VALUE_T i_sRTCValueCal;
        /* Set RTC date as 2017-11-22 23-59-20 */
        i_sRTCValueCal.YEARS   = 2017;
        i_sRTCValueCal.MONTHS  = 11;
        i_sRTCValueCal.DAYS    = 22;
        i_sRTCValueCal.HOURS   = 23;
        i_sRTCValueCal.MINUTES = 59;
        i_sRTCValueCal.SECONDS = 20;
        iRTCSetTicks = RTC_Convert2Tick(&i_sRTCValueCal);
        Chip_RTC_Time_SetValue(NSS_RTC, iRTCSetTicks);
    }

    /* oled_lpw_exit(); */
    /* Is Alarm Function Enabled? */
    if( ((g_AppStatus>>23) & 0x01) == 0x01 )  g_AlarmEnFlag  = 1;
    else                                      g_AlarmEnFlag  = 0;
    /* Temperature Unit oC or F */
    if( ((g_AppStatus>>22) & 0x01) == 0x01 )  g_TempUnitType = 1;
    else                                      g_TempUnitType = 0;
    /* Temperature Sampling */
    if( ((g_AppStatus>>20) & 0x03) != 0x00 )  g_TempPeriod   = (g_AppStatus>>20) & 0x03;
    else                                      g_TempPeriod   = 2;
    /* Need Display a text or not */
    if( ((g_AppStatus>>19) & 0x01) == 0x01 )  g_TextModeFlag = 1;
    else                                      g_TextModeFlag = 0;

    /* Get Alarm Value */
    g_AlarmHour = (g_AppStatus>>8)&0x000000FF;
    g_AlarmMin  = (g_AppStatus>>0)&0x000000FF;

    /* enter while loop */
    g_AppStatus   = 1;                                  // Enter while() loop	
    g_NFCDataUpdateFlag = 0;
    /* Read Temperature Record first */
    Chip_EEPROM_Read(NSS_EEPROM, 0, g_TempRecord, 20);

    g_DispTimeCnt = 0;                                  // Will update in NFC Powered

    while(g_AppStatus) {
        /* Measurement Temperature */
        /* Value used in App_TmeasCb */
        TMeas_Measure(TSEN_10BITS, TMEAS_FORMAT_CELSIUS, false, 0);
        while (Chip_TSen_ReadStatus(NSS_TSEN, NULL) & TSEN_STATUS_SENSOR_IN_OPERATION) {
            ; /* Wait until the temperature has become available in sQuickMeasurement. */
        }

        RTC_Convert2Date(&g_sRTCValue);
        
        /* Get LPC8N04 Power source */
        /* g_LPC8N04PSTAT == 1  ====> Antenna powered */
        /* g_LPC8N04PSTAT != 1  ====> Battery powered */
        g_LPC8N04PSTAT = Chip_PMU_Switch_GetVNFC();

        /* Initialize OLED panel */
        if( (g_OLEDInitFlag == 0) && (g_nfcOn == true) && (g_LPC8N04PSTAT != 1) ) {
            ssd1306_init();
            g_OLEDInitFlag = 1;
        }

        /* Update LED and OLED content */
        if(g_DispTimeCnt != 0) {

            /* Enable 6 LED or not */
            /* need use g_TempUnitType as update parameter */
            if(g_LPC8N04PSTAT != 1) {
                if(g_TempUnitType == 0) {
                    led_light_calc(0);
                }
                else {
                    led_light_calc(1);
                }
            }

            /* Temperature history */
            Chip_EEPROM_Read(NSS_EEPROM, 0, g_TempRecord, 20);

            g_MainTickCnt++;                    // every main cycle need 1 Second

            if(g_TempPeriod == 1) {
                /* Record temperature value every 5seconds */
                if( (g_MainTickCnt%5) == 1) {
                    g_TempRecord[0] = g_TempRecord[1];
                    g_TempRecord[1] = g_TempRecord[2];
                    g_TempRecord[2] = g_TempRecord[3];
                    g_TempRecord[3] = g_TempRecord[4];
                    g_TempRecord[4] = g_TemperatureValue;
                    Chip_EEPROM_Write(NSS_EEPROM, 0, g_TempRecord, 20);
                }
            }
            if(g_TempPeriod == 2) {
                /* Record temperature value every 1minutes */
                if( (g_MainTickCnt%60) == 2) {
                    g_TempRecord[0] = g_TempRecord[1];
                    g_TempRecord[1] = g_TempRecord[2];
                    g_TempRecord[2] = g_TempRecord[3];
                    g_TempRecord[3] = g_TempRecord[4];
                    g_TempRecord[4] = g_TemperatureValue;
                    Chip_EEPROM_Write(NSS_EEPROM, 0, g_TempRecord, 20);
                }
            }
            if(g_TempPeriod == 3) {
                /* Record temperature value every 5minutes */
                if( (g_MainTickCnt%300) == 3) {
                    g_TempRecord[0] = g_TempRecord[1];
                    g_TempRecord[1] = g_TempRecord[2];
                    g_TempRecord[2] = g_TempRecord[3];
                    g_TempRecord[3] = g_TempRecord[4];
                    g_TempRecord[4] = g_TemperatureValue;
                    Chip_EEPROM_Write(NSS_EEPROM, 0, g_TempRecord, 20);
                }
            }

            /* Get RTC date and time value */
            RTC_Convert2Date(&g_sRTCValue);
            if( (g_OLEDInitFlag == 1) && (g_LPC8N04PSTAT != 1) ) {
                // Clear OLED display buffer
                memset(g_OLEDDispBuf[1], 0x00, 16);

                if(g_DispTimeFlag== 0) {
                    if( (g_sRTCValue.HOURS<10) && (g_sRTCValue.MINUTES<10) ) {
                        sprintf(g_OLEDDispBuf[1] , "0%d:0%d  ", g_sRTCValue.HOURS, g_sRTCValue.MINUTES);
                    }
                    else if( (g_sRTCValue.HOURS<10) && (g_sRTCValue.MINUTES>=10) ) {
                        sprintf(g_OLEDDispBuf[1] , "0%d:%d  ", g_sRTCValue.HOURS, g_sRTCValue.MINUTES);
                    }
                    else if( (g_sRTCValue.HOURS>=10) && (g_sRTCValue.MINUTES<10) ) {
                        sprintf(g_OLEDDispBuf[1] , "%d:0%d  ", g_sRTCValue.HOURS, g_sRTCValue.MINUTES);
                    }
                    else {
                        sprintf(g_OLEDDispBuf[1] , "%d:%d  ", g_sRTCValue.HOURS, g_sRTCValue.MINUTES);
                    }
                    g_DispTimeFlag = 1;
                }
                else {
                    if( (g_sRTCValue.HOURS<10) && (g_sRTCValue.MINUTES<10) ) {
                        sprintf(g_OLEDDispBuf[1] , "0%d 0%d  ", g_sRTCValue.HOURS, g_sRTCValue.MINUTES);
                    }
                    else if( (g_sRTCValue.HOURS<10) && (g_sRTCValue.MINUTES>=10) ) {
                        sprintf(g_OLEDDispBuf[1] , "0%d %d  ", g_sRTCValue.HOURS, g_sRTCValue.MINUTES);
                    }
                    else if( (g_sRTCValue.HOURS>=10) && (g_sRTCValue.MINUTES<10) ) {
                        sprintf(g_OLEDDispBuf[1] , "%d 0%d  ", g_sRTCValue.HOURS, g_sRTCValue.MINUTES);
                    }
                    else {
                        sprintf(g_OLEDDispBuf[1] , "%d %d  ", g_sRTCValue.HOURS, g_sRTCValue.MINUTES);
                    }
                    g_DispTimeFlag = 0;
                }
                OLED_ShowTime(0,2,g_OLEDDispBuf[1]);

                memset(g_OLEDDispBuf[2], 0x00, 16);
                if( (g_sRTCValue.MONTHS<10) && (g_sRTCValue.DAYS<10) ) {
                    sprintf(g_OLEDDispBuf[3] , "   %d-0%d-0%d", g_sRTCValue.YEARS, g_sRTCValue.MONTHS, g_sRTCValue.DAYS);
                }
                else if( (g_sRTCValue.MONTHS<10) && (g_sRTCValue.DAYS>=10) ) {
                    sprintf(g_OLEDDispBuf[3] , "   %d-0%d-%d",  g_sRTCValue.YEARS, g_sRTCValue.MONTHS, g_sRTCValue.DAYS);
                }
                else if( (g_sRTCValue.MONTHS>=10) && (g_sRTCValue.DAYS<10) ) {
                    sprintf(g_OLEDDispBuf[3] , "   %d-%d-0%d",  g_sRTCValue.YEARS, g_sRTCValue.MONTHS, g_sRTCValue.DAYS);
                }
                else {
                    sprintf(g_OLEDDispBuf[3] , "   %d-%d-%d",   g_sRTCValue.YEARS, g_sRTCValue.MONTHS, g_sRTCValue.DAYS);
                }
                OLED_ShowStr(0,6,g_OLEDDispBuf[3]);

                memset(g_OLEDDispBuf[0], 0x00, 16);
                /* Display Temperature as Celsius */
                if(g_TempUnitType == 0) {
                    sprintf(g_OLEDDispBuf[0] , "%d.%doC" , g_TemperatureValue/10, g_TemperatureValue%10);
                }
                else {
                    g_TemperatureoFValue = (g_TemperatureValue*18+3200)/10;
                    /* Added a ' ' after 'F' to clear up oC before */
                    sprintf(g_OLEDDispBuf[0] , "%d.%dF " , g_TemperatureoFValue/10, g_TemperatureoFValue%10);
                }

                OLED_ShowStr(0,0,g_OLEDDispBuf[0]);
                OLED_ShowBat(0);

                if(g_AlarmEnFlag == 1)   OLED_ShowAlarm(1);
                else                     OLED_ShowAlarm(0);
            }

            if(sTargetWritten == false) {
                if(g_NFCDataUpdateFlag == 0) {
                    g_NFCDataUpdateFlag = 1;
                    bool success = true;
                    /* Creat NDEF Message */
                    NDEFT2T_CreateMessage(sNdefInstance, sData, sizeof(sData), false);
                    g_recordInfo.shortRecord = true;
                    g_recordInfo.pString = (uint8_t *)g_taglang;
                    success &= NDEFT2T_CreateTextRecord(sNdefInstance, &g_recordInfo);
                    if(g_TempRecord[0] > 2000) g_TempRecord[0] = 0;
                    if(g_TempRecord[1] > 2000) g_TempRecord[1] = 0;
                    if(g_TempRecord[2] > 2000) g_TempRecord[2] = 0;
                    if(g_TempRecord[3] > 2000) g_TempRecord[3] = 0;
                    if(g_TempRecord[4] > 2000) g_TempRecord[4] = 0;
                    memset(g_TagDataBuf, 0x00, 128);
                    sprintf(g_TagDataBuf, "TEMP0%5dTEMP1%5dTEMP2%5dTEMP3%5dTEMP4%5dTEMP5%5d\r\n", g_TemperatureValue, g_TempRecord[0], g_TempRecord[1], g_TempRecord[2], g_TempRecord[3], g_TempRecord[4]);
                    if (success) {
                        success = NDEFT2T_WriteRecordPayload(sNdefInstance, g_TagDataBuf, (strlen(g_TagDataBuf)));
                        if (success) {
                            NDEFT2T_CommitRecord(sNdefInstance);
                        }
                    }
                    if (success) {
                        NDEFT2T_CommitMessage(sNdefInstance);
                    }
                }
                g_NFCDataUpdateFlag = 0;
            }
        }

        // Alarm enable and vibration motor when powered by external power source
        if( (g_AlarmEnFlag == 1) && (g_LPC8N04PSTAT != 1) ) {
            if( (g_sRTCValue.HOURS == g_AlarmHour) && (g_sRTCValue.MINUTES == g_AlarmMin) && (g_sRTCValue.SECONDS <= 20) ) {
                /* Initialize OLED panel */
                if( g_OLEDInitFlag == 0 ) {
                    ssd1306_init();
                    g_OLEDInitFlag = 1;
                    g_RTCTicksBak = Chip_RTC_Time_GetValue(NSS_RTC);
                    g_DispTimeCnt = 10;	// Wake up 10sec
                    g_MainTickCnt = 0;

                    buzzer_start();
                    uint32_t i, j;
                    for(i=0; i<50; i++)
                        for(j=0; j<1000; j++);
                    buzzer_stop();
                }
                else {
                    // Nothing
                }
            }
        }

        // Determine wakeup resource 
        wakeupReason = Chip_PMU_PowerMode_GetDPDWakeupReason();
        // Wakeup by RTC timer from deep power down mode
        if(wakeupReason == PMU_DPD_WAKEUPREASON_RTC) {
            uint32_t i, j;
            Chip_GPIO_SetPinState(NSS_GPIO, 0, 0, 1);   // For LED
            for(i = 0; i<200; i++)
            for(j=0; j<1000; j++);
            //g_AppStatus = 0;
        }

        if(wakeupReason == PMU_DPD_WAKEUPREASON_NFCPOWER) {
            uint32_t i,j;
            nfcWriteMem = (uint32_t *)NSS_NFC->BUF;

            if(g_DispTimeCnt == 0) {
                RTC_Convert2Date(&g_sRTCValue);

                g_RTCTicksBak = Chip_RTC_Time_GetValue(NSS_RTC);
                g_DispTimeCnt = WAKEUP_MINS*60;   // Wake up WAKEUP_MINS min
                g_MainTickCnt = 0;
            }
            /* App required write */
            if ( (sTargetWritten) ) {
                sTargetWritten = false;
                /* Safest is to just try to communicate. It will be stopped or restarted using the callbacks provided by the
                 * NDEFT2T module. By using Timer_CheckHostTimeout in the while loop below, the while loop will be
                 * stopped when the field has been removed.
                 */
                hostTimeout = FIRST_HOST_TIMEOUT;
                hostTicks = 0;

                Chip_TIMER32_0_Init();
                Chip_TIMER_PrescaleSet(NSS_TIMER32_0, 1);
                Chip_TIMER_Reset(NSS_TIMER32_0);
                NVIC_EnableIRQ(CT32B0_IRQn);
                Chip_TIMER_Enable(NSS_TIMER32_0);

                /* Wait for a command. Send responses based on these commands. */
                while (hostTicks < hostTimeout) {
                    // Delay a while to get data correctly
                    for(i = 0; i<200; i++)
                        for(j=0; j<1000; j++);

                    if(g_LPC8N04PSTAT != 1)   buzzer_start();

                    for(i=0; i<100; i++) {
                        if( (nfcWriteMem[i] == 'N') && (nfcWriteMem[i+1] == 'E') && (nfcWriteMem[i+2] == 'W') ) {
                            RTC_VALUE_T g_sRTCValueCal;

                            g_sRTCValueCal.YEARS   = (nfcWriteMem[i+3] -0x30)*1000 + (nfcWriteMem[i+4] -0x30)*100 + (nfcWriteMem[i+5]-0x30)*10 + (nfcWriteMem[i+6]-0x30);
                            g_sRTCValueCal.MONTHS  = (nfcWriteMem[i+8] -0x30)*10   + (nfcWriteMem[i+9] -0x30);
                            g_sRTCValueCal.DAYS    = (nfcWriteMem[i+11]-0x30)*10   + (nfcWriteMem[i+12]-0x30);

                            g_sRTCValueCal.HOURS   = (nfcWriteMem[i+14]-0x30)*10   + (nfcWriteMem[i+15]-0x30);
                            g_sRTCValueCal.MINUTES = (nfcWriteMem[i+17]-0x30)*10   + (nfcWriteMem[i+18]-0x30);

                            g_sRTCValueCal.SECONDS = (nfcWriteMem[i+20]-0x30)*10 + (nfcWriteMem[i+21]-0x30);

                            if(nfcWriteMem[i+22] == 'E') g_AlarmEnFlag = 1;
                            else                         g_AlarmEnFlag = 0;

                            g_AlarmHour            = (nfcWriteMem[i+24]-0x30)*10   + (nfcWriteMem[i+25]-0x30);
                            g_AlarmMin             = (nfcWriteMem[i+27]-0x30)*10   + (nfcWriteMem[i+28]-0x30);

                            if(nfcWriteMem[i+30] == '1')            g_TempPeriod = 1;
                            else if(nfcWriteMem[i+30] == '2')       g_TempPeriod = 2;
                            else                                    g_TempPeriod = 3;

                            if(nfcWriteMem[i+32] == 'F')            g_TempUnitType = 1;
                            else                                    g_TempUnitType = 0;

                            if(nfcWriteMem[i+35] == '0')            g_TempStep   = 0;
                            else if(nfcWriteMem[i+35] == '1')       g_TempStep   = 1;
                            else if(nfcWriteMem[i+35] == '2')       g_TempStep   = 2;
                            else if(nfcWriteMem[i+35] == '3')       g_TempStep   = 3;
                            else                                    g_TempStep   = 0;

                            if(nfcWriteMem[i+38] == '0')            g_TempBase   = 0;
                            else if(nfcWriteMem[i+38] == '1')       g_TempBase   = 1;
                            else if(nfcWriteMem[i+38] == '2')       g_TempBase   = 2;
                            else if(nfcWriteMem[i+38] == '3')       g_TempBase   = 3;
                            else if(nfcWriteMem[i+38] == '4')       g_TempBase   = 4;
                            else if(nfcWriteMem[i+38] == '5')       g_TempBase   = 5;
                            else if(nfcWriteMem[i+38] == '6')       g_TempBase   = 6;
                            else if(nfcWriteMem[i+38] == '7')       g_TempBase   = 7;
                            else if(nfcWriteMem[i+38] == '8')       g_TempBase   = 8;
                            else if(nfcWriteMem[i+38] == '9')       g_TempBase   = 9;
                            else                                    g_TempBase   = 0;

                            // Temperature LED display settings
                            // bit 31:28   27:24   23:20   19:16   15:12   11:8    7:3     3:0
                            //     HeaderH HeaderL Base_H  Base_L  Step_H  Step_L  TBD_H   TBD_L
                            g_TempSettings = 0x5A000000 | (g_TempBase << 16) | ( g_TempStep << 8);
                            Chip_PMU_SetRetainedData(&g_TempSettings, 1, 1);

                            uint32_t RTCSetTicks;
                            RTCSetTicks = RTC_Convert2Tick(&g_sRTCValueCal);

                            Chip_RTC_Time_SetValue(NSS_RTC, RTCSetTicks);
                            i = 200;
                            hostTicks = hostTimeout + 1;
                        }
                        else if( (nfcWriteMem[i] == 'M') && (nfcWriteMem[i+1] == 'S') && (nfcWriteMem[i+2] == 'G') ) {
                            i = 200;
                            hostTicks = hostTimeout + 1;
                        }
                        else {
                            // TODO: Nothing
                        }
                    }

                    if ( Timer_CheckMeasurementTimeout( ) ) {
                        hostTicks++;
                        Timer_StartMeasurementTimeout(1);
                    }

                    if(g_nfcOn == false) {
                        hostTicks = hostTimeout + 1;
                    }
                    else {
                        // TODO:
                    }
                }

                /* delay a while, then mute buzzer */
                for(i=0; i<200; i++)
                    for(j=0; j<1000; j++);
                /* beep when save data into RetainedData area */
                if(g_LPC8N04PSTAT != 1)  buzzer_stop();

                /* get RTC tick value */
                g_RTCTicksBak = Chip_RTC_Time_GetValue(NSS_RTC);
                g_DispTimeCnt = WAKEUP_MINS*60;   // Set Wake up WAKEUP_MINS min
            }
        }

        /* nfc powered LPC8N04 */
        if(g_nfcOn == true) {
            g_RTCTicksBak = Chip_RTC_Time_GetValue(NSS_RTC);
            g_DispTimeCnt = 3*60;   // Wake up 3 min
        }

        g_RTCTicks = Chip_RTC_Time_GetValue(NSS_RTC);
        /* Check the Ticks is reach */
        if(g_RTCTicks >= (g_RTCTicksBak + g_DispTimeCnt)) {
            /* alive time reach */
            g_DispTimeCnt = 0;
            g_AppStatus = 0;
        }

        /* If DiapTimeCnt to 0, then jump out of while and enter low power mode */
        if(g_DispTimeCnt == 0) {
            g_AppStatus = 0;        // clear g_AppStatus to 0
        }
//      g_AppStatus = 1;            // TEST, if enable as 1, never enter low power modes.
    }

    // Save System Valuable Status
    // bit 31:28   27:24   23:20   19:16   15:12   11:8    7:3     3:0
    //     HeaderH HeaderL Mark_H  Mark_L  Hour_H  Hour_L  Mins_H  MinsL
    g_AppStatus = 0x5A000000                        // Header 0x5A
                | (g_AlarmEnFlag<<23)
                | (g_TempUnitType<<22)
                | ((g_TempPeriod&0x03)<<20)
                | (g_TextModeFlag<<19)
                | (g_AlarmHour<<8)
                | (g_AlarmMin << 0);
    /* Save g_AppStatus in the PMU_BUF[0] */
    Chip_PMU_SetRetainedData(&g_AppStatus, 0, 1);

    /* enter low power */
    if( (g_OLEDInitFlag == 1) && (g_LPC8N04PSTAT != 1) ) {
    	oled_lpw_enter();       // OLED enter low power mode or disable OLED power
    	g_OLEDInitFlag = 0;     // clear oled initialize flag
    }

    DeInit();                   // Does not return.

    return 0;
}

// end file
