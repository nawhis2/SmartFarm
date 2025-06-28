#include "network.h"
#define PORT 60000
#define BACKLOG 10

int socketNetwork(){
    int sockfd;
	struct sockaddr_in socket_addr;

	struct hostent *he;
	if ((he = gethostbyname("localhost")) == NULL) {
        perror("gethostbyname");
        exit(1);
    }

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	socket_addr.sin_family = AF_INET;
	socket_addr.sin_port = htons(PORT);
	socket_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(socket_addr.sin_zero), '\0', 8);

    return sockfd;
}

SSL* network(int sockfd, SSL_CTX *ctx)
{
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
	// 서버 연결
    if (connect(sockfd, (struct sockaddr*)&client_addr, addr_len) < 0) {
        perror("서버 연결 실패");
        exit(EXIT_FAILURE);
    }

    // SSL 객체 생성 및 핸드셰이크 수행
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
    }

	return ssl;
}