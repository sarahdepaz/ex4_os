#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "osqueue.h"
#include "threadPool.h"


void hello (void* a)
{
  int sleep_sec = rand() % 10;
  printf("[Thread %p] hello! sleeping for %d sec\n", (void*)pthread_self(), sleep_sec);
  sleep(sleep_sec);
}


void test_thread_pool_sanity()
{
   int i;

   ThreadPool* tp = tpCreate(5);

   for(i=0; i<10; ++i)
   {
      tpInsertTask(tp,hello,NULL);
   }

   tpDestroy(tp,1);
}


int main(int argc, char **argv)
{
  int poolSize;
  int taskCount;
  int i;
  ThreadPool* tp;

  if (argc < 3) {
    printf("Usage: %s <pool size> <task count>\n", argv[0]);
    return 1;
  }

  poolSize = atoi(argv[1]);
  taskCount = atoi(argv[2]);
  printf("poolSize=%d, taskCount=%d\n", poolSize, taskCount);

  tp = tpCreate(poolSize);
  if (tp == NULL) {
    printf("tpCreate failed\n");
    return 1;
  }

   for(i = 0; i < taskCount; ++i) {
      tpInsertTask(tp, hello, NULL);
   }

   tpDestroy(tp, 1);
   return 0;
}
