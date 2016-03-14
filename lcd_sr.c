/* 
 * lcd_sr.c: initalizes the LCD and allows text to be printed on the screen.
 *             access is granted through the drive file. This version of lcd
 *             code uses module stacking from the shift register for its
 *             data in rather than 8 individual gpio ports.
 */

#include "lcd_sr.h"

// runs on startup
// intializes module space and declares major number.
//assigns device structure data and intializes the LCD.
static int __init lcd_start(void)
{
   // assign device major dynamically from system.
   int errCode = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
   if (errCode < 0) {
      printk(KERN_ALERT "%s: Failed to allocate a major number\n", DEVICE_NAME);
		return errCode;
   }
	printk(KERN_INFO "%s: major number is %d\n", DEVICE_NAME, MAJOR(dev_num));
	//printk(KERN_INFO "Use mknod /dev/%s c %d 0 for device file\n", DEVICE_NAME, major_number);
	printk(KERN_INFO "%s: mknod /dev/%s c %d 0\n", DEVICE_NAME, DEVICE_NAME, MAJOR(dev_num));
	
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
	// initialize semaphore and index
	sema_init(&virtual_device.sem, 1);
	virtual_device.index = 0;
	
   // initialize GPIO
	errCode = initializeGPIO();
   if (errCode < 0)
	{
		printk(KERN_ALERT "%s: unable to intialize GPIO\n", DEVICE_NAME);
		return errCode;
	}
	// initialize LCD
	initializeLCD();
	printk(KERN_INFO "%s: succesfully initialized\n", DEVICE_NAME);
	return 0;
}

// called up exit.
// unregisters the device and all associated gpios with it.
static void __exit lcd_exit(void)
{
   setPinArray(0b0000000001, 10);
   clearGPIO();
	cdev_del(mcdev);
	unregister_chrdev_region(dev_num, 1);
	printk(KERN_ALERT "%s: successfully unloaded\n", DEVICE_NAME);
}

// Called on device file open
//	inode reference to file on disk, struct file represents an abstract
// checks to see if file is already open (semaphore is in use)
// prints error message if device is busy.
static int device_open(struct inode *inode, struct file* filp) {
	if (down_interruptible(&virtual_device.sem) != 0) {
		printk(KERN_INFO "%s: could not lock device during open\n", DEVICE_NAME);
		return -1;
	}
	
	printk(KERN_INFO "%s: device opened\n", DEVICE_NAME);
	return 0;
}

// Called upon close
// closes device and returns access to semaphore.
static int device_close(struct inode* inode, struct  file *filp)
{
	up(&virtual_device.sem);
	printk(KERN_INFO "%s: closing device\n", DEVICE_NAME);
	return 0;	
}

// Called when user wants to get info from device file
static ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset) {
	printk(KERN_INFO "%s: Reading from device...\n", DEVICE_NAME);
	return copy_to_user(bufStoreData, virtual_device.data, bufCount);
}

// Called when user wants to send info to device
// will store text into string buffer and print the string to the LCD.
// LCD can only print 24 characters at a time
static ssize_t device_write(struct file* filp, const char* bufSource, size_t bufCount, loff_t* curOffset) {
   //if (BUF_LEN - virtual_device.index < bufCount) return -1;
	//int errCode = copy_from_user(writeDest, bufSource, bufCount);
	//virtual_device.index += bufCount;
	printk(KERN_INFO "%s: writing '%s' to device\n", DEVICE_NAME, bufSource);
	if (*bufSource == '*') { // clear display
	   setPinArray(0b0000000001, 10);
      virtual_device.data[0] = '\0';
      virtual_device.index = 0;
   }
   else {
      // prints every character of bufSource to the storage array
      // and to the LCD. Will stop adding letters once max size is reached.
      char* writeString = (char *) bufSource;
      while (*writeString && virtual_device.index < BUF_LEN) {
         printk(KERN_INFO "letter printed: %c", *writeString);
         appendDevStr(*writeString);
         printChar(*writeString);
         writeString++;
      }
   }
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

// sends initialization commands to LCD.
// causes blinking cursor to appear once finished,
static void initializeLCD() {
   msleep(17);
   setPinArray(0b0000110000, 5);
   setPinArray(0b0000110000, 2);
   setPinArray(0b0000110000, 1);
   setPinArray(0b0000111000, 1);
   setPinArray(0b0000001000, 1);
   setPinArray(0b0000000001, 1);
   setPinArray(0b0000000110, 1);
   setPinArray(0b0000001111, 1);
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

// sets the individual bits of value to the contents of DB7 - DB0 and RW / RS.
// 
// value: 10 bit binary number
//
// bits:  10   9    8    7    6    5    4    3    2    1
// pins:  RS, RW, DB7, DB6, DB5, DB4, DB3, DB2, DB1, DB0
static void setPinArray(int value, int delay) {
   // send bottom 8 bits to data pins of LCD.
   sendByte(value % 0xFF);
   gpio_set_value(RW, (value >> 8) & 1);
   gpio_set_value(RS, (value >> 9) & 1);
   //for (i = 0; i < allPins.size - 1; i++)
   //   gpio_set_value(allPins.data[i], (value >> i) & 1);
   gpio_set_value(E, true);
   msleep(10);
   gpio_set_value(E, false);
   msleep(delay);
}

// prints a single character to the LCD
static void printChar(char letter) {
   gpio_set_value(RS, 1);
   gpio_set_value(RW, 0);
   msleep(1);
   gpio_set_value(E, 1);
   msleep(1);
   // set each pin to individual bits of letter
   //for (i = 0; i < 8; i++) gpio_set_value(allPins.data[i], letter >> i & 1);
   sendByte(letter);
   gpio_set_value(E, 0);
   msleep(1);
   gpio_set_value(RS, 0);
   gpio_set_value(RW, 1);
}

// appends a single letter to the virtual device string
static void appendDevStr(char letter) {
   // add letter
   virtual_device.data[virtual_device.index++] = letter;
   // set next value to be end of string.
   virtual_device.data[virtual_device.index] = '\0';
}

// module license: required to use some functionality.
MODULE_LICENSE("GPL");
module_init(lcd_start);
module_exit(lcd_exit);

