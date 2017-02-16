

#ifndef __THREAD_GROUP_H_
#define __THREAD_GROUP_H_

#include <pthread.h>
#include "Job.h"

struct Thread {
    pthread_t tid;
};

void * ThreadEntry(void *arg);

void InitThreads();

void FinitThread();

void EnqueJob(struct Job *job);


#endif


