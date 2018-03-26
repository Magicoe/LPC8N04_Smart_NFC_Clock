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
#include "trace/trace.h"

#include <stdarg.h>
#if TRACE_PRINTF_BUFSIZE == 0
    #include <string.h> /* Needed for "strlen" */
#endif
#if TRACE_SPRINTF_STDLIB == 1
    #include <stdio.h> /* Needed for "puts" and "getchar" */
#endif
#if TRACE_DATAPIPE_I2C0 == 1
    #include "i2cio/i2cio.h"
#endif
/*****************************************************************************
 * Private functions/variables/types
 ****************************************************************************/

#if TRACE_PRINTF_BUFSIZE != 0
char Trace_formattedString[TRACE_PRINTF_BUFSIZE];
#endif

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Initializes the Trace module, including the respective underlying data pipe module */
void Trace_Init(void)
{
#if TRACE_DATAPIPE_I2C0 == 1
    I2CIO_Init();
#endif

#if TRACE_DATAPIPE_CUSTOM == 1
    #if defined(TRACE_DATAPIPE_CUSTOM_INIT)
    extern void TRACE_DATAPIPE_CUSTOM_INIT(void); /* Matches "pTrace_CustomInit_t" */
    TRACE_DATAPIPE_CUSTOM_INIT();
    #endif
#endif
}


/* Writes the desired output to the outside world according to the provided formatted string */
void Trace_Printf(const char *format, ...)
{
#if TRACE_PRINTF_BUFSIZE == 0 /* If we support non-formatted strings only */
    #if TRACE_DATAPIPE_I2C0 == 1
    I2CIO_Tx((const uint8_t*)format, strlen(format));
    #endif

    #if TRACE_DATAPIPE_CUSTOM == 1
        #if defined(TRACE_DATAPIPE_CUSTOM_WRITE)
    extern void TRACE_DATAPIPE_CUSTOM_WRITE(const char * pData, int length);
    TRACE_DATAPIPE_CUSTOM_WRITE(format, strlen(format));
        #else
    /* gracefully do nothing and avoid compiler warnings */
    (void)format;
        #endif
    #endif

    #if TRACE_DATAPIPE_SEMIHOSTING == 1
    puts(format);
    #endif
#else /* If we support formatted strings */
    /* Compile everything out if we're using custom datapipe, but have no Tx direction */
    #if !(TRACE_DATAPIPE_CUSTOM == 1 && !defined(TRACE_DATAPIPE_CUSTOM_WRITE))
    int stringSize;
    va_list parg;
    va_start(parg, format);

        #if TRACE_SPRINTF_STDLIB == 0 /* Here, we use the "Format" module to format our string*/
    stringSize = Format_Vsnprintf(Trace_formattedString, TRACE_PRINTF_BUFSIZE, format, parg);
        #else /* Here, we use the stdlib to format our string*/
    stringSize = vsnprintf(Trace_formattedString, TRACE_PRINTF_BUFSIZE, format, parg);
    if (stringSize >= TRACE_PRINTF_BUFSIZE) {
        /* vsnprintf returns the full size assuming memory was available. */
        stringSize = TRACE_PRINTF_BUFSIZE - 1;
    }
        #endif

    va_end(parg);

        #if TRACE_DATAPIPE_I2C0 == 1
    I2CIO_Tx((const uint8_t*)Trace_formattedString, stringSize);
        #endif

        #if TRACE_DATAPIPE_CUSTOM == 1
    extern void TRACE_DATAPIPE_CUSTOM_WRITE(const char * pData, int length); /* Matches "pTrace_CustomWrite_t" */
    TRACE_DATAPIPE_CUSTOM_WRITE(Trace_formattedString, stringSize);
        #endif

        #if TRACE_DATAPIPE_SEMIHOSTING == 1
    Trace_formattedString[stringSize] = '\0'; /* We do not assume "vsnprintf" NULL terminates the formatted string
                                                 as some implementations do not take care of it */
    puts(Trace_formattedString);
        #endif
    #else
    /* gracefully do nothing and avoid compiler warnings */
    (void)(format);
    #endif
#endif
}


/* Reads characters from the outside world into the provided buffer until one of the 'out' conditions is met */
int Trace_Read(char *buf, int lenout, int timeout, int charout)
{
/* Compile everything out if we're using custom datapipe, but have no Rx direction */
#if !(TRACE_DATAPIPE_CUSTOM == 1 && !defined(TRACE_DATAPIPE_CUSTOM_READ))
    int dataSize = 0;
    int startTime;
    int charIn;
    bool dropOut = false;

    Chip_RTC_Init(NSS_RTC);
    startTime = Chip_RTC_Time_GetValue(NSS_RTC);

    do {
        /* Read chars until:
         *  (1) datapipe is empty (EOF), or
         *  (2) "charout" is found (but not TRACE_CHAROUT_NONE), or
         *  (3) "buf" is full */
        while(1) {
    #if TRACE_DATAPIPE_I2C0 == 1
            charIn = I2CIO_GetChar();
    #endif
    #if TRACE_DATAPIPE_SEMIHOSTING == 1
            charIn = getchar();
    #endif
    #if TRACE_DATAPIPE_CUSTOM == 1
            extern int TRACE_DATAPIPE_CUSTOM_READ(void); /* Matches "pTrace_CustomRead_t" */
            charIn = TRACE_DATAPIPE_CUSTOM_READ();
    #endif
            if(charIn == EOF) {
                break;
            }

            buf[dataSize] = (char)charIn;
            dataSize++;

            if(((charout != TRACE_CHAROUT_NONE) && (charIn == charout)) || (dataSize >= lenout)) {
                dropOut = true; /* for (2) and (3) we don't block and retry, so flag that we want to "dropOut" */
                break;
            }
        }

        /* - "droupOut" immediately in case of TRACE_TIMEOUT_NOWAIT
         * - Assuming RTC time overrun is never going to happen
         * - rtc value test against "timeout + 1" ensures error to be within [timeout,timeout+1] */
        if ((timeout == TRACE_TIMEOUT_NOWAIT) ||
            ((timeout != TRACE_TIMEOUT_INDEFINITE) && (Chip_RTC_Time_GetValue(NSS_RTC) - startTime) > (timeout + 1))) {
            dropOut = true;
        }

    }while(dropOut == false);

    return dataSize;
#else
    /* gracefully do nothing, avoid compiler warnings and return */
    (void)(buf && lenout && timeout && charout);
    return 0;
#endif
}


/* De-initializes the Trace module, including the respective underlying data pipe module */
void Trace_DeInit(void)
{
#if TRACE_DATAPIPE_I2C0 == 1
    I2CIO_DeInit();
#endif

#if TRACE_DATAPIPE_CUSTOM == 1
    #if defined(TRACE_DATAPIPE_CUSTOM_DEINIT)
    extern void TRACE_DATAPIPE_CUSTOM_DEINIT(void); /* Matches "pTrace_CustomDeInit_t" */
    TRACE_DATAPIPE_CUSTOM_DEINIT();
    #endif
#endif
}
