#include "sensorUtil.h"
#include <pthread.h>
#include <fcntl.h> 
#include <unistd.h>

int stop_pipe = 1;

void *sensorFire(void *arg){
    SSL* sesnor = (SSL*)arg;
    char buf[4096];
    int  n;
    while ((n = SSL_read(sesnor, buf, sizeof(buf))) > 0) {
        // 2) 파이프 열고 쓰기
        int fd = open(SENSOR_PIPE, O_WRONLY | O_NONBLOCK);
        if (fd >= 0) {
            write(fd, buf, n);
            close(fd);
        }
    }
    return NULL;
}

void *sensorUser(void *arg){
    int fd = open(SENSOR_PIPE, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return NULL;

    SSL* sesnor = (SSL*)arg;
    stop_pipe = 0;
    char buf[4096];
    while (!stop_pipe) {
        int n = read(fd, buf, sizeof(buf));
        if (n > 0) {
            // 받아서 SSL로 보내기
            SSL_write(sesnor, buf, n);
        }
        else if (n <= 0) {
            // 리더가 닫혔거나 에러
            usleep(100000);
        }
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