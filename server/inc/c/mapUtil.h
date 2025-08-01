#ifndef MAPUTIL_H
#define MAPUTIL_H

#include <openssl/ssl.h>
#include <openssl/err.h>

#define CLIENT_PIPE "/tmp/client_pipe"
#define STRAW_PIPE "/tmp/straw_pipe"
#define MAP_PIPE "/tmp/map_pipe"

extern int stopClientPipe;
extern int stopStrawPipe;
extern int stopMapPipe;

int writeToUser(const char*);
void *readUser(void*);

int writeToStraw(const char*);
void *readStraw(void*);

int writeToMap(const char*);
void *readMap(void*);

pthread_t regisReadUser(SSL*);
pthread_t regisReadStraw(SSL*);
pthread_t regisReadMap(SSL*);

#endif