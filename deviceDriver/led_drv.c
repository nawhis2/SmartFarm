#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#define DEV_NAME    "ledctl"
#define CLASS_NAME  "led"
static int major;
static struct class *led_class;
static struct device *led_device;
static char last_input[16] = {0};  // 디버깅용 버퍼
static int dev_open(struct inode *inode, struct file *file) {
    pr_info("%s: device opened\n", DEV_NAME);
    return 0;
}
static ssize_t dev_write(struct file *file, const char __user *buf,
                         size_t count, loff_t *ppos) {
    size_t len = min(count, sizeof(last_input) - 1);
    if (copy_from_user(last_input, buf, len))
        return -EFAULT;
    last_input[len] = '\0';
    pr_info("%s: received input: '%s'\n", DEV_NAME, last_input);
    return count;
}
static int dev_release(struct inode *inode, struct file *file) {
    pr_info("%s: device released\n", DEV_NAME);
    return 0;
}
static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = dev_open,
    .write   = dev_write,
    .release = dev_release,
};
static int __init led_init(void) {
    major = register_chrdev(0, DEV_NAME, &fops);
    if (major < 0) {
        pr_err("%s: failed to register character device\n", DEV_NAME);
        return major;
    }
    led_class = class_create(CLASS_NAME);
    if (IS_ERR(led_class))
        return PTR_ERR(led_class);
    led_device = device_create(led_class, NULL, MKDEV(major, 0), NULL, DEV_NAME);
    if (IS_ERR(led_device))
        return PTR_ERR(led_device);
    pr_info("%s: module loaded, /dev/%s created\n", DEV_NAME, DEV_NAME);
    return 0;
}
static void __exit led_exit(void) {
    device_destroy(led_class, MKDEV(major, 0));
    class_destroy(led_class);
    unregister_chrdev(major, DEV_NAME);
    pr_info("%s: module unloaded\n", DEV_NAME);
}
module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("수빈");
MODULE_DESCRIPTION("Simple LED device interface for user UART control");