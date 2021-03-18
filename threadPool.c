/*************
 * Sarah De-Paz
 *************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "threadPool.h"

struct tpTask {
  void (*computeFunc)(void *param);
  void* funcParam;
};

static void *workerRun(void* arg)
{
  struct workerThread *worker = arg;
  ThreadPool *threadPool = worker->threadPool;
  struct tpTask *tpTask;

  pthread_mutex_lock(&threadPool->workMutex);
  while (1) {
    if (threadPool->destroying) {
      if (!threadPool->waitForTasks ||
          osIsQueueEmpty(threadPool->workQueue)) {
        break;
      }
    }
    if (osIsQueueEmpty(threadPool->workQueue)) {
      pthread_cond_wait(&threadPool->workCond, &threadPool->workMutex);
      continue;
    }

    tpTask = osDequeue(threadPool->workQueue);
    pthread_mutex_unlock(&threadPool->workMutex);
    tpTask->computeFunc(tpTask->funcParam);
    free(tpTask);
    pthread_mutex_lock(&threadPool->workMutex);
  }

  pthread_mutex_unlock(&threadPool->workMutex);
  return NULL;
}

ThreadPool* tpCreate(int numOfThreads)
{
  int ret;
  int i;
  ThreadPool *threadPool = malloc(sizeof(*threadPool));
  if (threadPool == NULL) {
    fprintf(stderr, "Failed to alloc thread pool\n");
    goto fail;
  }

  memset(threadPool, 0, sizeof(*threadPool));

  threadPool->workers = malloc(sizeof(threadPool->workers[0]) * numOfThreads);
  if (threadPool->workers == NULL) {
    fprintf(stderr, "Failed to alloc thread pool workers\n");
    goto clean_tp;
  }

  memset(threadPool->workers, 0, sizeof(threadPool->workers[0]) * numOfThreads);

  threadPool->workQueue = osCreateQueue();
  if (threadPool->workQueue == NULL) {
    fprintf(stderr, "Failed to create work queue\n");
    goto clean_threads;
  }

  pthread_mutex_init(&threadPool->workMutex, NULL);
  pthread_cond_init(&threadPool->workCond, NULL);

  threadPool->workerCount = 0;
  for (i = 0; i < numOfThreads; i++) {
    threadPool->workers[i].index = i;
    threadPool->workers[i].threadPool = threadPool;
    ret = pthread_create(&threadPool->workers[i].thread,
                         NULL,
                         workerRun,
                         &threadPool->workers[i]);
    if (ret) {
      perror("Failed to create thread");
      goto destroy_threads;
    }
    threadPool->workerCount++;
  }

  return threadPool;

destroy_threads:
  pthread_mutex_lock(&threadPool->workMutex);
  threadPool->destroying = 1;
  pthread_cond_broadcast(&threadPool->workCond);
  pthread_mutex_unlock(&threadPool->workMutex);
  while (--i >= 0) {
    pthread_join(threadPool->workers[i].thread, NULL);
  }
  osDestroyQueue(threadPool->workQueue);
clean_threads:
  free(threadPool->workers);
clean_tp:
  free(threadPool);
fail:
  return NULL;
}

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks)
{
  int i;
  struct tpTask *tpTask;

  pthread_mutex_lock(&threadPool->workMutex);
  threadPool->destroying = 1;
  threadPool->waitForTasks = shouldWaitForTasks;
  pthread_cond_broadcast(&threadPool->workCond);
  pthread_mutex_unlock(&threadPool->workMutex);

  for (i = 0; i < threadPool->workerCount; i++) {
    pthread_join(threadPool->workers[i].thread, NULL);
  }

  while (!osIsQueueEmpty(threadPool->workQueue)) {
    tpTask = osDequeue(threadPool->workQueue);
    free(tpTask);
  }

  osDestroyQueue(threadPool->workQueue);
  free(threadPool->workers);
  free(threadPool);
}

int tpInsertTask(ThreadPool* threadPool,
                 void (*computeFunc) (void *),
                 void* param)
{
  int ret;
  struct tpTask *tpTask;

  tpTask = malloc(sizeof(*tpTask));
  if (tpTask == NULL) {
    fprintf(stderr, "Failed to alloc thread pool task\n");
    return -ENOMEM;
  }
  tpTask->computeFunc = computeFunc;
  tpTask->funcParam = param;

  pthread_mutex_lock(&threadPool->workMutex);
  if (threadPool->destroying) {
    fprintf(stderr, "Insert is not allowed during thread pool destruction\n");
    ret = -EINVAL;
    goto exit;
  }

  osEnqueue(threadPool->workQueue, tpTask);
  ret = 0;

exit:
  pthread_mutex_unlock(&threadPool->workMutex);
  return ret;
}
