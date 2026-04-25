#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_PROCESSES 3
#define TOTAL_RESOURCES 6

static const int max_need[NUM_PROCESSES] = {3, 3, 4};

static int allocated[NUM_PROCESSES] = {0};
static int completed[NUM_PROCESSES] = {0};
static int aborted[NUM_PROCESSES] = {0};
static int available_resources = TOTAL_RESOURCES;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static int remaining_need(int pid)
{
    return max_need[pid] - allocated[pid];
}

static int all_done(void)
{
    int i;

    for (i = 0; i < NUM_PROCESSES; i++) {
        if (!completed[i] && !aborted[i]) {
            return 0;
        }
    }

    return 1;
}

static int is_safe(void)
{
    int i;
    int progress;
    int work = available_resources;
    int finish[NUM_PROCESSES];

    for (i = 0; i < NUM_PROCESSES; i++) {
        finish[i] = completed[i] || aborted[i];
    }

    do {
        progress = 0;
        for (i = 0; i < NUM_PROCESSES; i++) {
            if (!finish[i] && remaining_need(i) <= work) {
                work += allocated[i];
                finish[i] = 1;
                progress = 1;
            }
        }
    } while (progress);

    for (i = 0; i < NUM_PROCESSES; i++) {
        if (!finish[i]) {
            return -1;
        }
    }

    return 0;
}

static void recover_from_unsafe(void)
{
    int i;
    int victim = -1;
    int max_alloc = -1;

    for (i = 0; i < NUM_PROCESSES; i++) {
        if (!completed[i] && !aborted[i] && allocated[i] > max_alloc) {
            max_alloc = allocated[i];
            victim = i;
        }
    }

    if (victim >= 0) {
        available_resources += allocated[victim];
        printf("Detector: abort process %d, release %d resources (available=%d)\n",
               victim, allocated[victim], available_resources);
        allocated[victim] = 0;
        aborted[victim] = 1;
    }
}

static void print_state(const char *tag)
{
    int i;

    printf("%s available=%d | ", tag, available_resources);
    for (i = 0; i < NUM_PROCESSES; i++) {
        printf("P%d alloc=%d need=%d %s%s", i,
               allocated[i],
               remaining_need(i),
               completed[i] ? "[done]" : "",
               aborted[i] ? "[aborted]" : "");
        if (i + 1 < NUM_PROCESSES) {
            printf("; ");
        }
    }
    printf("\n");
}

static void *process_worker(void *arg)
{
    int pid = *(int *)arg;

    while (1) {
        pthread_mutex_lock(&lock);

        if (completed[pid] || aborted[pid]) {
            pthread_mutex_unlock(&lock);
            break;
        }

        if (allocated[pid] < max_need[pid] && available_resources > 0) {
            allocated[pid]++;
            available_resources--;
            printf("Process %d allocated 1 (alloc=%d, need=%d, available=%d)\n",
                   pid, allocated[pid], remaining_need(pid), available_resources);
        }

        if (allocated[pid] == max_need[pid]) {
            completed[pid] = 1;
            available_resources += allocated[pid];
            printf("Process %d completed, released %d (available=%d)\n",
                   pid, allocated[pid], available_resources);
            allocated[pid] = 0;
            pthread_mutex_unlock(&lock);
            break;
        }

        pthread_mutex_unlock(&lock);
        usleep(250000);
    }

    return NULL;
}

static void *periodical_detector(void *arg)
{
    (void)arg;

    while (1) {
        sleep(5);

        pthread_mutex_lock(&lock);

        if (all_done()) {
            pthread_mutex_unlock(&lock);
            break;
        }

        print_state("Detector tick:");

        if (is_safe() < 0) {
            printf("Detector: unsafe state detected\n");
            recover_from_unsafe();
            print_state("After recovery:");
        }

        if (all_done()) {
            pthread_mutex_unlock(&lock);
            break;
        }

        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

int main(void)
{
    int i;
    int pids[NUM_PROCESSES];
    pthread_t workers[NUM_PROCESSES];
    pthread_t detector_tid;

    for (i = 0; i < NUM_PROCESSES; i++) {
        pids[i] = i;
        pthread_create(&workers[i], NULL, process_worker, &pids[i]);
    }

    pthread_create(&detector_tid, NULL, periodical_detector, NULL);

    for (i = 0; i < NUM_PROCESSES; i++) {
        pthread_join(workers[i], NULL);
    }
    pthread_join(detector_tid, NULL);

    pthread_mutex_destroy(&lock);

    printf("Simulation finished.\n");
    return 0;
}
