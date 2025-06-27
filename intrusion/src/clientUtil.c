/*
    [기능 이름]: OpenSSL TCP 메세지 수신
    [작성자]: 조찬우 (chanwoo)
    [파일 설명]:
      - OpenSSL용 TCP 수신 함수

    [참고 사항]:
      - OpenSSL에서 메세지를 보내는 함수의 원형
      - Send 프로토콜 형태 TYPE, SIZE, EXTENSION, DATA
*/
#include "clientUtil.h"

void sendFile(SSL *ssl, const char *filepath, const char *type) {
    if (strncmp(type, "TEXT", 4) == 0) {
        int size = strlen(filepath);
        SSL_write(ssl, "TEXT", 4);
        SSL_write(ssl, &size, sizeof(int));
        SSL_write(ssl, filepath, size);
        return;
    }

    FILE *fp = fopen(filepath, "rb");
    if (!fp) return;

    fseek(fp, 0, SEEK_END);
    int filesize = ftell(fp);
    rewind(fp);

    const char *ext = strrchr(filepath, '.');
    if (!ext || strlen(ext) > 10) ext = ".bin";

    int ext_len = strlen(ext);

    // [1] TYPE
    SSL_write(ssl, type, strlen(type));  // "TEXT", "IMG ", "VIDEO"

    // [2] SIZE
    SSL_write(ssl, &filesize, sizeof(int));

    // [3] EXTENSION LENGTH
    SSL_write(ssl, &ext_len, sizeof(int));

    // [4] EXTENSION
    SSL_write(ssl, ext, ext_len);

    // [5] DATA
    char buf[2048];
    int n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        SSL_write(ssl, buf, n);
    }

    fclose(fp);
}
