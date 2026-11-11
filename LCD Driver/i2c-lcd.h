
#ifndef __I2C_LCD_H
#define __I2C_LCD_H

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/nls.h>

#define LCD_SCREEN_NUM_COLUMNS 16
#define LCD_SCREEN_NUM_ROWS 2
#define LCD_SCREEN_MAX_CHARS (LCD_SCREEN_NUM_COLUMNS * LCD_SCREEN_NUM_ROWS)

void lcd_init(struct i2c_client *client); // initialize lcd
void lcd_deinit(void);
int  lcd_send_cmd (char cmd);  // send command to the lcd
int lcd_send_data(char data); // send data to the lcd
int lcd_send_custom_symbol(wchar_t wchar);
u32 lcd_send_string (const char *str);  // send string to the lcd. Returns characters read
void lcd_put_cur(int row, int col);  // put cursor at the entered position row (0 or 1), col (0-15);
void lcd_clear (void);

#endif