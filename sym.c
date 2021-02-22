//
//  sym.c
//  tinycc
//
//  Created by sanluisrey on 2021/02/15.
//

#include "tinycc.h"

// 現在のスコープ
static int level;

static Scope ids = {GLOBAL};
static Scope cnt = {CONST};
Scope *globals = &ids;
Scope *identifiers = &ids;
Scope *strings = &cnt;
// コンストラクター
Scope *scope(Scope *p, int level) {
   Scope *new = calloc(1, sizeof(Scope));
   new->previous = p;
   new->level = level;
   return new;
}

Var *install(char *name, Scope **spp, int level, Type *ty) {
    Scope *sp = *spp;
    if(level > 0 && sp->level < level) sp = scope(sp, level);
    Var *p = calloc(1, sizeof(Var));
    p->str = name;
    p->len = (int) strlen(name);
    p->type = ty;
    p->next = sp->entry;
    p->scope = level;
    sp->entry = p;
    *spp = sp;
    return p;
}

Var *lookup(char *name, Scope *sp) {
    Var *p;
    do {
        p = sp->entry;
        do {
            if(p && !strcmp(name, p->str)) return p;
        } while (p && (p = p->next) != NULL);
    } while ((sp = sp->previous) != NULL);
    return NULL;
}

void enterscope() {
   ++level;
}

void exitscope() {
    if(identifiers->level == level) {
        identifiers = identifiers->previous;
    }
    assert(level >= GLOBAL);
    --level;
}

void initscope() {
    level = GLOBAL;
}

int getlevel() {
    return level;
}
