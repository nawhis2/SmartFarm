/*
    [기능 이름]: OpenSSL TCP 메세지 수신
    [작성자]: 조찬우 (chanwoo)
    [파일 설명]:
      - OpenSSL용 TCP 수신 함수

    [참고 사항]:
      - OpenSSL에서 메세지를 보내는 함수의 원형
      - Send 프로토콜 형태 TYPE, SIZE, DATA
*/
#include "clientUtil.h"

void send_command(SSL *sock, const char *msg)
{
    int msg_len = strlen(msg);
    
    SSL_write(sock, "TEXT", 4);

    SSL_write(sock, &msg_len, sizeof(int));

    int ret = SSL_write(sock, msg, msg_len);
    if (ret <= 0)
    {
        int err = SSL_get_error(sock, ret);
        printf("SSL_write failed: %d\n", err);
        ERR_print_errors_fp(stderr);
    }
}

void send_image(SSL *sock, const char *filepath)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        perror("fopen failed");
        return;
    }

    fseek(fp, 0, SEEK_END);
    int filesize = ftell(fp);
    rewind(fp);

    SSL_write(sock, "IMG ", 4);

    SSL_write(sock, &filesize, sizeof(int));

    char buf[1024];
    int n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        SSL_write(sock, buf, n);
    }

    fclose(fp);
}
