#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>

#include "seqlock.h"  /* TODO implement this header file */

pthread_seqlock_t lock;

int main()
{
   int val = 0;
   int read_val;

   pthread_seqlock_init(&lock);

   pthread_seqlock_wrlock(&lock);
   val++;
   pthread_seqlock_wrunlock(&lock);

   while (1) {
      pthread_seqlock_rdlock(&lock);
      read_val = val;
      if (pthread_seqlock_rdunlock(&lock) == 0) {
         break;
      }
   }

   printf("val = %d\n", read_val);
   pthread_seqlock_destroy(&lock);
   return 0;
}
