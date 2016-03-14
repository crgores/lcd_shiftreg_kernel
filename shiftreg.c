/* 
 * shiftreg.c: initalizes the LCD and allows text to be printed on the screen.
 *             access is granted through the drive file.
 * written by Chris Gores (crgores@uw.edu)
 */
/* NOTES:
 * small things to include to improve this file.
 * might be good to grade on:
 * init function will clear the contents of the data array and reset index
 * init function will always clear screen after init as well
 */
#include "shiftreg.h"
// runs on startup
// intializes module space and declares major number.
//assigns device structure data and intializes the LCD.
static int __init shiftreg_start(void) {
   // assign device major dynamically from system.
   int errCode = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
   if (errCode < 0) {
      printk(KERN_ALERT "%s: Failed to allocate a major number\n", DEVICE_NAME);
		return errCode;
   }
	printk(KERN_INFO "%s: major number is %d\n", DEVICE_NAME, MAJOR(dev_num));
	printk(KERN_INFO "type 'mknod /dev/%s c %d 0' to create dev file.\n", DEVICE_NAME, MAJOR(dev_num));
	
	// (2) CREATE CDEV STRUCTURE, INITIALIZING CDEV
	mcdev = cdev_alloc();
	mcdev->ops = &fops;
	mcdev->owner = THIS_MODULE;
	errCode = cdev_add(mcdev, dev_num, 1);
	if (errCode < 0)
	{
		printk(KERN_ALERT "%s: unable to add cdev to kernel\n", DEVICE_NAME);
		return errCode;
	}
	
	// (3) initialization code.
	// initialize GPIO
	errCode = initializeGPIO();
   if (errCode < 0)
	{
		printk(KERN_ALERT "%s: unable to intialize GPIO\n", DEVICE_NAME);
		return errCode;
	}
	// initialize semaphore.
	sema_init(&virtual_device.sem, 1);
	printk( KERN_ALERT"%s: succesfully initialized\n", DEVICE_NAME);
	return 0;
}

// called up exit.
// unregisters the device and all associated gpios with it.
static void __exit shiftreg_exit(void) {
   clearGPIO();
	cdev_del(mcdev);
	unregister_chrdev_region(dev_num, 1);
	printk(KERN_ALERT "%s: successfully unloaded\n", DEVICE_NAME);
}

// Called on device file open
//	inode reference to file on disk, struct file represents an abstract
// checks to see if file is already open (semaphore is in use)
// prints error message if device is busy.
static int device_open(struct inode* inode, struct file* filp) {
	if (down_interruptible(&virtual_device.sem) != 0) {
		printk(KERN_ALERT "%s: could not lock device during open\n", DEVICE_NAME);
		return -1;
	}
	printk(KERN_INFO "%s: device opened\n", DEVICE_NAME);
	return 0;
}

// Called upon close
// closes device and returns access to semaphore.
static int device_close(struct inode* inode, struct  file *filp) {
	up(&virtual_device.sem);
	printk(KERN_INFO "%s: closing device\n", DEVICE_NAME);
	return 0;	
}

// Called when user wants to get info from device file
static ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset) {
	printk(KERN_INFO "%s: Reading from device...\n", DEVICE_NAME);
	*bufStoreData = virtual_device.data;
	//return copy_to_user(bufStoreData, virtual_device.data, bufCount);
   return 0;
}

// Called when user wants to send info to device
// will send a given byte out through the shift register.
// one important note to make is that this byte can be sent with
// either most signifigant bit or or least signifigant bit first.
// in this example I think I do MSB first
static ssize_t device_write(struct file* filp, const char* bufSource, size_t bufCount, loff_t* curOffset) {
	//int errCode = copy_from_user(writeDest, bufSource, bufCount);
	virtual_device.data = *bufSource;
	printk(KERN_INFO "%s: char recieved: %c", DEVICE_NAME, virtual_device.data);
   sendByte(virtual_device.data);
	printk(KERN_INFO "%s: writing to device...\n", DEVICE_NAME);
   return 0;
}

// initializes state of all gpios.
// if a negative value is returned then an error has occured during init.
static int initializeGPIO() {
   int i;
   for (i = 0; i < allPins.size; i++) {
      if (!gpio_is_valid(allPins.data[i])) return -ENODEV;
      gpio_request(allPins.data[i], "sysfs");
      gpio_direction_output(allPins.data[i], false);
      gpio_export(allPins.data[i], false);
   }
   return 0;
}

// clears usage of all gpio.
// unexports and releases the pins used in the allPin array.
static void clearGPIO() {
   int i;
   for (i = 0; i < allPins.size; i++) {
      gpio_set_value(allPins.data[i], false);
      gpio_unexport(allPins.data[i]);
      gpio_free(allPins.data[i]);
   }
}

// converts 1 serially byte of input data to one parallel output byte.
static void sendByte(char byte) {
   int i;
   gpio_set_value(ST_CLK, false);
	for (i = 0; i < 8; i++) {
	   bool bit = ((byte >> i) & 1);
	   //bool bit = !!(byte & (1 << i)); // also works
	   gpio_set_value(S_DATA , bit);
	   gpio_set_value(SH_CLK, true);
	   gpio_set_value(SH_CLK, false);
   }
   gpio_set_value(ST_CLK, true);
}

EXPORT_SYMBOL(sendByte);   // allows other modules to use this function.
MODULE_LICENSE("GPL"); // module license: required to use some functionality.
module_init(shiftreg_start); // declares which function runs on init.
module_exit(shiftreg_exit);  // declares which function runs on exit.
