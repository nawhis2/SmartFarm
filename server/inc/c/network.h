#ifndef NETWORK_H
#define NETWORK_H

#include "main.h"

int serverNetwork(int);
SSL *clientNetwork(int, SSL_CTX*);
void returnSocket(SSL*);

#endif