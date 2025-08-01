#include "sensorUtil.h"
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

int stop_pipe = 0;

void *sensorFire(void *arg)
{
    SSL *ssl = (SSL *)arg;
    char buf[4096];
    int n;

    int fd;
    while ((fd = open(SENSOR_PIPE, O_WRONLY | O_NONBLOCK)) < 0)
    {
        if (errno != ENXIO && errno != EAGAIN)
        {
            perror("open pipe");
            return NULL;
        }
        usleep(100000);
    }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);

    while ((n = SSL_read(ssl, buf, sizeof(buf))) > 0)
    {
        write(fd, buf, n);
    }
    close(fd);
    return NULL;
}

void *sensorUser(void *arg)
{
    SSL   *ssl = (SSL*)arg;
    char   buf[4096];
    ssize_t n;
    int    fd, flags;

    while (!stop_pipe) {
        // 1) FIFO open (non-blocking → blocking으로 전환)
        while ((fd = open(SENSOR_PIPE, O_RDONLY | O_NONBLOCK)) < 0) {
            if (errno != ENOENT && errno != EAGAIN) {
                perror("open pipe");
                return NULL;
            }
            usleep(100000);
        }
        flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

        // 2) read loop: writer가 write()할 때까지 블록,
        //    write가 끝나 pipe가 닫히면 read()가 0을 리턴
        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            if (SSL_write(ssl, buf, n) <= 0) {
                perror("SSL_write");
                break;
            }
        }

        // 3) EOF(=n==0) or error 시 닫고, 다시 open 하러 올라간다
        close(fd);

        if (n < 0 && errno != EINTR) {
            perror("read pipe");
            break;
        }
        // n==0이면 writer가 닫힌 것이므로, 다시 open/reopen
    }

    return NULL;
}

pthread_t regisSensorFire(SSL *ssl)
{
    pthread_t tid;
    pthread_create(&tid, NULL, sensorFire, ssl);

    return tid;
}

pthread_t regisSensorUser(SSL *ssl)
{
    pthread_t tid;
    pthread_create(&tid, NULL, sensorUser, ssl);

    return tid;
}