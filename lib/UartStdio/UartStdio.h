#pragma once

#include <Arduino.h>
#include <stdio.h>

class UartStdio {
public:
	static void init(unsigned long baud = 9600);
	static int available();

private:
	static int uart_putchar(char c, FILE* stream);
	static int uart_getchar(FILE* stream);

	static FILE uart_out;
	static FILE uart_in;
};
