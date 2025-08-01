#ifndef MAIN_H
#define MAIN_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
\
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <QImage>

QImage gstSampleToQImage(GstSample *sample);
void updateFrame();

void init_openssl();
SSL_CTX *create_context();

#endif // MAIN_H
