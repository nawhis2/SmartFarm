#include "network.h"
#include "clientUtil.h"

#define BACKLOG 10

SSL* sock_fd = NULL;
static int uart_fd = -1;

int socketNetwork(const char *ipAddress, const int port){
    int sockfd;
	struct sockaddr_in socket_addr;

	struct hostent *he;
	if ((he = gethostbyname(ipAddress)) == NULL) {
        perror("gethostbyname");
        exit(1);
    }

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	socket_addr.sin_family = AF_INET;
	socket_addr.sin_port = htons(port);
	socket_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(socket_addr.sin_zero), '\0', 8);

    return sockfd;
}

int network(int sockfd, SSL_CTX *ctx)
{
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
	// 서버 연결
    if (connect(sockfd, (struct sockaddr*)&client_addr, addr_len) < 0) {
        perror("서버 연결 실패");
        return 0;
    }

    // SSL 객체 생성 및 핸드셰이크 수행
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        return 0;
    }
    sock_fd = ssl;

	return 1;
}

// UART 초기화
bool initUart(const char *dev, int baud)
{

  uart_fd = open(UART_DEV, O_RDWR | O_NOCTTY | O_SYNC);
  if (uart_fd < 0) {
    perror("UART open failed");
    return false;
  }


  struct termios tty;
  if (tcgetattr(uart_fd, &tty) != 0)
  {
    perror("tcgetattr");
    close(uart_fd);
    return false;
  }

  // 원시 모드 설정
  cfsetospeed(&tty, baud);
  cfsetispeed(&tty, baud);
  cfmakeraw(&tty);

  tty.c_cflag |= (CLOCAL | CREAD); // 로컬 연결, 수신 모드
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;      // 8 data bits
  tty.c_cflag &= ~PARENB;  // No parity
  tty.c_cflag &= ~CSTOPB;  // 1 stop bit
  tty.c_cflag &= ~CRTSCTS; // No flow control

  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 0;

  if (tcsetattr(uart_fd, TCSANOW, &tty) != 0)
  {
    perror("tcsetattr");
    close(uart_fd);
    return false;
  }
printf("[DEBUG] initUart succeeded\n");

  return true;
}

// UART 송신
bool sendUartCommand(const uint8_t *cmd, size_t len)
{
  if (uart_fd < 0)
    return false;
  ssize_t written = write(uart_fd, cmd, len);
  return written == (ssize_t)len;
}

// UART 수신 (select 기반)
int readUartResponse(uint8_t *resp, size_t maxlen, int timeout_ms)
{
  if (uart_fd < 0)
    return -1;

  fd_set readfds;
  struct timeval tv;
  FD_ZERO(&readfds);
  FD_SET(uart_fd, &readfds);

  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;

  int ret = select(uart_fd + 1, &readfds, NULL, NULL, &tv);
  if (ret <= 0)
    return -1; // timeout or error

  ssize_t len = read(uart_fd, resp, maxlen);
  return (len > 0) ? len : -1;
}

void handleControlCommand()
{
  uint8_t cmd[] = { CMD_MOVE_FORWARD };
  printf("[PI] 0xA1 명령 전송 중...\n");
bool success = sendUartCommand(cmd, sizeof(cmd));
printf("[PI] 명령 전송 %s\n", success ? "성공" : "실패");
  // if (!sendUartCommand(cmd, sizeof(cmd)))
  //   return;

  uint8_t resp[32];
  int rlen = readUartResponse(resp, sizeof(resp), 20000);
printf("[PI] 응답 길이: %d, 값: 0x%02X\n", rlen, resp[0]);

  if (rlen > 0 && resp[0] == ACK_CODE)
  {
    
    printf("Send msg to Server: done\n");
    sendFile("done", "TEXT");
  }
  else
  {
    printf("time out!\n");
    printf("Send msg to Server: error\n");
    sendFile("error", "TEXT");;
  }
}


void returnSocket(){
    SSL_shutdown(sock_fd);
    close(SSL_get_fd(sock_fd));
	SSL_free(sock_fd);
}

// UART 종료 함수
void closeUart() {
    if (uart_fd >= 0) {
        close(uart_fd);
        uart_fd = -1;
        printf("[DEBUG] UART closed\n");
    }
}