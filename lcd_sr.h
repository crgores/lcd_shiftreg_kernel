/*
 * lcd_sr.h: holds all include statements, preprocessor constants
 *             data structures, global variables, and function prototypes
 *             used throughout the lcd + shift register module.
 * written by Chris Gores (crgores@uw.edu)
 */
#ifndef LCD_SR_H_
#define LCD_SR_H_

/* pre-processor constants. */
#define DEVICE_NAME  "lcd_sr" // /dev/ file name
#define BUF_LEN      16       // length of char buffer.
#define E            79       // enable pin
#define RW           86       // read/write pin
#define RS           62       // 
/* inclusions */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>            // for usage of file system
#include <linux/cdev.h>          // for device file structs
#include <linux/semaphore.h>     // for usage of semaphore
#include <asm/uaccess.h>
#include <linux/gpio.h>          // for the GPIO functions
#include <linux/delay.h>         // for msleep
#include "shiftreg.h"            // for shift register functionality
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
// data : character data stored that is printed on the lcd
// sem  : semaphore value
struct fake_device {
	char data[BUF_LEN + 1];
	int index;
	struct semaphore sem;
} virtual_device;

/* global variables */
// stores info about this char device.
static struct cdev* mcdev;
// holds major and minor number granted by the kernel
static dev_t dev_num;
// holds all pins -- used for init and c
static array allPins = {.size = 3, .data = {RW, RS, E}};

/* function prototypes */
// file operations
static int __init lcd_start(void);
static void __exit lcd_exit(void);
static int  device_open(struct inode*, struct file*);
static int device_close(struct inode*, struct file *);
static ssize_t device_read(struct file*, char*, size_t, loff_t*);
static ssize_t device_write(struct file*, const char*, size_t, loff_t*);
// kernel-space functions
static int initializeGPIO(void);    // initializes all GPIO for use.
static void initializeLCD(void);    // initializes LCD for use.
static void clearGPIO(void);        // clears and deinitializes all gpios.
static void setPinArray(int, int);  // sets values of pins in order of data 
                                    //   sheet to specified binary value.
static void printChar(char);        // prints a single character to the board.
static void appendDevStr(char);     // appends data structure for string.
extern void sendByte(char);         // sends 1 parallely byte out from serial.

/* operations usable by this file. */
static struct file_operations fops = {
	.owner = THIS_MODULE,
   .read = device_read,
   .write = device_write,
	.open = device_open,
	.release = device_close
};
#endif
