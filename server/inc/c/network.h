#ifndef NETWORK_H
#define NETWORK_H

#include "main.h"

int serverNetwork();
SSL *clientNetwork(int, SSL_CTX *);

#endif