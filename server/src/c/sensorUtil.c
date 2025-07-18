#include "sensorUtil.h"
#include <pthread.h>
#include <fcntl.h> 
#include <unistd.h>
#include <poll.h>

int stop_pipe = 1;

void *sensorFire(void *arg){
    int fd;
    while (!stop_pipe) {
        fd = open(SENSOR_PIPE, O_WRONLY | O_NONBLOCK);
        if (fd >= 0) break;
        if (errno != ENXIO) return NULL;  // 그 외 에러는 포기
        usleep(100000);
    }
    if (fd < 0) return NULL;

    char buf[4096];
    int n;
    while (!stop_pipe && (n = SSL_read((SSL*)arg, buf, sizeof(buf))) > 0) {
        write(fd, buf, n);
    }
    close(fd);
    return NULL;
}

void *sensorUser(void *arg){
    int fd = open(SENSOR_PIPE, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return NULL;

    struct pollfd pfd = { .fd = fd, .events = POLLIN|POLLHUP };
    while (!stop_pipe) {
        int ret = poll(&pfd, 1, 500);    // 500ms 타임아웃
        if (ret > 0) {
            if (pfd.revents & POLLIN) {
                char buf[4096];
                int n = read(fd, buf, sizeof(buf));
                if (n > 0) SSL_write((SSL*)arg, buf, n);
            }
            if (pfd.revents & POLLHUP) {
                // 파이프가 닫혔을 때
                break;
            }
        }
        // ret == 0 이면 타임아웃 → stop_pipe 체크 후 루프 계속
    }
    close(fd);
    return NULL;
}

pthread_t regisSensorFire(SSL *ssl){
    pthread_t tid;
    pthread_create(&tid, NULL, sensorFire, ssl);

    return tid;
}

pthread_t regisSensorUser(SSL *ssl){
    pthread_t tid;
    pthread_create(&tid, NULL, sensorUser, ssl);

    return tid;
}