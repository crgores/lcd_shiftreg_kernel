# Makefile for GPIO User Space

CC=arm-linux-gnueabihf-gcc
SOURCES= lcd_sr.c
EXECUTABLE = lcd_sr_test 

lcd_sr_test: lcd_sr_test.c
	$(CC) -std=gnu99 -g -o $@ $^ 

.PHONY: clean

clean:
	rm -f *.o *~ $(EXECUTABLE)

