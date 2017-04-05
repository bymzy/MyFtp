

#include <stdlib.h>
#include <assert.h>
#include "list.h"

struct ListNode* _found_tail(struct ListNode *root)
{
    if (root == NULL) {
        return root;
    }

    while (root->next != NULL) {
        root = root->next;
    }
    return root;
}

struct ListNode* _insert(struct ListTable *table, void *value)
{
    struct ListNode *tail = _found_tail(table->root);
    struct ListNode *new = (struct ListNode*)malloc(sizeof(struct ListNode));
    new->next = NULL;
    new->prev = tail;
    new->insert = _insert;

    tail->next = new;
    return new;
}

struct ListTable *createListTable()
{
    return (struct ListTable*)malloc(sizeof(struct ListTable));
}

struct ListNode* insertListItem(struct ListTable *table, void *value)
{
    assert(table != NULL);
    table->size += 1;
    if (table->root == NULL) {
        /* malloc root node */
        struct ListNode *new = (struct ListNode*)malloc(sizeof(struct ListNode));
        new->prev = NULL;
        new->next = NULL;
        new->insert = _insert;
        table->root = new;
        return new;
    } else {
        return table->root->insert(table, value);
    }
}



