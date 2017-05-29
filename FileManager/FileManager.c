

#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <libgen.h>
#include <memory.h>
#include <openssl/md5.h>
#include <fcntl.h>

#include "FileManager.h"
#include "avl.h"
#include "list.h"
#include "../Main/Util.h"
#include "Err.h"

static struct FileManager *g_file_manager = NULL;
static pthread_mutex_t s_mutex;
char g_repo[200];

static void Lock()
{

}

static void UnLock()
{

}

char * GetRealPath(const char *fileName)
{
    return addStr(g_repo, fileName);
}

void DebugDir(struct Dir *dir)
{
    printf("type:%d, refCount:%d, name: %s\n"
            ,dir->type
            ,dir->refCount
            ,dir->name);
}

char * addStr(const char *left, const char *right)
{
    uint32_t leftLen = strlen(left);
    uint32_t rightLen = strlen(right);
    char *ret = malloc(leftLen + rightLen + 1);
    strncpy(ret, left, leftLen);
    strncpy(ret + leftLen, right, rightLen);
    strncpy(ret + leftLen + rightLen, "\0", 1);

    return ret;
}

int InitFileManager(const char *repo)
{
    /* init avl table */
    if (g_file_manager == NULL) {
        g_file_manager = (struct FileManager*)malloc(sizeof(struct FileManager));
        g_file_manager->dirTable = createTable(charCompare);
        g_file_manager->uploadFileTable = createTable(charCompare);
    }
    
    /* add root to g_file_manager dirTable */
    struct Dir *root = (struct Dir*)malloc(sizeof(struct Dir));
    root->type = FILE_dir;
    root->lock.type = LOCK_null;
    root->lock.userId = -1;
    root->refCount = 0;
    root->fileTable = NULL;
    root->name = addStr("/", "");
    root->parent = NULL;
    root->subDirTable = NULL;
    printf("dirname: %s\n", root->name);

    insertNode(g_file_manager->dirTable, addStr("", root->name), root);
    insertNode(g_file_manager->uploadFileTable, addStr("/", ""), NULL);

    SearchDir(repo, root);

#if 0
    struct AVLTraverseTable *traverseTable = malloc(sizeof(struct AVLTraverseTable));
    traverse_init(traverseTable, g_file_manager->dirTable);
    struct Dir *dir = NULL;

    while ((dir = (struct Dir *)traverse_get_next(traverseTable)) != NULL) {
        printf("loop, get dirname:%s\n", dir->name);
    }
    free(traverseTable);

    /*get dir */
    dir = (struct Dir *)findNode(g_file_manager->dirTable, "/project/")->value;
    DebugDir(dir);
#endif
}

int SearchDir(const char *dirName, struct Dir *parent)
{
    int err = 0;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    do {
        dp = opendir(dirName);
        if (dp == NULL) {
            err = errno;
            break;
        }

        chdir(dirName);

        while ((entry = readdir(dp)) != NULL) {
            lstat(entry->d_name, &statbuf);
            if (S_ISDIR(statbuf.st_mode)) {
                if (strcmp(".", entry->d_name) == 0
                        || strcmp("..", entry->d_name) == 0) {
                    continue;
                }

                /* add dir to g_file_manager dirTable */
                struct Dir *dir = (struct Dir*)malloc(sizeof(struct Dir));
                dir->type = FILE_dir;
                dir->lock.type = LOCK_null;
                dir->lock.userId = -1;
                dir->refCount = 0;
                dir->fileTable = NULL;
                char *temp = addStr(entry->d_name, "/");
                dir->name = addStr(parent->name, temp);
                DebugDir(dir);
                free(temp);
                dir->parent = parent;

                insertNode(g_file_manager->dirTable, strdup(dir->name), dir);

                if (parent->subDirTable == NULL) {
                    parent->subDirTable = createListTable();
                }
                insertListItem(parent->subDirTable, dir);

                char *nextDir = addStr(entry->d_name, "");
                SearchDir(nextDir, dir);

                free(nextDir);
            } else if (S_ISREG(statbuf.st_mode)) {
                /* add file to dir fileTable*/
                if (parent->fileTable == NULL) {
                    parent->fileTable = createTable(charCompare);
                }

                struct File *file = (struct File*)malloc(sizeof(struct File));
                file->lock.type = LOCK_null;
                file->lock.userId = -1;
                file->name = addStr(parent->name, entry->d_name);
                file->md5 = NULL;
                file->parent = parent;
                printf("filename: %s\n", file->name);

                insertNode(parent->fileTable, addStr(parent->name, entry->d_name), file);
            }
        }

        chdir("..");
    } while(0);

    return 0;
}

int AddIndex(const char *fileName, const char *md5)
{

    struct AVLNode *node = NULL;
    char *tempFileName = strdup(fileName);
    char *dirName = addStr(dirname(tempFileName), "/");
    if (strcmp(dirName,"//") == 0) {
        dirName[1] = '\0';
    }

    struct Dir *parent = NULL;
    /* step 1, find parent dir */
    do {
        node = findNode(g_file_manager->dirTable, (void *)dirName);
        assert(node != NULL);

        parent = (struct Dir*)node->value;
        
        struct File *file = (struct File*)malloc(sizeof(struct File));
        file->lock.type = LOCK_null;
        file->lock.userId = -1;
        file->name = strdup(fileName);
        file->md5 = strdup(md5);
        file->parent = parent;

        insertNode(parent->fileTable, strdup(fileName), file);

    } while(0);

    free(tempFileName);
    free(dirName);

    return 0;
}

int ListDir(const char *dirName, char **buf, uint32_t *bufLen) {
    uint32_t dirCount = 0;
    uint32_t fileCount = 0;
    int err = 0;
    char *errStr = NULL;
    uint32_t errStrLen = 0;
    
    struct AVLNode *node = NULL;
    struct Dir *dir = NULL;
    uint32_t len = 0;
    char *writeIndex = NULL;
    struct AVLTraverseTable *traverseTable = NULL;
    struct File *file = NULL;
    struct ListNode *item = NULL;
    struct Dir *subdir = NULL;

    /* errno */
    len += 4;

    /* calc buf len */
    do {
        node = findNode(g_file_manager->dirTable, (void *)dirName);
        if (node == NULL) {
            errStr = "dir not found!";
            errStrLen = strlen(errStr);
            len += 4 + errStrLen;
            err = EINVAL;
            break;
        }

        dir = (struct Dir *) node->value;
        item = dir->subDirTable->root;
        dirCount = dir->subDirTable->size;
        /* subdir count */
        len += 4;
        while (item != NULL) {
            subdir = (struct Dir*)item->value;
            len += 4 + strlen(subdir->name);
            item = item->next;
        }

        /* file count */
        len += 4;
        fileCount = dir->fileTable->size;
        traverseTable = malloc(sizeof(struct AVLTraverseTable));
        traverse_init(traverseTable, dir->fileTable);
        while ((file = (struct File *)traverse_get_next(traverseTable)) != NULL) {
            len +=4 + strlen(file->name);
        }

        /* errstr */
        errStr = "";
        errStrLen = 0;
        len +=4 + errStrLen;
    } while(0);

    /* malloc buf */
    *buf = malloc(len + 4);
    *bufLen = len + 4;
    writeIndex = *buf;

    /* encode buf */
    writeIndex = writeInt(writeIndex, len);
    writeIndex = writeInt(writeIndex, err);
    writeIndex = writeInt(writeIndex, errStrLen);
    writeIndex = writeBytes(writeIndex, errStr, errStrLen);

    if (err == 0) {
        writeIndex = writeInt(writeIndex, dir->subDirTable->size);
        item = dir->subDirTable->root;
        while (item != NULL) {
            subdir = (struct Dir*)item->value;
            writeIndex = writeString(writeIndex, subdir->name, strlen(subdir->name));
            item = item->next;
        }

        writeIndex = writeInt(writeIndex, dir->fileTable->size);
        traverse_init(traverseTable, dir->fileTable);
        while ((file = (struct File *)traverse_get_next(traverseTable)) != NULL) {
            writeIndex = writeString(writeIndex, file->name, strlen(file->name));
        }
    }

    if (traverseTable != NULL) {
        free(traverseTable);
    }

    assert(writeIndex == (*buf + len + 4));
    return err;
}

int UploadLockFile(char *fileName, int lockType, int fileType, char **buf, uint32_t *bufLen)
{
    struct AVLNode *node = NULL;
    int err = 0;
    char *errStr = "";
    uint32_t totalLen = 0;
    char *writeIndex = NULL;


    do {
        /* step 1 , try find in uploadFileTable */
        node = findNode(g_file_manager->uploadFileTable, (void *)fileName);
        if (node != NULL) {
            err = ERR_file_uploading;
            errStr = "file with same name is uploading!!!";
            break;
        }

        /* step 2 , insert */
        insertNode(g_file_manager->uploadFileTable, strdup(fileName), NULL);
    } while(0);

    totalLen += 4 + 4 + strlen(errStr);
    *buf = malloc(totalLen + 4);
    writeIndex = *buf;
    *bufLen = totalLen + 4;

    writeIndex = writeInt(writeIndex, totalLen);
    writeIndex = writeInt(writeIndex, err);
    writeIndex = writeString(writeIndex, errStr, strlen(errStr));

    return err;
}

int LockFile(char *fileName, int lockType, int fileType, char **buf, uint32_t *bufLen)
{
    char *dirName = NULL;
    int err = 0;
    char *errStr = "";
    struct AVLNode *node = NULL;
    struct AVLTable *fileTable = NULL;
    struct Dir *dir = NULL;
    struct File *file = NULL;
    uint32_t len = 0;
    char *writeIndex = NULL;

    if (fileType == FILE_reg) {
        char *temp = strdup(fileName);
        char *tempdir = dirname(temp);
        if (strcmp(tempdir, "/") == 0) {
            dirName = addStr("/", "");
        } else {
            dirName = addStr(tempdir, "/");
        }
        free(temp);
    }

    do {
        /* search dir */
        node = findNode(g_file_manager->dirTable, dirName);
        if (node == NULL) {
            err = EINVAL;
            errStr = "dir not found!";
            break;
        }
        
        dir = (struct Dir *)node->value;
        if (fileType == FILE_dir) {
            err = TryLock(&dir->lock, lockType, &errStr);
        } else {
            /* search for file */
            fileTable = dir->fileTable;
            node = findNode(fileTable, fileName);
            if (node == NULL) {
                err = ERR_file_not_exist;
                errStr = "file not exist!";
                break;
            }
            file = (struct File *)node->value;
            err = TryLock(&file->lock, lockType, &errStr);
        }

    } while(0);

    if (fileType == FILE_reg && dirName != NULL) {
        free(dirName);
    }

    uint32_t totalLen = 8 + strlen(errStr);
    len += 4;
    len += 4;
    len += 4 + strlen(errStr);

    *buf = malloc(len);
    *bufLen = len;
    writeIndex = *buf;

    writeIndex = writeInt(writeIndex, totalLen);
    writeIndex = writeInt(writeIndex, err);
    writeIndex = writeString(writeIndex, errStr, strlen(errStr));
    printf("lock file err:%d, errstr:%s\n", err, errStr);

    return err;
}

int UploadUnLockFile(char *fileName, int lockType, int fileType, char **buf, uint32_t *bufLen)
{
    struct AVLNode *node = NULL;
    int err = 0;
    char *errStr = "";
    uint32_t totalLen = 0;
    char *writeIndex = NULL;


    do {
        /* step 1 , try find in uploadFileTable */
        node = findNode(g_file_manager->uploadFileTable, (void *)fileName);
        assert(node != NULL);
        deleteNode(g_file_manager->uploadFileTable, (void *)fileName);
    } while(0);

    totalLen += 4 + 4 + strlen(errStr);
    *buf = malloc(totalLen + 4);
    writeIndex = *buf;
    *bufLen = totalLen + 4;

    writeIndex = writeInt(writeIndex, totalLen);
    writeIndex = writeInt(writeIndex, err);
    writeIndex = writeString(writeIndex, errStr, strlen(errStr));

    return err;
}

int UnLockFile(char *fileName, int lockType, int fileType, char **buf, uint32_t *bufLen)
{
    char *dirName = NULL;
    int err = 0;
    char *errStr = "";
    struct AVLNode *node = NULL;
    struct AVLTable *fileTable = NULL;
    struct Dir *dir = NULL;
    struct File *file = NULL;
    uint32_t len = 0;
    char *writeIndex = NULL;

    if (fileType == FILE_reg) {
        char *temp = strdup(fileName);
        char *tempdir = dirname(temp);
        if (strcmp(tempdir, "/") == 0) {
            dirName = addStr("/", "");
        } else {
            dirName = addStr(tempdir, "/");
        }
        free(temp);
    }

    do {
        /* search dir */
        node = findNode(g_file_manager->dirTable, dirName);
        if (node == NULL) {
            err = EINVAL;
            errStr = "dir not found!";
            break;
        }
        
        dir = (struct Dir *)node->value;
        if (fileType == FILE_dir) {
            err = TryUnLock(&dir->lock, lockType, &errStr);
        } else {
            /* search for file */
            fileTable = dir->fileTable;
            node = findNode(fileTable, fileName);
            if (node == NULL)  {
                err = ERR_file_not_exist;
                errStr = "file not exist!";
                break;
            }
            file = (struct File *)node->value;
            err = TryUnLock(&file->lock, lockType, &errStr);
        }

    } while(0);

    if (fileType == FILE_reg && dirName != NULL) {
        free(dirName);
    }

    uint32_t totalLen = 8 + strlen(errStr);
    len += 4;
    len += 4;
    len += 4 + strlen(errStr);

    *buf = malloc(len);
    *bufLen = len;
    writeIndex = *buf;

    writeIndex = writeInt(writeIndex, totalLen);
    writeIndex = writeInt(writeIndex, err);
    writeIndex = writeString(writeIndex, errStr, strlen(errStr));
    printf("unlock file err:%d, errstr:%s\n", err, errStr);

    return err;
}

int TryLock(struct FileLock *lock, int lockType, char **errStr)
{
    int err = 0;

    do {
        if (lock->type == LOCK_write) {
            err =  ERR_can_not_lock;
            *errStr = "already has write lock!";
            break;
        }
        
        if (lock->type == LOCK_read && lockType == LOCK_write) {
            err = ERR_can_not_lock;
            *errStr = "cant add wirte lock, cause has read lock!";
            break;
        }

        lock->type = lockType;
    } while(0);

    return err;
}

int TryUnLock(struct FileLock *lock, int lockType, char **errStr)
{
    lock->type = LOCK_null;

    return 0;
}

char *GetMd5FromFile(char *fileName)
{
    unsigned char c[MD5_DIGEST_LENGTH];
    int i;
    FILE *inFile = fopen (fileName, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];
    char *md5 = malloc(33);
    bzero(md5, 33);

    if (inFile == NULL) {
        printf ("%s can't be opened.\n", fileName);
        return NULL;
    }

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, inFile)) != 0)
        MD5_Update (&mdContext, data, bytes);
    MD5_Final (c,&mdContext);
    for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(md5 + i * 2, "%02x", c[i]);
    }
    md5[33] = 0;
    fclose (inFile);
    return md5;
}

int CalcMd5(char *fileName, char **buf, uint32_t *sendLen)
{
    int err = 0;
    char *errStr = "";
    char *path = GetRealPath(fileName);
    char *md5 = NULL;
    uint32_t totalLen = 0; /* data len not include first 4 bytes */
    char *writeIndex = NULL;

    do {
        md5 = GetMd5FromFile(path);
        if (md5 == NULL) {
            err = ERR_get_file_md5;
            errStr = "get file md5 failed!";
            break;
        }

        printf("md5 of %s is %s \n", path, md5);
    } while(0);
    
    totalLen += 4 + 4 + strlen(errStr);
    if (err == 0) {
        totalLen += 4 + strlen(md5);
    }

    *buf = malloc(totalLen + 4);
    writeIndex = *buf;
    *sendLen = totalLen + 4;

    writeIndex = writeInt(writeIndex, totalLen);
    writeIndex = writeInt(writeIndex, err);
    writeIndex = writeString(writeIndex, errStr, strlen(errStr));

    if (err == 0) {
        writeIndex = writeString(writeIndex, md5, strlen(md5));
    }

    printf("CalcMd5, err %d, errStr %s\n", err, errStr);
    free(path);
    free(md5);

    return err;
}

int ReadData(char *fileName, uint64_t offset, uint32_t size, char ** buf, uint32_t *sendLen)
{
    int err = 0;
    char *errStr = "";
    char *path = GetRealPath(fileName);
    int fd = -1;
    uint32_t toRead = size;
    uint32_t readed= 0;
    uint32_t ret = 0;
    char *data = NULL;
    uint32_t totalLen = 0;
    char *writeIndex = NULL;

    /* TODO file is opened frequently */
 
    printf("try read data!!!");
    do {
        fd = open(path, O_RDWR);   
        if (fd < 0) {
            err = errno;
            errStr = strerror(err);
            break;
        }

        data = malloc(size);
        if (data == NULL) {
            err = ENOMEM;
            errStr = strerror(err);
            break;
        }

        while (toRead > 0) {
            ret = pread(fd, data + readed, toRead, offset + readed);
            if (ret < 0) {
                err = errno;
                errStr = strerror(err);
                break;
            } else if (ret == 0) {
                toRead = 0;
                break;
            } else {
                toRead -= ret;
                readed += ret;
            }
        }

    } while(0);

    totalLen += 4 + 4 + strlen(errStr);
    if (err == 0) {
        totalLen += 4;
        totalLen += readed;
    }
    
    *buf = malloc(totalLen + 4);
    writeIndex = *buf;
    *sendLen = totalLen + 4;

    writeIndex = writeInt(writeIndex, totalLen);
    writeIndex = writeInt(writeIndex, err);
    writeIndex = writeString(writeIndex, errStr, strlen(errStr));

    if (err == 0) {
        writeIndex = writeInt(writeIndex, readed);
        writeIndex = writeBytes(writeIndex, data, readed);
    }

    if (fd != -1) {
        close(fd);
        fd = -1;
    }

    if (data != NULL) {
        free(data);
        data = NULL;
    }

    free(path);

    return err;
}

int TryCreateFile(char *fileName, char *md5, uint64_t size, char **buf, uint32_t *bufLen)
{
    int err = 0;
    char *errStr = "";
    char *filePath= GetRealPath(fileName);
    uint32_t totalLen = 0;
    char *writeIndex = NULL;
    uint64_t offset = 0;
    char tempFilePath[256] = {'\0'};

    char *_tempFilePath = strdup(filePath);
    char *_tempFileName = strdup(fileName);
    char *_tempFileDir = dirname(_tempFilePath);
    sprintf(tempFilePath, "%s/.%s_%s", _tempFileDir, md5, basename(_tempFileName));
    free(_tempFilePath);
    free(_tempFileName);
    
    /* check file exist */
    struct stat st;

    printf("filepath: %s, tempFilePath: %s\n", filePath, tempFilePath);
    do {
        err = stat(filePath, &st);
        if (err == 0) {
            err = EEXIST;
            errStr = "file exists! can't upload";
            break;
        }

        err = stat(tempFilePath, &st);
        if (err == 0) {
            printf("find temp put file!!!, offset: %ldd\n", st.st_size);
            offset = st.st_size;
            break;
        }
        err = 0;

        /* open 会变相的判断目录是否存在 */
        int fd = open(tempFilePath, O_RDWR | O_CREAT | O_SYNC, 0644);
        if (fd < 0) {
            err = errno;
            errStr = strerror(err);
            printf("create file failed!!!");
            break;
        }

        /*
        err = posix_fallocate(fd, 0, size);
        if (err != 0) {
            err = errno;
            errStr = strerror(err);
            printf("posix fallocate failed!!!");
            break;
        }
        */
        offset = 0;

    } while(0);

    totalLen += 4 + 4 + strlen(errStr);
    if (err == 0) {
        totalLen += 8; /* start offset */
    }

    *buf = malloc(totalLen + 4);
    writeIndex = *buf;
    *bufLen = totalLen + 4;

    writeIndex = writeInt(writeIndex, totalLen);
    writeIndex = writeInt(writeIndex, err);
    writeIndex = writeString(writeIndex, errStr, strlen(errStr));

    if (err == 0) {
        writeIndex = write64Int(writeIndex, offset);
    }

    
    if (filePath != NULL) {
        free(filePath);
        filePath = NULL;
    }

    printf("try create file , err: %d, errstr: %s\n", err, errStr);
    return err;
}

int WriteFile(char *fileName, char *md5, uint64_t offset, char *data, uint32_t dataLen, char **buf, uint32_t *bufLen)
{
    int err = 0;
    char *errStr = "";
    char *filePath= GetRealPath(fileName);
    uint32_t totalLen = 0;
    char *writeIndex = NULL;
    char tempFilePath[256] = {'\0'};
    int fd = -1;
    size_t toWrite = dataLen;
    size_t writed = 0;
    ssize_t size = -1;

    char *_tempFilePath = strdup(filePath);
    char *_tempFileName = strdup(fileName);
    sprintf(tempFilePath, "%s/.%s_%s", dirname(_tempFilePath), md5, basename(_tempFileName));
    free(_tempFilePath);
    free(_tempFileName);

    do {
        fd = open(tempFilePath, O_WRONLY);
        if (fd < 0) {
            err = errno;
            errStr = strerror(err);
            break;
        }

        while(toWrite > 0) {
            writed = pwrite(fd, data + writed, toWrite, offset);
            if (writed < 0) {
                err = errno;
                errStr = strerror(err);
                break;
            }
            toWrite -= writed;
            offset += writed;
        }
    } while(0);

    totalLen += 4 + 4 + strlen(errStr);
    *buf = malloc(totalLen + 4);
    writeIndex = *buf;
    *bufLen = totalLen + 4;

    writeIndex = writeInt(writeIndex, totalLen);
    writeIndex = writeInt(writeIndex, err);
    writeIndex = writeString(writeIndex, errStr, strlen(errStr));

    if (fd > 0) {
        close(fd);
        fd = -1;
    }

    if (filePath != NULL) {
        free(filePath);
    }

    //printf("write file, err:%d, errstr: %s\n", err, errStr);
    return err;
}

int WriteFileEnd(char *fileName, char *md5, char **buf, uint32_t *bufLen)
{
    int err = 0;
    char *errStr = "";
    char *filePath= GetRealPath(fileName);
    uint32_t totalLen = 0;
    char *writeIndex = NULL;
    char tempFilePath[256] = {'\0'};
    int fd = -1;
    char *localMd5 = NULL;

    char *_tempFilePath = strdup(filePath);
    char *_tempFileName = strdup(fileName);
    sprintf(tempFilePath, "%s/.%s_%s", dirname(_tempFilePath), md5, basename(_tempFileName));
    free(_tempFilePath);
    free(_tempFileName);

    do {
        /* get file md5 */
        localMd5 = GetMd5FromFile(tempFilePath);
        if (localMd5 == NULL) {
            err = ERR_get_file_md5;
            errStr = "get file md5 failed!";
            break;
        }

        if (strcmp(localMd5, md5) != 0) {
            unlink(tempFilePath);
            err = ERR_md5_not_match;
            errStr = "put file failed, md5 not match!";
            printf("put file, but md5 not match, unlink it!!!, local: %s, client: %s \n",
                    localMd5,
                    md5);
            break;
        }

        /* add index */
        err = AddIndex(fileName, md5);
        if (err != 0) {
            errStr = "add index for file failed!";
            unlink(tempFilePath);
            break;
        }

        /* rename file */
        err = rename(tempFilePath, filePath);
        if (err < 0) {
            err = errno;
            errStr = strerror(err);
            break;
        }

    } while(0);

    totalLen += 4 + 4 + strlen(errStr);
    *buf = malloc(totalLen + 4);
    writeIndex = *buf;
    *bufLen = totalLen + 4;

    writeIndex = writeInt(writeIndex, totalLen);
    writeIndex = writeInt(writeIndex, err);
    writeIndex = writeString(writeIndex, errStr, strlen(errStr));

    if (filePath != NULL) {
        free(filePath);
    }

    return err;
}


