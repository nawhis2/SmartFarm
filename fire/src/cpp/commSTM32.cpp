#include <commSTM32.h>

UARTdevice::UARTdevice() {
    // Initialize UART device
    fd = open(uartDev, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        std::cerr << "Failed to open UART device: " << strerror(errno) << std::endl;
    }

    // Configure UART options
    if (tcgetattr(fd, &options) < 0) {
        std::cerr << "Failed to get UART attributes: " << strerror(errno) << std::endl;
        close(fd);
    }

    if (tcsetattr(fd, TCSANOW, &options) < 0) {
        std::cerr << "Failed to set UART attributes: " << strerror(errno) << std::endl;
        close(fd);
    }

    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
    options.c_cflag |= (CLOCAL | CREAD); // Ignore modem control lines and enable receiver
    options.c_cflag &= ~PARENB; // No parity
    options.c_cflag &= ~CSTOPB; // 1 stop bit
    options.c_cflag &= ~CSIZE; // Clear data size bits
    options.c_cflag |= CS8; // 8 data bits
}

UARTdevice::~UARTdevice() {
    if (fd >= 0)
        close(fd);
}

int UARTdevice::sendCommand(const char* command) {
    if (fd < 0) {
        std::cerr << "Failed to open UART device: " << strerror(errno) << std::endl;
        return -1;
    }

    ssize_t bytesWritten = write(fd, command, strlen(command));
    if (bytesWritten < 0) {
        std::cerr << "Failed to write to UART device: " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}