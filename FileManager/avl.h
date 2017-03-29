

#ifndef __AVL_H_
#define __AVL_H_


struct AVLTable;

struct AVLNode {
    void *key;
    void *value;
    struct AVLNode *parent;
    struct AVLNode *leftChild;
    struct AVLNode *rightChild;
    int hight;

    /* function zone */
    int (*compare)(void *leftKey, void *rightkey);
    void (*free)(struct AVLNode *tree);
    void (*freeNode)(struct AVLNode *node);
    void (*inOrder)(struct AVLNode *tree);

    /* return pointer to pointer, it's helpful in insert and remove */
    struct AVLNode* (*search)(struct AVLNode *tree, void *key);
    struct AVLNode* (*insert)(struct AVLTable *table, void *key, void *value);

    /* @return: 0 success, 1 failed*/
    int (*remove)(struct AVLTable *table, char *key);

    int (*getHight)(struct AVLNode *node);
    int (*setHight)(struct AVLNode *node);
};


/* user AVLTable to wrap AVLNode
 * */

typedef int (*CmpType)(void *leftKey, void *rightKey);

struct AVLTable {
    struct AVLNode * root;
    CmpType cmp;
};

struct AVLTable* createTable(CmpType m); 
struct AVLNode* findNode(struct AVLTable *table, void *key);
struct AVLNode* insertNode(struct AVLTable *table, void *key, void *value);
/* success remove return 0 , falied remove return 1 */
int deleteNode(struct AVLTable *table, void *key);
void freeTable(struct AVLTable *table);
void traverseTable(struct AVLTable *table);

#endif


