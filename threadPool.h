/*************
 * Sarah De-Paz
 *************/
#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <pthread.h>
#include "osqueue.h"

struct workerThread {
    pthread_t thread;
    int index;
    struct thread_pool *threadPool;
};

typedef struct thread_pool
{
  struct workerThread *workers;
  int workerCount;
  OSQueue* workQueue;
  pthread_cond_t workCond;
  pthread_mutex_t workMutex;
  int destroying;
  int waitForTasks;
}ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
