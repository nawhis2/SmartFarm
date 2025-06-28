#ifndef CLIENTUTIL_H
#define CLIENTUTIL_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <openssl/ssl.h>
#include <openssl/err.h>

void sendFile(SSL*, const char*, const char*);

#endif
