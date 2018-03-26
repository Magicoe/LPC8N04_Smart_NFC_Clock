/*
 * rtc.c
 *
 *  Created on: 2018-3-13
 *      Author: nxp58695
 */

#include "stdint.h"
#include "board.h"
#include "rtc.h"

// Calculate date and time
// Start from 1970/1/1
// 1970 ~ 2099 is legal date

const uint8_t table_week[12]  = {0,3,3,6,1,4,6,2,5,0,3,5};
const uint8_t table_month[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

uint8_t LEAP_Year_Calculate(uint32_t year)
{
    if( (year%4) == 0 ) { // Can be divided as 4
        if( (year%100) == 0 ) {
            if( (year%400) == 0 ) {
                return 1;
            }
            else {
                return 0;
            }
        }
        else {
            return 1;
        }
    }
    else {
        return 0;
    }
}

void RTC_Convert2Date(RTC_VALUE_T *rtc)
{
    uint32_t temp, temp1;
    uint32_t day_cnt = 0;
	uint32_t ticks;

	ticks = (uint32_t)Chip_RTC_Time_GetValue(NSS_RTC);

	temp = ticks / 86400;

	if(day_cnt != temp) {
		day_cnt = temp;
		temp1   = 1970;
		while(temp >= 365) {
			if(LEAP_Year_Calculate(temp1)) {
				if(temp >= 366) {
					temp = temp - 366;
				}
				else {
					temp1++;
					break;
				}
			}
			else {
				temp = temp - 365;
			}
			temp1++;
		}
		rtc->YEARS = temp1; // Get Year

		temp1 = 0;
		while(temp >= 28) { // Beyond a month
			if( (LEAP_Year_Calculate(rtc->YEARS)) && (temp1 == 1) ) {
				if(temp >= 29) {
					temp = temp - 29;
				}
				else {
					break;
				}
			}
			else {
				if(temp > table_month[temp1]) {
					temp = temp - table_month[temp1];
				}
				else {
					break;
				}
			}
			temp1++;
		}
		rtc->MONTHS = (uint8_t)(temp1 + 1);           // Get Month   Value
		rtc->DAYS   = (uint8_t)(temp  + 1);           // Get Day     Value
	}

	temp = ticks % 86400;                  // Get seconds ticks

	rtc->SECONDS = (uint8_t)((temp%3600)%60);         // Get Seconds Value
	rtc->MINUTES = (uint8_t)((temp%3600)/60%60);      // Get Minutes Value
	rtc->HOURS   = (uint8_t)((temp/3600));            // Get Hours   Value
}


uint32_t RTC_Convert2Tick(RTC_VALUE_T *rtc)
{
	uint32_t i;
	uint32_t ticks = 0;

	if( (rtc->YEARS < 2000) || (rtc->YEARS > 2099) ) {
		return 0xFFFFFFFF;
	}
	for(i=1970; i<(rtc->YEARS); i++) {
		if(LEAP_Year_Calculate(i)) {
			ticks += 31622400;
		}
		else {
			ticks += 31536000;
		}
	}

// Calculate Month
	rtc->MONTHS = (uint8_t)(rtc->MONTHS - 1);
	for(i=0; i<(rtc->MONTHS); i++) {
		ticks += (uint32_t)table_month[i] * 86400;

		if( (LEAP_Year_Calculate(rtc->YEARS)) && (i == 1) ) {
			ticks += 86400;
		}
	}

	ticks += (uint32_t)(rtc->DAYS-1)*86400;
	ticks += (uint32_t)(rtc->HOURS)*3600;
	ticks += (uint32_t)(rtc->MINUTES)*60;
	ticks += rtc->SECONDS;

	return ticks;
}

uint8_t RTC_GetWeek(RTC_VALUE_T *rtc)
{
	uint32_t temp;
	uint8_t  yearH, yearL;

	yearH = (uint8_t)(rtc->YEARS/100);
	yearL = (uint8_t)(rtc->YEARS%100);

	if(yearH > 19) yearL = (uint8_t)(yearL + 100);

	temp = (uint32_t)(yearL + yearL/4);
	temp = temp%7;
	temp = temp + rtc->DAYS + table_week[rtc->MONTHS - 1];

	if( (yearL%4 == 0) && (rtc->MONTHS < 3) ) {
		temp = temp - 1;
	}

	return ((uint8_t)(temp%7));
}

// End file
