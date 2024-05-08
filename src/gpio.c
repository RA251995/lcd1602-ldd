#include "gpio.h"

int gpio_config_dir(struct gpio_desc *gpio_desc, int dir)
{
    if(dir == GPIO_DIR_IN)
    {
        return gpiod_direction_input(gpio_desc);
    }
    else if(dir == GPIO_DIR_OUT)
    {
        return gpiod_direction_output(gpio_desc, 0);
    }
    
    return -EINVAL;
}

int gpio_write(struct gpio_desc *gpio_desc, int value)
{
    if ((value != GPIO_HIGH) && (value != GPIO_LOW))
    {
        return -EINVAL;
    }
    
    gpiod_set_value(gpio_desc, value);
    return 0;
}
