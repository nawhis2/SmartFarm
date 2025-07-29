#include "mapUtil.h"
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

int stopClientPipe = 0;
int stopStrawPipe = 0;
int stopMapPipe = 0;

// straw에서 cmd가 들어왔을경우
void writeToUser(const char *cmd)
{
    ssize_t len = strlen(cmd);
    int fd;

    // non‑blocking open → reader가 없으면 retry
    while ((fd = open(CLINET_PIPE, O_WRONLY | O_NONBLOCK)) < 0)
    {
        if (errno != ENXIO && errno != EAGAIN)
        {
            perror("open client pipe for write");
            return;
        }
        usleep(100000); // 0.1초 대기
    }

    // 이제 blocking 모드로 전환 (선택 사항)
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

    // 실제 쓰기
    if (write(fd, cmd, len) != len)
    {
        perror("write to client pipe");
    }
    write(fd, "\0", 1);

    close(fd);
}

// 유저 클라이언트로 보낼 신호들 읽기
void *readUser(void *arg)
{
    SSL *ssl = (SSL *)arg;
    char buf[4096];
    ssize_t n;
    int fd, flags;

    while (!stopClientPipe)
    {
        // non‑blocking open → writer가 나타날 때까지 retry
        while ((fd = open(CLINET_PIPE, O_RDONLY | O_NONBLOCK)) < 0)
        {
            if (errno != ENOENT && errno != EAGAIN)
            {
                perror("open client pipe for read");
                return NULL;
            }
            usleep(100000);
        }

        // 이후 read()는 블록 모드로 전환
        flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

        // 파이프에서 데이터가 들어올 때까지 블록 → SSL_write
        while ((n = read(fd, buf, sizeof(buf))) > 0)
        {
            if (SSL_write(ssl, buf, n) <= 0)
            {
                perror("SSL_write");
                break;
            }
        }

        close(fd);

        // n==0 → writer가 닫힌 것이므로, 다시 open 루프로 돌아감
        if (n < 0 && errno != EINTR)
        {
            perror("read client pipe");
            break;
        }
    }

    return NULL;
}

// USER에서 원하는 cmd가 들어왔을경우
void writeToStraw(const char *cmd)
{
    ssize_t len = strlen(cmd);
    int fd;

    // non‑blocking open → reader가 없으면 retry
    while ((fd = open(STRAW_PIPE, O_WRONLY | O_NONBLOCK)) < 0)
    {
        if (errno != ENXIO && errno != EAGAIN)
        {
            perror("open client pipe for write");
            return;
        }
        usleep(100000); // 0.1초 대기
    }

    // 이제 blocking 모드로 전환 (선택 사항)
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

    // 실제 쓰기
    if (write(fd, cmd, len) != len)
    {
        perror("write to client pipe");
    }
    write(fd, "\0", 1);

    close(fd);
}

// Straw파이프 확인해서 전송
void *readStraw(void *arg)
{
    SSL *ssl = (SSL *)arg;
    char buf[4096];
    ssize_t n;
    int fd, flags;

    while (!stopStrawPipe)
    {
        // non‑blocking open → writer가 나타날 때까지 retry
        while ((fd = open(STRAW_PIPE, O_RDONLY | O_NONBLOCK)) < 0)
        {
            if (errno != ENOENT && errno != EAGAIN)
            {
                perror("open client pipe for read");
                return NULL;
            }
            usleep(100000);
        }

        // 이후 read()는 블록 모드로 전환
        flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

        // 파이프에서 데이터가 들어올 때까지 블록 → SSL_write
        while ((n = read(fd, buf, sizeof(buf))) > 0)
        {
            if (strncmp(buf, "done", 4) == 0)
            {
                writeToMap(buf);
            }
            else if (strncmp(buf, "start", 5) == 0)
            {
                // strawberry cctv에게 start전송
                if (SSL_write(ssl, buf, n) <= 0)
                {
                    perror("SSL_write");
                    break;
                }
            }
        }

        close(fd);

        // n==0 → writer가 닫힌 것이므로, 다시 open 루프로 돌아감
        if (n < 0 && errno != EINTR)
        {
            perror("read client pipe");
            break;
        }
    }

    return NULL;
}

// Straw에서 Map 파이프로 cmd 전송
void writeToMap(const char *cmd)
{
    ssize_t len = strlen(cmd);
    int fd;

    // non‑blocking open → reader가 없으면 retry
    while ((fd = open(MAP_PIPE, O_WRONLY | O_NONBLOCK)) < 0)
    {
        if (errno != ENXIO && errno != EAGAIN)
        {
            perror("open client pipe for write");
            return;
        }
        usleep(100000); // 0.1초 대기
    }

    // 이제 blocking 모드로 전환 (선택 사항)
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

    // 실제 쓰기
    if (write(fd, cmd, len) != len)
    {
        perror("write to client pipe");
    }
    write(fd, "\0", 1);

    close(fd);
}

// Straw에게서 받은 cmd에 따라 처리
void *readMap(void *arg)
{
    SSL *ssl = (SSL *)arg;
    char buf[4096];
    ssize_t n;
    int fd, flags;

    while (!stopMapPipe)
    {
        // non‑blocking open → writer가 나타날 때까지 retry
        while ((fd = open(MAP_PIPE, O_RDONLY | O_NONBLOCK)) < 0)
        {
            if (errno != ENOENT && errno != EAGAIN)
            {
                perror("open client pipe for read");
                return NULL;
            }
            usleep(100000);
        }

        // 이후 read()는 블록 모드로 전환
        flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

        // 파이프에서 데이터가 들어올 때까지 블록 → SSL_write
        while ((n = read(fd, buf, sizeof(buf))) > 0)
        {
            if (strncmp(buf, "done", 4) == 0)
            {
                if (SSL_write(ssl, buf, n) <= 0)
                {
                    perror("SSL_write");
                    break;
                }
            }
            else if (strncmp(buf, "start", 5) == 0)
            {
                // strawberry cctv에게 start전송
                if (SSL_write(ssl, buf, n) <= 0)
                {
                    perror("SSL_write1");
                    break;
                }
            }
        }

        close(fd);

        // n==0 → writer가 닫힌 것이므로, 다시 open 루프로 돌아감
        if (n < 0 && errno != EINTR)
        {
            perror("read client pipe");
            break;
        }
    }

    return NULL;
}

pthread_t regisReadUser(SSL *ssl)
{
    pthread_t tid;
    pthread_create(&tid, NULL, readUser, ssl);

    return tid;
}

pthread_t regisReadStraw(SSL *ssl)
{
    pthread_t tid;
    pthread_create(&tid, NULL, readStraw, ssl);

    return tid;
}

pthread_t regisReadMap(SSL *ssl)
{
    pthread_t tid;
    pthread_create(&tid, NULL, readMap, ssl);

    return tid;
}