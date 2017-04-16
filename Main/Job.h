

#ifndef __JOB_H_
#define __JOB_H_

enum JobKind {
    Job_null = 0,
    Job_list,
    Job_lock,
};

struct Job {
    int fd;
    int jobKind;
    char *data;
    char *buf;
    struct Job *next;
};

void SendAll(int fd, char *buf, int total);

void ParseJob(int fd, char *data);
void FreeJob(struct Job *job);
void DoJob(struct Job *job);

void CreateJob(int fd, int jobType, char *data, char *buf);
void DoList(int fd, char *data);
void DoLock(int fd, char *data);
#endif

