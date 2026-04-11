#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SHARED_FILE "mmap.bin"
#define MAP_SIZE 4096

struct shared_data {
    volatile int ready;
    pid_t writer_pid;
    char message[256];
};

int main(void) {
    int fd = open(SHARED_FILE, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (ftruncate(fd, MAP_SIZE) == -1) {
        perror("ftruncate");
        close(fd);
        return 1;
    }

    void *mapped = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    struct shared_data *data = (struct shared_data *)mapped;
    memset(data, 0, sizeof(*data));

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        munmap(mapped, MAP_SIZE);
        close(fd);
        return 1;
    }

    if (pid == 0) {
        struct timespec pause_time = {0, 100000000L};
        while (!data->ready) {
            nanosleep(&pause_time, NULL);
        }

        printf("Child process read from mmap:\n");
        printf("Message: %s\n", data->message);
        printf("Written by PID: %d\n", (int)data->writer_pid);

        munmap(mapped, MAP_SIZE);
        close(fd);
        return 0;
    }

    data->writer_pid = getpid();
    snprintf(
        data->message,
        sizeof(data->message),
        "Hello from parent process %d via mmap file mapping.",
        (int)data->writer_pid
    );
    data->ready = 1;

    if (msync(mapped, MAP_SIZE, MS_SYNC) == -1) {
        perror("msync");
        munmap(mapped, MAP_SIZE);
        close(fd);
        return 1;
    }

    printf("Parent wrote message to mapped file: %s\n", SHARED_FILE);

    wait(NULL);

    munmap(mapped, MAP_SIZE);
    close(fd);
    return 0;
}
