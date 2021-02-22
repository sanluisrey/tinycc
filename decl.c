//
//  decl.c
//  tinycc
//
//  Created by sanluisrey on 2021/02/15.
//

#include "tinycc.h"


static int stack_size;

// 次のトークンが引数の記号と等しいかどうか真偽を返す。
bool equal(char *op, Token **rest) {
    Token *token = *rest;
    if(token->len != strlen(op)) return false;
    return strncmp(token->str, op, strlen(op)) == 0;
}

// 次のトークンが引数の記号と等しいかどうか真偽を返す。
bool equal_tk(TokenKind kind, Token **rest) {
    Token *token = *rest;
    return token->kind == kind;
}

// 次のトークンが期待している記号の時には、トークンを一つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op, Token **rest){
    Token *token = *rest;
    if (token->kind != TK_RESERVED || strncmp(token->str, op, strlen(op)) != 0) {
        return false;
    }
    *rest = token->next;
    return true;
}

// 次のトークンが期待している記号のときには、トークンを一つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op, Token **rest){
    Token *token = *rest;
    if (token->kind != TK_RESERVED || strncmp(token->str, op, strlen(op)) != 0) {
        error_tok(token, "'%s'ではありません", op);
    }
    *rest = token->next;
}

// 次のトークンが数値の場合、トークンを一つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number(Token **rest){
    Token *token = *rest;
    if (token->kind != TK_NUM) {
        error_tok(token, "数ではありません");
    }
    int val = token->val;
    *rest = token->next;
    return val;
}

// トークンから型構造体を作成し、一つ読み進める
Type *new_type(TypeKind ty, Token **rest, Token *tok) {
    *rest = tok->next;
    switch (ty) {
        case INT:
            return IntType;
        
        case CHAR:
            return CharType;
            
        default:{
            Type *ret = calloc(1, sizeof(Type));
            ret->ty = ty;
            return ret;
        }
    }
}


// TK_EOF
// 現在のトークンがEOFかどうかを返す。
bool at_eof(Token *token) {
    return token->kind == TK_EOF;
}

// 次のトークンの種類がkindの時には、トークンを一つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume_token(TokenKind kind, Token **rest) {
    Token *token = *rest;
    if (token->kind != kind) {
        return false;
    }
    *rest = token->next;
    return true;
}

// 次のトークンの種類がkindの時には、トークンを一つ読み進める
// それ以外の場合にはエラー処理。
void expect_token(TokenKind kind, Token **rest) {
    Token *token = *rest;
    if (token->kind != kind) {
        error_tok(token, "期待したトークン種類ではありません");
    }
    *rest = token->next;
}

// ローカル変数と仮引数のスタックトップからのオフセットの設定と
// 関数全体のスタックサイズの設定を行う(dclparamとdecltn内)
void set_offset(Var *p) {
    stack_size += roundup(p->type->size, 8);
    p->offset = stack_size;
}

/*
 trns_unit      =   ex_decltn
                |   trns_unit ex_decltn
 ex_decltn      =   func_def
                |   decltn
 func_def       =   decltn_spcf? decltr decltn_list? cmp_stmt
 decltn         =   decltn_spcf ini_decltr_lst? ";"
 decltn_list    =   decltn
                |   decltn_list decltn
 decltn_spcf    =   type_spcf decltn_spcf? (TODO)
 type_spcf      =   char | int  (TODO)
 ini_decltr_lst =   ini_decltr  (TODO)
                |   ini_decltr_lst ini_decltr
 ini_decltr     =   decltr
                |   decltr "=" initzr (TODO)
 decltr         =   pointer? drct_decltr
 drct_decltr    =   ident
                |   "(" decltr ")"
                |   drct_decltr "[" cnst_expr? "]"
                |   drct_decltr "(" p_type_lst ")"
                |   drct_decltr "(" ident_lst? ")"
 p_type_lst     =   p_list
                |   p_list "," "..." (TODO)
 p_list         =   p_decltn
                |   p_list "," p_decltn
 p_decltn       =   decltn_spcf decltr
                |   decltn_spcf abst_decltr?
 ident_lst      =   ident
                |   ident_lst "," ident
 */

Function *func_defn(Token **, char *, Type *, Var *[]);
Var *decltn(Token **, char *, Type *);
Type *decltn_spcf(Token **);
Type *decltr(Token **, char **, Type *, Var ***);
void exitparams(Var *params[]);
Type *decltr1(Token **rest, char **id, Var ***params);
Var **parameters(Token **, Type *);

Code *trns_unit(Token **rest) {
    Function **flist;
    List *list = NULL;
    for (; ; ) {
        if(at_eof(*rest)) break;
        initscope();
        Function *car = ex_decltn(rest);
        if(car != NULL) list = append(car, list);
    }
    int n = list_length(list);
    flist = calloc(n, sizeof(flist[0]));
    Function **temp = ltov(&list);
    for (int i = 0; i < n; i++) {
        flist[i] = temp[n - i - 1];
    }
    Code *ret = calloc(1, sizeof(Code));
    ret->function = flist;
    ret->n = n;
    return ret;
}

Function *ex_decltn(Token **rest) {
    // tyは型指定子、ty1は宣言子
    Type *ty, *ty1;
    char *id = NULL;
    ty = decltn_spcf(rest);
    if (getlevel() == GLOBAL) {
        // 1回目の宣言子をパース。 TODO 2回目以降
        // 関数の変数宣言のためのスタック領域初期化と記号表の初期化
        Var **params = NULL;
        stack_size = 0;
        identifiers = globals;
        ty1 = decltr(rest, &id, ty, &params);
        // 関数の宣言・定義のどちらが続くのか確認
        // 関数定義ならばdecltnによるパースに移らない
        // TODO 関数宣言の実装
        if (equal("{", rest)) {
            Function *ret = func_defn(rest, id, ty1, params);
            ret->stack_size = stack_size;
            return ret;
        }
    } else {
        ty1 = decltr(rest, &id, ty, NULL);
    }
    for (; ; ) {
        decltn(rest, id, ty1);
        if (!consume(",", rest)) {
            break;
        }
    }
    expect(";", rest);
    return NULL;
}

//decltn         =   decltn_spcf ini_decltr_lst? ";"
// TODO global変数、仮引数、ローカル変数で文法の異なる部分は実装しない
Var *decltn(Token **rest, char *id, Type *ty) {
    Var *p;
    p = lookup(id, identifiers);
    // TODO redeclaration error check
    if (p == NULL) {
        if (getlevel() == GLOBAL) p = install(id, &globals, GLOBAL, ty);
        else {
            p = install(id, &identifiers, getlevel(), ty);
            // TODO スコープや関数ごとにスタックサイズを設定
            set_offset(p);
        }
    } else if(p->scope < getlevel()) {
        p = install(id, &identifiers, getlevel(), ty);
        // TODO スコープや関数ごとにスタックサイズを設定
        set_offset(p);
    } else {
        error("redeclaration error");
    }
    p->type = ty;
    return p;
}

//decltn_spcf    =   type_spcf decltn_spcf? (TODO)
Type *decltn_spcf(Token **rest) {
    return type_spcf(rest, *rest);
}
//type_spcf      =   char | int  (TODO 型の追加)
Type *type_spcf(Token **rest, Token *tok) {
    if (equal("char", rest)) {
        return new_type(CHAR, rest, tok);
    } else if(equal("int", rest)) {
        return new_type(INT, rest, tok);
    } else {
        return NULL;
    }
};

/*
 decltr         =   pointer? drct_decltr
 drct_decltr    =   ident
                |   "(" decltr ")"
                |   drct_decltr "[" cnst_expr? "]"
                |   drct_decltr "(" p_type_lst ")"
                |   drct_decltr "(" ident_lst? ")"
 */
Type *decltr(Token **rest, char **id, Type *basety, Var ***params) {
    Type *ty = decltr1(rest, id, params);
    for (; ty; ty = ty->ptr_to) {
        switch (ty->ty) {
            case PTR:
                basety = ptr(basety);
                break;
                
            case FUNCTION:
                basety = func(basety, ty->p_list);
                break;
                
            case ARRAY:
                basety = array(basety, ty->array_size);
                break;
        }
    }
    return basety;
};
static Type *tnode(int op, Type *type) {
    Type *ty = calloc(1, sizeof(Type));
    ty->ty = op;
    ty->ptr_to = type;
    return ty;
}
// 逆順の型式
Type *decltr1(Token **rest, char **id, Var ***params) {
    Type *ty = NULL;
    Token *tok = *rest;
    if (consume("(", rest)) {
        ty = decltr1(rest, id, params);
        expect(")", rest);
        return ty;
    } else if(consume("*", rest)) {
        ty = decltr1(rest, id, params);
        ty = tnode(PTR, ty);
    } else if(consume_token(TK_IDENT, rest)) {
        *id = tok->str;
    } else {
        return ty;
    }
    tok = *rest;
    while (equal("(", rest) || equal("[", rest)) {
        if (consume("(", rest)) {
            Var **args;
            ty = tnode(FUNCTION, ty);
            enterscope();
            if (getlevel() > PARAM) {
                enterscope();
            }
            args = parameters(rest, ty);
            if (params && *params == NULL) *params = args;
            /*if (params && *params == NULL) *params = args;
            else exitparams(params);
             */
        } else {
            consume("[", rest);
            int n = expect_number(rest);
            expect("]", rest);
            ty = tnode(ARRAY, ty);
            ty->array_size = n;
        }
    }
    
    return ty;
}
/*
 p_type_lst     =   p_list
                |   p_list "," "..." (TODO)
 p_list         =   p_decltn
                |   p_list "," p_decltn
 p_decltn       =   decltn_spcf decltr
                |   decltn_spcf abst_decltr?
 */
Var **parameters(Token **rest, Type *fty){
    Var **params = NULL;
    List *list = NULL;
    int n = 0;
    Type *ty1 = NULL;
    // parse new-style prameter list
    if (equal_tk(TK_TYPE, rest)) {
        for (; ; ) {
            char *id = NULL;
            if(!equal_tk(TK_TYPE, rest)) error("missing parameter type");
            n++;
            Type *spcf = decltn_spcf(rest);
            Type *ty = decltr(rest, &id, spcf, NULL);
            // declare a parameter and append it to list
            if (ty1 == NULL) {
                ty1 = ty;
            }
            if(ty != NULL) {
                list = append(dclparam(rest, id, ty), list);
            }
            if (!equal(",", rest)) {
                break;
            }
            consume(",", rest);
        }
        params = ltov(&list);
    }
    // build prototype
    expect(")", rest);
    return params;
}

Var *dclparam(Token **rest, char *id, Type *ty) {
    Var *p;
    if (isfunc(ty)) {
        ty = ptr(ty);
    } else if(isarray(ty)) {
        ty = atop(ty);
    }
    p = lookup(id, identifiers);
    if (p && p->scope == getlevel()) {
        error("duplicate declaration for `%s' previously declared", id);
    } else {
        p = install(id, &identifiers, getlevel(), ty);
        // TODO スコープや関数ごとにスタックサイズを設定
        set_offset(p);
    }
    p->type = ty;
    p->defined = 1;
    return p;
}

// TODO old-style parametersの実装
Function *func_defn(Token **rest, char *id, Type *fty, Var *params[]){
    int i, n = 0;
    Var *p;
    // 定義文の始まりが'{'であるかどうかは呼び出し元が確認するので、単にトークンを一つ進めるのみ
    consume("{", rest);
    // TODO calleeとcallerで引数の型が異なるときの対応
    // 引数の関数型から返却の型を抽出
    Type *rty = get_return_ty(fty);
    if (params) {
        // 引数の数を数える TODO 引数の数が6個より大きい関数への対応
        while (params[n] != NULL) {
            n++;
            if(n > 6) error("the number of parameters > 6");
        }
        for (i = 0; (p = params[i]) != NULL; i++) {
            // TODO main関数のとき返り値の型の指定がなくてもいいが、必ず型が指定されているものとする
            if (strcmp(id, "main") == 0) {
                if(rty->ty != INT) error("'%s()' is a non-ANSI definition");
            }
        }
    }
    p = lookup(id, identifiers);
    if(p && isfunc(p->type) && p->defined) error("redefinition of %s", id);
    Function *ret = calloc(1, sizeof(Function));
    ret->code = cmp_stmt(rest);
    // TODO 本来不要なコード
    consume("}", rest);
    ret->locals = identifiers->entry;
    ret->params = params;
    ret->nparams = n;
    ret->name = id;
    ret->globals = globals->entry;
    // 定義文の終わりが'}'であるかどうか呼び出し先で確認
    //expect("}", rest);
    return ret;
}
void exitparams(Var *params[]) {
    assert(params);
    if (getlevel() > PARAM)
        exitscope();
    exitscope();
}
