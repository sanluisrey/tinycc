//
//  type.c
//  tinycc
//
//  Created by sanluisrey on 2021/02/12.
//

#include "tinycc.h"

Type inttype = {INT, 4};
Type chartype = {CHAR, 1};

Type *IntType = &inttype;
Type *CharType = &chartype;

// 引数の型のサイズを求める。
int basety_size(Type *type) {
    switch (type->ty) {
        case INT:
            return 4;
            
        case PTR:
            return 8;
            
        case CHAR:
            return 1;
    }
    return 0;
}

Type *type(int op, Type *operand) {
    Type *ret = calloc(1, sizeof(Type));
    ret->ty = op;
    if (op == FUNCTION) {
        ret->return_ty = operand;
    } else if(op == ARRAY) {
        ret->ptr_to = operand;
        if(operand->base) ret->base = operand->base;
    } else {
        ret->ptr_to = operand;
        ret->size = basety_size(ret);
    }
    return ret;
}
Type *ptr(Type *ty) {
    return type(PTR, ty);
};
Type *deref(Type *ty) {
    if (isptr(ty) || isarray(ty)) {
        return ty->ptr_to;
    }
    error("type error: %s\n", "pointer expected");
    return NULL;
};
Type *array(Type *ty, int n) {
    if (isarray(ty) && ty->size == 0) error("missing array size");
    ty = type(ARRAY, ty);
    ty->array_size = n;
    ty->size = n;
    ty->size *= ty->ptr_to->size;
    if(ty->base) ty->size *= ty->base->size;
    return ty;
};
Type *atop(Type *ty) {
    if (isarray(ty)) {
        return ptr(ty->ptr_to);
    }
    error("type error: %s\n", "array expected");
    return ptr(ty);
};

Type *get_return_ty(Type *ty) {
    if (isfunc(ty)) {
        return ty->return_ty;
    }
    error("type error: %s\n", "not found function return");
    return NULL;
}

Type *func(Type *ty, Type *proto) {
    if (ty && (isarray(ty) || isfunc(ty)))
        error("illegal return type `%t'\n", ty);
    ty = type(FUNCTION, ty);
    ty->p_list = proto;
    if(proto) ty->next = proto->next;
    return ty;
}
