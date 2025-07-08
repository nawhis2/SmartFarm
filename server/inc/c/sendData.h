#ifndef SENDDATA_H
#define SENDDATA_H

#include <openssl/ssl.h>
#include <openssl/err.h>

void sendData(SSL*, const char*);

#endif