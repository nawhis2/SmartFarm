#include "main.h"

int receive_packet(SSL *ssl)
{
    char type[10] = {0};
    int size = 0;

    // [1] TYPE 수신
    if (SSL_read(ssl, type, 5) <= 0)
        return -1;

    // [2] SIZE 수신
    if (SSL_read(ssl, &size, sizeof(int)) <= 0 || size <= 0)
        return -1;

    // TEXT 처리
    if (strncmp(type, "TEXT", 4) == 0) {
        char *msg = (char *)malloc(size + 1);
        if (!msg) return -1;

        int received = 0;
        while (received < size) {
            int n = SSL_read(ssl, msg + received, size - received);
            if (n <= 0) {
                free(msg);
                return -1;
            }
            received += n;
        }
        msg[size] = '\0';
        printf("[TEXT] %s\n", msg);
        free(msg);
        return 0;
    }

    // [3] EXT LENGTH
    int ext_len = 0;
    if (SSL_read(ssl, &ext_len, sizeof(int)) <= 0 || ext_len <= 0 || ext_len > 10)
        return -1;

    // [4] EXTENSION 수신
    char ext[16] = {0};
    if (SSL_read(ssl, ext, ext_len) <= 0)
        return -1;
    ext[ext_len] = '\0';

    // [5] 디렉토리 준비
    mkdir("./received", 0755);

    char subdir[64] = {0};
    if (strncmp(type, "IMG", 3) == 0) {
        sprintf(subdir, "./received/images");
    } else if (strncmp(type, "VIDEO", 5) == 0) {
        sprintf(subdir, "./received/videos");
    } else {
        printf("[UNKNOWN TYPE] %s\n", type);
        return -1;
    }
    mkdir(subdir, 0755);

    // [6] 고유한 타임스탬프 파일 이름 생성
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm_info = localtime(&tv.tv_sec);

    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);

    char filename[256];
    if (strncmp(type, "IMG", 3) == 0) {
        snprintf(filename, sizeof(filename), "%s/%s_%06ld_img%s", subdir, timestamp, tv.tv_usec, ext);
    } else if (strncmp(type, "VIDEO", 5) == 0) {
        snprintf(filename, sizeof(filename), "%s/%s_%06ld_video%s", subdir, timestamp, tv.tv_usec, ext);
    }

    // [7] 파일 저장
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    int received = 0;
    char buf[2048];
    while (received < size) {
        int n = SSL_read(ssl, buf, sizeof(buf));
        if (n <= 0) {
            fclose(fp);
            return -1;
        }
        fwrite(buf, 1, n, fp);
        received += n;
    }

    fclose(fp);
    printf("[%s] saved to %s\n", type, filename);
    return 0;
}
