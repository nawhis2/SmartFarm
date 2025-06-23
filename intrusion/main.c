#include "main.h"
#include "client_util.h"

#define PORT 60000
#define BUF_SIZE 1024
#define BACKLOG 10

int sockfd;

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

SSL* network(SSL_CTX *ctx)
{
	struct sockaddr_in server_addr;

	struct hostent *he;
	if ((he = gethostbyname("vedaR")) == NULL) {
        perror("gethostbyname");
        exit(1);
    }

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(server_addr.sin_zero), '\0', 8);

	// 서버 연결
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("서버 연결 실패");
        exit(EXIT_FAILURE);
    }

    // SSL 객체 생성 및 핸드셰이크 수행
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
    }

	return ssl;
}

int main(){
    int num;
	char buffer[BUF_SIZE];
	
	sigset_t set;

    sigfillset(&set);
    sigdelset(&set, SIGINT);
    sigprocmask(SIG_SETMASK, &set, NULL);

	init_openssl();
    SSL_CTX *ctx = create_context();

	SSL* sock_fd = network(ctx);
	
	send_command(sock_fd, "exit");
	close(sockfd);
	SSL_shutdown(sock_fd);
    SSL_CTX_free(ctx);
	SSL_free(sock_fd);
	EVP_cleanup();

    return 0;
}
