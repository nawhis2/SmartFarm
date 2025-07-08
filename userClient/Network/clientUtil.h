#ifndef CLIENTUTIL_H
#define CLIENTUTIL_H

typedef unsigned char uchar;

void sendFile(const char*, const char*);
void sendData(const uchar*, int, const char*);

#endif // CLIENTUTIL_H
