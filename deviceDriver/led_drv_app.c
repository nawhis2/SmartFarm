#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#define DEVICE_PATH "/dev/ledctl"
int main() {
    char input[16];
    int fd;
    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }
    printf("Type 1 to turn LED ON, 0 to turn LED OFF. Ctrl+C to exit.\n");
    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) {
            break;  // EOF or error
        }
        // Remove newline character
        input[strcspn(input, "\n")] = 0;
        if (strcmp(input, "1") == 0) {
            write(fd, "L1", 2);
        } else if (strcmp(input, "0") == 0) {
            write(fd, "L0", 2);
        } else {
            printf("Invalid input. Type 1 or 0.\n");
        }
    }
    close(fd);
    return 0;
}