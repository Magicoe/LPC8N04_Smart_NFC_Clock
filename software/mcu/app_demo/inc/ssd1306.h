/**
  ******************************************************************************
  * @file    ssd1306.h
  * @author  YANDLD
  * @date    2015.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */

#ifndef __SSD1306__
#define __SSD1306__

#include <stdint.h>


//API functions
/**
 * oled pins and functions.
 * @return NULL.
 */
extern void ssd1306_init(void);
extern void OLED_Fill(uint8_t dat);
extern void OLED_Enable(void);
extern void OLED_Disable(void);
extern void OLED_ShowStr(uint8_t x, uint8_t y, uint8_t *str);
extern void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t BMP[]);
extern void OLED_ShowBat(uint8_t data);
extern void OLED_ShowTime(uint8_t x, uint8_t y, uint8_t *str);
extern void OLED_ShowAlarm(uint8_t data);

extern void oled_lpw_enter(void);
extern void oled_lpw_exit(void);

#endif

