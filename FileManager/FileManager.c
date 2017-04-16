

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

#include "FileManager.h"
#include "avl.h"
#include "list.h"
#include "../Main/Util.h"
#include "Err.h"

static struct FileManager *g_file_manager = NULL;
static pthread_mutex_t s_mutex;

static void Lock()
{

}

static void UnLock()
{

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
    int leftLen = strlen(left);
    int rightLen = strlen(right);
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

int ListDir(const char *dirName, char **buf, int *bufLen) {
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

    assert(writeIndex == (*buf + len + 4));
    return err;
}

int LockFile(char *fileName, int lockType, int fileType, char **buf, int *bufLen)
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
            if (node == NULL)  {
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

        lock->type = LOCK_read;
    } while(0);

    return err;
}




