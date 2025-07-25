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
#include "sensorUtil.h"
#include "mapUtil.h"

#define PORT 60000
#define CCTVPORT 60001
#define SENSORPORT 60002
#define USERPORT 60003
#define MAPPORT 60004
#define BUF_SIZE 1024

volatile sig_atomic_t clientFdCnt = 0;
int maxFdCnt = 10;

void sigchld_handler(int sig)
{
    // 좀비 프로세스 방지
    while (waitpid(-1, NULL, WNOHANG) > 0){
        clientFdCnt--;
        fprintf(stderr, "[SIGCHLD] clientFdCnt-- → now: %d\n", clientFdCnt);
    }
}

int main()
{
    mkfifo(SENSOR_PIPE, 0666);
    mkfifo(MAP_PIPE, 0666);
    mkfifo(STRAW_PIPE, 0666);
    mkfifo(CLINET_PIPE, 0666);

    init_openssl();
    SSL_CTX *ctx = create_context();
    configure_context(ctx);

    // daemonP();

    int handshake_fd = serverNetwork(PORT);
    int cctv_fd = serverNetwork(CCTVPORT);
    int sensor_fd = serverNetwork(SENSORPORT);
    int user_fd = serverNetwork(USERPORT);
    int map_fd = serverNetwork(MAPPORT);

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
    signal(SIGPIPE, SIG_IGN);

    initStorage();

    init_mysql("localhost",
               "sfapp", // 새로 만든 계정
               "StrongAppPass!",
               "smartfarm",
               3306);

    while (1)
    {
        SSL *ssl = clientNetwork(handshake_fd, ctx);
        if (!ssl)
        {
            fprintf(stderr, "ssl create fail");
            exit(0);
        }

        if (clientFdCnt > maxFdCnt)
        {
            SSL_shutdown(ssl);
            close(SSL_get_fd(ssl));
            SSL_free(ssl);
            perror("client over");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            SSL *sensor = NULL;

            char camName[32] = {0};
            Role role = handshakeRole(ssl, camName, sizeof(camName));
            if (role == ROLE_CCTV)
            {
                SSL *cctv = clientNetwork(cctv_fd, ctx);
                if (!cctv)
                {
                    fprintf(stderr, "cctv ssl create fail");
                    exit(0);
                }

                pthread_t fireTid;
                pthread_t mapTid;
                int startFireThread = 0;
                int startMapThread = 0;
                printf("CCTV\n");
                registerCCTV(cctv, camName);
                if (strncmp(camName, "Fire", 4) == 0)
                {
                    sensor = clientNetwork(sensor_fd, ctx);
                    if (!sensor)
                    {
                        fprintf(stderr, "sensor ssl create fail");
                        exit(0);
                    }

                    fireTid = regisSensorFire(sensor);
                    startFireThread = 1;
                }
                else if (strncmp(camName, "Strawberry", 10) == 0)
                {
                    mapTid = regisReadStraw(cctv);
                    startMapThread = 1;
                }
                else if (strncmp(camName, "Map", 3) == 0)
                {
                    mapTid = regisReadMap(cctv);
                    startMapThread = 1;
                }

                receiveCCTV(cctv);
                removeActiveCCTV(camName);
                stopStrawPipe = 1;
                stopMapPipe = 1;

                if (startFireThread)
                {
                    pthread_join(fireTid, NULL);
                    returnSocket(sensor);
                }

                if (startMapThread)
                {
                    pthread_join(mapTid, NULL);
                }

                returnSocket(cctv);
            }
            else if (role == ROLE_USER)
            {
                printf("User\n");
                SSL *user = clientNetwork(user_fd, ctx);
                if (!user)
                {
                    fprintf(stderr, "ssl create fail");
                    exit(0);
                }

                sensor = clientNetwork(sensor_fd, ctx);
                if (!sensor)
                {
                    fprintf(stderr, "sensor ssl create fail");
                    exit(0);
                }

                SSL *mapfd = clientNetwork(map_fd, ctx);
                if (!mapfd)
                {
                    fprintf(stderr, "map ssl create fail");
                    exit(0);
                }
                pthread_t sensorTid = regisSensorUser(sensor);
                pthread_t mapTid = regisReadUser(mapfd);
                receiveUser(user);

                stop_pipe = 1;
                stopClientPipe = 1;
                pthread_join(sensorTid, NULL);
                pthread_join(mapTid, NULL);

                returnSocket(sensor);
                returnSocket(user);
                returnSocket(mapfd);
            }

            returnSocket(ssl);
            exit(0);
        }
        else if (pid > 0)
            clientFdCnt++;
    }

    close_mysql();
    close(handshake_fd);
    close(sensor_fd);
    close(user_fd);
    close(cctv_fd);
    close(map_fd);
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
