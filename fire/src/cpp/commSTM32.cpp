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


int UARTdevice::receiveResponse(char* buffer, size_t bufferSize) {
    if (fd < 0) {
        std::cerr << "UART device not initialized." << std::endl;
        return -1;
    }

    ssize_t bytesRead = read(fd, buffer, bufferSize - 1);
    if (bytesRead < 0) {
        std::cerr << "Failed to read from UART device: " << strerror(errno) << std::endl;
        return -1;
    }

    buffer[bytesRead] = '\0'; // Null-terminate the string
    return bytesRead;
}

int UARTdevice::getFd() const {
    return fd;
}

std::vector<std::string> UARTdevice::parseSensorData(std::string uartData) {
    std::vector<std::string> sensorData;
    size_t pos = 0;
    while ((pos = uartData.find(',')) != std::string::npos) {
        sensorData.push_back(uartData.substr(0, pos));
        uartData.erase(0, pos + 1);
    }
    if (!uartData.empty()) {
        sensorData.push_back(uartData); // Add the last part
    }
    return sensorData;
}