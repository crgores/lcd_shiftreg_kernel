## lcd_shiftreg-kernel

This code acts as a simple driver for the Optrex LCD running on the phytec board.  It runs using GPIO pins on the board, but the board can easily be swapped if the gpio pins are changed. Instead of the 11 pins required before, it only requires 5. It utilizies a shift register in order to push the character data in parallel instead of serial. 


In order to run:

type make (makefile runs)

a kernel object file will be created (lcd.ko) as well as a lot of other different files. Upload the .ko file to the board

To run the module on the board, type:

insmod lcd_sr.ko

If initialized correctly the LCD screen will have a blinking cursor!

Very basic testing code is included to test reading and writing with the kernel module file.