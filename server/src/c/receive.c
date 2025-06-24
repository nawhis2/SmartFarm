#include "main.h"
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int receive_packet(SSL *ssl)
{
    char type[5] = {0};
    int size = 0;

    // [1] TYPE 수신
    int t = SSL_read(ssl, type, 4);
    if (t <= 0) {
        int err = SSL_get_error(ssl, t);
        if (err == SSL_ERROR_ZERO_RETURN || t == 0) {
            printf("[DISCONNECTED] Client closed the connection.\n");
            return 1;  // 정상 종료
        } else {
            printf("[ERROR] Failed to read TYPE. err = %d\n", err);
            ERR_print_errors_fp(stderr);
            return -1;
        }
    }

    // [2] SIZE 수신
    int s = SSL_read(ssl, &size, sizeof(int));
    if (s <= 0 || size <= 0) {
        int err = SSL_get_error(ssl, s);
        printf("[ERROR] Failed to read SIZE. err = %d\n", err);
        ERR_print_errors_fp(stderr);
        return -1;
    }

    // [3] TEXT 처리
    if (strncmp(type, "TEXT", 4) == 0) {
        char *msg = (char *)malloc(size + 1);
        if (!msg) return -1;

        int received = 0;
        while (received < size) {
            int n = SSL_read(ssl, msg + received, size - received);
            if (n <= 0) {
                printf("[ERROR] Disconnected during TEXT read.\n");
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

    // [4] IMAGE 처리
    else if (strncmp(type, "IMG ", 4) == 0) {
        FILE *fp = fopen("received_image.jpg", "wb");
        if (!fp) {
            perror("fopen");
            return -1;
        }

        int received = 0;
        char buf[1024];
        while (received < size) {
            int n = SSL_read(ssl, buf, sizeof(buf));
            if (n <= 0) {
                printf("[ERROR] Disconnected during IMAGE read.\n");
                fclose(fp);
                return -1;
            }
            fwrite(buf, 1, n, fp);
            received += n;
        }
        fclose(fp);

        display_image("received_image.jpg");
        return 0;
    }

    else {
        printf("[UNKNOWN TYPE] %s\n", type);
        return -1;
    }
}
