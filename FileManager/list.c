

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
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
    struct ListNode **tail = &table->tail;
    struct ListNode *new = (struct ListNode*)malloc(sizeof(struct ListNode));
    new->next = NULL;
    new->prev = *tail;
    new->insert = _insert;
    new->value = value;

    (*tail)->next = new;
    *tail = new;
    return new;
}

struct ListTable *createListTable()
{
    struct ListTable *table = (struct ListTable*)malloc(sizeof(struct ListTable));
    table->root = NULL;
    table->tail = NULL;
    table->size = 0;

    return table;
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
        new->value = value;

        table->root = new;
        table->tail = new;
        return new;
    } else {
        return table->root->insert(table, value);
    }
}

int deleteListItem(struct ListTable *table, void *value)
{
    int err = ENOENT;
    struct ListNode *idx = NULL;
    do {
        if (table->root == NULL) {
            break;
        }

        idx = table->root;
        while (idx != NULL) {
           if (idx->value == value) {
               err = 0;

               if (idx->prev != NULL) {
                   idx->prev->next = idx->next;
               }

               if (idx->next != NULL) {
                   idx->next->prev = idx->prev;
               }

               if (idx == table->root) {
                   table->root = idx->next;
               }

               if (idx == table->tail) {
                   table->tail = idx->prev;
               }

               --table->size;

               free(idx);
               break;
           }

           idx = idx->next;
        }
    } while(0);

    return err;
}






