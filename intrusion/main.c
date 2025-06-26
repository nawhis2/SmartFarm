/*
    [기능 이름]: 침입 감지 client
    [작성자]: 조찬우 (chanwoo)
    [파일 설명]:
      - 침입 감지 client 코드

    [TODO]:
      - [ ] 침입 감지 추가
      - [ ] while문 추가

    [참고 사항]:
*/

#include "main.h"
#include "clientUtil.h"
#include "network.h"
#include "handshake.h"

#define BUF_SIZE 1024

int main(){
    int num;
	char buffer[BUF_SIZE];
	
	sigset_t set;
    
    sigfillset(&set);
    sigdelset(&set, SIGINT);
    sigprocmask(SIG_SETMASK, &set, NULL);
    
	init_openssl();
    SSL_CTX *ctx = create_context();
    
    int sockfd = socketNetwork();
	SSL* sock_fd = network(sockfd, ctx);

    handshakeClient(sock_fd, "CCTV", "Intrusion");

	send_file(sock_fd, "hi Server\n", "TEXT");
    send_file(sock_fd, "/home/chanwoo/images/opencv.png", "IMG");
    send_file(sock_fd, "/home/chanwoo/images/trailer.mp4", "VIDEO");
    sleep(1);

    while(1);

	close(sockfd);
	SSL_shutdown(sock_fd);
    SSL_CTX_free(ctx);
	SSL_free(sock_fd);
	EVP_cleanup();
    
    return 0;
}

// OpenSSL 초기화
void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

// SSL 컨텍스트 생성 (클라이언트용)
SSL_CTX *create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_client_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("SSL 컨텍스트 생성 실패");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}
