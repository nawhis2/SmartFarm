#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int buzzer_fd = open("/dev/gpiobuz", O_WRONLY);
    if (buzzer_fd < 0) {
        perror("buzzer open 실패");
        return 1;
    }

    // 테스트용 부저 신호: 440Hz, 300ms
    const char* note = "440 300";
    write(buzzer_fd, note, strlen(note));

    close(buzzer_fd);
    return 0;
}
