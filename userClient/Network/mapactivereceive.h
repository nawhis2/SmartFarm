#ifndef MAPACTIVERECEIVE_H
#define MAPACTIVERECEIVE_H

#include <openssl/ssl.h>
#include <openssl/err.h>

class QThread;
extern int mapActiveStop;

QThread* startReceiveMapThread(SSL* sensor);
void receiveMapActive(SSL* ssl);

#endif // MAPACTIVERECEIVE_H
