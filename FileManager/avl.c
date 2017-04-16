

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "avl.h"

static struct AVLNode* _mallocNode(int (*cmp)(const void *leftKey, const void *rightKey));

#define IsLChild(n) ((n)->parent != NULL && ((n)->parent->leftChild == (n)))
#define IsRChild(n) ((n)->parent != NULL && ((n)->parent->rightChild == (n)))


static void _swap_data(struct AVLNode *left, struct AVLNode *right)
{
    char *tempKey = left->key;
    int *tempValue = left->value;
    left->key = right->key;
    left->value = right->value;

    right->key = tempKey;
    right->value = tempValue;
}

static struct AVLNode *_succ(struct AVLNode *node)
{
    if (node->rightChild == NULL) {
        struct AVLNode* parent = node->parent;
        struct AVLNode* tempNode = node;
        while(parent != NULL && parent->rightChild == tempNode) {
            tempNode = parent;
            parent = parent->parent;
        }
        return parent;
    } else {
        struct AVLNode* rc = node->rightChild;
        while (rc->leftChild != NULL) {
            rc = rc->leftChild;
        }

        return rc;
    }
}

static int _get_hight(struct AVLNode *node)
{
    if (node == NULL) {
        return -1;
    }

    return node->hight;
}

static int _set_hight(struct AVLNode *node)
{

    int leftChildHight = _get_hight(node->leftChild);
    int rightChildHight = _get_hight(node->rightChild);

    return (node->hight = (leftChildHight > rightChildHight)? (leftChildHight + 1):(rightChildHight + 1));
}

static void _update_hight(struct AVLNode *node)
{
    while (node != NULL) {
        node->setHight(node);
        node = node->parent;
    }
}

static int _not_balanced(struct AVLNode *node)
{
    int leftChildHight = _get_hight(node->leftChild);
    int rightChildHight = _get_hight(node->rightChild);
    int diff = leftChildHight - rightChildHight;

    if (diff < -1 || diff > 1) {
        return 1;
    }

    return 0;
}

static struct AVLNode* _tallChild(struct AVLNode *node)
{
    return (_get_hight(node->leftChild) > _get_hight(node->rightChild))?
        node->leftChild:node->rightChild; 
}

static struct AVLNode* _connect34(struct AVLNode *left, struct AVLNode *mid, struct AVLNode *right,
        struct AVLNode *ll, struct AVLNode *lm, struct AVLNode *rm, struct AVLNode *rr)
{
    left->leftChild = ll; 
    if (ll != NULL) {
        ll->parent = left;
    }
    left->rightChild = lm;
    if (lm != NULL) {
        lm->parent = left;
    }
    _update_hight(left);

    right->leftChild = rm;
    if (rm != NULL) {
        rm->parent = right;
    }
    right->rightChild = rr;
    if (rr != NULL) {
        rr->parent = right;
    }
    _update_hight(right);

    left->parent = mid;
    right->parent = mid;
    mid->leftChild = left;
    mid->rightChild = right;
    _update_hight(mid);

    return mid;
}

static struct AVLNode* _rotateAt(struct AVLNode *node)
{
    struct AVLNode *parent = node->parent;
    struct AVLNode *gparent = parent->parent;

    if (IsLChild(parent)) {
        if (IsLChild(node)) {
            /* LL */
            parent->parent = gparent->parent;
            return _connect34(node, parent, gparent,
                    node->leftChild, node->rightChild, parent->rightChild, gparent->rightChild);
        } else {
            /* LR */
            node->parent = gparent->parent;
            return _connect34(parent, node, gparent,
                    parent->leftChild, node->leftChild, node->rightChild, gparent->rightChild);
        }
    } else {
        if (IsLChild(node)) {
            /* RL */
            node->parent = gparent->parent;
            return _connect34(gparent, node, parent,
                    gparent->leftChild, node->leftChild, node->rightChild, parent->rightChild);
        } else {
            /* RR */
            parent->parent = gparent->parent;
            return _connect34(gparent, parent, node,
                    gparent->leftChild, parent->leftChild, node->leftChild, node->rightChild);
        }
    }

    assert(0);
}

static void _free_node(struct AVLNode *node)
{
    /* TODO free more */
    node->leftChild = NULL;
    node->rightChild = NULL;
    node->parent = NULL;
    free(node->key);
    free(node->value);
    free(node);
}

static void _free_tree(struct AVLNode* tree)
{
   if (tree == NULL) {
       return;
   }

   if (tree->leftChild != NULL) {
       tree->free(tree->leftChild); 
   }

   if (tree->rightChild != NULL) {
       tree->free(tree->rightChild);
   }

   _free_node(tree);
}

static struct AVLNode** _search_in(struct AVLNode **ptree, struct AVLNode **phot, char *key)
{
    struct AVLNode *tree = *ptree;
    if (!tree || tree->compare(tree->key, key) == 0) {
        return ptree;
    }

    *phot = tree;

    return _search_in((tree->compare(tree->key, key) < 0? &tree->rightChild:&tree->leftChild),
            phot,
            key);
}

static struct AVLNode* _search(struct AVLNode *tree, void *key)
{
    struct AVLNode* hot = NULL;
    struct AVLNode** ret = _search_in(&tree, &hot, key);
    return *ret;
}   


static struct AVLNode* _insert(struct AVLTable *table, void *key, void *value, int* err)
{
    struct AVLNode* hot = NULL;
    struct AVLNode* temp= NULL;
    struct AVLNode** pfound = NULL;
    struct AVLNode* tree = table->root;
    pfound = _search_in(&tree, &hot, key);

    if(*pfound != NULL) {
        *err = 1;
        return *pfound;
    }

    *err = 0;

    /* malloc value */
    *pfound = _mallocNode(table->cmp);

    /* copy key */
    char *stor = malloc(strlen(key) + 1);
    bzero(stor, strlen(key) + 1);
    strncpy(stor, key, strlen(key));

    (*pfound)->key = stor;
    (*pfound)->value = value;

    /* set parent */
    (*pfound)->parent = hot;

    /* rotate */
    for (;hot; hot = hot->parent) {
        if (_not_balanced(hot)) {
            struct AVLNode **pparent = NULL;
            pparent = (hot == table->root)?&table->root:((hot == hot->parent->leftChild)?&hot->parent->leftChild:&hot->parent->rightChild);
            *pparent = _rotateAt(_tallChild(_tallChild(hot)));
            break;
        } else {
            _update_hight(hot);
        }
    }

    return *pfound;
}

static struct AVLNode* _remove_at(struct AVLNode **pfound, struct AVLNode ** phot)
{
    /* 从树中删除一个元素，返回值位接替该位置元素指针,
     * pfound为接替元素的父节点的某一个孩子指针地址
     * phot 返回被删除的元素的父节点
     * */
    struct AVLNode *toDel= NULL;
    struct AVLNode *succ = NULL;

    toDel = *pfound;
    /* only left child */
    if (toDel->leftChild != NULL && toDel->rightChild == NULL) {
        *pfound = succ = toDel->leftChild;
    /* only right child */
    } else if (toDel->leftChild == NULL && toDel->rightChild != NULL) {
        *pfound = succ = toDel->rightChild;
    } else if (toDel->leftChild != NULL && toDel->rightChild != NULL) {
        /* find it's succ */
        struct AVLNode *succ = _succ(toDel);
        _swap_data(succ, toDel);

        *phot = succ->parent;
        pfound = (*phot == toDel? &(*phot)->rightChild:&(*phot)->leftChild);
        *pfound = succ->rightChild;

        toDel = succ;

        succ = succ->rightChild;
    } else {
        /* null, null*/
        *pfound = succ = NULL;
    }

    if (succ != NULL) {
        (*pfound)->parent = *phot;
    }

    _free_node(toDel);

    return succ;
}

static int _remove(struct AVLTable *table, char *key)
{
    struct AVLNode *hot = NULL;
    struct AVLNode **pfound = NULL;
    struct AVLNode *tree = table->root;

    pfound = _search_in(&tree, &hot, key);
    if (*pfound == NULL) {
        return 1;
    }

    _remove_at(pfound, &hot);

    /* rotate */
    for (;hot; hot = hot->parent) {
        if (_not_balanced(hot)) {
            struct AVLNode ** pparent = NULL;
            pparent = (hot == table->root)?&table->root:
                ((hot == hot->parent->leftChild)?&hot->parent->leftChild:&hot->parent->rightChild);
            /* 与insert的区别是 */
            hot = *pparent = _rotateAt(_tallChild(_tallChild(hot)));
        } 
        _update_hight(hot);
    }

    return 0;
}

static void _inOrder(struct AVLNode *tree)
{
    if (tree == NULL) {
        return;
    }

    _inOrder(tree->leftChild);
    printf("key:%s, value:%d, hight:%d\n", (char*)tree->key, *((int*)tree->value), tree->hight);
    _inOrder(tree->rightChild);
}

struct AVLNode* _mallocNode(int (*cmp)(const void *leftKey, const void *rightKey))
{
    struct AVLNode *ret = malloc(sizeof(struct AVLNode));

    ret->parent = NULL;
    ret->leftChild = NULL;
    ret->rightChild = NULL;
    ret->compare = cmp;
    ret->search = _search;
    ret->insert = _insert;
    ret->free = _free_tree;
    ret->freeNode = _free_node;
    ret->inOrder = _inOrder;
    ret->remove = _remove;
    ret->getHight = _get_hight;
    ret->setHight = _set_hight;
    ret->hight = 0;
}

struct AVLTable* createTable(CmpType cmp)
{
    struct AVLTable *t = (struct AVLTable *)malloc(sizeof(struct AVLTable));
    t->cmp = cmp;
    t->size = 0;
    t->root = NULL;

    return t;
}

struct AVLNode* findNode(struct AVLTable* table, void *key)
{
    return table->root->search(table->root, key);
}

struct AVLNode* insertNode(struct AVLTable* table, void *key, void *value)
{
    int err = 0;
    if (table->root == NULL) {
        table->root = _mallocNode(table->cmp);
        table->root->key = key;
        table->root->value = value;
        table->size += 1;
        return table->root;
    } else {
        struct AVLNode *ret = NULL;
        ret = table->root->insert(table, key, value, &err);
        if (err != 0) {
            printf("insert failed!!!\n");
            free(key);
            free(value);
        } else {
            table->size += 1;
        }
        return ret;
    }
}

int deleteNode(struct AVLTable* table, void *key)
{
    int err = 0;
    err = table->root->remove(table, key);
    if (err == 0) {
       table->size -= 1;
    }

    return err;
}

void freeTable(struct AVLTable* table)
{
    table->root->free(table->root);
    free(table);
}

void traverse_init(struct AVLTraverseTable *traverseTable, struct AVLTable *avlTable)
{
    traverseTable->avlTable = avlTable;
    traverseTable->cur = NULL;
}

void* traverse_get_first(struct AVLTraverseTable *traverseTable)
{
    struct AVLNode *min = traverseTable->avlTable->root;

    while (min != NULL && min->leftChild != NULL) {
        min = min->leftChild;
    }
    traverseTable->cur = min;

    return min == NULL? NULL:min->value;
}

void* traverse_get_last(struct AVLTraverseTable *traverseTable)
{
    struct AVLNode *max = traverseTable->avlTable->root;

    while (max != NULL && max->rightChild != NULL) {
        max = max->rightChild;
    }
    traverseTable->cur = max;

    return max == NULL?NULL:max->value;
}

void* traverse_get_next(struct AVLTraverseTable *traverseTable)
{
    struct AVLNode *ret = NULL;
    if (traverseTable->cur == NULL) {
        return traverse_get_first(traverseTable);
    }

    ret = _succ(traverseTable->cur);
    traverseTable->cur = ret;
    return ret == NULL?NULL:ret->value;
}


void traverseTable(struct AVLTable *table)
{
    table->root->inOrder(table->root);
}


char *mallocKey(char *key)
{
    char *ret = malloc(strlen(key) + 1);
    bzero(ret, strlen(key) + 1);
    strcpy(ret, key);
    return ret;
}

int *mallocValue(int v)
{
    int *ret = malloc(sizeof(v));
    *ret = v;
    return ret;
}

int charCompare(const void *leftKey, const void *rightKey)
{
    return strcmp((char*)leftKey, (char*)rightKey);
}

#if 0

int main()
{
    struct AVLTable *table = createTable(charCompare);
    insertNode(table, mallocKey("a"), mallocValue(8));
    insertNode(table, mallocKey("b"), mallocValue(8));
    insertNode(table, mallocKey("c"), mallocValue(5));
    insertNode(table, mallocKey("d"), mallocValue(800));
    insertNode(table, mallocKey("e"), mallocValue(90));
    insertNode(table, mallocKey("f"), mallocValue(9));
    insertNode(table, mallocKey("g"), mallocValue(100));
    insertNode(table, mallocKey("h"), mallocValue(101));
    traverseTable(table);

    struct AVLNode *f = findNode(table, "e");
    printf("key:%s, value:%d\n", (char*)f->key, *(int*)f->value);
    deleteNode(table, "b");

    printf("====\n");
    traverseTable(table);


    /* traverse */
    struct AVLTraverseTable *traverseTable = malloc(sizeof(struct AVLTraverseTable));
    traverse_init(traverseTable, table);

    int *data = traverse_get_last(traverseTable);
    printf("last: %d\n", *(int*)traverseTable->cur->value);

    data = traverse_get_first(traverseTable);
    printf("first: %d\n", *(int*)traverseTable->cur->value);

    while((data = traverse_get_next(traverseTable)) != NULL) {
        printf("val: %d\n", *(int*)data);
    }

    freeTable(table);
    free(traverseTable);
    return 0;
}

#endif


