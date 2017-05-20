

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>

#include "Job.h"
#include "Util.h"
#include "ThreadGroup.h"
#include "../FileManager/FileManager.h"

void ParseJob(int fd, char *buf)
{
    char *index = buf;

    /* parse msg type */
    uint32_t typelen;
    index = readInt(index, &typelen);

    assert(typelen < 20);
    char reqType[20];
    bzero(reqType, 20);
    index = readBytes(index, reqType, typelen);

    printf("req type: %s\n", reqType);
    if (strcmp(reqType, "list") == 0) {
        CreateJob(fd, Job_list, index, buf);
    } else if (strcmp(reqType, "lock") == 0) {
        CreateJob(fd, Job_lock, index, buf);
    } else if (strcmp(reqType, "md5") == 0) {
        CreateJob(fd, Job_md5, index, buf);
    } else if (strcmp(reqType, "read") == 0) {
        CreateJob(fd, Job_read, index, buf);
    } else {
        assert(0);
    }
}

/* @param buf, use this to free buffer we malloced in main thread */
void CreateJob(int fd, int jobType, char *data, char *buf)
{
    struct Job *job = (struct Job *)malloc(sizeof(struct Job));
    job->fd = fd;
    job->jobKind = jobType;
    job->data = data;
    job->buf = buf;

    EnqueJob(job);
}

void DoList(int fd, char *data)
{
    /* get target dir */
    uint32_t targetLen = 0;
    data = readInt(data, &targetLen);

    char *dir = (char *)malloc(targetLen + 1);
    memcpy(dir, data, targetLen);
    printf("request dir is %s\n", dir);

    /* encode buf by FileManager */
    char *buf = NULL;
    int sendLen = 0;
    ListDir(dir, &buf, &sendLen);

    SendAll(fd, buf, sendLen);
    free(buf);
    free(dir);
}

void DoLock(int fd, char *data)
{
    /* get lock type */
    uint32_t lockType = LOCK_null;
    data = readInt(data, &lockType);

    /* get file type */
    uint32_t fileType = FILE_reg;
    data = readInt(data, &fileType);

    /* get lock file */
    uint32_t fileNameLen = 0;
    data = readInt(data, &fileNameLen);
    char *name = malloc(fileNameLen + 1);
    bzero(name, fileNameLen + 1);
    data = readBytes(data, name, fileNameLen);
    
    char *buf = NULL;
    int sendLen = 0;
    LockFile(name, lockType, fileType, &buf, &sendLen);
    SendAll(fd, buf, sendLen);
    free(buf);
    free(name);
}

void DoMd5(int fd, char *data)
{
    /* get file name */
    uint32_t fileNameLen = 0;
    data = readInt(data, &fileNameLen);
    char *name = malloc(fileNameLen + 1);
    bzero(name, fileNameLen + 1);
    data = readBytes(data, name, fileNameLen);

    char *buf = NULL;
    uint32_t sendLen = 0;
    CalcMd5(name, &buf, &sendLen);
    SendAll(fd, buf, sendLen);
    free(buf);
    free(name);
}

void DoRead(int fd, char *data)
{
    char *fileName;
    data = readString(data, &fileName);

    uint64_t offset = 0;
    data = read64Int(data, &offset);

    uint32_t size = 0;
    data = readInt(data, &size);

    char *buf = NULL;
    uint32_t sendLen = 0;

    ReadData(fileName, offset, size, &buf, &sendLen);
    SendAll(fd, buf, sendLen);

    free(buf);
    free(fileName);
}

void FreeJob(struct Job *job)
{
    free(job->buf);
    free(job);
}

void DoJob(struct Job *job)
{
    switch (job->jobKind) {
        case Job_list:
            DoList(job->fd, job->data);
            break;
        case Job_lock:
            DoLock(job->fd, job->data);
            break;
        case Job_md5:
            DoMd5(job->fd, job->data);
            break;
        case Job_read:
            DoRead(job->fd, job->data);
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

