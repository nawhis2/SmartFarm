#ifndef CLIENTUTIL_H
#define CLIENTUTIL_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <openssl/ssl.h>
#include <openssl/err.h>

void send_command(SSL* sock, const char* msg);
void send_image(SSL *sock, const char *filepath);
void send_video(SSL *sock, const char *filepath);

#endif
