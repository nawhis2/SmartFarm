/*
    [기능 이름]: OpenSSL TCP서버
    [작성자]: 조찬우 (chanwoo)
    [파일 설명]:
      - TCP 서버

    [TODO]:
      - [x] 서버 코드 작성
      - [x] client fd 관리 코드 추가
      - [ ] 측정값을 서버로 전송하는 로직 추가 (MQTT 예정)

    [참고 사항]:
*/

#include "main.h"
#include <fcntl.h>
#include <signal.h>
#include <dlfcn.h>
#include <wait.h>
#include <wiringPi.h>
#include <sys/stat.h>
#include <sys/resource.h>

#define PORT 60000
#define BUF_SIZE 1024
#define BACKLOG 10

typedef void (*FUNC)(const char *);
int clientFdCnt = 0;
int maxFdCnt = 4;
int sensorOn;

void sigchld_handler(int sig)
{
    // 좀비 프로세스 방지
    while (waitpid(-1, NULL, WNOHANG) > 0)
        clientFdCnt--;
}

void daemonP();
int serverNetwork();
SSL *clientNetwork(int, SSL_CTX *);
void init_openssl();
SSL_CTX *create_context();
void configure_context(SSL_CTX *ctx);
void receiveMsg(SSL *);

int main()
{
    init_openssl();
    SSL_CTX *ctx = create_context();
    configure_context(ctx);

    daemonP();

    int server_fd = serverNetwork();

    if (wiringPiSetupGpio() == -1)
        return 1;

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // accept()가 중단되지 않도록
    sigaction(SIGCHLD, &sa, NULL);
    SSL *ssl;
    while (1)
    {
        ssl = clientNetwork(server_fd, ctx);
        if (!ssl)
        {
            perror("ssl create");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            receiveMsg(ssl);
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(SSL_get_fd(ssl));
            exit(0);
        }
        else if(pid > 0)  
            clientFdCnt++;
    }

    SSL_free(ssl);
    close(server_fd);
    SSL_CTX_free(ctx);
    EVP_cleanup();
    return 0;
}

void daemonP()
{
    struct sigaction sa;
    struct rlimit rl;
    pid_t pid;
    int fd0, fd1, fd2;

    umask(0);

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
    {
        perror("getrlimit");
        exit(1);
    }

    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        exit(1);
    }
    if (pid > 0)
    {
        exit(0);
    }

    if (setsid() < 0)
    {
        perror("setsid");
        exit(1);
    }

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
    {
        perror("sigaction");
        exit(1);
    }

    if (chdir("/") < 0)
    {
        perror("chdir");
        exit(1);
    }

    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (int i = 0; i < rl.rlim_max; i++)
        close(i);

    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);
}

int serverNetwork()
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
    server_addr.sin_port = htons(PORT);
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
    int addr_len = sizeof(struct sockaddr_in);
    
    if(clientFdCnt >= maxFdCnt){
        perror("클라이언트 수 초과");
        return NULL;
    }

    int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0)
    {
        close(client_fd);
        perror("클라이언트 연결 실패");
        return NULL;
    }

    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_fd);
    if (SSL_accept(ssl) <= 0)
    {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);       // 추가하면 더 안전
        close(client_fd);    // SSL 실패 시 소켓도 닫기
        return NULL;
    }
    return ssl;
}

// OpenSSL 초기화
void init_openssl()
{
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

// SSL 컨텍스트 생성 (서버용)
SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        perror("SSL 컨텍스트 생성 실패");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

// 서버 인증서 및 개인 키 로드
void configure_context(SSL_CTX *ctx)
{
    SSL_CTX_set_ecdh_auto(ctx, 1);
    if (SSL_CTX_use_certificate_file(ctx, "server-cert.pem", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "server-key.pem", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

void receiveMsg(SSL *ssl)
{
    while (1)
    {
        char buffer[BUF_SIZE] = "";
        int bytes = SSL_read(ssl, buffer, BUF_SIZE);
        if (bytes <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        buffer[bytes] = '\0';
        printf("%s\n", buffer);
    }
}
