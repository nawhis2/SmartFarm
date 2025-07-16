#include "network.h"

SSL* sock_fd = NULL;

int socketNetwork(const char *ipAddress, const char* port) {
    int sockfd;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        exit(1);
    }
#endif

    struct addrinfo hints, *res, *p;
    int rv;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP

    if ((rv = getaddrinfo(ipAddress, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // 여러 결과 중 하나 선택
    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;

        if (connect(sockfd, p->ai_addr, (int)p->ai_addrlen) == -1) {
#ifdef _WIN32
            closesocket(sockfd);
#else
            close(sockfd);
#endif
            continue;
        }
        break;  // 연결 성공!
    }

    freeaddrinfo(res);

    if (p == NULL) {
        fprintf(stderr, "연결 실패: 어떤 주소에도 연결할 수 없음\n");
        return -1;
    }

    return sockfd;
}

int network(int sockfd, SSL_CTX *ctx) {
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        return 0;
    }

    sock_fd = ssl;
    return 1;
}

SSL* sensorNetwork(int sockfd, SSL_CTX *ctx) {
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        return 0;
    }

    return ssl;
}

void returnSocket(){
    SSL_shutdown(sock_fd);
#ifdef _WIN32
    closesocket(SSL_get_fd(sock_fd));
#else
    close(sockfd);
#endif
    SSL_free(sock_fd);
}
