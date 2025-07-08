#ifndef NETWORK_H
#define NETWORK_H

#include "main.h"

extern SSL* sock_fd;

int socketNetwork(const char*);
int network(int, SSL_CTX *);

#endif // NETWORK_H
