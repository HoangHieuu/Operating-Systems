#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define PRODUCER_THREADS 4
#define ITEMS_PER_PRODUCER 5000
#define TOTAL_ITEMS (PRODUCER_THREADS * ITEMS_PER_PRODUCER)

typedef struct node {
    int value;
    struct node *next;
} node_t;

typedef struct LockFreeStack {
    _Atomic(node_t *) head;
} LockFreeStack;

static LockFreeStack stack;
static atomic_long popped_count = 0;
static atomic_llong popped_sum = 0;

static void stack_init(LockFreeStack *s)
{
    atomic_store_explicit(&s->head, NULL, memory_order_relaxed);
}

/* Treiber stack push using compare-and-swap. */
static bool push(LockFreeStack *s, int value)
{
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    node_t *current_head;

    if (new_node == NULL) {
        return false;
    }

    new_node->value = value;

    do {
        current_head = atomic_load_explicit(&s->head, memory_order_acquire);
        new_node->next = current_head;
    } while (!atomic_compare_exchange_weak_explicit(
        &s->head,
        &current_head,
        new_node,
        memory_order_release,
        memory_order_acquire));

    return true;
}

/*
 * Pop is lock-free. This lab demo uses a single consumer thread to avoid
 * introducing advanced memory reclamation techniques.
 */
static bool pop(LockFreeStack *s, int *value)
{
    node_t *current_head;
    node_t *next;

    do {
        current_head = atomic_load_explicit(&s->head, memory_order_acquire);
        if (current_head == NULL) {
            return false;
        }
        next = current_head->next;
    } while (!atomic_compare_exchange_weak_explicit(
        &s->head,
        &current_head,
        next,
        memory_order_acq_rel,
        memory_order_acquire));

    *value = current_head->value;
    free(current_head);
    return true;
}

static void stack_destroy(LockFreeStack *s)
{
    int value;
    while (pop(s, &value)) {
    }
}

static void *producer(void *arg)
{
    int id = *(int *)arg;
    int i;

    for (i = 0; i < ITEMS_PER_PRODUCER; i++) {
        int value = (id * ITEMS_PER_PRODUCER) + i + 1;
        if (!push(&stack, value)) {
            fprintf(stderr, "push failed for value %d\n", value);
            return NULL;
        }
    }

    return NULL;
}

static void *consumer(void *arg)
{
    (void)arg;

    while (atomic_load_explicit(&popped_count, memory_order_acquire) < TOTAL_ITEMS) {
        int value;
        if (pop(&stack, &value)) {
            atomic_fetch_add_explicit(&popped_count, 1, memory_order_acq_rel);
            atomic_fetch_add_explicit(&popped_sum, value, memory_order_acq_rel);
        } else {
            sched_yield();
        }
    }

    return NULL;
}

int main(void)
{
    int i;
    int ids[PRODUCER_THREADS];
    pthread_t producers[PRODUCER_THREADS];
    pthread_t consumer_tid;
    long count;
    long long sum;
    long long expected_sum;

    stack_init(&stack);

    pthread_create(&consumer_tid, NULL, consumer, NULL);

    for (i = 0; i < PRODUCER_THREADS; i++) {
        ids[i] = i;
        pthread_create(&producers[i], NULL, producer, &ids[i]);
    }

    for (i = 0; i < PRODUCER_THREADS; i++) {
        pthread_join(producers[i], NULL);
    }
    pthread_join(consumer_tid, NULL);

    count = atomic_load_explicit(&popped_count, memory_order_acquire);
    sum = atomic_load_explicit(&popped_sum, memory_order_acquire);
    expected_sum = ((long long)TOTAL_ITEMS * (TOTAL_ITEMS + 1)) / 2;

    printf("Popped count: %ld (expected %d)\n", count, TOTAL_ITEMS);
    printf("Popped sum:   %lld (expected %lld)\n", sum, expected_sum);

    if (count == TOTAL_ITEMS && sum == expected_sum) {
        printf("Lock-free stack check: PASS\n");
    } else {
        printf("Lock-free stack check: FAIL\n");
    }

    stack_destroy(&stack);
    return 0;
}
