#include "gpio.h"
#include "lcd.h"
#include <linux/delay.h>

/* LCD commands */
#define LCD_CMD_4DL_2N_5X8F (0x28)
#define LCD_CMD_DON_CURON (0x0E)
#define LCD_CMD_INCADD (0x06)
#define LCD_CMD_DIS_CLEAR (0x01)
#define LCD_CMD_DIS_RETURN_HOME (0x02)
#define LCD_CMD_DIS_SHIFT_LEFT (0x18)
#define LCD_CMD_DIS_SHIFT_RIGHT (0x1C)
#define LCD_CMD_DIS_ON_CUR_ON_BLINK_ON (0x0F)
#define LCD_CMD_DIS_OFF_CUR_OFF_BLINK_OFF (0x08)
#define LCD_CMD_CURSOR_SHIFT_RIGHT (0x14)
#define LCD_CMD_CURSOR_SHIFT_LEFT (0x10)

#define LCD_MAX_CHAR_COUNT (80U)

static int init_gpios(struct gpio_desc *gpio_descs[])
{
	int ret;

	for (int i = 0; i < LCD_GPIO_COUNT; i++)
	{
		ret = gpio_config_dir(gpio_descs[i], GPIO_DIR_OUT);
		if (ret != 0)
		{
			return ret;
		}

		ret = gpio_write(gpio_descs[i], GPIO_LOW);
		if (ret != 0)
		{
			return ret;
		}
	}

	return 0;
}

/*
 * @brief call this function to make LCD latch the data lines in to its internal registers.
 */
static void enable(struct gpio_desc *gpio_descs[])
{
	gpio_write(gpio_descs[LCD_GPIO_EN], GPIO_LOW);
	udelay(1);
	gpio_write(gpio_descs[LCD_GPIO_EN], GPIO_HIGH);
	udelay(1);
	gpio_write(gpio_descs[LCD_GPIO_EN], GPIO_LOW);
	udelay(100); /* execution time > 37 micro seconds */
}

/* writes 4 bits of data/command on to D4,D5,D6,D7 lines */
static void write_4_bits(struct gpio_desc *gpio_descs[], uint8_t data)
{
	/* 4 bits parallel data write */
	gpio_write(gpio_descs[LCD_GPIO_D4], (data >> 0) & 0x1);
	gpio_write(gpio_descs[LCD_GPIO_D5], (data >> 1) & 0x1);
	gpio_write(gpio_descs[LCD_GPIO_D6], (data >> 2) & 0x1);
	gpio_write(gpio_descs[LCD_GPIO_D7], (data >> 3) & 0x1);

	enable(gpio_descs);
}

/*
 * This function sends a command to the LCD
 */
static void send_command(struct gpio_desc *gpio_descs[], uint8_t command)
{
	/* RS=0 for LCD command */
	gpio_write(gpio_descs[LCD_GPIO_RS], GPIO_LOW);

	/* R/nW = 0, for write */
	gpio_write(gpio_descs[LCD_GPIO_RW], GPIO_LOW);

	write_4_bits(gpio_descs, command >> 4); /* higher nibble */
	write_4_bits(gpio_descs, command);		/* lower nibble */
}

/*
 * This function sends a character to the LCD
 * Here we used 4 bit parallel data transmission.
 * First higher nibble of the data will be sent on to the data lines D4,D5,D6,D7
 * Then lower niblle of the data will be set on to the data lines D4,D5,D6,D7
 */
static void print_char(struct gpio_desc *gpio_descs[], uint8_t *cur_pos, uint8_t data)
{
	/* RS=1, for user data */
	gpio_write(gpio_descs[LCD_GPIO_RS], HIGH_VALUE);

	/* R/nW = 0, for write */
	gpio_write(gpio_descs[LCD_GPIO_RW], LOW_VALUE);

	write_4_bits(gpio_descs, data >> 4); /* higher nibble */
	write_4_bits(gpio_descs, data);		 /* lower nibble */

	*cur_pos += 1;
}

int lcd_init(struct gpio_desc *gpio_descs[], uint8_t *cur_pos)
{
	int ret;

	ret = init_gpios(gpio_descs);
	if (ret != 0)
	{
		return ret;
	}

	mdelay(40);

	/* RS = 0, for LCD command */
	gpio_write(gpio_descs[LCD_GPIO_RS], GPIO_LOW);

	/* R/nW = 0, for write */
	gpio_write(gpio_descs[LCD_GPIO_RW], GPIO_LOW);

	write_4_bits(gpio_descs, 0x03);
	mdelay(5);

	write_4_bits(gpio_descs, 0x03);
	udelay(100);

	write_4_bits(gpio_descs, 0x03);
	write_4_bits(gpio_descs, 0x02);

	/* 4 bit data mode, 2 lines selection, font size 5x8 */
	send_command(gpio_descs, LCD_CMD_4DL_2N_5X8F);

	/* Display ON, Cursor ON */
	send_command(gpio_descs, LCD_CMD_DON_CURON);

	lcd_display_clear(gpio_descs, cur_pos);

	/* Address auto increment */
	send_command(gpio_descs, LCD_CMD_INCADD);

	mdelay(1000);

	return 0;
}

void lcd_deinit(struct gpio_desc *gpio_descs[], uint8_t *cur_pos)
{
	lcd_display_clear(gpio_descs, cur_pos);
}

void lcd_printf(struct gpio_desc *gpio_descs[], uint8_t *cur_pos, const char *fmt, ...)
{
	static char text_buffer[LCD_MAX_CHAR_COUNT + 1];
	int text_size;
	va_list args;

	va_start(args, fmt);
	text_size = vscnprintf(text_buffer, LCD_MAX_CHAR_COUNT, fmt, args);

	for (int idx = 0; idx < text_size; idx++)
	{
		print_char(gpio_descs, cur_pos, text_buffer[idx]);
		if (idx >= LCD_MAX_CHAR_COUNT)
		{
			*cur_pos = 0;
		}
	}
}

int lcd_set_cursor(struct gpio_desc *gpio_descs[], uint8_t *cur_pos, uint8_t new_cur_pos)
{
	if (new_cur_pos >= LCD_MAX_CHAR_COUNT)
	{
		return -EINVAL;
	}

	while (*cur_pos != new_cur_pos)
	{
		if (*cur_pos < new_cur_pos)
		{
			send_command(gpio_descs, LCD_CMD_CURSOR_SHIFT_RIGHT);
			*cur_pos += 1;
		}
		else
		{
			send_command(gpio_descs, LCD_CMD_CURSOR_SHIFT_LEFT);
			*cur_pos -= 1;
		}
	}

	return 0;
}

/* Clear the display */
void lcd_display_clear(struct gpio_desc *gpio_descs[], uint8_t *cur_pos)
{
	send_command(gpio_descs, LCD_CMD_DIS_CLEAR);
	*cur_pos = 0;
	/*
	 * check page number 24 of datasheet.
	 * display clear command execution wait time is around 2ms
	 */
	mdelay(2);
}

/* Cursor returns to home position */
void lcd_display_return_home(struct gpio_desc *gpio_descs[], uint8_t *cur_pos)
{

	send_command(gpio_descs, LCD_CMD_DIS_RETURN_HOME);
	*cur_pos = 0;
	/*
	 * check page number 24 of datasheet.
	 * return home command execution wait time is around 2ms
	 */
	mdelay(2);
}

void lcd_display_shift_left(struct gpio_desc *gpio_descs[])
{
	send_command(gpio_descs, LCD_CMD_DIS_SHIFT_LEFT);
	udelay(100); /* execution time > 37 micro seconds */
}

void lcd_display_shift_right(struct gpio_desc *gpio_descs[])
{
	send_command(gpio_descs, LCD_CMD_DIS_SHIFT_RIGHT);
	udelay(100); /* execution time > 37 micro seconds */
}

void lcd_display_on(struct gpio_desc *gpio_descs[])
{
	send_command(gpio_descs, LCD_CMD_DIS_ON_CUR_ON_BLINK_ON);
	udelay(100); /* execution time > 37 micro seconds */
}

void lcd_display_off(struct gpio_desc *gpio_descs[])
{
	send_command(gpio_descs, LCD_CMD_DIS_OFF_CUR_OFF_BLINK_OFF);
	udelay(100); /* execution time > 37 micro seconds */
}