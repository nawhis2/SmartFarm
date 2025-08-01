#include "sendData.h"

void sendData(SSL* sock_fd, const char* data){
    if(!data) return;
    int size = strlen(data);
    SSL_write(sock_fd, data, size);
}