#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include "lcd.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt, __func__
#undef dev_fmt
#define dev_fmt(fmt) "%s: " fmt, __func__

struct lcd_dev_private_data
{
    char label[20];
    struct gpio_desc *lcd_gpio_descs[LCD_GPIO_COUNT];
    uint8_t cur_pos;
};

struct lcd_drv_private_data
{
    struct class *class;
    int total_devices;
};

struct lcd_drv_private_data lcd_drv_data;

static int get_gpio_descs(struct device *dev, struct gpio_desc *lcd_gpio_descs[])
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

ssize_t text_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct lcd_dev_private_data *dev_data = dev_get_drvdata(dev);
    lcd_printf(dev_data->lcd_gpio_descs, &dev_data->cur_pos, buf);

    return count;
}

ssize_t cmd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct lcd_dev_private_data *dev_data = dev_get_drvdata(dev);

    if (sysfs_streq(buf, "clear") == true)
    {
        lcd_display_clear(dev_data->lcd_gpio_descs, &dev_data->cur_pos);
    }
    else if (sysfs_streq(buf, "home") == true)
    {
        lcd_display_return_home(dev_data->lcd_gpio_descs, &dev_data->cur_pos);
    }
    else  if (sysfs_streq(buf, "shift_left") == true)
    {
        lcd_display_shift_left(dev_data->lcd_gpio_descs);
    }
    else if (sysfs_streq(buf, "shift_right") == true)
    {
        lcd_display_shift_right(dev_data->lcd_gpio_descs);
    }
    else  if (sysfs_streq(buf, "on") == true)
    {
        lcd_display_on(dev_data->lcd_gpio_descs);
    }
    else if (sysfs_streq(buf, "off") == true)
    {
        lcd_display_off(dev_data->lcd_gpio_descs);
    }
    else
    {
        return -EINVAL;
    }

    return count;
}

ssize_t cmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", "clear home shift_left shift_right on off");
}

static DEVICE_ATTR_WO(text);
static DEVICE_ATTR_RW(cmd);

static struct attribute *lcd_attrs[] = {
    &dev_attr_text.attr,
    &dev_attr_cmd.attr,
    NULL,
};

static struct attribute_group lcd_attr_grp = {
    .attrs = lcd_attrs,
};

int lcd1602_probe(struct platform_device *pdev)
{
    struct device *parent_dev = &pdev->dev;
    struct device *child_dev;
    struct lcd_dev_private_data *child_dev_data;
    const char *name;
    int ret;

    lcd_drv_data.total_devices++;

    child_dev_data = devm_kzalloc(parent_dev, sizeof(struct lcd_dev_private_data), GFP_KERNEL);
    if (child_dev_data == NULL)
    {
        return -ENOMEM;
    }

    ret = of_property_read_string(parent_dev->of_node, "label", &name);
    if (ret != 0)
    {
        dev_warn(parent_dev, "Missing label property\n");
        sprintf(child_dev_data->label, "l1602-%d", lcd_drv_data.total_devices);
    }
    else
    {
        strcpy(child_dev_data->label, name);
    }

    ret = get_gpio_descs(parent_dev, child_dev_data->lcd_gpio_descs);
    if (ret != 0)
    {
        return ret;
    }

    ret = lcd_init(child_dev_data->lcd_gpio_descs, &child_dev_data->cur_pos);
    if (ret != 0)
    {
        dev_warn(parent_dev, "LCD initialization failed!\n");
        return ret;
    }
    lcd_printf(child_dev_data->lcd_gpio_descs, &child_dev_data->cur_pos, "!!!LCD by LDD!!!");

    child_dev = device_create(lcd_drv_data.class, parent_dev, 0, child_dev_data, "%s", child_dev_data->label);
    if (IS_ERR(child_dev))
	{
		ret = PTR_ERR(child_dev);
		return ret;
	}

    ret = sysfs_create_group(&child_dev->kobj, &lcd_attr_grp);
	if (ret != 0)
	{
		device_unregister(child_dev);
		return ret;
	}

    /* Store child device in parent */
    dev_set_drvdata(parent_dev, child_dev);

    dev_info(parent_dev, "Probed %s\n", child_dev_data->label);
    return 0;
}

int lcd1602_remove(struct platform_device *pdev)
{
    struct device *parent_dev = &pdev->dev;
    struct device *child_dev = dev_get_drvdata(parent_dev);
    struct lcd_dev_private_data *child_dev_data = dev_get_drvdata(child_dev);

    sysfs_remove_group(&child_dev->kobj, &lcd_attr_grp);
    device_unregister(child_dev);

    lcd_deinit(child_dev_data->lcd_gpio_descs, &child_dev_data->cur_pos);

    dev_info(parent_dev, "Removed %s\n", child_dev_data->label);
    return 0;
}

const struct of_device_id lcd1602_dt_matches[] = {
    {.compatible = "lcd1602"},
    {},
};

struct platform_driver lcd1602_platform_driver = {
    .probe = lcd1602_probe,
    .remove = lcd1602_remove,
    .driver = {
        .name = "lcd1602_driver",
        .of_match_table = of_match_ptr(lcd1602_dt_matches),
    },
};

static int __init lcd1602_driver_init(void)
{
    int ret;

    lcd_drv_data.class = class_create(THIS_MODULE, "lcd_ext");
    if (IS_ERR(lcd_drv_data.class))
    {
        return PTR_ERR(lcd_drv_data.class);
    }

    ret = platform_driver_register(&lcd1602_platform_driver);
    if (ret != 0)
    {
        class_destroy(lcd_drv_data.class);
        return ret;
    }

    pr_info("Driver initialized\n");

    return 0;
}

static void __exit lcd1602_driver_exit(void)
{
    platform_driver_unregister(&lcd1602_platform_driver);
    class_destroy(lcd_drv_data.class);
    pr_info("Driver removed\n");
}

module_init(lcd1602_driver_init);
module_exit(lcd1602_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rijo Abraham");
MODULE_DESCRIPTION("Driver for LCD1602");
