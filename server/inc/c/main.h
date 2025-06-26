#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

void daemonP();

void init_openssl();
SSL_CTX *create_context();
void configure_context(SSL_CTX *);

void receiveMsg(SSL *);
int receive_packet(SSL *ssl);

#endif
