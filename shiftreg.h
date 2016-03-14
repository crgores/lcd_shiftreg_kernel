/*
 * shiftreg.h: holds all include statements, preprocessor constants
 *             data structures, global variables, and function prototypes
 *             used throughout this file.
 * written by Chris Gores (crgores@uw.edu)
 */
#ifndef SHIFTREG_H_
#define SHIFTREG_H_

/* pre-processor constants. */
#define DEVICE_NAME  "shiftreg"  // /dev/ file name
#define S_DATA       70          // current data pin
#define SH_CLK       73          // clock each bit
#define ST_CLK       76          // clock entire data out

/* inclusions */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>            // required for usage of file system
#include <linux/cdev.h>          // required for device file structs
#include <linux/semaphore.h>     // required for usage of semaphore
#include <asm/uaccess.h>
#include <linux/gpio.h>          // Required for the GPIO functions
#include <linux/delay.h>         // Required for msleep

/* 
 * general rule:
 * global variables should be declared static,
 * including global variables within the file.
 */
 
/* data structures */
// holds an array with a field for its size
typedef struct {
   unsigned int size;
   unsigned int data[];
} array;
// contains data about the device.
// data : one byte data that is currently being sent to the shif register.
// sem  : semaphore value
struct fake_device {
	char data;
	struct semaphore sem;
} virtual_device;

/* global variables */
// stores info about this char device.
static struct cdev* mcdev;
// holds major and minor number granted by the kernel
static dev_t dev_num;
// holds all pins -- used for init and c
static array allPins = {.data = {S_DATA, SH_CLK, ST_CLK }, .size = 3};

/* function prototypes */
// file operations
static int __init shiftreg_start(void);
static void __exit shiftreg_exit(void);
static int  device_open(struct inode*, struct file*);
static int device_close(struct inode*, struct file *);
static ssize_t device_read(struct file*, char*, size_t, loff_t*);
static ssize_t device_write(struct file*, const char*, size_t, loff_t*);
// kernel-space functions
static int initializeGPIO(void); // initializes all GPIO for use.
static void clearGPIO(void);     // clears and deinitializes all GPIOS.
static void sendByte(char byte); // sends 1 parallely byte out from serial.

/* operations usable by this file. */
static struct file_operations fops = {
	.owner = THIS_MODULE,
   .read = device_read,
   .write = device_write,
	.open = device_open,
	.release = device_close
};
#endif
