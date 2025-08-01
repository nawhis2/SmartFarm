#include "clientUtil.h"
#include "network.h"

#define BUF_SIZE 1024
#define CTRL_TIMEOUT_SEC 5
#define CTRL_TIMEOUT_USEC 0

void sendFile(const char *filepath_or_data, const char *type)
{
    int size = strlen(filepath_or_data);
    SSL_write(sock_fd, type, 5);
    SSL_write(sock_fd, &size, sizeof(int));
    SSL_write(sock_fd, filepath_or_data, size);
    return;
}

void sendData(const uchar* data, int size, const char* ext){
    // [1] TYPE
    SSL_write(sock_fd, "DATA", 5);

    // [2] SIZE
    SSL_write(sock_fd, &size, sizeof(int));

    // [3] EXTENSION LENGTH
    int ext_len = strlen(ext);
    SSL_write(sock_fd, &ext_len, sizeof(int));

    // [4] EXTENSION (예: ".jpg")
    SSL_write(sock_fd, ext, ext_len);

    // [5] DATA (in memory)
    SSL_write(sock_fd, data, size);
}

void* controlListenerLoop(void* arg) {
    char cmd_buf[BUF_SIZE];
    while (1) {
        int n = SSL_read(sock_fd, cmd_buf, BUF_SIZE - 1);
        if (n <= 0)
            continue;
        cmd_buf[n] = '\0';
        if (strcmp(cmd_buf, CTRL_CMD) == 0) {
            handleControlCommand();
        }
    }
}
