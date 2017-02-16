

#ifndef __JOB_H_
#define __JOB_H_

enum JobKind {
    Job_List,
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

void DoList(int fd, char *data, char *buf);
void ReplyList(int fd, char *data);
#endif

