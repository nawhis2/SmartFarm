#ifndef CLIENTUTIL_H
#define CLIENTUTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

typedef unsigned char uchar;

void sendFile(const char*, const char*);
void sendData(const uchar*, int, const char*);
void* controlListenerLoop(void* arg);

#endif
