/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SSD1306 driver
 */

#ifndef __SSD1306_H__
#define __SSD1306_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "ssd1306_fonts.h"
#include "stdbool.h"
#include "stdint.h"
#include "stm32f10x.h"

/**
 * @brief  I2C address.
 */
#define SSD1306_I2C_ADDRESS ((uint8_t)0x3C)

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_X_OVERSCAN_ENABLED 0  // 启用X轴超出边界 换行到下一行
#define SSD1306_Y_OVERSCAN_ENABLED 0  // 启用Y轴过扫描 超出边界 回到顶部

#define OLED_COMPATIBLE_MODE 1  // 兼容OLED.h 库的函数接口

#define UTF8 (0)
#define GB2312 (1)
#define FONT_CODE GB2312  // 字体编码

	typedef enum
	{
		NORMAL = 1,
		INVERSE = 0
	} ssd1306_mode_t;
	/**
	 * @brief   device initialization
	 *
	 * @param   scl_port SCL port
	 * @param   scl_pin SCL pin
	 * @param   sda_port SDA port
	 * @param   sda_pin SDA pin
	 *
	 * @return
	 *     - true Success
	 *     - false Fail
	 */
	bool oled_init(GPIO_TypeDef *scl_port, uint16_t scl_pin, GPIO_TypeDef *sda_port, uint16_t sda_pin);

	/**
	 * @brief   draw point on (x, y)
	 *
	 * @param   chXpos Specifies the X position
	 * @param   chYpos Specifies the Y position
	 * @param   chPoint fill point
	 */
	void oled_fill_point(uint8_t chXpos, uint8_t chYpos, uint8_t chPoint);

	/**
	 * @brief   Draw rectangle on (x1,y1)-(x2,y2)
	 *
	 * @param   chXpos1
	 * @param   chYpos1
	 * @param   chXpos2
	 * @param   chYpos2
	 * @param   chDot fill point
	 */
	void oled_fill_rectangle(uint8_t chXpos1, uint8_t chYpos1, uint8_t chXpos2, uint8_t chYpos2, uint8_t chDot);

	/**
	 * @brief   display char on (x, y),and set size, mode
	 *
	 * @param   chXpos Specifies the X position
	 * @param   chYpos Specifies the Y position
	 * @param   chSize char size
	 * @param   chChr draw char
	 * @param   chMode display mode
	 */
	void oled_draw_char(uint8_t chXpos, uint8_t chYpos, uint8_t chChr, uint8_t chSize, uint8_t chMode);

	/**
	 * @brief   display number on (x, y),and set length, size, mode
	 *
	 * @param   chXpos Specifies the X position
	 * @param   chYpos Specifies the Y position
	 * @param   chNum draw num
	 * @param   chLen length
	 * @param   chSize display size
	 */
	void oled_draw_num(uint8_t chXpos, uint8_t chYpos, uint32_t chNum, uint8_t chLen, uint8_t chSize);

	/**
	 * @brief   display 1616char on (x, y)
	 *
	 * @param   chXpos Specifies the X position
	 * @param   chYpos Specifies the Y position
	 * @param   chChar draw char
	 */
	void oled_draw_1616char(uint8_t chXpos, uint8_t chYpos, uint8_t chChar);

	/**
	 * @brief   display 3216char on (x, y)
	 *
	 * @param   chXpos Specifies the X position
	 * @param   chYpos Specifies the Y position
	 * @param   chChar draw char
	 */
	void oled_draw_3216char(uint8_t chXpos, uint8_t chYpos, uint8_t chChar);
	void oled_draw_3216char_mode(uint8_t chXpos, uint8_t chYpos, uint8_t chChar, uint8_t chMode);

	/**
	 * @brief   draw bitmap on (x, y),and set width, height
	 *
	 * @param   chXpos Specifies the X position
	 * @param   chYpos Specifies the Y position
	 * @param   pchBmp point to BMP data
	 * @param   chWidth picture width
	 * @param   chHeight picture heght
	 */
	void oled_draw_bitmap(uint8_t chXpos, uint8_t chYpos, const uint8_t *pchBmp, uint8_t chWidth, uint8_t chHeight);

	/**
	 * @brief   draw line between two specified points
	 *
	 * @param   chXpos1 Specifies the X position of the starting point of the line
	 * @param   chYpos1 Specifies the Y position of the starting point of the line
	 * @param   chXpos2 Specifies the X position of the ending point of the line
	 * @param   chYpos2 Specifies the Y position of the ending point of the line
	 */
	void oled_draw_line(int16_t chXpos1, int16_t chYpos1, int16_t chXpos2, int16_t chYpos2);

	/**
	 * @brief   refresh dot matrix panel
	 *
	 * @return
	 *     - ESP_OK Success
	 *     - ESP_FAIL Fail
	 **/
	bool oled_refresh_gram(void);

	/**
	 * @brief   Clear screen
	 *
	 * @param   chFill whether fill and fill char
	 **/
	void oled_clear_screen(uint8_t chFill);

	/**
	 * @brief   Displays a string on the screen
	 *
	 * @param   chXpos Specifies the X position
	 * @param   chYpos Specifies the Y position
	 * @param   pchString Pointer to a string to display on the screen
	 * @param   chSize char size
	 * @param   chMode display mode
	 **/
	void oled_draw_string(uint8_t chXpos, uint8_t chYpos, const uint8_t *pchString, uint8_t chSize, uint8_t chMode);

	/**
	 * @brief   display chinese char on (x, y), default size 16x16
	 * @param   chXpos Specifies the X position
	 * @param   chYpos Specifies the Y position
	 * @param   pchChar draw char
	 * @param   mode display mode
	 * @param   char_len character length
	 **/
	void oled_draw_chinese_char(uint8_t chXpos, uint8_t chYpos,
	                            const uint8_t *pchChar, uint8_t mode,
	                            uint8_t char_len);
	/**
	 * @brief   Displays a text on the screen, default size 16x16(chinese)/8x16(english)
	 *
	 * @param   chXpos Specifies the X position
	 * @param   chYpos Specifies the Y position
	 * @param   pchText Pointer to a text to display on the screen
	 * @param   chSize char size
	 * @param   chMode display mode
	 **/
	void oled_draw_text(uint8_t chXpos, uint8_t chYpos, const uint8_t *pchText, uint8_t chMode);

	/** * @brief   Clear a line on the screen
	 * @param   line line number
	 **/
	void oled_clear_line(uint8_t line);

	/** * @brief   Displays a text line on the screen
	 * @param   line line number
	 * @param   xpos Specifies the X position
	 * @param   pchText Pointer to a text to display on the screen
	 * @param   chMode display mode
	 **/
	void oled_draw_text_line(uint8_t line, uint8_t xpos, const uint8_t *pchText, uint8_t chMode);

	/** * @brief   Displays a formatted text line on the screen
	 * @param   line line number
	 * @param   xpos Specifies the X position
	 * @param   chMode display mode
	 * @param   fmt format string
	 **/
	void oled_draw_text_line_fmt(uint8_t line, uint8_t xpos, uint8_t chMode, const char *fmt, ...);

	/**
	 * @brief   Set data for a specific page and column in the display buffer
	 * @param   page page number
	 * @param   column column number
	 * @param   data data byte to set
	 **/
	void oled_set_page_data(uint8_t page, uint8_t column, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif  // __SSD1306_H__
