#include "network.h"
#include <arpa/inet.h>      
#include <sys/socket.h> 

#define BACKLOG 10

int serverNetwork(int port)
{
    int sockfd;
    struct sockaddr_in server_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
        perror("setsockopt");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(&(server_addr.sin_zero), '\0', 8);
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    return sockfd;
}

SSL *clientNetwork(int sockfd, SSL_CTX *ctx)
{
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0)
    {
        perror("[SSL] accept 실패");
        return NULL;
    }

    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        fprintf(stderr, "[SSL] SSL_new 실패\n");
        ERR_print_errors_fp(stderr);
        close(client_fd);
        return NULL;
    }

    SSL_set_fd(ssl, client_fd);
    if (SSL_accept(ssl) <= 0)
    {
        fprintf(stderr, "[SSL] SSL_accept 실패: 클라이언트 IP=%s, 포트=%d\n",
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(client_fd);
        return NULL;
    }

    return ssl;
}
