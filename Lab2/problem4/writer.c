#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define SHARED_FILE "mmap_shared.bin"
#define MAP_SIZE 4096

struct shared_block {
    volatile int ready;
    pid_t writer_pid;
    time_t write_time;
    char message[256];
};

int main(void) {
    int fd;
    void *mapped;
    struct shared_block *block;

    fd = open(SHARED_FILE, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (ftruncate(fd, MAP_SIZE) == -1) {
        perror("ftruncate");
        close(fd);
        return 1;
    }

    mapped = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    block = (struct shared_block *)mapped;

    block->ready = 0;
    block->writer_pid = getpid();
    block->write_time = time(NULL);
    snprintf(block->message, sizeof(block->message),
             "Hello from writer process %d using mmap shared file!", (int)block->writer_pid);
    block->ready = 1;

    if (msync(mapped, MAP_SIZE, MS_SYNC) == -1) {
        perror("msync");
        munmap(mapped, MAP_SIZE);
        close(fd);
        return 1;
    }

    printf("Writer mapped file '%s' at %p\n", SHARED_FILE, mapped);
    printf("Writer wrote message: %s\n", block->message);
    printf("Writer keeps mapping alive for 10 seconds...\n");
    sleep(10);

    if (munmap(mapped, MAP_SIZE) == -1) {
        perror("munmap");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
