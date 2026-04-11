#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Filename: msgsnd.c
 */

#define PERMS 0644
#define MSG_KEY 0x123
#define MSG_TYPE_FROM_MSGSND 1
#define MSG_TYPE_FROM_MSGRCV 2
#define EXIT_TOKEN "__CHAT_EXIT__"

struct my_msgbuf {
   long mtype;
   char mtext[200];
};

struct thread_context {
   int msqid;
   long send_type;
   long recv_type;
};

static volatile bool running = true;
static pthread_mutex_t send_lock = PTHREAD_MUTEX_INITIALIZER;

static int send_text(int msqid, long type, const char *text) {
   struct my_msgbuf buf;
   size_t len;

   buf.mtype = type;
   strncpy(buf.mtext, text, sizeof(buf.mtext) - 1);
   buf.mtext[sizeof(buf.mtext) - 1] = '\0';
   len = strlen(buf.mtext);

   if (msgsnd(msqid, &buf, len + 1, 0) == -1) {
      return -1;
   }
   return 0;
}

static void *send_thread(void *arg) {
   struct thread_context *ctx = (struct thread_context *)arg;
   char line[200];

   while (running) {
      fd_set set;
      struct timeval timeout;
      int ready;

      FD_ZERO(&set);
      FD_SET(STDIN_FILENO, &set);
      timeout.tv_sec = 0;
      timeout.tv_usec = 200000;

      ready = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
      if (ready == -1) {
         if (errno == EINTR) {
            continue;
         }
         perror("select");
         running = false;
         break;
      }
      if (ready == 0) {
         continue;
      }

      if (fgets(line, sizeof(line), stdin) == NULL) {
         pthread_mutex_lock(&send_lock);
         if (send_text(ctx->msqid, ctx->send_type, EXIT_TOKEN) == -1) {
            perror("msgsnd");
         }
         pthread_mutex_unlock(&send_lock);
         running = false;
         break;
      }

      line[strcspn(line, "\n")] = '\0';

      pthread_mutex_lock(&send_lock);
      if (send_text(ctx->msqid, ctx->send_type, line) == -1) {
         perror("msgsnd");
         pthread_mutex_unlock(&send_lock);
         running = false;
         break;
      }
      pthread_mutex_unlock(&send_lock);

   }

   return NULL;
}

static void *recv_thread(void *arg) {
   struct thread_context *ctx = (struct thread_context *)arg;
   struct my_msgbuf buf;

   while (running) {
      if (msgrcv(ctx->msqid, &buf, sizeof(buf.mtext), ctx->recv_type, IPC_NOWAIT) == -1) {
         if (errno == ENOMSG) {
            usleep(100000);
            continue;
         }
         if (errno == EIDRM || errno == EINVAL) {
            running = false;
            break;
         }
         perror("msgrcv");
         running = false;
         break;
      }

      if (strcmp(buf.mtext, EXIT_TOKEN) == 0) {
         running = false;
         break;
      }

      printf("Received: \"%s\"\n", buf.mtext);
      fflush(stdout);
   }

   return NULL;
}

int main(void) {
   int msqid;
   pthread_t sender;
   pthread_t receiver;
   struct thread_context ctx;

   if ((msqid = msgget(MSG_KEY, PERMS | IPC_CREAT)) == -1) {
      perror("msgget");
      return 1;
   }

   ctx.msqid = msqid;
   ctx.send_type = MSG_TYPE_FROM_MSGSND;
   ctx.recv_type = MSG_TYPE_FROM_MSGRCV;

   printf("Chat started. Type messages and press Enter to send. Press Ctrl + D to exit.\n");

   if (pthread_create(&sender, NULL, send_thread, &ctx) != 0) {
      perror("pthread_create sender");
      return 1;
   }
   if (pthread_create(&receiver, NULL, recv_thread, &ctx) != 0) {
      perror("pthread_create receiver");
      running = false;
      pthread_join(sender, NULL);
      return 1;
   }

   pthread_join(sender, NULL);
   running = false;
   pthread_join(receiver, NULL);

   if (msgctl(msqid, IPC_RMID, NULL) == -1 && errno != EIDRM) {
      perror("msgctl");
      return 1;
   }

   printf("Your friend left. Press Ctrl + D to exit.\nChat ended\n");
   return 0;
}
