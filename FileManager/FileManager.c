

#include <stdlib.h>
#include "FileManager.h"


static struct FileNode *RootDir = NULL;

struct FileNode* AllocNode()
{
    return (struct FilNode*) malloc(sizeof(struct FileNode));
}

int InitFileManager(char *dir)
{
    RootDir = AllocNode();
}




