

#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

enum FileType {
    File_dir,
    File_reg
};

enum FileLock {
    Lock_null,
    Lock_read,
    Lock_write
};

struct FileNode {
    enum FileLock lock;
    enum FileType type;
    int ref;
    char *name;
    /* only available in reg file */
    char *md5;
    struct FileNode *leftDir;
    struct FileNode *rightDir;
    struct FileNode *leftReg;
    struct FileNode *rightReg;

    /* Function Zone*/

    enum FileType (* GetType)(struct FileNode *node);
    int (*Equal)(struct FileNode *node, char *name);
};


int InitFileManager(char *dir);


#endif

