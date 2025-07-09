/*
    [기능 이름]: OpenSSL TCP서버
    [작성자]: 조찬우 (chanwoo)
    [파일 설명]:
      - TCP 서버

    [TODO]:
      - [x] 서버 코드 작성
      - [x] client fd 관리 코드 추가
      - [ ] CCTV IP 주소 저장
      - [ ] CCTV, User 클라이언트 별로 별도 리시브 함수 구현
      - [ ] 측정값을 서버로 전송하는 로직 추가 (MQTT 예정)

    [참고 사항]:
*/

#include "main.h"
#include "network.h"
#include "storage.h" 
#include "handshake.h"
#include "receive.h"
#include "db.h"

#define BUF_SIZE 1024

int clientFdCnt = 0;
int maxFdCnt = 10;

void sigchld_handler(int sig)
{
    // 좀비 프로세스 방지
    while (waitpid(-1, NULL, WNOHANG) > 0)
        clientFdCnt--;
}

int main()
{
    init_openssl();
    SSL_CTX *ctx = create_context();
    configure_context(ctx);

    //daemonP();

    int server_fd = serverNetwork();
    signal(SIGCHLD, sigchld_handler);

    initStorage();
    
    init_mysql("localhost",
           "sfapp",         // 새로 만든 계정
           "StrongAppPass!",
           "smartfarm",
           3306);

    SSL *ssl = NULL;
    while (1)
    {     
        if(clientFdCnt > maxFdCnt){
            perror("client over");
            continue;
        }

        ssl = clientNetwork(server_fd, ctx);
        if (!ssl)
        {
            perror("ssl create");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            char cam_name[32]={0};
            Role role = handshakeRole(ssl, cam_name, sizeof(cam_name));
            if (role==ROLE_CCTV) {
                printf("CCTV\n");
                registerCCTV(ssl, cam_name);
                receiveCCTV(ssl);
                removeActiveCCTV(cam_name);
            }
            else if (role==ROLE_USER){
                printf("User\n");
                receiveUser(ssl);
            }
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(SSL_get_fd(ssl));
            return 0;
        }
        else if(pid > 0)  
            clientFdCnt++;
    }

    close_mysql();
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
