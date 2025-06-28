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

void sendFile(SSL *ssl, const char *filepath_or_data, const char *type) {
    // TEXT 전송: filepath_or_data는 단순 문자열
    if (strncmp(type, "TEXT", 4) == 0) {
        int size = strlen(filepath_or_data);
        SSL_write(ssl, "TEXT", 5);
        SSL_write(ssl, &size, sizeof(int));
        SSL_write(ssl, filepath_or_data, size);
        return;
    }

    // JSON 전송: filepath_or_data는 JSONL/JSON 문자열
    if (strncmp(type, "JSON", 4) == 0) {
        int size = strlen(filepath_or_data);
        SSL_write(ssl, "JSON", 5);
        SSL_write(ssl, &size, sizeof(int));
        SSL_write(ssl, filepath_or_data, size);
        return;
    }

    // DATA 전송 (이미지/영상 파일)
    FILE *fp = fopen(filepath_or_data, "rb");
    if (!fp) return;

    fseek(fp, 0, SEEK_END);
    int filesize = ftell(fp);
    rewind(fp);

    // 확장자 추출
    const char *ext = strrchr(filepath_or_data, '.');
    if (!ext || strlen(ext) > 10) ext = ".bin";
    int ext_len = strlen(ext);

    // [1] TYPE
    SSL_write(ssl, "DATA", 5);

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