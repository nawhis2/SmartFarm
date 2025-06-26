#include "handshake.h"
#include "clientUtil.h"
void handshakeClient(SSL *ssl, char* role, char *cctvName){
	send_file(ssl, role, "TEXT");
	send_file(ssl, cctvName, "TEXT");
}