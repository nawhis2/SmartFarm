#ifndef __COMMSTM32_H__
#define __COMMSTM32_H__

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <termios.h>
#include <vector>

class UARTdevice {
    const char* uartDev = "/dev/serial0";
    int fd;
    struct termios options;

public:
    UARTdevice();
    ~UARTdevice();
    UARTdevice(const UARTdevice&) = delete; // Disable copy constructor
    UARTdevice& operator=(const UARTdevice&) = delete; // Disable copy assignment operator

    int receiveResponse(char* buffer, size_t bufferSize);
    int getFd() const;

};

#endif