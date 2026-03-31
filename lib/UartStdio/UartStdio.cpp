#include "UartStdio.h"

FILE UartStdio::uart_out;
FILE UartStdio::uart_in;

int UartStdio::uart_putchar(char c, FILE* stream)
{
    (void)stream;
    if (c == '\n') {
        Serial.write('\r');
    }
    Serial.write(c);
    return 0;
}

int UartStdio::uart_getchar(FILE* stream)
{
    (void)stream;
    while (!Serial.available()) {
    }
    return Serial.read();
}

void UartStdio::init(unsigned long baud)
{
    Serial.begin(baud);

    fdev_setup_stream(&uart_out, uart_putchar, NULL, _FDEV_SETUP_WRITE);
    fdev_setup_stream(&uart_in, NULL, uart_getchar, _FDEV_SETUP_READ);
    stdout = &uart_out;
    stdin = &uart_in;
    stderr = &uart_out;
}

int UartStdio::available()
{
    return Serial.available();
}
