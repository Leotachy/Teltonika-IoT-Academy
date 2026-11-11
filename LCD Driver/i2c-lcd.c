

#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/array_size.h>
#include <linux/printk.h>
#include <linux/nls.h>

#include "i2c-lcd.h"
#include "util.h"
#include "lt_lcd_letter.h"
static struct i2c_client *i2c_client;

#define sleep_mics(x) (usleep_range(x, x + 5000))

static const u8 *lcd_custom_char_cache[8];
static int next_use_index;

static int lcd_get_cache_slot_index(const u8 *char_matrix)
{
	int idx = -1;

	for (int i = 0; i < ARRAY_SIZE(lcd_custom_char_cache); i++) {
		if (lcd_custom_char_cache[i] == char_matrix) {
			idx = i;
			break;
		}
	}

	return idx;
}

static int lcd_add_to_cache(const u8 *char_matrix)
{
	int idx = next_use_index;

	if (++next_use_index >= ARRAY_SIZE(lcd_custom_char_cache))
		next_use_index = 0;

	lcd_custom_char_cache[idx] = char_matrix;
	return idx;
}

int lcd_send_cmd(char cmd)
{
	char data_u, data_l;
	uint8_t send_bytes[4];

	data_u = (cmd & 0xf0);
	data_l = ((cmd << 4) & 0xf0);
	send_bytes[0] = data_u | 0x0C; //en=1, rs=0
	send_bytes[1] = data_u | 0x08; //en=0, rs=0
	send_bytes[2] = data_l | 0x0C; //en=1, rs=0
	send_bytes[3] = data_l | 0x08; //en=0, rs=0
	int bytes_sent = i2c_master_send(i2c_client, send_bytes, 4);

	if (bytes_sent < 0)
		pr_err(MYLCD_FMT("Failed sending lcd command"));

	return bytes_sent;
}

int lcd_send_data(char data)
{
	char data_u, data_l;
	uint8_t send_bytes[4];

	data_u = (data & 0xf0);
	data_l = ((data << 4) & 0xf0);
	send_bytes[0] = data_u | 0x0D; //en=1, rs=0
	send_bytes[1] = data_u | 0x09; //en=0, rs=0
	send_bytes[2] = data_l | 0x0D; //en=1, rs=0
	send_bytes[3] = data_l | 0x09; //en=0, rs=0
	int bytes_sent = i2c_master_send(i2c_client, send_bytes, 4);

	if (bytes_sent < 0)
		pr_err(MYLCD_FMT("Failed sending lcd data"));

	return bytes_sent;
}

void lcd_clear(void)
{
	lcd_send_cmd(0x01);
	sleep_mics(5000);
}

void lcd_put_cur(int row, int col)
{
	switch (row) {
	case 0:
		col |= 0x80;
		break;
	case 1:
		col |= 0xC0;
		break;
	}

	lcd_send_cmd(col);
}

void lcd_init(struct i2c_client *client)
{
	i2c_client = client;

	// 4 bit initialisation
	sleep_mics(50000);
	lcd_send_cmd(0x30);
	sleep_mics(5000);
	lcd_send_cmd(0x30);
	sleep_mics(200);
	lcd_send_cmd(0x30);
	sleep_mics(10000);
	lcd_send_cmd(0x20); // 4bit mode
	sleep_mics(10000);

	// dislay initialisation
	lcd_send_cmd(
		0x28); // Function set --> DL=0 (4 bit mode), N = 1 (2 line display) F = 0 (5x8 characters)
	sleep_mics(1000);
	lcd_send_cmd(
		0x08); //Display on/off control --> D=0,C=0, B=0  ---> display off
	sleep_mics(1000);
	lcd_send_cmd(0x01); // clear display
	sleep_mics(1000);
	sleep_mics(1000);
	lcd_send_cmd(
		0x06); //Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift)
	sleep_mics(1000);
	lcd_send_cmd(
		0x0C); //Display on/off control --> D = 1, C and B = 0. (Cursor and blink, last two bits)
	sleep_mics(1000);
}

void lcd_deinit(void)
{
	i2c_client = NULL;
	// clear the cache
	memset(lcd_custom_char_cache, 0, sizeof(lcd_custom_char_cache));
}
u32 lcd_send_string(const char *str)
{
	int c = 0;
	int r = 0;
	u32 chars_read = 0;

	lcd_clear();
	lcd_put_cur(0, 0);

	while (*str) {
		char print_char = *str;
		char next_char = str[1];
		u32 jump = 1;

		if (c + 1 == LCD_SCREEN_NUM_COLUMNS) {
			if (print_char != ' ' &&
			    (next_char != ' ' && next_char != 0)) {
				print_char = '-';
				jump = 0;
			}

		} else if (c >= LCD_SCREEN_NUM_COLUMNS) {
			r++;
			c = 0;

			if (r >= LCD_SCREEN_NUM_ROWS)
				break;

			lcd_put_cur(r, c);
		}

		// is unicode character
		if ((print_char == 0xC4 || print_char == 0xC5) && next_char) {
			// convert to wchar
			wchar_t wchar;

			int res = utf8s_to_utf16s(str, 2, UTF16_HOST_ENDIAN,
						  &wchar, 1);
			if (res < 0) {
				pr_err(MYLCD_FMT(
					"Failed converting utf8 to wchar"));
				break;
			}

			if (lcd_send_custom_symbol(wchar) <= 0) {
				pr_err(MYLCD_FMT(
					"Failed sending custom symbol"));
				break;
			}
			jump = 2;
		} else if (print_char == '\n') {
			c--;
			// ignore new line character
		} else {
			if (lcd_send_data(print_char) <= 0) {
				pr_err(MYLCD_FMT("Failed sending data"));
				break;
			}
		}
		c++;
		str = &str[jump];
		chars_read += jump;
	}

	return chars_read;
}

static void lcd_load_custom(const u8 *pixel_matrix, int slot)
{
	lcd_send_cmd(0x40 | (slot * 8));

	for (int i = 0; i < 8; i++)
		lcd_send_data(pixel_matrix[i]);

	lcd_send_cmd(0x80);
}

int lcd_send_custom_symbol(wchar_t wchar)
{
	const u8 *pixel_matrix = get_lithuanian_char_pixel_matrix(wchar);

	if (pixel_matrix == NULL) {
		pr_err(MYLCD_FMT("Unknown wide char 0x%x"), wchar);
		return 0;
	}

	int idx = lcd_get_cache_slot_index(pixel_matrix);

	if (idx == -1) {
		idx = lcd_add_to_cache(pixel_matrix);
		lcd_load_custom(pixel_matrix, idx);
	}

	return lcd_send_data(idx);
}
