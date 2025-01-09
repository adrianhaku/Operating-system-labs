#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "my_char_device"
#define BUFFER_SIZE 1024

static int major_number;
static char message[BUFFER_SIZE] = {0};
static short size_of_message;
static struct class* char_class = NULL;
static struct device* char_device = NULL;

static int dev_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "CharDevice: Device opened\n");
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "CharDevice: Device closed\n");
    return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    int error_count = 0;
    error_count = copy_to_user(buffer, message, size_of_message);

    if (error_count == 0) {
        printk(KERN_INFO "CharDevice: Sent %d characters to the user\n", size_of_message);
        return (size_of_message = 0); // clear the position to the beginning
    } else {
        printk(KERN_INFO "CharDevice: Failed to send %d characters to the user\n", error_count);
        return -EFAULT; // Failed -- return a bad address message
    }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    snprintf(message, BUFFER_SIZE, "%s(%zu letters)", buffer, len); // appending received string
    size_of_message = strlen(message); // store length of the stored message
    printk(KERN_INFO "CharDevice: Received %zu characters from the user\n", len);
    return len;
}

static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int __init char_init(void) {
    printk(KERN_INFO "CharDevice: Initializing the CharDevice\n");

    // Registering the character device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "CharDevice failed to register a major number\n");
        return major_number;
    }
    printk(KERN_INFO "CharDevice: registered correctly with major number %d\n", major_number);

    // Register the device class
    char_class = class_create(THIS_MODULE, "char");
    if (IS_ERR(char_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(char_class);
    }
    printk(KERN_INFO "CharDevice: device class registered correctly\n");

    // Register the device driver
    char_device = device_create(char_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(char_device)) {
        class_destroy(char_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(char_device);
    }
    printk(KERN_INFO "CharDevice: device class created correctly\n");
    return 0;
}

static void __exit char_exit(void) {
    device_destroy(char_class, MKDEV(major_number, 0)); // remove the device
    class_unregister(char_class); // unregister the device class
    class_destroy(char_class); // remove the device class
    unregister_chrdev(major_number, DEVICE_NAME); // unregister the major number
    printk(KERN_INFO "CharDevice: Goodbye from the CharDevice!\n");
}

module_init(char_init);
module_exit(char_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Linux char driver");
MODULE_VERSION("0.1");
