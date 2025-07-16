#ifndef NETWORK_H
#define NETWORK_H

#include "main.h"

extern SSL* sock_fd;

int socketNetwork(const char*, const char*);
int network(int, SSL_CTX *);
SSL* sensorNetwork(int, SSL_CTX *);
void returnSocket();

#endif // NETWORK_H
