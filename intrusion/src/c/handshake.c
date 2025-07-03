#include "handshake.h"
#include "clientUtil.h"
void handshakeClient(const char* role, const char *cctvName){
	if (role)
		sendFile(role, "TEXT");
	if (cctvName)
		sendFile(cctvName, "TEXT");
}