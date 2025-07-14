#ifndef CLIENTUTIL_H
#define CLIENTUTIL_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <openssl/ssl.h>
#include <openssl/err.h>
typedef unsigned char uchar;

void sendFile(const char*, const char*);
void sendData(const uchar*, int, const char*);

#endif
