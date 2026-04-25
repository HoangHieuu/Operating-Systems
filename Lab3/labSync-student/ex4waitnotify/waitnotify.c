#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define PRODUCERS 2
#define ITEMS_PER_PRODUCER 10

static int buffer[BUFFER_SIZE];
static int head = 0;
static int tail = 0;
static int count = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

typedef struct {
    int id;
    int start_value;
} producer_arg_t;

static int write_item(int value)
{
    pthread_mutex_lock(&mutex);

    while (count == BUFFER_SIZE) {
        /* pthread_cond_wait releases mutex while waiting, then reacquires it. */
        pthread_cond_wait(&not_full, &mutex);
    }

    buffer[tail] = value;
    tail = (tail + 1) % BUFFER_SIZE;
    count++;

    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);

    return 0;
}

static int read_item(int *value)
{
    pthread_mutex_lock(&mutex);

    while (count == 0) {
        pthread_cond_wait(&not_empty, &mutex);
    }

    *value = buffer[head];
    head = (head + 1) % BUFFER_SIZE;
    count--;

    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&mutex);

    return 0;
}

static void *producer(void *arg)
{
    int i;
    producer_arg_t *cfg = (producer_arg_t *)arg;

    for (i = 0; i < ITEMS_PER_PRODUCER; i++) {
        int item = cfg->start_value + i;
        write_item(item);
        printf("Producer %d wrote %d\n", cfg->id, item);
        usleep(50000);
    }

    return NULL;
}

static void *consumer(void *arg)
{
    int i;
    int total = *(int *)arg;

    for (i = 0; i < total; i++) {
        int value;
        read_item(&value);
        printf("Consumer read %d\n", value);
        usleep(80000);
    }

    return NULL;
}

int main(void)
{
    int i;
    pthread_t prod_tid[PRODUCERS];
    pthread_t cons_tid;
    producer_arg_t args[PRODUCERS];
    int total_to_read = PRODUCERS * ITEMS_PER_PRODUCER;

    for (i = 0; i < PRODUCERS; i++) {
        args[i].id = i;
        args[i].start_value = i * ITEMS_PER_PRODUCER;
        pthread_create(&prod_tid[i], NULL, producer, &args[i]);
    }

    pthread_create(&cons_tid, NULL, consumer, &total_to_read);

    for (i = 0; i < PRODUCERS; i++) {
        pthread_join(prod_tid[i], NULL);
    }
    pthread_join(cons_tid, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&not_full);
    pthread_cond_destroy(&not_empty);

    return 0;
}
