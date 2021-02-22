//
//  list.c
//  tinycc
//
//  Created by sanluisrey on 2021/02/18.
//

#include "tinycc.h"

static List *listhead;

// 汎用循環リスト
List *append(void *x, List *list) {
    List *new = calloc(1, sizeof(List));
    if (list == NULL) {
        new->next = new;
        listhead = new;
    } else {
        new->next = list->next;
        list->next = new;
    }
    new->x = x;
    return new;
}

int list_length(List *list) {
    int n = 0;
    if (list != NULL) {
        List *lp = list;
        do {
            n++;
        } while ((lp = lp->next) != list);
    }
    return n;
}

void *ltov(List **list) {
    int i = 0;
    void **array;
    array = (void **) calloc(list_length(*list) + 1, sizeof(void*));
    if (*list) {
        List *lp = *list;
        do {
            lp = lp->next;
            array[i++] = lp->x;
        } while (lp != *list);
    }
    *list = NULL;
    array[i] = NULL;
    return array;
}

List *get_head() {
    return listhead;
}

void list_delete(List **list) {
    *list = NULL;
}
