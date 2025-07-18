#ifndef USERCLIENTSEND_H
#define USERCLIENTSEND_H

#include <openssl/ssl.h>
#include <openssl/err.h>

void ipSend(SSL*, const char*);
void jsonSend(SSL*, const char*);
void imageSend(SSL*, const char*);
void graphDailySend(SSL*, const char*);
void graphWeeklySend(SSL*, const char*);

#endif