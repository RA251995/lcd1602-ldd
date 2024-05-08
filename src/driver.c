#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include "lcd.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt, __func__
#undef dev_fmt
#define dev_fmt(fmt) "%s: " fmt, __func__

struct lcd_drv_private_data
{
    struct class *class;
    int total_devices;
};

struct lcd_drv_private_data lcd_drv_data = {
    .total_devices = 0,
};

ssize_t text_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    Lcd *lcd = dev_get_drvdata(dev);
    Lcd_printf(lcd, buf);

    return count;
}

ssize_t cursor_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    uint8_t new_cur_pos;
    int ret;
    Lcd *lcd = dev_get_drvdata(dev);

    ret = kstrtou8(buf, 0, &new_cur_pos);
    if (ret != 0)
    {
        return ret;
    }
    ret = Lcd_setCursor(lcd, new_cur_pos);

    return ret ?: count;
}

ssize_t cmd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    Lcd *lcd = dev_get_drvdata(dev);

    if (sysfs_streq(buf, "clear") == true)
    {
        Lcd_clearDisplay(lcd);
    }
    else if (sysfs_streq(buf, "home") == true)
    {
        Lcd_returnDisplayHome(lcd);
    }
    else if (sysfs_streq(buf, "shift_left") == true)
    {
        Lcd_shiftLeftDisplay(lcd);
    }
    else if (sysfs_streq(buf, "shift_right") == true)
    {
        Lcd_shiftRightDisplay(lcd);
    }
    else if (sysfs_streq(buf, "on") == true)
    {
        Lcd_turnOnDisplay(lcd);
    }
    else if (sysfs_streq(buf, "off") == true)
    {
        Lcd_turnOffDisplay(lcd);
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
static DEVICE_ATTR_WO(cursor);
static DEVICE_ATTR_RW(cmd);

static struct attribute *lcd_attrs[] = {
    &dev_attr_text.attr,
    &dev_attr_cursor.attr,
    &dev_attr_cmd.attr,
    NULL,
};

static struct attribute_group lcd_attr_grp = {
    .attrs = lcd_attrs,
};

int lcd1602_probe(struct platform_device *platform_dev)
{
    struct device *parent_dev = &platform_dev->dev;
    struct device *child_dev;
    int ret;

    Lcd *lcd = Lcd_ctor(parent_dev, lcd_drv_data.total_devices);
    ret = Lcd_init(lcd);
    if (ret != 0)
    {
        return ret;
    }

    child_dev = device_create(lcd_drv_data.class, parent_dev, 0, lcd, "%s", Lcd_getLabel(lcd));
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

    /* Store child device in parent device driver data */
    dev_set_drvdata(parent_dev, child_dev);

    lcd_drv_data.total_devices += 1;

    dev_info(parent_dev, "Probed %s\n", Lcd_getLabel(lcd));
    return 0;
}

int lcd1602_remove(struct platform_device *platform_dev)
{
    struct device *parent_dev = &platform_dev->dev;
    struct device *child_dev = dev_get_drvdata(parent_dev);
    Lcd *lcd = dev_get_drvdata(child_dev);

    sysfs_remove_group(&child_dev->kobj, &lcd_attr_grp);
    device_unregister(child_dev);

    Lcd_deinit(lcd);

    dev_info(parent_dev, "Removed %s\n", Lcd_getLabel(lcd));
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
