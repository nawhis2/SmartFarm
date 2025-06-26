#ifndef HANDSHAKE_H
#define HANDSHAKE_H

#include "main.h"

typedef enum { ROLE_UNKNOWN=0, ROLE_CCTV, ROLE_USER } Role;
Role handshakeRole(SSL *ssl, char *out_name, size_t name_sz);

#endif