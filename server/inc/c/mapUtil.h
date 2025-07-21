#ifndef MAPUTIL_H
#define MAPUTIL_H

#include <openssl/ssl.h>
#include <openssl/err.h>

#define CLINET_PIPE "/tmp/client_pipe"
#define STRAW_PIPE "/tmp/straw_pipe"
#define MAP_PIPE "/tmp/map_pipe"

extern int stopClientPipe;
extern int stopStrawPipe;
extern int stopMapPipe;

void wirteClientFromStraw(const char*);
void *readClient(void*);

void wirteStrawFromUser(const char*);
void *readStraw(void*);

void wirteMapFromStraw(const char*);
void *readMap(void*);

pthread_t regisReadClient(SSL*);
pthread_t regisReadStraw(SSL*);
pthread_t regisReadMap(SSL*);

#endif