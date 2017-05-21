

#ifndef __JOB_H_
#define __JOB_H_

enum JobKind {
    Job_null = 0,
    Job_list, /* list dir */
    Job_lock, /* lock file */
    Job_md5,  /* calc file md5 */
    Job_read, /* read data */
    Job_unlock, /* unlock file */
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
void DoMd5(int fd, char *data);
void DoRead(int fd, char *data);
void DoUnlock(int fd, char *data);

#endif

