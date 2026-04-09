#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
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
    int waited = 0;

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

    printf("Reader mapped file '%s' at %p\n", SHARED_FILE, mapped);
    printf("Reader waiting for writer data...\n");

    while (block->ready != 1 && waited < 20) {
        sleep(1);
        waited++;
    }

    if (block->ready != 1) {
        fprintf(stderr, "Timed out waiting for writer.\n");
        munmap(mapped, MAP_SIZE);
        close(fd);
        return 1;
    }

    printf("Reader got message: %s\n", block->message);
    printf("Written by PID: %d\n", (int)block->writer_pid);
    printf("Write time: %s", ctime(&block->write_time));

    if (munmap(mapped, MAP_SIZE) == -1) {
        perror("munmap");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
