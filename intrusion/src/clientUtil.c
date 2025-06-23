#include "client_util.h"

void send_command(SSL *sock, const char *msg)
{
    int ret = SSL_write(sock, msg, strlen(msg));
    if (ret <= 0)
    {
        int err = SSL_get_error(sock, ret);
        printf("SSL_write failed: %d\n", err);
        ERR_print_errors_fp(stderr);
    }
}
