

#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

#include "FileManager.h"
#include "avl.h"
#include "list.h"

static struct FileManager *g_file_manager = NULL;
static pthread_mutex_t s_mutex;

static void Lock()
{

}

static void UnLock()
{

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

    SearchDir(repo, root);
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

                /* add to g_file_manager dirTable */
                struct Dir *dir = (struct Dir*)malloc(sizeof(struct Dir));
                dir->type = FILE_dir;
                dir->lock.type = LOCK_null;
                dir->lock.userId = -1;
                dir->refCount = 0;
                dir->fileTable = NULL;
                char *temp = addStr(entry->d_name, "/");
                dir->name = addStr(parent->name, temp);
                printf("dirname: %s\n", dir->name);
                free(temp);
                dir->parent = parent;

                insertNode(g_file_manager->dirTable, addStr(parent->name, entry->d_name), dir);

                if (parent->subDirTable == NULL) {
                    parent->subDirTable = createListTable();
                }

                insertListItem(parent->subDirTable, dir);

                char *nextDir = addStr(entry->d_name, "");
                SearchDir(nextDir, dir);

                free(nextDir);
            } else if (S_ISREG(statbuf.st_mode)) {
                /* add to dir fileTable*/
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

int ListDir(const char *dirName, char *buf) {
    uint32_t dirCount = 0;
    uint32_t fileCount = 0;
    int err = 0;
    void *node = NULL;
    struct Dir *dir = NULL;
    uint32_t len = 0;

    len += 4;

    do {
        node = findNode(g_file_manager->dirTable, dirName);
        if (node == NULL) {
            err = EINVAL;
            break;
        }

        dir = (struct Dir *) node;
        struct ListNode *item = dir->subDirTable->root;
        dirCount = dir->subDirTable->size;
        /* subdir count */
        len += 4;
        while (item != NULL) {
            struct Dir*subdir = (struct Dir*)item;
            len += strlen(subdir->name);
            item = item->next;
        }

        /* file count */
        len += 4;


    } while(0);

    return err;
}



