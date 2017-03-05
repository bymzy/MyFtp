

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>

#include "Job.h"
#include "Util.h"
#include "ThreadGroup.h"

void ParseJob(int fd, char *buf)
{
    char *index = buf;

    /* parse msg type*/
    int typelen;
    index = readInt(index, &typelen);

    assert(typelen < 20);
    char reqType[20];
    bzero(reqType, 20);
    index = readBytes(index, reqType, typelen);

    printf("req type: %s\n", reqType);
    if (strcmp(reqType, "list") == 0) {
        DoList(fd, index, buf);
    } else {
        assert(0);
    }
}

void DoList(int fd, char *data, char *buf)
{
    struct Job *job = (struct Job *)malloc(sizeof(struct Job));
    job->fd = fd;
    job->jobKind = Job_List;
    job->data = data;
    job->buf = buf;

    EnqueJob(job);
}

void ReplyList(int fd, char *data)
{
    /* TODO this is a test */
    char words[] = "helleo";
    int len = strlen(words);
    char *buf = (char *)malloc(4 + len + 4);
    int dataSize = htonl(len + 4);
    len = htonl(len);

    memcpy(buf, &dataSize, 4);
    memcpy(buf + 4, &len, 4);
    memcpy(buf + 8, words, sizeof(words));

    SendAll(fd, buf, 4 + 4 + strlen(words));
    free(buf);
}

void FreeJob(struct Job *job)
{
    free(job->buf);
    free(job);
}

void DoJob(struct Job *job)
{
    switch (job->jobKind) {
        case Job_List:
            ReplyList(job->fd, job->data);
            break;
    }
}

void SendAll(int fd, char *buf, int total)
{
    int toSend = total;
    int sent = 0;
    int ret = 0;
    int err = 0;
    
    while (toSend > 0) {
        ret = send(fd, buf + sent, toSend, 0);
        if (ret < 0) {
            err = errno;
            if (err == EAGAIN || err == EINPROGRESS) {
                err = 0;
                continue;
            }
            perror("send socket failed");
            return;
        }  

        sent += ret;
        toSend -= ret;
    }
}

