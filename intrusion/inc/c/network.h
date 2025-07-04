#ifndef NETWORK_H
#define NETWORK_H

#include "main.h"

extern SSL* sock_fd;

int socketNetwork();
int network(int, SSL_CTX *ctx);

#endif