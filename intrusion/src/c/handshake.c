#include "handshake.h"
#include "clientUtil.h"
void handshakeClient(SSL *ssl, const char* role, const char *cctvName){
	if (role)
		sendFile(ssl, role, "TEXT");
	if (cctvName)
		sendFile(ssl, cctvName, "TEXT");
}