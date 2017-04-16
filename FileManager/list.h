

#ifndef __LIST_H__
#define __LIST_H__

#include <stdint.h>
struct ListTable;

struct ListNode {
    struct ListNode *prev;
    struct ListNode *next;
    void *value;
    struct ListNode* (*insert)(struct ListTable *root, void *value);
};

struct ListTable {
    struct ListNode *root;
    struct ListNode *tail;
    uint32_t size;
};

struct ListTable* createListTable();
struct ListNode* insertListItem(struct ListTable *l, void *value);

#endif


