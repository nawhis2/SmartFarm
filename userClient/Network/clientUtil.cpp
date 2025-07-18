#include "clientUtil.h"
#include "network.h"
#include <QDebug>
void sendFile(const char *filepath_or_data, const char *type)
{
    //int size = strlen(filepath_or_data);
    int size = static_cast<int>(strlen(filepath_or_data));
    SSL_write(sock_fd, type, strlen(type));
    SSL_write(sock_fd, &size, sizeof(int));
    SSL_write(sock_fd, filepath_or_data, size);
    return;
}
