#ifndef NETWORK_H
#define NETWORK_H

#include "main.h"

#include <sys/select.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

extern SSL* sock_fd;
#define CTRL_CMD "start"
#define UART_DEV "/dev/ttyAMA0"      // UART 디바이스 경로
#define UART_BAUD B115200          // 통신 속도
#define CMD_MOVE_FORWARD    0xA1
#define CMD_MOVE_BACKWARD   0xA2
#define CMD_TURN_LEFT       0xA3
#define CMD_TURN_RIGHT      0xA4
#define CMD_STOP            0xA5

#define ACK_CODE            0xAA
#define NACK_CODE           0xFF


int socketNetwork(const char*, const int);
int network(int, SSL_CTX *);
bool initUart(const char *dev, int baud);

// UART 수신 (select 기반)
int readUartResponse(uint8_t *resp, size_t maxlen, int timeout_ms);
void sendCompletionToServer(const char *msg);
void handleControlCommand();
void closeUart();
void returnSocket();
#endif