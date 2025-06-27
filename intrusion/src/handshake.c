#include "handshake.h"
#include "clientUtil.h"
void handshakeClient(SSL *ssl, char* role, char *cctvName){
	if (role)
		sendFile(ssl, role, "TEXT");
	if (cctvName)
		sendFile(ssl, cctvName, "TEXT");
}