#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdint.h>

#define NUM_MOVIES 1682
#define FILE1 "movie-100k_1.txt"
#define FILE2 "movie-100k_2.txt"

struct SharedData {
    double sums1[NUM_MOVIES + 1];
    int counts1[NUM_MOVIES + 1];
    double sums2[NUM_MOVIES + 1];
    int counts2[NUM_MOVIES + 1];
};

void process_file(const char *filename, double *sums, int *counts) {
    FILE *fp = fopen(filename, "r"); 
    if (!fp) {
        perror("fopen");
        exit(1);
    }
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        int user, movie, rating;
        long timestamp;
        if (sscanf(line, "%d\t%d\t%d\t%ld", &user, &movie, &rating, &timestamp) == 4) {
            if (movie >= 1 && movie <= NUM_MOVIES) {
                sums[movie] += rating;
                counts[movie]++;
            }
        }
    }
    fclose(fp);
}

int main() {
    key_t key_shm = ftok(".", 'a');
    int shmid = shmget(key_shm, sizeof(struct SharedData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    struct SharedData *data = shmat(shmid, NULL, 0);
    if ((intptr_t)data == -1) {
        perror("shmat");
        exit(1);
    }
    memset(data, 0, sizeof(struct SharedData));

    pid_t child1 = fork();
    if (child1 == 0) {
        // child1
        struct SharedData *data_c = shmat(shmid, NULL, 0);
        if ((intptr_t)data_c == -1) {
            perror("shmat");
            exit(1);
        }
        process_file(FILE1, data_c->sums1, data_c->counts1);
        shmdt(data_c);
        exit(0);
    }

    pid_t child2 = fork();
    if (child2 == 0) {
        // child2
        struct SharedData *data_c = shmat(shmid, NULL, 0);
        if ((intptr_t)data_c == -1) {
            perror("shmat");
            exit(1);
        }
        process_file(FILE2, data_c->sums2, data_c->counts2);
        shmdt(data_c);
        exit(0);
    }

    // parent waits for both children finish their work
    wait(NULL);
    wait(NULL);

    // compute and print averages
    for (int i = 1; i <= NUM_MOVIES; i++) {
        double total_sum = data->sums1[i] + data->sums2[i];
        int total_count = data->counts1[i] + data->counts2[i];
        if (total_count > 0) {
            printf("Movie %d: %.4f\n", i, total_sum / total_count);
        }
    }

    shmdt(data);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}
