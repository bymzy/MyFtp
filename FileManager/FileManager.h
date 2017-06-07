

#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include <stdint.h>
#include "list.h"

extern char * g_repo;

enum FileType {
    FILE_null = 0,
    FILE_dir,
    FILE_reg
};

enum FileLockType {
    LOCK_null = 0,
    LOCK_read,
    LOCK_write
};

struct FileLock {
    enum FileLockType type;
    uint64_t userId;
};


struct Dir {
    enum FileType type;
    struct FileLock lock;
    int refCount; 
    char *name;
    struct AVLTable *fileTable; //存放本目录下所有的文件
    struct Dir *parent; //指向父目录
    struct ListTable *subDirTable;
};

struct File {
    struct FileLock lock;
    char *name;
    char *md5;
    struct Dir *parent;
};

struct FileManager {
    struct AVLTable *dirTable; //存放当前所有的目录信息
    struct AVLTable *uploadFileTable;
};


int InitFileManager(const char *repo);
int SearchDir(const char *dir, struct Dir *parent);
int TryLock(struct FileLock *lock, int lockType, char **errStr);
int TryUnLock(struct FileLock *lock, int lockType, char **errStr);

int ListDir(const char *dir, char **buf, uint32_t *bufLen);
int LockFile(char *fileName, int lockType, int fileType, char **buf, uint32_t *bufLen);
int UploadLockFile(char *fileName, int lockType, int fileType, char **buf, uint32_t *bufLen);
int CalcMd5(char *fileName, char **buf, uint32_t *bufLen);
int ReadData(char *fileName, uint64_t offset, uint32_t size, char ** buf, uint32_t *bufLen);
int UnLockFile(char *fileName, int lockType, int fileType, char **buf, uint32_t *bufLen);
int UploadUnLockFile(char *fileName, int lockType, int fileType, char **buf, uint32_t *bufLen);
int TryCreateFile(char *fileName, char *md5, uint64_t size, char **buf, uint32_t *bufLen);
int WriteFile(char *fileName, char *md5, uint64_t offset, char *data, uint32_t dataLen, char **buf, uint32_t *bufLen);
int WriteFileEnd(char *fileName, char *md5, char **buf, uint32_t *sendLen);
int DeleteFile(char *fileName, char **buf, uint32_t *sendLen);
int MakeDir(char *dirName, char **buf, uint32_t *sendLen);
int DeleteDir(char *dirName, char **buf, uint32_t *sendLen);
int ChangeDir(char *dirName, char **buf, uint32_t *sendLen);

int AddFileIndex(const char *fileName, const char *md5);
int EraseFileIndex(const char *fileName);

int AddDirIndex(const char *fileName);
int EraseDirIndex(const char *fileName);

char * addStr(const char *left, const char *right);
char *GetRealPath(const char *fileName);
void DebugDir(struct Dir *dir);


#endif

