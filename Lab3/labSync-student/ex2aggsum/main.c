#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* for getopt */
#include <pthread.h>

#include "utils.h"
#include <errno.h>
#include <limits.h>

/** process command line argument.
 *  values are made available through the 'conf' struct.
 *  using the parsed conf to get arguments: the arrsz, tnum, and seednum
 */
extern int processopts (int argc, char **argv, struct _appconf *conf); 

/** process string to number.
 *  string is stored in 'nptr' char array.
 *  'num' is returned the valid integer number.
 *  return 0 valid number stored in num
 *        -1 invalid and num is useless value.
 */
extern int tonum (const char * nptr, int * num);

/** validate the array size argument.
 *  the size must be splitable "num_thread".
 */
extern int validate_and_split_argarray (int arraysize, int num_thread, struct _range* thread_idx_range);

/** generate "arraysize" data for the array "buf"
 *  validate the array size argument.
 *  the size must be splitable "num_thread".
 */
extern int generate_array_data (int* buf, int arraysize, int seednum);

/** display help */
extern void help (int xcode);

void* sum_worker (void *arg);
long validate_sum(int arraysize);

/* Global sum buffer */
long sumbuf = 0;
int* shrdarrbuf;
pthread_mutex_t mtx;

void* sum_worker (void *arg) {
   int i;
   long local_sum = 0;
   struct _range *idx_range = (struct _range *)arg;

   for (i = idx_range->start; i <= idx_range->end; i++)
      local_sum += shrdarrbuf[i];

   pthread_mutex_lock(&mtx);
   sumbuf += local_sum;
   pthread_mutex_unlock(&mtx);

   return NULL;
}

int main(int argc, char * argv[]) {
   int i;
   long seqsum;
   struct _range* thread_idx_range;
   pthread_t* tid;
   int rc;

    if (argc < 3 || argc > 4) /* only accept 2 or 3 arguments */
      help(EXIT_SUCCESS);

#if(DBGSTDERR == 1)
   freopen("/dev/null","w",stderr); /* redirect stderr by default */
#endif
   
   if (processopts(argc, argv, &appconf) != 0) {
      fprintf(stderr, "Invalid input arguments.\n");
      help(EXIT_FAILURE);
   }

   fprintf(stdout,"%s runs with %s=%d \t %s=%d \t %s=%d\n", PACKAGE,
                  ARG1, appconf.arrsz, ARG2, appconf.tnum, ARG3, appconf.seednum);

   thread_idx_range = malloc(appconf.tnum * sizeof(struct _range));
   if(thread_idx_range == NULL)
   {
      printf("Error! memory for index storage not allocated.\n");
      return EXIT_FAILURE;
   }
	
   if (validate_and_split_argarray(appconf.arrsz, appconf.tnum, thread_idx_range) < 0)
   {
      printf("Error! array index not splitable. Each partition need at least %d item\n", THRSL_MIN);
      free(thread_idx_range);
      return EXIT_FAILURE;
   }
    
   /* Generate array data */
   shrdarrbuf = malloc(appconf.arrsz * sizeof(int));
   if(shrdarrbuf == NULL)
   {
      printf("Error! memory for array buffer not allocated.\n");
      free(thread_idx_range);
      return EXIT_FAILURE;
   }
 
   if(generate_array_data(shrdarrbuf, appconf.arrsz, appconf.seednum) < 0) 
   {
      printf("Error! array index not splitable.\n");
      free(shrdarrbuf);
      free(thread_idx_range);
      return EXIT_FAILURE;
   }
   seqsum = validate_sum(appconf.arrsz);
   printf("sequence sum results %ld\n", seqsum);
	   
   /** Create <tnum> thead to calculate partial non-volatile sum
    *  the non-volatile mechanism require value added to global sum buffer
    */
   tid = malloc (appconf.tnum * sizeof(pthread_t));
   if (tid == NULL) {
      printf("Error! memory for thread ids not allocated.\n");
      free(shrdarrbuf);
      free(thread_idx_range);
      return EXIT_FAILURE;
   }

   rc = pthread_mutex_init(&mtx, NULL);
   if (rc != 0) {
      fprintf(stderr, "Error! pthread_mutex_init failed (%d)\n", rc);
      free(tid);
      free(shrdarrbuf);
      free(thread_idx_range);
      return EXIT_FAILURE;
   }

   for (i = 0; i < appconf.tnum; i++) {
      rc = pthread_create(&tid[i], NULL, sum_worker, (void *)&thread_idx_range[i]);
      if (rc != 0) {
         fprintf(stderr, "Error! pthread_create failed at thread %d (%d)\n", i, rc);
         appconf.tnum = i;
         break;
      }
   }
   for (i = 0; i < appconf.tnum; i++) {
      pthread_join(tid[i], NULL);
   }

   pthread_mutex_destroy(&mtx);

   fflush(stdout);
	
   printf("%s gives sum result %ld\n", PACKAGE, sumbuf);
   if (sumbuf != seqsum) {
      fprintf(stderr, "Error! mismatch detected (seq=%ld, parallel=%ld)\n", seqsum, sumbuf);
      free(tid);
      free(shrdarrbuf);
      free(thread_idx_range);
      return EXIT_FAILURE;
   }

   free(tid);
   free(shrdarrbuf);
   free(thread_idx_range);

   return EXIT_SUCCESS;
}

long validate_sum(int arraysize)
{
   long validsum = 0;
   int i;

   for (i=0; i < arraysize; i++)
      validsum += shrdarrbuf[i];

   return validsum;
}
