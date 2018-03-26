/*
 * rtc.h
 *
 *  Created on: 2018-3-13
 *      Author: nxp58695
 */

#ifndef RTC_H_
#define RTC_H_

/** RTC block register block structure */
typedef struct RTC_VALUE_S {
    __IO uint8_t    SECONDS;         /*!< second */
    __IO uint8_t    MINUTES;         /*!< minute */
    __IO uint8_t    HOURS;           /*!< hour */
    __IO uint8_t    DAYS;            /*!< day */
    __IO uint8_t    MONTHS;          /*!< month */
    __IO uint8_t    WEEKS;			 /*!< week */
    __IO uint32_t   YEARS;	         /*!< year */
} RTC_VALUE_T;

/**
 * Convert tick values to Date format.
 * @return NULL.
 */
void RTC_Convert2Date(RTC_VALUE_T *rtc);

/**
 * Convert Date format to tick values.
 * @return ticks value.
 */
uint32_t RTC_Convert2Tick(RTC_VALUE_T *rtc);

#endif /* RTC_H_ */
