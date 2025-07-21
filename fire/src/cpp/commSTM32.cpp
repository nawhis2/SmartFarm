#include <commSTM32.h>

UARTdevice::UARTdevice() {
    // Initialize UART device
    fd = open(uartDev, O_RDWR | O_NOCTTY | O_NDELAY | O_SYNC);
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

    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    // 8N1, no flow control
    options.c_cflag = (options.c_cflag & ~CSIZE) | CS8;
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CRTSCTS;
    options.c_cflag |= CREAD | CLOCAL;
    options.c_iflag &= ~(IXON | IXOFF | IXANY); // no software flow ctrl
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // raw mode
    options.c_oflag &= ~OPOST; // raw output
    // 읽기 조건: 최소 1바이트, 타임아웃 0.5초
    options.c_cc[VMIN]  = 1;
    options.c_cc[VTIME] = 5;
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        perror("tcsetattr");
        close(fd);
        return ;
    }

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
