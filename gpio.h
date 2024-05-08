#ifndef GPIO_H_
#define GPIO_H_

#include <linux/gpio/consumer.h>

#define HIGH_VALUE      1
#define LOW_VALUE       0

#define GPIO_DIR_IN     LOW_VALUE
#define GPIO_DIR_OUT    HIGH_VALUE

#define GPIO_LOW        LOW_VALUE
#define GPIO_HIGH       HIGH_VALUE

int gpio_config_dir(struct gpio_desc *gpio_desc, int dir);
int gpio_write(struct gpio_desc *gpio_desc, int value);

#endif /* GPIO_H_ */
