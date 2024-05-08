#ifndef LCD_H_
#define LCD_H_

typedef struct Lcd Lcd;

Lcd *Lcd_ctor(struct device *parent_dev, int id);
int Lcd_init(Lcd *const me);
void Lcd_deinit(Lcd *const me);
const char * Lcd_getLabel(Lcd *const me);
void Lcd_clearDisplay(Lcd *const me);
void Lcd_returnDisplayHome(Lcd *const me);
void Lcd_printf(Lcd *const me, const char *fmt, ...);
int Lcd_setCursor(Lcd *const me, uint8_t new_cur_pos);
void Lcd_shiftLeftDisplay(Lcd *const me);
void Lcd_shiftRightDisplay(Lcd *const me);
void Lcd_turnOnDisplay(Lcd *const me);
void Lcd_turnOffDisplay(Lcd *const me);

#endif /* LCD_H_ */
