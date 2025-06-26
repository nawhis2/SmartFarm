#ifndef NETWORK_H
#define NETWORK_H

#include "main.h"

int socketNetwork();
SSL* network(int, SSL_CTX *ctx);

#endif