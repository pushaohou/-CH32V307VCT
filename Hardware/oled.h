/**
 * @file    oled.h
 * @brief   SSD1306 OLED 128×64 I2C 驱动 (I2C1)
 */
#ifndef __OLED_H
#define __OLED_H

#include "guide_cane_config.h"

/* OLED 颜色模式 */
#define OLED_COLOR_NORMAL  0
#define OLED_COLOR_INVERT  1

/* OLED 显示方向 */
#define OLED_DIR_NORMAL    0
#define OLED_DIR_ROTATE    1

/* 字体大小 */
#define OLED_FONT_6x8      8
#define OLED_FONT_8x16     16

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_ColorTurn(uint8_t mode);
void OLED_DisplayTurn(uint8_t mode);
void OLED_Set_Pos(uint8_t x, uint8_t y);
void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t font_size);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t font_size);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t font_size);
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t int_len, uint8_t dec_len, uint8_t font_size);
void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t index, uint8_t font_size);

#endif /* __OLED_H */
