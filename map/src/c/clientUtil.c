#include "clientUtil.h"
#include "network.h"
#include "mappingUtils.h"

#include <chrono> // 디버깅용
#include <thread>
#include <sys/select.h>
using namespace std;


void sendFile(const char *filepath_or_data, const char *type)
{
    int size = strlen(filepath_or_data);
    SSL_write(sock_fd, type, 5);
    SSL_write(sock_fd, &size, sizeof(int));
    SSL_write(sock_fd, filepath_or_data, size);
    return;
}

void sendData(const uchar *data, int size, const char *ext)
{
    // [1] TYPE
    SSL_write(sock_fd, "DATA", 5);

    // [2] SIZE
    SSL_write(sock_fd, &size, sizeof(int));

    // [3] EXTENSION LENGTH
    int ext_len = strlen(ext);
    SSL_write(sock_fd, &ext_len, sizeof(int));

    // [4] EXTENSION (예: ".jpg")
    SSL_write(sock_fd, ext, ext_len);

    // [5] DATA (in memory)
    SSL_write(sock_fd, data, size);
}

void *signal_listener(void *unused)
{
    char buf[32];
    chrono::steady_clock::time_point start_time;
    chrono::steady_clock::time_point tracking_start_time;
    bool running = false;
    bool tracking_check = true;

    int fd = SSL_get_fd(sock_fd);

    while (1)
    {
        // 네트워크 입력 폴링
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        struct timeval tv = {0, 10000}; // 10ms

        int sel = select(fd + 1, &readfds, NULL, NULL, &tv);

        if (sel > 0 && FD_ISSET(fd, &readfds))
        {
            int n = SSL_read(sock_fd, buf, sizeof(buf) - 1);
            if (n > 0)
            {
                buf[n] = 0;
                // 1. start 수신 → 감지 모드 진입
                if (strstr(buf, "start"))
                {
                    running = true;
                    mapping_set_detecting(true);
                    mapping_set_tracking(false);
                    start_time = chrono::steady_clock::now();
                    tracking_check = true;
                    printf("start 수신 - 객체 감지 시작\n");
                }
                // 2. done 수신 → tracking/감지 종료, 상태 초기화
                if (strstr(buf, "done"))
                {
                    mapping_set_tracking(false);
                    mapping_set_detecting(false);
                    running = false;
                    tracking_check = true;
                    save_timed_points_csv();
                    // sendFile("done", "TEXT");
                    printf("done 수신\n");
                }
            }
            else if (n == 0)
            {
                printf("SSL connection closed\n");
                continue;
            }
            else
            {
                perror("SSL_read error");
                break;
            }
        }

        if(running)
        {
            // 감지 5초 타임아웃 처리 detect를 하고 있다면
        if (mapping_is_detecting())
        {
            auto now = chrono::steady_clock::now();
            auto elapsed = chrono::duration_cast<chrono::seconds>(now - start_time).count();
            if (elapsed >= 5)
            {
                printf("detect: 5sec timeout!\n");
                mapping_set_tracking(false);
                mapping_set_detecting(false);
                tracking_check = true;
                running = false;
                
                this_thread::sleep_for(chrono::milliseconds(10));
                sendFile("error", "TEXT");
            }
        }
        else
        {
            // Detect가 끝나고 tracking이 시작될 때 시간 측정 시작
            if (!mapping_is_tracking())
            {
                mapping_set_tracking(true);
                printf("end detecting - tracking start!\n");
            }
            if(tracking_check)
            {
                tracking_start_time = chrono::steady_clock::now();
                tracking_check = false;
            }
        }
        // tracking이 30초 넘게 지속되면 timeout 처리
        if (mapping_is_tracking())
        {
            auto now = chrono::steady_clock::now();
            auto elapsed = chrono::duration_cast<chrono::seconds>(now - tracking_start_time).count();
            if (elapsed >= 30)
            {
                printf("tracking 30 sec timeout\n");
                mapping_set_tracking(false);
                mapping_set_detecting(false);
                tracking_check = true;
                running = false;
                this_thread::sleep_for(chrono::milliseconds(10));
                sendFile("error", "TEXT");
            }
        }
        }
        this_thread::sleep_for(chrono::milliseconds(5));
    }
    return NULL;
}
