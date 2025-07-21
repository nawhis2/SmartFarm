#include "mapUtil.h"
#include <pthread.h>
#include <fcntl.h> 
#include <unistd.h>
#include <poll.h>

int stopClientPipe = 0;
int stopStrawPipe = 0;
int stopMapPipe = 0;

// straw에서 cmd가 들어왔을경우
void wirteClientFromStraw(const char *cmd){
    int fd;
    fd = open(CLINET_PIPE, O_WRONLY | O_NONBLOCK);
    if (fd < 0) return;

    write(fd, cmd, strlen(cmd));

    close(fd);
}

// 유저 클라이언트로 보낼 신호들 읽기
void *readClient(void *arg){
    int fd = open(CLINET_PIPE, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return NULL;

    struct pollfd pfd = { .fd = fd, .events = POLLIN|POLLHUP };
    while (!stopClientPipe) {
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

// USER에서 원하는 cmd가 들어왔을경우
void wirteStrawFromUser(const char *cmd){
    int fd;
    fd = open(STRAW_PIPE, O_WRONLY | O_NONBLOCK);
    if (fd < 0) return;

    write(fd, cmd, strlen(cmd));

    close(fd);
}

// 파이프 확인해서 Map파이프로
void *readStraw(void *arg){
    int fd = open(STRAW_PIPE, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return NULL;

    struct pollfd pfd = { .fd = fd, .events = POLLIN|POLLHUP };
    while (!stopStrawPipe) {
        int ret = poll(&pfd, 1, 500);    // 500ms 타임아웃
        if (ret > 0) {
            if (pfd.revents & POLLIN) {
                char buf[4096];
                int n = read(fd, buf, sizeof(buf));

                if (n > 0){ 
                    if(strncmp(buf, "done", 4)){
                        wirteMapFromStraw(buf);
                    }
                    else if(strncmp(buf, "start", 5)){
                        // strawberry cctv에게 start전송
                        SSL_write((SSL*)arg, buf, n);
                    }
                }
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

// Straw에서 Map 파이프로 cmd 전송
void wirteMapFromStraw(const char *cmd){
    int fd;
    fd = open(MAP_PIPE, O_WRONLY | O_NONBLOCK);
    if (fd < 0) return;

    write(fd, cmd, strlen(cmd));

    close(fd);
}


// Straw에게서 받은 cmd에 따라 처리
void *readMap(void *arg){
    int fd = open(MAP_PIPE, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return NULL;

    struct pollfd pfd = { .fd = fd, .events = POLLIN|POLLHUP };
    while (!stopMapPipe) {
        int ret = poll(&pfd, 1, 500);    // 500ms 타임아웃
        if (ret > 0) {
            if (pfd.revents & POLLIN) {
                char buf[4096];
                int n = read(fd, buf, sizeof(buf));
                if (n > 0){
                    if(strncmp(buf, "done", 4)){
                        // MapCCTV에게 서버로 맵을 전송하라고 write
                        SSL_write((SSL*)arg, buf, n);
                    }
                }
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

pthread_t regisReadClient(SSL *ssl){
    pthread_t tid;
    pthread_create(&tid, NULL, readClient, ssl);

    return tid;
}

pthread_t regisReadStraw(SSL *ssl){
    pthread_t tid;
    pthread_create(&tid, NULL, readStraw, ssl);

    return tid;
}

pthread_t regisReadMap(SSL *ssl){
    pthread_t tid;
    pthread_create(&tid, NULL, readMap, ssl);

    return tid;
}