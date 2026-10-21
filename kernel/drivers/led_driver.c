#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/gpio/consumer.h>

#define DEV_NAME "my_led"
#define CLASS_NAME "my_led_class"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Engineer");
MODULE_DESCRIPTION("Standalone LED Platform Driver");

struct led_dev_data {
    struct gpio_desc *gpio;
    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *device;
};

static struct led_dev_data led_data;

static ssize_t led_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    char kbuf;

    if (len == 0) return 0;
    if (copy_from_user(&kbuf, buf, 1)) return -EFAULT;

    if (kbuf == '1') {
        gpiod_set_value(led_data.gpio, 1);
        pr_info("LED_DRIVER: LED ON\n");
    } else if (kbuf == '0') {
        gpiod_set_value(led_data.gpio, 0);
        pr_info("LED_DRIVER: LED OFF\n");
    }

    return len;
}

static const struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .write = led_write,
};

static int led_probe(struct platform_device *pdev)
{
    int ret;
    struct device *dev = &pdev->dev;

    pr_info("LED_DRIVER: Probing LED device...\n");

    led_data.gpio = devm_gpiod_get(dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(led_data.gpio)) {
        pr_err("LED_DRIVER: Failed to get GPIO\n");
        return PTR_ERR(led_data.gpio);
    }

    ret = alloc_chrdev_region(&led_data.dev_num, 0, 1, DEV_NAME);
    if (ret) return ret;

    cdev_init(&led_data.cdev, &led_fops);
    ret = cdev_add(&led_data.cdev, led_data.dev_num, 1);
    if (ret) goto unreg_chrdev;

    led_data.class = class_create(CLASS_NAME);
    if (IS_ERR(led_data.class)) {
        ret = PTR_ERR(led_data.class);
        goto del_cdev;
    }

    led_data.device = device_create(led_data.class, NULL, led_data.dev_num, NULL, DEV_NAME);
    if (IS_ERR(led_data.device)) {
        ret = PTR_ERR(led_data.device);
        goto destroy_cls;
    }

    pr_info("LED_DRIVER: LED initialized at /dev/%s\n", DEV_NAME);
    return 0;

destroy_cls:
    class_destroy(led_data.class);
del_cdev:
    cdev_del(&led_data.cdev);
unreg_chrdev:
    unregister_chrdev_region(led_data.dev_num, 1);
    return ret;
}

static int led_remove(struct platform_device *pdev)
{
    gpiod_set_value(led_data.gpio, 0);
    device_destroy(led_data.class, led_data.dev_num);
    class_destroy(led_data.class);
    cdev_del(&led_data.cdev);
    unregister_chrdev_region(led_data.dev_num, 1);
    pr_info("LED_DRIVER: LED Driver removed\n");
    return 0;
}

static const struct of_device_id led_of_match[] = {
    { .compatible = "clean,custom-led" },
    { }
};
MODULE_DEVICE_TABLE(of, led_of_match);

static struct platform_driver led_platform_driver = {
    .probe = led_probe,
    .remove = led_remove,
    .driver = {
        .name = "my_custom_led_driver",
        .of_match_table = led_of_match,
    },
};

module_platform_driver(led_platform_driver);