// uart_monitor.cpp
#include <wiringPi.h>
#include <wiringSerial.h>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <iostream>
#include <iomanip>

// ───────── 도우미: STDIN 논블로킹 전환 ─────────
class StdinNonBlock {
public:
    StdinNonBlock() {
        tcgetattr(STDIN_FILENO, &orig_);          // 현재 터미널 설정 저장
        termios t = orig_;
        t.c_lflag &= ~(ICANON | ECHO);            // 즉시 입력·에코 끄기
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        flags_ = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags_ | O_NONBLOCK);
    }
    ~StdinNonBlock() {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_); // 원상 복구
        fcntl(STDIN_FILENO, F_SETFL, flags_);
    }
private:
    termios orig_;
    int     flags_;
};
// ──────────────────────────────────────────────

#define CMD_MOVE_FORWARD    0xA1
#define CMD_MOVE_BACKWARD   0xA2
#define CMD_TURN_LEFT       0xA3
#define CMD_TURN_RIGHT      0xA4
#define CMD_STOP            0xA5

int main() {
    const char* device = "/dev/serial0";
    const int   baud   = 115200;

    if (wiringPiSetupGpio() == -1) {
        std::cerr << "wiringPi 초기화 실패\n";
        return 1;
    }
    int fd = serialOpen(device, baud);
    if (fd < 0) {
        std::cerr << "UART 오픈 실패: " << device << '\n';
        return 1;
    }

    StdinNonBlock kb;               // 프로그램 종료 시 자동 복구

    std::cout << "STM32-Pi 통신 모니터 시작\n"
              << "키보드 조작:\n"
              << "  q: 전진 (0xA1)\n"
              << "  w: 후진 (0xA2)\n"
              << "  e: 좌회전 (0xA3)\n"
              << "  r: 우회전 (0xA4)\n"
              << "  t: 정지   (0xA5)\n";

    while (true) {
        int key = getchar();            // 데이터 없으면 -1 즉시 반환

        uint8_t txByte = 0;
        switch (key) {
            case 'q': txByte = CMD_MOVE_FORWARD;   break;
            case 'w': txByte = CMD_MOVE_BACKWARD;  break;
            case 'e': txByte = CMD_TURN_LEFT;      break;
            case 'r': txByte = CMD_TURN_RIGHT;     break;
            case 't': txByte = CMD_STOP;           break;
            default:  txByte = 0;                  break;
        }
        if (txByte) {
            serialPutchar(fd, txByte);
            std::cout << "[TX] 0x" << std::hex << std::setw(2)
                      << std::setfill('0') << static_cast<int>(txByte) << std::dec << '\n';
        }

        // ── UART 수신 폴링 ──
        while (serialDataAvail(fd) > 0) {
            int rx = serialGetchar(fd); // int 형으로 들어옴
            std::cout << "[RX] 0x" << std::hex << std::setw(2)
                      << std::setfill('0') << rx << std::dec << '\n';
        }

        delay(1);   // 1 ms 슬립(≈1000 Hz)
    }

    serialClose(fd);
    return 0;
}
