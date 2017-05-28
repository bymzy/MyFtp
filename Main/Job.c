

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
    int jobType = 0;
    index = readInt(index, &typelen);

    assert(typelen < 20);
    char reqType[20];
    bzero(reqType, 20);
    index = readBytes(index, reqType, typelen);

    printf("req type: %s\n", reqType);
    if (strcmp(reqType, "list") == 0) {
        jobType = Job_list;
    } else if (strcmp(reqType, "lock") == 0) {
        jobType = Job_lock;
    } else if (strcmp(reqType, "unlock") == 0) {
        jobType = Job_unlock;
    } else if (strcmp(reqType, "md5") == 0) {
        jobType = Job_md5;
    } else if (strcmp(reqType, "read") == 0) {
        jobType = Job_read;
    } else if (strcmp(reqType, "create") == 0) {
        jobType = Job_create;
    } else if (strcmp(reqType, "write") == 0) {
        jobType = Job_write;
    } else if (strcmp(reqType, "put_file_end") == 0) {
        jobType = Job_put_file_end;
    } else {
        assert(0);
    }

    CreateJob(fd, jobType, index, buf);
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
        case Job_unlock:
            DoUnlock(job->fd, job->data);
            break;
        case Job_create:
            DoCreate(job->fd, job->data);
            break;
        case Job_write:
            DoWrite(job->fd, job->data);
            break;
        case Job_put_file_end:
            DoPutFileEnd(job->fd, job->data);
            break;
        default:
            assert(0);
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

void DoUnlock(int fd, char *data)
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
    UnLockFile(name, lockType, fileType, &buf, &sendLen);
    SendAll(fd, buf, sendLen);
    free(buf);
    free(name);
}

void DoCreate(int fd, char *data)
{
    /* get file path */
    char *fileName = NULL;
    data = readString(data, &fileName);

    /* get file md5 */
    char *md5 = NULL;
    data = readString(data, &md5);

    uint64_t size = 0;
    data = read64Int(data, &size);

    char *buf = NULL;
    int sendLen = 0;
    printf("job.c DoCreate!!!, fileName: %s, md5: %s, size: %lld\n", fileName, md5, size);
    TryCreateFile(fileName, md5, size, &buf, &sendLen);
    SendAll(fd, buf, sendLen);

    free(buf);
    free(fileName);
    free(md5);
}

void DoWrite(int fd, char *data)
{
    /* get file path */
    char *fileName = NULL;
    data = readString(data, &fileName);

    char *md5= NULL;
    data = readString(data, &md5);

    uint64_t offset = 0;
    data = read64Int(data, &offset);

    uint32_t dataLen = 0;
    data = readInt(data, &dataLen);

    //printf("DoWrite, fileName:%s, offset:%lld, date len:%d\n", fileName, offset, dataLen);
    char *buf = NULL;
    int sendLen = 0;
    WriteFile(fileName, md5, offset, data, dataLen, &buf, &sendLen);
    SendAll(fd, buf, sendLen);

    free(buf);
    free(fileName);
    free(md5);
}

void DoPutFileEnd(int fd, char *data)
{
    /* get file path */
    char *fileName = NULL;
    data = readString(data, &fileName);

    char *md5= NULL;
    data = readString(data, &md5);

    printf("DoPutFileEnd, filename: %s, md5: %s\n", fileName, md5);
    char *buf = NULL;
    int sendLen = 0;
    WriteFileEnd(fileName, md5, &buf, &sendLen);
    SendAll(fd, buf, sendLen);

    free(buf);
    free(fileName);
    free(md5);
}



