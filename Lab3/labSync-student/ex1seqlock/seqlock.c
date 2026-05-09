#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "seqlock.h"  /* TODO implement this header file */

pthread_seqlock_t lock;

static int val = 0;

/* Testcase 1: Writer holds the lock while reader attempts to read.
 * Expected: reader will wait until writer finishes, then read updated value.
 */
void *writer_hold(void *arg)
{
   (void)arg;
   pthread_seqlock_wrlock(&lock);
   printf("[writer_hold] acquired write lock\n");
   usleep(200 * 1000); /* hold for 200ms before updating */
   val = 1;
   printf("[writer_hold] updated val -> %d (still holding)\n", val);
   usleep(200 * 1000); /* hold a bit longer while reader attempts */
   pthread_seqlock_wrunlock(&lock);
   printf("[writer_hold] released write lock\n");
   return NULL;
}

void *reader_wait_for_writer(void *arg)
{
   (void)arg;
   int r;
   pthread_seqlock_rdlock(&lock);
   r = val;
   int rc = pthread_seqlock_rdunlock(&lock);
   printf("[reader_wait_for_writer] read val = %d, rdunlock rc=%d\n", r, rc);
   return NULL;
}

/* Testcase 2: Reader starts reading, then writer starts writing.
 * Expected: first read attempt will detect concurrent writer (EAGAIN) and retry.
 */
void *reader_may_be_interrupted(void *arg)
{
   (void)arg;
   int attempt = 0;
   while (1) {
      attempt++;
      pthread_seqlock_rdlock(&lock);
      int r = val;
      /* simulate long read to allow writer to start */
      usleep(300 * 1000);
      int rc = pthread_seqlock_rdunlock(&lock);
      if (rc == 0) {
         printf("[reader_may_be_interrupted] success on attempt %d: read val=%d\n", attempt, r);
         break;
      } else {
         printf("[reader_may_be_interrupted] attempt %d: detected concurrent writer (rc=%d), retrying...\n", attempt, rc);
         usleep(100 * 1000);
      }
   }
   return NULL;
}

void *writer_interrupt(void *arg)
{
   (void)arg;
   /* wait briefly so reader enters read section first */
   usleep(50 * 1000);
   pthread_seqlock_wrlock(&lock);
   printf("[writer_interrupt] acquired write lock (interrupting)\n");
   val = 2;
   printf("[writer_interrupt] updated val -> %d\n", val);
   usleep(100 * 1000);
   pthread_seqlock_wrunlock(&lock);
   printf("[writer_interrupt] released write lock\n");
   return NULL;
}

int main()
{
   pthread_t w, r;

   pthread_seqlock_init(&lock);

   printf("==== Testcase 1: writer holds lock while reader attempts to read ====");
   printf("\n");
   val = 0;
   pthread_create(&w, NULL, writer_hold, NULL);
   usleep(100 * 1000); /* ensure writer starts first */
   pthread_create(&r, NULL, reader_wait_for_writer, NULL);
   pthread_join(w, NULL);
   pthread_join(r, NULL);

   printf("\n==== Testcase 2: reader starts reading and writer enters ====");
   printf("\n");
   val = 0;
   pthread_t r2, w2;
   pthread_create(&r2, NULL, reader_may_be_interrupted, NULL);
   pthread_create(&w2, NULL, writer_interrupt, NULL);
   pthread_join(r2, NULL);
   pthread_join(w2, NULL);

   pthread_seqlock_destroy(&lock);
   return 0;
}
