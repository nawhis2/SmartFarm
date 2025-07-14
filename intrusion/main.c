/*
    [기능 이름]: 침입 감지 client
    [작성자]: 조찬우 (chanwoo)
    [파일 설명]:
      - 침입 감지 client 코드

    [TODO]:
      - [ ] 침입 감지 추가
      - [ ] while문 추가

    [참고 사항]:
*/

#include "main.h"
#include "clientUtil.h"
#include "network.h"
#include "handshake.h"
#include "jsonlParse.h"
#include "rtspStreaming.h"

#define PORT 60000
#define CCTVPORT 60001

#define BUF_SIZE 1024

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "FORMAT: %s <IP ADDRESS>\n", argv[0]);
        return 1;
    }
    char *ip = argv[1];
    if (!ip)
    {
        fprintf(stderr, "NO IP ADDRESS");
        return 1;
    }
    int num;
    char buffer[BUF_SIZE];

    sigset_t set;

    sigfillset(&set);
    sigdelset(&set, SIGINT);
    sigprocmask(SIG_SETMASK, &set, NULL);

    init_openssl();
    SSL_CTX *ctx = create_context();

    // handshake
    int sockfd = socketNetwork(ip, PORT);
    {
        if (network(sockfd, ctx) < 1)
        {
            perror("HandShake");
            exit(1);
        }
        handshakeClient("CCTV", "Intrusion");
        returnSocket();
        close(sockfd);
    }

    // cctv connect
    int cctvfd = socketNetwork(ip, CCTVPORT);
    {
        if (network(cctvfd, ctx) < 1)
        {
            perror("CCTV");
            exit(1);
        }
        sendFile("hi Server\n", "TEXT");
    }

    RtspStreaming(1920, 1080, 30);
    sleep(1);

    while (1);

    returnSocket();
    close(cctvfd);
    SSL_CTX_free(ctx);
    EVP_cleanup();

    return 0;
}

// OpenSSL 초기화
void init_openssl()
{
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

// SSL 컨텍스트 생성 (클라이언트용)
SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_client_method();
    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        perror("SSL 컨텍스트 생성 실패");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}
