#ifndef __COMMSTM32_H__
#define __COMMSTM32_H__

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <termios.h>

class UARTdevice {
    const char* uartDev = "/dev/serial0";
    int fd;
    struct termios options;

public:
    UARTdevice();
    ~UARTdevice();

    int sendCommand(const char* command);
    int receiveResponse(char* buffer, size_t bufferSize);
    int getFd() const { return fd; }

};

#endif