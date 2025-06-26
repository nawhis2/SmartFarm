#include "handshake.h"

Role handshakeRole(SSL *ssl, char *out_name, size_t name_sz) {
    // 1) 역할 읽기
    char buf[32]={0};
    int n, size;
    // TYPE
    if ((n=SSL_read(ssl, buf, 5))<=0) return ROLE_UNKNOWN;
    buf[n]='\0';                     // "TEXT"
    // SIZE
    if (SSL_read(ssl, &size, sizeof(int))!=sizeof(int) || size<=0||size>30)
      return ROLE_UNKNOWN;
    // DATA
    if (SSL_read(ssl, buf, size)!=size) return ROLE_UNKNOWN;
    buf[size]='\0';                  // e.g. "CCTV"
    if (strcmp(buf,"CCTV")==0) {
        // 2) CCTV면 이름도 읽기
        if (SSL_read(ssl, buf, 5)<=0) return ROLE_UNKNOWN; // expect "TEXT"
        if (SSL_read(ssl, &size, sizeof(int))!=sizeof(int)) return ROLE_UNKNOWN;
        if (size>0 && size<name_sz && SSL_read(ssl, out_name, size)==size) {
            out_name[size]='\0';
        } else return ROLE_UNKNOWN;
        return ROLE_CCTV;
    }
    else if (strcmp(buf,"USER")==0) {
        return ROLE_USER;
    }
    return ROLE_UNKNOWN;
}