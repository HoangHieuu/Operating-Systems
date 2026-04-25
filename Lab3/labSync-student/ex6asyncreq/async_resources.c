#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_RESOURCES 5
#define NUM_PROCESSES 7

typedef struct {
    int id;
    int requested_resources;
    void (*callback)(int);
} process_request_t;

typedef struct request_node {
    process_request_t req;
    struct request_node *next;
} request_node_t;

static int available_resources = NUM_RESOURCES;
static int submissions_done = 0;
static int active_processes = 0;

static pthread_mutex_t resource_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t resource_cond = PTHREAD_COND_INITIALIZER;

static request_node_t *queue_head = NULL;
static request_node_t *queue_tail = NULL;

static void enqueue_request(request_node_t *node)
{
    node->next = NULL;
    if (queue_tail == NULL) {
        queue_head = node;
        queue_tail = node;
        return;
    }

    queue_tail->next = node;
    queue_tail = node;
}

static request_node_t *dequeue_request(void)
{
    request_node_t *node = queue_head;

    if (node == NULL) {
        return NULL;
    }

    queue_head = queue_head->next;
    if (queue_head == NULL) {
        queue_tail = NULL;
    }

    return node;
}

static void resource_callback(int process_id)
{
    printf("Callback: process %d has been allocated resources\n", process_id);
}

static void submit_request(process_request_t req)
{
    request_node_t *node = (request_node_t *)malloc(sizeof(request_node_t));

    if (node == NULL) {
        fprintf(stderr, "submit_request: malloc failed\n");
        return;
    }

    node->req = req;

    pthread_mutex_lock(&resource_lock);
    enqueue_request(node);
    printf("Process %d submitted request for %d resources (non-blocking)\n",
           req.id, req.requested_resources);
    pthread_cond_broadcast(&resource_cond);
    pthread_mutex_unlock(&resource_lock);
}

static void *run_process(void *arg)
{
    process_request_t req = *(process_request_t *)arg;
    free(arg);

    sleep((unsigned int)(1 + (req.id % 3)));

    pthread_mutex_lock(&resource_lock);
    available_resources += req.requested_resources;
    active_processes--;
    printf("Process %d released %d resources (available=%d)\n",
           req.id, req.requested_resources, available_resources);
    pthread_cond_broadcast(&resource_cond);
    pthread_mutex_unlock(&resource_lock);

    return NULL;
}

static void *resource_manager(void *arg)
{
    (void)arg;

    pthread_mutex_lock(&resource_lock);

    while (1) {
        while (queue_head == NULL && !(submissions_done && active_processes == 0)) {
            pthread_cond_wait(&resource_cond, &resource_lock);
        }

        if (queue_head == NULL && submissions_done && active_processes == 0) {
            pthread_mutex_unlock(&resource_lock);
            return NULL;
        }

        while (queue_head != NULL &&
               queue_head->req.requested_resources <= available_resources) {
            request_node_t *node;
            process_request_t req;
            process_request_t *req_copy;
            pthread_t tid;

            node = dequeue_request();
            req = node->req;
            free(node);

            available_resources -= req.requested_resources;
            active_processes++;

            printf("Manager: allocated %d resources to process %d (available=%d)\n",
                   req.requested_resources, req.id, available_resources);

            req.callback(req.id);

            req_copy = (process_request_t *)malloc(sizeof(process_request_t));
            if (req_copy == NULL) {
                fprintf(stderr, "resource_manager: malloc failed\n");
                available_resources += req.requested_resources;
                active_processes--;
                pthread_cond_broadcast(&resource_cond);
                continue;
            }

            *req_copy = req;
            if (pthread_create(&tid, NULL, run_process, req_copy) != 0) {
                fprintf(stderr, "resource_manager: pthread_create failed\n");
                free(req_copy);
                available_resources += req.requested_resources;
                active_processes--;
                pthread_cond_broadcast(&resource_cond);
                continue;
            }

            pthread_detach(tid);
        }

        if (queue_head != NULL &&
            queue_head->req.requested_resources > available_resources) {
            /* FIFO fairness: wait for enough resources for the oldest request. */
            pthread_cond_wait(&resource_cond, &resource_lock);
        }
    }
}

static void *requester_thread(void *arg)
{
    int id = *(int *)arg;
    int request_table[NUM_PROCESSES] = {3, 2, 4, 1, 2, 3, 1};
    process_request_t req;

    req.id = id;
    req.requested_resources = request_table[id];
    req.callback = resource_callback;

    submit_request(req);
    printf("Process %d continues running after async request\n", id);
    usleep((unsigned int)(120000 + id * 30000));

    return NULL;
}

int main(void)
{
    int i;
    int pids[NUM_PROCESSES];
    pthread_t manager_tid;
    pthread_t requester_tid[NUM_PROCESSES];

    pthread_create(&manager_tid, NULL, resource_manager, NULL);

    for (i = 0; i < NUM_PROCESSES; i++) {
        pids[i] = i;
        pthread_create(&requester_tid[i], NULL, requester_thread, &pids[i]);
        usleep(80000);
    }

    for (i = 0; i < NUM_PROCESSES; i++) {
        pthread_join(requester_tid[i], NULL);
    }

    pthread_mutex_lock(&resource_lock);
    submissions_done = 1;
    pthread_cond_broadcast(&resource_cond);
    pthread_mutex_unlock(&resource_lock);

    pthread_join(manager_tid, NULL);

    pthread_mutex_destroy(&resource_lock);
    pthread_cond_destroy(&resource_cond);

    return 0;
}
