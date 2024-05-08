#ifndef LCD_H_
#define LCD_H_

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

int lcd_init(struct gpio_desc *gpio_descs[], uint8_t *cur_pos);
void lcd_deinit(struct gpio_desc *gpio_descs[], uint8_t *cur_pos);
void lcd_display_clear(struct gpio_desc *gpio_descs[], uint8_t *cur_pos);
void lcd_display_return_home(struct gpio_desc *gpio_descs[], uint8_t *cur_pos);
void lcd_display_shift_left(struct gpio_desc *gpio_descs[]);
void lcd_display_shift_right(struct gpio_desc *gpio_descs[]);
void lcd_display_on(struct gpio_desc *gpio_descs[]);
void lcd_display_off(struct gpio_desc *gpio_descs[]);
void lcd_printf(struct gpio_desc *gpio_descs[], uint8_t *cur_pos, const char *fmt, ...);
int lcd_set_cursor(struct gpio_desc *gpio_descs[], uint8_t *cur_pos, uint8_t new_cur_pos);

#endif /* LCD_H_ */
