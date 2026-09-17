/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ssd1306.h"
#include "../../Driver/SOFT_I2C/soft_i2c_master.h"
#include "stdarg.h"  // for va_list, va_start, va_end
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"  // for memset

#define SSD1306_WRITE_CMD (0x00)
#define SSD1306_WRITE_DAT (0x40)

#define COORDINATE_SWAP(x1, x2, y1, y2) \
	{                                   \
		int16_t temp = x1;              \
		x1 = x2, x2 = temp;             \
		temp = y1;                      \
		y1 = y2;                        \
		y2 = temp;                      \
	}

typedef struct
{
	soft_i2c_master_bus_t bus;
	uint16_t              dev_addr;
	uint8_t               s_chDisplayBuffer[128][8];  // OLED显存
} ssd1306_dev_t;
static ssd1306_dev_t oled_dev;  // OLED设备句柄

static uint32_t _pow(uint8_t m, uint8_t n)
{
	uint32_t result = 1;
	while (n--)
	{
		result *= m;
	}
	return result;
}

static bool oled_write_data(const uint8_t *const data, const uint16_t data_len)
{
	soft_i2c_err_t ret;
	uint8_t        tx[17];
	uint16_t       offset = 0;
	uint16_t       chunk_len;

	if (oled_dev.bus == 0 || data == NULL || data_len == 0U)
	{
		return false;
	}

	tx[0] = SSD1306_WRITE_DAT;
	while (offset < data_len)
	{
		chunk_len = (uint16_t)(data_len - offset);
		if (chunk_len > 16U)
		{
			chunk_len = 16U;
		}
		memcpy(&tx[1], &data[offset], chunk_len);
		ret = soft_i2c_master_write(oled_dev.bus, (uint8_t)oled_dev.dev_addr, tx, (uint16_t)(chunk_len + 1U));
		if (ret != SOFT_I2C_OK)
		{
			return false;
		}
		offset = (uint16_t)(offset + chunk_len);
	}

	return true;
}

static bool oled_write_cmd(const uint8_t *const data, const uint16_t data_len)
{
	soft_i2c_err_t ret;
	uint8_t        tx[2];

	if (oled_dev.bus == 0 || data == NULL || data_len == 0U)
	{
		return false;
	}

	tx[0] = SSD1306_WRITE_CMD;
	for (uint16_t i = 0; i < data_len; i++)
	{
		tx[1] = data[i];
		ret = soft_i2c_master_write(oled_dev.bus, (uint8_t)oled_dev.dev_addr, tx, 2U);
		if (ret != SOFT_I2C_OK)
		{
			return false;
		}
	}

	return true;
}

static inline bool oled_write_cmd_byte(const uint8_t cmd)
{
	return oled_write_cmd(&cmd, 1);
}

void oled_fill_rectangle(uint8_t chXpos1, uint8_t chYpos1, uint8_t chXpos2, uint8_t chYpos2, uint8_t chDot)
{
	uint8_t chXpos, chYpos;

	for (chXpos = chXpos1; chXpos <= chXpos2; chXpos++)
	{
		for (chYpos = chYpos1; chYpos <= chYpos2; chYpos++)
		{
			oled_fill_point(chXpos, chYpos, chDot);
		}
	}
}

void oled_draw_num(uint8_t chXpos, uint8_t chYpos, uint32_t chNum, uint8_t chLen, uint8_t chSize)
{
	uint8_t i;
	uint8_t chTemp, chShow = 0;

	for (i = 0; i < chLen; i++)
	{
		chTemp = (chNum / _pow(10, chLen - i - 1)) % 10;
		if (chShow == 0 && i < (chLen - 1))
		{
			if (chTemp == 0)
			{
				oled_draw_char(chXpos + (chSize / 2) * i, chYpos, ' ', chSize, 1);
				continue;
			}
			else
			{
				chShow = 1;
			}
		}
		oled_draw_char(chXpos + (chSize / 2) * i, chYpos, chTemp + '0', chSize, 1);
	}
}

void oled_draw_char(uint8_t chXpos, uint8_t chYpos, uint8_t chChr, uint8_t chSize, uint8_t chMode)
{
	uint8_t i, j;
	uint8_t chTemp, chYpos0 = chYpos;

	chChr = chChr - ' ';
	for (i = 0; i < chSize; i++)
	{
		if (chSize == 12)
		{
			if (chMode)
			{
				chTemp = c_chFont1206[chChr][i];
			}
			else
			{
				chTemp = ~c_chFont1206[chChr][i];
			}
		}
		else
		{
			if (chMode)
			{
				chTemp = c_chFont1608[chChr][i];
			}
			else
			{
				chTemp = ~c_chFont1608[chChr][i];
			}
		}

		for (j = 0; j < 8; j++)
		{
			if (chTemp & 0x80)
			{
				oled_fill_point(chXpos, chYpos, 1);
			}
			else
			{
				oled_fill_point(chXpos, chYpos, 0);
			}
			chTemp <<= 1;
			chYpos++;

			if ((chYpos - chYpos0) == chSize)
			{
				chYpos = chYpos0;
				chXpos++;
				break;
			}
		}
	}
}

void oled_draw_string(uint8_t chXpos, uint8_t chYpos, const uint8_t *pchString, uint8_t chSize, uint8_t chMode)
{
	while (*pchString != '\0')
	{
		if (chXpos > (SSD1306_WIDTH - chSize / 2))
		{
			chXpos = 0;
			chYpos += chSize;
#if SSD1306_Y_OVERSCAN_ENABLED
			if (chYpos > (SSD1306_HEIGHT - chSize))
			{
				chYpos = chXpos = 0;
				oled_clear_screen(0x00);
			}
#endif
		}
		oled_draw_char(chXpos, chYpos, *pchString, chSize, chMode);
		chXpos += chSize / 2;
		pchString++;
	}
}

void oled_draw_utf8_char(uint8_t chXpos, uint8_t chYpos, const uint8_t *pchChar, uint8_t mode)
{
	oled_draw_chinese_char(chXpos, chYpos, pchChar, mode, 3);
}

void oled_draw_gb2312_char(uint8_t chXpos, uint8_t chYpos, const uint8_t *pchChar, uint8_t mode)
{
	oled_draw_chinese_char(chXpos, chYpos, pchChar, mode, 2);
}

/**
 * @brief 在OLED上绘制中文字符
 * 字符点阵：16x16
 *  取模方式：逐行式
 *  取模方向：顺向
 */

/**
 * @brief 绘制16x16中文字符，转换为32位
 */
void oled_draw_chinese_char(uint8_t chXpos, uint8_t chYpos,
                            const uint8_t *pchChar, uint8_t mode, uint8_t char_len)
{
	uint16_t    i, j;
	uint8_t     chTemp, chYpos0 = chYpos;
	ChineseCode chCharData;

	for (i = 0; i < c_chChineseFont16x16_count; i++)
	{
		if (memcmp((const char *)c_chChineseFont16x16[i].text,
		           (const char *)pchChar, char_len) == 0)
		{
			chCharData = c_chChineseFont16x16[i];
			break;
		}
	}
	if (i == c_chChineseFont16x16_count)
	{

		return;  // 未找到中文字符
	}

	// 绘制16x16中文字符，转换为32位
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			chTemp = chCharData.code[i * 2 + j];
			chTemp = mode ? chTemp : ~chTemp;

			for (int k = 0; k < 8; k++)
			{
				if (chTemp & 0x80)
				{
					oled_fill_point(chXpos + j * 8 + k, chYpos + i, 1);
				}
				else
				{
					oled_fill_point(chXpos + j * 8 + k, chYpos + i, 0);
				}
				chTemp <<= 1;
			}
		}
	}
}

void oled_draw_text(uint8_t chXpos, uint8_t chYpos, const uint8_t *pchText,
                    uint8_t chMode)
{
	uint8_t size = 16;
	while (*pchText != '\0')
	{
		if (*pchText & 0x80)
		{
#if FONT_CODE == UTF8
			oled_draw_utf8_char(chXpos, chYpos, pchText, chMode);
			pchText += 3;  // 跳过3个字节 UTF-8编码
#else
			oled_draw_gb2312_char(chXpos, chYpos, pchText, chMode);
			pchText += 2;  // 跳过2个字节 GB2312编码
#endif
			chXpos += size;  // 移动到下一个字符位置
		}
		else
		{
			// 绘制ASCII字符
			oled_draw_char(chXpos, chYpos, *pchText, size, chMode);
			pchText += 1;
			chXpos += size / 2;
		}
	}
}

#if OLED_COMPATIBLE_MODE
// 修复1：修正 ssd1306_fill_point 的坐标映射
void oled_fill_point(uint8_t chXpos, uint8_t chYpos, uint8_t chPoint)
{
	uint8_t chPos, chBx, chTemp = 0;

	if (chXpos > 127 || chYpos > 63)
	{
		return;
	}

	// 页地址模式：Y坐标除以8得到页地址（0-7）
	chPos = chYpos / 8;  // ? 修正：不需要反转
	chBx = chYpos % 8;   // 页内偏移（0-7）
	chTemp = 1 << chBx;  // ? 修正：位0在下，位7在上

	if (chPoint)
	{
		oled_dev.s_chDisplayBuffer[chXpos][chPos] |= chTemp;
	}
	else
	{
		oled_dev.s_chDisplayBuffer[chXpos][chPos] &= ~chTemp;
	}
}

#else
void oled_fill_point(uint8_t chXpos, uint8_t chYpos, uint8_t chPoint)
{
	uint8_t chPos, chBx, chTemp = 0;

	if (chXpos > 127 || chYpos > 63)
	{
		return;
	}
	chPos = 7 - chYpos / 8;
	chBx = chYpos % 8;
	chTemp = 1 << (7 - chBx);

	if (chPoint)
	{
		oled_dev.s_chDisplayBuffer[chXpos][chPos] |= chTemp;
	}
	else
	{
		oled_dev.s_chDisplayBuffer[chXpos][chPos] &= ~chTemp;
	}
}
#endif

void oled_draw_1616char(uint8_t chXpos, uint8_t chYpos, uint8_t chChar)
{
	uint8_t i, j;
	uint8_t chTemp = 0, chYpos0 = chYpos, chMode = 0;

	for (i = 0; i < 32; i++)
	{
		chTemp = c_chFont1612[chChar - 0x30][i];
		for (j = 0; j < 8; j++)
		{
			chMode = chTemp & 0x80 ? 1 : 0;
			oled_fill_point(chXpos, chYpos, chMode);
			chTemp <<= 1;
			chYpos++;
			if ((chYpos - chYpos0) == 16)
			{
				chYpos = chYpos0;
				chXpos++;
				break;
			}
		}
	}
}

void oled_draw_3216char_mode(uint8_t chXpos, uint8_t chYpos, uint8_t chChar, uint8_t chMode)
{
	uint8_t i, j;
	uint8_t chTemp = 0, chYpos0 = chYpos, chPoint = 0;

	for (i = 0; i < 64; i++)
	{
		chTemp = c_chFont3216[chChar - 0x30][i];
		if (chMode == INVERSE)
		{
			chTemp = (uint8_t)~chTemp;
		}
		for (j = 0; j < 8; j++)
		{
			chPoint = chTemp & 0x80 ? 1 : 0;
			oled_fill_point(chXpos, chYpos, chPoint);
			chTemp <<= 1;
			chYpos++;
			if ((chYpos - chYpos0) == 32)
			{
				chYpos = chYpos0;
				chXpos++;
				break;
			}
		}
	}
}

void oled_draw_3216char(uint8_t chXpos, uint8_t chYpos, uint8_t chChar)
{
	oled_draw_3216char_mode(chXpos, chYpos, chChar, NORMAL);
}

void oled_draw_bitmap(uint8_t chXpos, uint8_t chYpos, const uint8_t *pchBmp, uint8_t chWidth, uint8_t chHeight)
{
	uint16_t i, j, byteWidth = (chWidth + 7) / 8;

	for (j = 0; j < chHeight; j++)
	{
		for (i = 0; i < chWidth; i++)
		{
			if (*(pchBmp + j * byteWidth + i / 8) & (128 >> (i & 7)))
			{
				oled_fill_point(chXpos + i, chYpos + j, 1);
			}
		}
	}
}

void oled_draw_line(int16_t chXpos1, int16_t chYpos1, int16_t chXpos2, int16_t chYpos2)
{
	// 16-bit variables allowing a display overflow effect
	int16_t x_len = abs(chXpos1 - chXpos2);
	int16_t y_len = abs(chYpos1 - chYpos2);

	if (y_len < x_len)
	{
		if (chXpos1 > chXpos2)
		{
			COORDINATE_SWAP(chXpos1, chXpos2, chYpos1, chYpos2);
		}
		int16_t len = x_len;
		int16_t diff = y_len;

		do
		{
			if (diff >= x_len)
			{
				diff -= x_len;
				if (chYpos1 < chYpos2)
				{
					chYpos1++;
				}
				else
				{
					chYpos1--;
				}
			}

			diff += y_len;
			oled_fill_point(chXpos1++, chYpos1, 1);
		} while (len--);
	}

	else
	{
		if (chYpos1 > chYpos2)
		{
			COORDINATE_SWAP(chXpos1, chXpos2, chYpos1, chYpos2);
		}
		int16_t len = y_len;
		int16_t diff = x_len;

		do
		{
			if (diff >= y_len)
			{
				diff -= y_len;
				if (chXpos1 < chXpos2)
				{
					chXpos1++;
				}
				else
				{
					chXpos1--;
				}
			}

			diff += x_len;
			oled_fill_point(chXpos1, chYpos1++, 1);
		} while (len--);
	}
}

bool oled_init(GPIO_TypeDef *scl_port, uint16_t scl_pin, GPIO_TypeDef *sda_port, uint16_t sda_pin)
{
	bool                     ret;
	soft_i2c_master_config_t i2c_cfg;

	oled_dev.dev_addr = SSD1306_I2C_ADDRESS;
	if (oled_dev.bus == 0)
	{
		i2c_cfg.scl_port = scl_port;
		i2c_cfg.scl_pin = scl_pin;
		i2c_cfg.sda_port = sda_port;
		i2c_cfg.sda_pin = sda_pin;
		i2c_cfg.freq = SOFT_I2C_200KHZ;
		if (soft_i2c_master_new(&i2c_cfg, &oled_dev.bus) != SOFT_I2C_OK)
		{
			return false;
		}
	}
#if OLED_COMPATIBLE_MODE
	oled_write_cmd_byte(0xAE);  // display off

	// 【关键】改为页地址模式，与旧代码兼容
	oled_write_cmd_byte(0x20);  // Set Memory Addressing Mode
	oled_write_cmd_byte(0x10);  // Page Addressing Mode (0x10)

	oled_write_cmd_byte(0xb0);  // Set Page Start Address

	// 【关键】改为与旧代码相同的扫描方向
	oled_write_cmd_byte(0xc8);  // COM从下往上扫描（如果要倒置）
	// 或保持 0xC0（正常方向）

	oled_write_cmd_byte(0x00);  // 列低地址
	oled_write_cmd_byte(0x10);  // 列高地址
	oled_write_cmd_byte(0x40);  // 起始行地址

	oled_write_cmd_byte(0x81);  // 对比度
	oled_write_cmd_byte(0xFF);  // 最大亮度（与旧代码一致）

	oled_write_cmd_byte(0xa1);  // Segment重映射
	oled_write_cmd_byte(0xa6);  // 正常显示
	oled_write_cmd_byte(0xa8);  // 复用率
	oled_write_cmd_byte(0x3F);  // 1/64 duty

	oled_write_cmd_byte(0xa4);  // 跟随RAM内容
	oled_write_cmd_byte(0xd3);  // 显示偏移
	oled_write_cmd_byte(0x00);  // 无偏移

	oled_write_cmd_byte(0xd5);  // 时钟分频
	oled_write_cmd_byte(0xf0);  // 与旧代码一致

	oled_write_cmd_byte(0xd9);  // 预充电
	oled_write_cmd_byte(0x22);  // 与旧代码一致

	oled_write_cmd_byte(0xda);  // COM配置
	oled_write_cmd_byte(0x12);

	oled_write_cmd_byte(0xdb);  // VCOMH
	oled_write_cmd_byte(0x20);  // 与旧代码一致

	oled_write_cmd_byte(0x8d);  // 电荷泵
	oled_write_cmd_byte(0x14);  // 使能
#else
	oled_write_cmd_byte(0xAE);          //--turn off oled panel
	oled_write_cmd_byte(0x40);          //--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	oled_write_cmd_byte(0x81);          //--set contrast control register
	oled_write_cmd_byte(0xCF);          // Set SEG Output Current Brightness
	oled_write_cmd_byte(0xA1);          //--Set SEG/Column Mapping
	oled_write_cmd_byte(0xC0);          // Set COM/Row Scan Direction
	oled_write_cmd_byte(0xA6);          //--set normal display
	oled_write_cmd_byte(0xA8);          //--set multiplex ratio(1 to 64)
	oled_write_cmd_byte(0x3f);          //--1/64 duty
	oled_write_cmd_byte(0xd5);          //--set display clock divide ratio/oscillator frequency
	oled_write_cmd_byte(0x80);          //--set divide ratio, Set Clock as 100 Frames/Sec
	oled_write_cmd_byte(0xD9);          //--set pre-charge period
	oled_write_cmd_byte(0xF1);          // Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	oled_write_cmd_byte(0xDA);          //--set com pins hardware configuration
	oled_write_cmd_byte(0xDB);          //--set vcomh
	oled_write_cmd_byte(0x40);          // Set VCOM Deselect Level
	ssd1306_write_cmd_byte(dev, 0x8D);  //--set Charge Pump enable/disable
	oled_write_cmd_byte(0x14);          //--set(0x10) disable
	oled_write_cmd_byte(0xA4);          // Disable Entire Display On (0xa4/0xa5)
	oled_write_cmd_byte(0xA6);          // Disable Inverse Display On (0xa6/a7)

	const uint8_t cmd[2] = {0x20, 1};  //-- set vertical adressing mode
	oled_write_cmd(cmd, sizeof(cmd));

	uint8_t cmd2[3] = {0x21, 0, 127};
	oled_write_cmd(cmd2, sizeof(cmd2));  //--set column address to zero
	cmd2[0] = 0x22;
	cmd2[2] = 7;
	ssd1306_write_cmd(dev, cmd2, sizeof(cmd2));  //--set row address to zero
#endif

	ret = oled_write_cmd_byte(0xAF);  //--turn on oled panel

	oled_clear_screen(0x00);
	return ret;
}

#if OLED_COMPATIBLE_MODE
// 修复3：修正 ssd1306_refresh_gram 以适配页地址模式
bool oled_refresh_gram(void)
{
	uint8_t page_buf[128];

	// 页地址模式：需要逐页发送
	for (uint8_t page = 0; page < 8; page++)
	{
		// 设置页地址
		oled_write_cmd_byte(0xB0 + page);  // 设置页地址（0xB0-0xB7）
		oled_write_cmd_byte(0x00);         // 列低地址
		oled_write_cmd_byte(0x10);         // 列高地址

		// 发送该页的128列数据
		for (uint8_t col = 0; col < 128; col++)
		{
			page_buf[col] = oled_dev.s_chDisplayBuffer[col][page];
		}
		if (!oled_write_data(page_buf, 128U))
		{
			return false;
		}
	}

	return true;
}
#else
bool oled_refresh_gram(void)
{
	return ssd1306_write_data(&oled_dev.s_chDisplayBuffer[0][0], sizeof(oled_dev.s_chDisplayBuffer));
}
#endif

void oled_clear_screen(uint8_t chFill)
{
	memset(oled_dev.s_chDisplayBuffer, chFill, sizeof(oled_dev.s_chDisplayBuffer));
}

void oled_clear_line(uint8_t line)
{
	// 计算Y坐标：每行高度为16像素
	uint8_t y = line * 16;

	// 确保Y坐标不超出屏幕范围
	if (y >= SSD1306_HEIGHT)
	{
		return;
	}

	// 清除该行内容
	oled_fill_rectangle(0, y, SSD1306_WIDTH - 1, y + 15, 0);
}

void oled_draw_text_line(uint8_t line, uint8_t xpos, const uint8_t *pchText, uint8_t chMode)
{
	oled_draw_text(xpos, line * 16, pchText, chMode);
}

void oled_draw_text_line_fmt(uint8_t line, uint8_t xpos, uint8_t chMode,
                             const char *fmt, ...)
{
	char    buffer[128];  // 假设最大长度为128
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	oled_draw_text(xpos, line * 16, (uint8_t *)buffer, chMode);
}

#if OLED_COMPATIBLE_MODE
// 修复2：修正 ssd1306_set_page_data 的索引顺序
void oled_set_page_data(uint8_t page, uint8_t column, uint8_t data)
{
	if (page < 8 && column < 128)
	{
		// ? 修正：显存是[列][页]结构
		oled_dev.s_chDisplayBuffer[column][page] = data;
	}
}
#else
void oled_set_page_data(uint8_t page, uint8_t column, uint8_t data)
{
	if (page < 8 && column < 128)
	{
		oled_dev.s_chDisplayBuffer[page][column] = data;
	}
}
#endif