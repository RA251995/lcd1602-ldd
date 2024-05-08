#include "gpio.h"
#include "lcd.h"
#include <linux/device.h>
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

typedef enum
{
	LCD_GPIO_RS = 0,
	LCD_GPIO_RW,
	LCD_GPIO_EN,
	LCD_GPIO_D4,
	LCD_GPIO_D5,
	LCD_GPIO_D6,
	LCD_GPIO_D7,

	LCD_GPIO_COUNT
} LCD_GPIOS;

struct Lcd
{
	int id;
	uint8_t cur_pos;
	struct gpio_desc *gpio_descs[LCD_GPIO_COUNT];
	struct device *parent_dev;
	struct mutex lock;
};

static int get_gpios(struct device *dev, struct gpio_desc *lcd_gpio_descs[]);
static int init_gpios(struct gpio_desc *gpio_descs[]);
static void init_lcd(struct gpio_desc *gpio_descs[], uint8_t *cur_pos);
static void enable_lcd(struct gpio_desc *gpio_descs[]);
static void send_command(struct gpio_desc *gpio_descs[], uint8_t command);
static void print_char(struct gpio_desc *gpio_descs[], uint8_t *cur_pos, uint8_t data);
static void write_4_bits(struct gpio_desc *gpio_descs[], uint8_t data);

Lcd *Lcd_ctor(struct device *parent_dev, int id)
{
	Lcd *me = devm_kzalloc(parent_dev, sizeof(Lcd), GFP_KERNEL);
	me->parent_dev = parent_dev;
	me->id = id;
	mutex_init(&me->lock);
	return me;
}

int Lcd_init(Lcd *me)
{
	int ret = 0;

	ret = get_gpios(me->parent_dev, me->gpio_descs);
	if (ret != 0)
	{
		return ret;
	}

	ret = init_gpios(me->gpio_descs);
	if (ret != 0)
	{
		return ret;
	}

	init_lcd(me->gpio_descs, &me->cur_pos);

	return ret;
}

void Lcd_deinit(Lcd *const me)
{
	Lcd_clearDisplay(me);
}

int Lcd_getId(Lcd *const me)
{
	return me->id;
}

void Lcd_clearDisplay(Lcd *const me)
{
	if (mutex_lock_interruptible(&me->lock) != 0)
	{
		return;
	}

	send_command(me->gpio_descs, LCD_CMD_DIS_CLEAR);
	me->cur_pos = 0;
	mdelay(2); /* execution time > 1.52 ms */

	mutex_unlock(&me->lock);
}

void Lcd_returnDisplayHome(Lcd *const me)
{
	if (mutex_lock_interruptible(&me->lock) != 0)
	{
		return;
	}

	send_command(me->gpio_descs, LCD_CMD_DIS_RETURN_HOME);
	me->cur_pos = 0;
	mdelay(2); /* execution time > 1.52 ms */

	mutex_unlock(&me->lock);
}

void Lcd_printf(Lcd *const me, const char *fmt, ...)
{
	static char text_buffer[LCD_MAX_CHAR_COUNT + 1];
	int text_size;
	va_list args;

	va_start(args, fmt);
	text_size = vscnprintf(text_buffer, LCD_MAX_CHAR_COUNT, fmt, args);

	if (mutex_lock_interruptible(&me->lock) != 0)
	{
		return;
	}

	for (int idx = 0; idx < text_size; idx++)
	{
		print_char(me->gpio_descs, &me->cur_pos, text_buffer[idx]);
		if (idx >= LCD_MAX_CHAR_COUNT)
		{
			me->cur_pos = 0;
		}
	}

	mutex_unlock(&me->lock);
}

int Lcd_setCursor(Lcd *const me, uint8_t new_cur_pos)
{
	if (new_cur_pos >= LCD_MAX_CHAR_COUNT)
	{
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&me->lock) != 0)
	{
		return -EINTR;
	}

	while (me->cur_pos != new_cur_pos)
	{
		if (me->cur_pos < new_cur_pos)
		{
			send_command(me->gpio_descs, LCD_CMD_CURSOR_SHIFT_RIGHT);
			me->cur_pos += 1;
		}
		else
		{
			send_command(me->gpio_descs, LCD_CMD_CURSOR_SHIFT_LEFT);
			me->cur_pos -= 1;
		}
	}

	mutex_unlock(&me->lock);

	return 0;
}

void Lcd_shiftLeftDisplay(Lcd *const me)
{
	if (mutex_lock_interruptible(&me->lock) != 0)
	{
		return;
	}

	send_command(me->gpio_descs, LCD_CMD_DIS_SHIFT_LEFT);
	udelay(100); /* execution time > 37 micro seconds */

	mutex_unlock(&me->lock);
}

void Lcd_shiftRightDisplay(Lcd *const me)
{
	if (mutex_lock_interruptible(&me->lock) != 0)
	{
		return;
	}

	send_command(me->gpio_descs, LCD_CMD_DIS_SHIFT_RIGHT);
	udelay(100); /* execution time > 37 us */

	mutex_unlock(&me->lock);
}

void Lcd_turnOnDisplay(Lcd *const me)
{
	if (mutex_lock_interruptible(&me->lock) != 0)
	{
		return;
	}

	send_command(me->gpio_descs, LCD_CMD_DIS_ON_CUR_ON_BLINK_ON);
	udelay(100); /* execution time > 37 us */

	mutex_unlock(&me->lock);
}

void Lcd_turnOffDisplay(Lcd *const me)
{
	if (mutex_lock_interruptible(&me->lock) != 0)
	{
		return;
	}

	send_command(me->gpio_descs, LCD_CMD_DIS_OFF_CUR_OFF_BLINK_OFF);
	udelay(100); /* execution time > 37 us */

	mutex_unlock(&me->lock);
}

static int get_gpios(struct device *dev, struct gpio_desc *lcd_gpio_descs[])
{
	const char *func_names[LCD_GPIO_COUNT] = {"rs", "rw", "en", "d4", "d5", "d6", "d7"};

	for (int idx = 0; idx < LCD_GPIO_COUNT; idx++)
	{
		lcd_gpio_descs[idx] = devm_fwnode_gpiod_get(dev, dev->fwnode, func_names[idx], GPIOD_ASIS, func_names[idx]);
		if (IS_ERR(lcd_gpio_descs[idx]))
		{
			dev_warn(dev, "Missing gpios-%s property\n", func_names[idx]);
			return PTR_ERR(lcd_gpio_descs[idx]);
		}
	}

	return 0;
}

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

	mdelay(40);

	return 0;
}

static void enable_lcd(struct gpio_desc *gpio_descs[])
{
	gpio_write(gpio_descs[LCD_GPIO_EN], GPIO_LOW);
	udelay(1);
	gpio_write(gpio_descs[LCD_GPIO_EN], GPIO_HIGH);
	udelay(1);
	gpio_write(gpio_descs[LCD_GPIO_EN], GPIO_LOW);
	udelay(100); /* execution time > 37 micro seconds */
}

static void write_4_bits(struct gpio_desc *gpio_descs[], uint8_t data)
{
	/* 4 bits parallel data write */
	gpio_write(gpio_descs[LCD_GPIO_D4], (data >> 0) & 0x1);
	gpio_write(gpio_descs[LCD_GPIO_D5], (data >> 1) & 0x1);
	gpio_write(gpio_descs[LCD_GPIO_D6], (data >> 2) & 0x1);
	gpio_write(gpio_descs[LCD_GPIO_D7], (data >> 3) & 0x1);

	enable_lcd(gpio_descs);
}

static void send_command(struct gpio_desc *gpio_descs[], uint8_t command)
{
	/* RS = 0 for LCD command */
	gpio_write(gpio_descs[LCD_GPIO_RS], GPIO_LOW);

	/* R/nW = 0, for write */
	gpio_write(gpio_descs[LCD_GPIO_RW], GPIO_LOW);

	write_4_bits(gpio_descs, command >> 4); /* higher nibble */
	write_4_bits(gpio_descs, command);		/* lower nibble */
}

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

static void init_lcd(struct gpio_desc *gpio_descs[], uint8_t *cur_pos)
{


	/* See 'Initializing by Instruction' section in HD44780U manual */
	mdelay(40);
	
	/* RS = 0, for LCD command, R/nW = 0, for write */
	gpio_write(gpio_descs[LCD_GPIO_RS], GPIO_LOW);
	gpio_write(gpio_descs[LCD_GPIO_RW], GPIO_LOW);
	
	write_4_bits(gpio_descs, 0x03);
	
	mdelay(5);
	
	write_4_bits(gpio_descs, 0x03);
	
	udelay(100);
	
	write_4_bits(gpio_descs, 0x03);
	
	write_4_bits(gpio_descs, 0x02);

	/* 4 bit data mode, 2 lines selection, font size 5x8 */
	send_command(gpio_descs, LCD_CMD_4DL_2N_5X8F);
	udelay(100);

	/* Display ON, Cursor ON */
	send_command(gpio_descs, LCD_CMD_DON_CURON);
	udelay(100);

	send_command(gpio_descs, LCD_CMD_DIS_CLEAR);
	*cur_pos = 0;
	mdelay(2);

	/* Address auto increment */
	send_command(gpio_descs, LCD_CMD_INCADD);
	udelay(100);
}
