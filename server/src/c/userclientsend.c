#include "userclientsend.h"
#include "db.h"
#include "sendData.h"
#include "storage.h"

void ipSend(SSL *sock_fd, const char *data)
{
    char buf[16];
    if (lookupActiveCCTV(data, buf))
    {
        printf("[Test] : %s\n", buf);
        sendData(sock_fd, buf);
    }
    else
        printf("[TEXT] %s\n", data);
}

void jsonSend(SSL *sock_fd, const char *event_type)
{
    query_and_send(sock_fd, event_type);
}

void imageSend(SSL *sock_fd, const char *file_path)
{
    // 1) 파일 열기
    FILE *fp = fopen(file_path, "rb");
    if (!fp) {
        perror("fopen");
        return;
    }

    // 2) 파일 크기 구하기
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    rewind(fp);
    
    SSL_write(sock_fd, &file_size, sizeof(int));

    // 3) DATA 전송
    char buf[4096];
    int bytes_read;
    while ((bytes_read = fread(buf, 1, sizeof(buf), fp)) > 0) {
        int sent = 0;
        while (sent < bytes_read) {
            int n = SSL_write(sock_fd, buf + sent, bytes_read - sent);
            if (n <= 0) {
                // 쓰기 에러 발생 시 루프 탈출
                perror("write");
                fclose(fp);
                return;
            }
            sent += n;
        }
    }

    fclose(fp);
}
