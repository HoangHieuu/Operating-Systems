#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define NUM_MOVIES 1682
#define NUM_CHILDREN 2
#define FILE1 "movie-100k_1.txt"
#define FILE2 "movie-100k_2.txt"

struct SharedData {
    long long sums[NUM_CHILDREN][NUM_MOVIES + 1];
    int counts[NUM_CHILDREN][NUM_MOVIES + 1];
    long total_rows[NUM_CHILDREN];
    long valid_rows[NUM_CHILDREN];
};

static void process_file(
    const char *filename,
    long long *sums,
    int *counts,
    long *total_rows,
    long *valid_rows
) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        int user;
        int movie;
        int rating;
        long timestamp;

        (*total_rows)++;

        if (sscanf(line, "%d\t%d\t%d\t%ld", &user, &movie, &rating, &timestamp) != 4) {
            continue;
        }
        if (movie < 1 || movie > NUM_MOVIES) {
            continue;
        }

        sums[movie] += rating;
        counts[movie]++;
        (*valid_rows)++;
    }

    fclose(fp);
}

int main(void) {
    key_t key = ftok(".", 'a');
    if (key == -1) {
        perror("ftok");
        return EXIT_FAILURE;
    }

    int shmid = shmget(key, sizeof(struct SharedData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        return EXIT_FAILURE;
    }

    struct SharedData *data = shmat(shmid, NULL, 0);
    if (data == (void *) -1) {
        perror("shmat");
        return EXIT_FAILURE;
    }
    memset(data, 0, sizeof(*data));

    pid_t child1 = fork();
    if (child1 < 0) {
        perror("fork");
        shmdt(data);
        shmctl(shmid, IPC_RMID, NULL);
        return EXIT_FAILURE;
    }
    if (child1 == 0) {
        process_file(
            FILE1,
            data->sums[0],
            data->counts[0],
            &data->total_rows[0],
            &data->valid_rows[0]
        );
        shmdt(data);
        _exit(EXIT_SUCCESS);
    }

    pid_t child2 = fork();
    if (child2 < 0) {
        perror("fork");
        wait(NULL);
        shmdt(data);
        shmctl(shmid, IPC_RMID, NULL);
        return EXIT_FAILURE;
    }
    if (child2 == 0) {
        process_file(
            FILE2,
            data->sums[1],
            data->counts[1],
            &data->total_rows[1],
            &data->valid_rows[1]
        );
        shmdt(data);
        _exit(EXIT_SUCCESS);
    }

    wait(NULL);
    wait(NULL);

    printf("=== Movie Rating Statistics (combined from 2 files) ===\n");
    printf("%-8s %-10s %-10s\n", "MovieID", "Ratings", "Average");

    long total_ratings = 0;
    int rated_movies = 0;

    for (int movie_id = 1; movie_id <= NUM_MOVIES; movie_id++) {
        long long total_sum = data->sums[0][movie_id] + data->sums[1][movie_id];
        int total_count = data->counts[0][movie_id] + data->counts[1][movie_id];

        if (total_count > 0) {
            double avg = (double) total_sum / (double) total_count;
            printf("%-8d %-10d %-10.4f\n", movie_id, total_count, avg);
            total_ratings += total_count;
            rated_movies++;
        }
    }

    shmdt(data);
    shmctl(shmid, IPC_RMID, NULL);
    return EXIT_SUCCESS;
}
