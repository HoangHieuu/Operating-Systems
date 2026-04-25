#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LOG_LENGTH 10
#define MAX_BUFFER_SLOT 5
#define MAX_LOOPS 30

char **logbuf;

static int count = 0;
static int stop_flush = 0;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

struct _args {
   unsigned int interval;
};

int wrlog(char **buf, char *new_data)
{
   if (buf == NULL || new_data == NULL) {
      return -1;
   }

   pthread_mutex_lock(&mtx);

   while (count == MAX_BUFFER_SLOT && !stop_flush) {
      pthread_cond_wait(&not_full, &mtx);
   }

   if (stop_flush) {
      pthread_mutex_unlock(&mtx);
      return -1;
   }

   strncpy(buf[count], new_data, MAX_LOG_LENGTH - 1);
   buf[count][MAX_LOG_LENGTH - 1] = '\0';
   count++;

   pthread_cond_signal(&not_empty);
   pthread_mutex_unlock(&mtx);
   return 0;
}

int flushlog(char **buf)
{
   int i;

   if (buf == NULL) {
      return -1;
   }

   pthread_mutex_lock(&mtx);

   while (count == 0 && !stop_flush) {
      pthread_cond_wait(&not_empty, &mtx);
   }

   if (count == 0 && stop_flush) {
      pthread_mutex_unlock(&mtx);
      return 0;
   }

   for (i = 0; i < count; i++) {
      printf("Slot  %d: %s\n", i, buf[i]);
      buf[i][0] = '\0';
   }
   fflush(stdout);

   count = 0;
   pthread_cond_broadcast(&not_full);
   pthread_mutex_unlock(&mtx);
   return 0;
}

void *wrlog_worker(void *data)
{
   char str[MAX_LOG_LENGTH];
   int id = *(int *)data;

   usleep(20);
   snprintf(str, sizeof(str), "%d", id);
   wrlog(logbuf, str);

   return NULL;
}

void *timer_start(void *args)
{
   while (1) {
      usleep(((struct _args *)args)->interval);
      flushlog(logbuf);

      pthread_mutex_lock(&mtx);
      if (stop_flush && count == 0) {
         pthread_mutex_unlock(&mtx);
         break;
      }
      pthread_mutex_unlock(&mtx);
   }

   return NULL;
}

int main(void)
{
   int i;
   pthread_t tid[MAX_LOOPS];
   pthread_t lgrid;
   int id[MAX_LOOPS];
   struct _args args;

   args.interval = 500000;

   logbuf = malloc(MAX_BUFFER_SLOT * sizeof(char *));
   if (logbuf == NULL) {
      fprintf(stderr, "Error: log buffer allocation failed.\n");
      return EXIT_FAILURE;
   }

   for (i = 0; i < MAX_BUFFER_SLOT; i++) {
      logbuf[i] = calloc((size_t)MAX_LOG_LENGTH, sizeof(char));
      if (logbuf[i] == NULL) {
         fprintf(stderr, "Error: log slot allocation failed.\n");
         while (--i >= 0) {
            free(logbuf[i]);
         }
         free(logbuf);
         return EXIT_FAILURE;
      }
   }

   pthread_create(&lgrid, NULL, timer_start, (void *)&args);

   for (i = 0; i < MAX_LOOPS; i++) {
      id[i] = i;
      pthread_create(&tid[i], NULL, wrlog_worker, (void *)&id[i]);
   }

   for (i = 0; i < MAX_LOOPS; i++) {
      pthread_join(tid[i], NULL);
   }

   pthread_mutex_lock(&mtx);
   stop_flush = 1;
   pthread_cond_broadcast(&not_empty);
   pthread_cond_broadcast(&not_full);
   pthread_mutex_unlock(&mtx);

   pthread_join(lgrid, NULL);

   pthread_mutex_destroy(&mtx);
   pthread_cond_destroy(&not_full);
   pthread_cond_destroy(&not_empty);

   for (i = 0; i < MAX_BUFFER_SLOT; i++) {
      free(logbuf[i]);
   }
   free(logbuf);

   return EXIT_SUCCESS;
}
