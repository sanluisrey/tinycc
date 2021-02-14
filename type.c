//
//  type.c
//  tinycc
//
//  Created by sanluisrey on 2021/02/12.
//

#include "tinycc.h"

// 引数の型のサイズを求める。
int type_size(Type *type) {
    switch (type->ty) {
        case INT:
            return 4;
            
        case PTR:
            return 8;
            
        case ARRAY:{
            return (int) type->array_size * type_size(type->ptr_to);
        }
            
        case CHAR:
            return 1;
    }
}

Type *type(int op, Type *operand) {
    Type *ret = calloc(1, sizeof(Type));
    ret->ty = op;
    ret->ptr_to = operand;
    ret->size = type_size(ret);
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
/*Type *array(Type *ty) {
    TODO
};*/
Type *atop(Type *ty) {
    if (isarray(ty)) {
        return ptr(ty->ptr_to);
    }
    error("type error: %s\n", "array expected");
    return ptr(ty);
};
