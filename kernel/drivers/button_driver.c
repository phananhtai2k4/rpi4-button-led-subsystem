#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>

#define DEV_NAME    "my_button"
#define CLASS_NAME  "my_button_class"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Engineer");
MODULE_DESCRIPTION("Standalone Button Platform Driver with Blocking Read");

struct button_dev_data {
    struct gpio_desc *gpio;
    int irq_number;
    dev_t dev_num;
    struct cdev cdev;
    struct class *class;
    struct device *device;

    /* Cơ chế đồng bộ Blocking I/O */
    wait_queue_head_t wq;
    int event_ready;
    char event_type; /* 'P': Pressed, 'R': Released */
};

static struct button_dev_data btn_data;

static irqreturn_t button_isr(int irq, void *dev_id)
{
    int val = gpiod_get_value(btn_data.gpio);

    btn_data.event_type = (val == 1) ? 'P' : 'R';
    btn_data.event_ready = 1;

    wake_up_interruptible(&btn_data.wq);

    return IRQ_HANDLED;
}

static ssize_t button_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    if (len == 0)
        return 0;

    if (wait_event_interruptible(btn_data.wq, btn_data.event_ready != 0)) {
        return -ERESTARTSYS;
    }

    if (copy_to_user(buf, &btn_data.event_type, 1)) {
        return -EFAULT;
    }

    btn_data.event_ready = 0;

    return 1;
}

static const struct file_operations btn_fops = {
    .owner = THIS_MODULE,
    .read  = button_read,
};

static int button_probe(struct platform_device *pdev)
{
    int ret;
    struct device *dev = &pdev->dev;

    pr_info("BTN_DRIVER: Probing Button device...\n");

    btn_data.gpio = devm_gpiod_get(dev, "btn", GPIOD_IN);
    if (IS_ERR(btn_data.gpio)) {
        pr_err("BTN_DRIVER: Failed to get Button GPIO\n");
        return PTR_ERR(btn_data.gpio);
    }

    ret = alloc_chrdev_region(&btn_data.dev_num, 0, 1, DEV_NAME);
    if (ret) return ret;

    cdev_init(&btn_data.cdev, &btn_fops);
    ret = cdev_add(&btn_data.cdev, btn_data.dev_num, 1);
    if (ret) goto unreg_chrdev;

    btn_data.class = class_create(CLASS_NAME);
    if (IS_ERR(btn_data.class)) {
        ret = PTR_ERR(btn_data.class);
        goto del_cdev;
    }

    btn_data.device = device_create(btn_data.class, NULL, btn_data.dev_num, NULL, DEV_NAME);
    if (IS_ERR(btn_data.device)) {
        ret = PTR_ERR(btn_data.device);
        goto destroy_cls;
    }

    init_waitqueue_head(&btn_data.wq);
    btn_data.event_ready = 0;

    btn_data.irq_number = gpiod_to_irq(btn_data.gpio);
    if (btn_data.irq_number < 0) {
        ret = btn_data.irq_number;
        goto destroy_dev;
    }

    ret = devm_request_threaded_irq(dev, btn_data.irq_number, NULL, button_isr,
                                    IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                                    "my_button_irq", &btn_data);
    if (ret) {
        pr_err("BTN_DRIVER: Failed to request IRQ\n");
        goto destroy_dev;
    }

    pr_info("BTN_DRIVER: Initialized successfully at /dev/%s, IRQ: %d\n", DEV_NAME, btn_data.irq_number);
    return 0;

destroy_dev:
    device_destroy(btn_data.class, btn_data.dev_num);
destroy_cls:
    class_destroy(btn_data.class);
del_cdev:
    cdev_del(&btn_data.cdev);
unreg_chrdev:
    unregister_chrdev_region(btn_data.dev_num, 1);
    return ret;
}

static int button_remove(struct platform_device *pdev)
{
    device_destroy(btn_data.class, btn_data.dev_num);
    class_destroy(btn_data.class);
    cdev_del(&btn_data.cdev);
    unregister_chrdev_region(btn_data.dev_num, 1);
    pr_info("BTN_DRIVER: Button Driver removed\n");
    return 0;
}

static const struct of_device_id btn_of_match[] = {
    { .compatible = "clean,custom-button" },
    { }
};
MODULE_DEVICE_TABLE(of, btn_of_match);

static struct platform_driver btn_platform_driver = {
    .probe = button_probe,
    .remove = button_remove,
    .driver = {
        .name = "my_custom_button_driver",
        .of_match_table = btn_of_match,
    },
};

module_platform_driver(btn_platform_driver);