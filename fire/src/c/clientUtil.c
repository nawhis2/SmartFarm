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
#include "network.h"

void sendFile(const char *filepath_or_data, const char *type)
{
    int size = strlen(filepath_or_data);
    SSL_write(sock_fd, type, 5);
    SSL_write(sock_fd, &size, sizeof(int));
    SSL_write(sock_fd, filepath_or_data, size);
    return;
}

void sendData(const uchar* data, int size, const char* ext){
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