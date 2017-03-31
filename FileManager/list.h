

#ifndef __LIST_H__
#define __LIST_H__

struct ListTable;

struct ListNode {
    struct ListNode *prev;
    struct ListNode *next;
    void *value;
    struct ListNode* (*insert)(struct ListTable *root, void *value);
};

struct ListTable {
    struct ListNode *root;
};

struct ListTable* createListTable();
struct ListNode* insertListTable(struct ListTable *l, void *value);

#endif


