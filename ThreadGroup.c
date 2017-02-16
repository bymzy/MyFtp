

#include <stdlib.h>
#include <assert.h>
#include "ThreadGroup.h"

#define ThreadCount 3

struct Thread *threads = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int JobCount = 0;
static struct Job * head = NULL;
static struct Job * tail = NULL;
static int threadStatus = 0;

void InitThreads()
{
    int i = 0;
    int err = 0;
    threadStatus = 1;

    /**/
    pthread_t tid;
    threads = (struct Thread *)malloc(sizeof(struct Thread) * ThreadCount);
    for (; i < ThreadCount; ++i) {
        err = pthread_create(&tid, NULL, ThreadEntry, NULL);
        assert(err == 0);
        threads[i].tid = tid;
    }
}

void * ThreadEntry(void *arg)
{
    struct Job *job = NULL;
    while (1) {
        pthread_mutex_lock(&mutex);
        while (JobCount <= 0 && (threadStatus == 1)) {
            pthread_cond_wait(&cond, &mutex);
        }

        if (threadStatus == 0) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        job = head;
        head = head->next;
        JobCount--;

        if (head == NULL) {
            tail = NULL;
        }
        pthread_mutex_unlock(&mutex);

        DoJob(job);
        FreeJob(job);
        job = NULL;
    }
}

void FinitThread()
{
    pthread_mutex_lock(&mutex);
    threadStatus = 0;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

    int i = 0;
    for (;i < ThreadCount; ++i) {
        pthread_join(threads[i].tid, NULL);
    }

    free(threads);
}

void EnqueJob(struct Job *job)
{
    pthread_mutex_lock(&mutex);
    if (tail == NULL) {
        head = job;
        tail = job;
    } else {
        tail->next = job;
        tail = job;
    }

    tail->next = NULL;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}




