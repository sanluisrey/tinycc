//
//  parser.c
//  tinycc
//
//  Created by sanluisrey on 2020/12/28.
//

#include "tinycc.h"

// ローカル変数リスト
static Var *locals;

// グローバル変数リスト
static Var *globals;

// 関数のスタック領域のサイズ
static int stack_size;

// 抽象構文木の部分木のヘッダー
Node head;

// 抽象構文木の部分木の末尾
Node *tail;

// 配列型のポインタレコードの末尾
static Type *ty_recar;

// 文字列リテラルのリスト
static Token *literals;

// パーサーの宣言
static Function *glbl_def(Token **rest);
static Type *size_list(Token **rest);
static Type *array_sz(Token **rest);
static Type *type_prim(Token **rest);
static void decl(Token **rest, bool global);
static Function *f_head(Token **rest);
static Node *p_list(Token **rest, int depth);
static Node *p_decl(Token **rest);
static Function *func_def(Token **rest);

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

// 変数を名前で検索する。見つからなかった場合はエラー処理。
Var *find_var(Token *tok) {
    Var *cur = NULL;
    int i = 0;
    for (cur = locals; cur != NULL; cur = cur->next, i++) {
        if (tok->len == cur->len && strncmp(tok->str, cur->str, tok->len) == 0) {
            break;
        }
    }
    if (cur == NULL) {
        //error_tok(tok, "Undeclared identifier used");
        return NULL;
    }
    if (!cur->used) {
        cur->used = true;
        if (cur->type->ty == ARRAY) {
            stack_size += 8;
            for (Type *ptr = cur->type->ptr_to; ptr; ptr = ptr->ptr_to) {
                stack_size *= cur->type->array_size;
            }
            stack_size *= 4;
        } else if(cur->type->ty == CHAR) stack_size += 1;
        else stack_size += 8;
        cur->offset = stack_size;
    }
    return cur;
}

// ローカル変数リストへの登録
void lvarDecl(Token *tok, Type *type){
    Var *cur = NULL;
    for (cur = locals; cur != NULL; cur = cur->next) {
        if (tok->len == cur->len && strncmp(tok->str, cur->str, tok->len) == 0 && cur->global == false) {
            error_tok(tok, "Duplicated declaration");
        }
    }
    if (locals == NULL) {
        if (globals == NULL) {
            locals = calloc(1, sizeof(Var));
            locals->str = tok->str;
            locals->len = tok->len;
            locals->type = type;
            locals->global = false;
        } else {
            locals = globals;
            Var *new = calloc(1, sizeof(Var));
            new->len = tok->len;
            new->str = tok->str;
            new->next = locals;
            new->type = type;
            new->global = false;
            locals = new;
        }
    } else {
        Var *new = calloc(1, sizeof(Var));
        new->len = tok->len;
        new->str = tok->str;
        new->next = locals;
        new->type = type;
        new->global = false;
        locals = new;
    }
}
// グローバル変数リストへの登録
void glvarDecl(Token *tok, Type *type){
    for (Var *cur = globals; cur != NULL; cur = cur->next) {
        if (tok->len == cur->len && strncmp(tok->str, cur->str, tok->len) == 0) {
            error_tok(tok, "Duplicated declaration");
        }
    }
    if (globals == NULL) {
        globals = calloc(1, sizeof(Var));
        globals->str = tok->str;
        globals->len = tok->len;
        globals->type = type;
        globals->global = true;
    } else {
        Var *new = calloc(1, sizeof(Var));
        new->len = tok->len;
        new->str = tok->str;
        new->next = globals;
        new->type = type;
        new->global = true;
        globals = new;
    }
}

void varDecl(Token *tok, Type *type, bool global){
    if (global) {
        glvarDecl(tok, type);
    } else {
        lvarDecl(tok, type);
    }
}

// 文字列リストへの登録
Token *strDecl(Token *tok) {
    Token *last = literals;
    for (Token *cur = literals; cur != NULL; cur = cur->next) {
        if (tok->len == cur->len && strncmp(tok->str, cur->str, tok->len) == 0) {
            return cur;
        }
        last = cur;
    }
    if (literals == NULL) {
        literals = calloc(1, sizeof(Token));
        literals->str = tok->str;
        literals->len = tok->len;
        literals->pos = 1;
        last = literals;
    } else {
        Token *new = calloc(1, sizeof(Token));
        new->str = tok->str;
        new->len = tok->len;
        new->pos = last->pos + 1;
        last->next = new;
        last = new;
    }
    return last;
}

// 1次型式のパース TODO char*
Type *RefType(Token **rest, Type *type) {
    Token *tok = *rest;
    if(strncmp(tok->str, "*", 1) == 0) {
        Type *ret = calloc(1, sizeof(Type));
        ret->ty = PTR;
        *rest = tok->next;
        ret->ptr_to = RefType(rest, type);
        return ret;
    } else return type;
}

// 仮引数の変数リストへの登録
void pvarDecl(Node *args, Token *tok) {
    Var *cur = NULL;
    for (cur = locals; cur != NULL; cur = cur->next) {
        if (strlen(args->name) == cur->len && strncmp(args->name, cur->str, cur->len) == 0) {
            error_tok(tok, "Duplicated declaration");
        }
    }
    if (locals == NULL) {
        locals = calloc(1, sizeof(Var));
        locals->str = args->name;
        locals->len = (int) strlen(args->name);
        locals->offset = args->ireg * 8;
        locals->used = true;
        locals->type = args->type;
    } else {
        Var *new = calloc(1, sizeof(Var));
        new->len = (int) strlen(args->name);
        new->str = args->name;
        new->offset = args->ireg * 8;
        new->used = true;
        new->next = locals;
        new->type = args->type;
        locals = new;
    }
}

// トークンから型構造体を作成し、一つ読み進める
Type *new_type(TypeKind ty, Token **rest, Token *tok) {
    Type *ret = calloc(1, sizeof(Type));
    ret->ty = ty;
    *rest = tok->next;
    return ret;
}

// 次のトークンの種類がTK_IDENTの時には、ローカル変数リストを検索し、トークンを一つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume_ident(Token **rest, int *offset) {
    Token *token = *rest;
    if (token->kind != TK_IDENT) {
        return false;
    }
    Var *lvar = find_var(token);
    *offset = lvar->offset;
    *rest = token->next;
    return true;
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

/*
 生成規則(TODO) K&R bookに合わせる
 trns_unit      =   ex_decltn
                |   trns_unit ex_decltn
 ex_decltn      =   func_def
                |   decltn
 func_def       =   decltn_spcf? decltr decltn_list? cmp_stmt
 decltn         =   decltn_spcf ini_decltr_lst? ";"                type.c
 decltn_list    =   decltn                                      type.c
                |   decltn_list decltn
 decltn_spcf    =   type_spcf decltn_spcf? (TODO)               type.c
 type_spcf      =   char | int  (TODO)                          type.c
 ini_decltr_lst =   ini_decltr                                  type.c
                |   ini_decltr_lst ini_decltr
 ini_decltr     =   decltr
                |   decltr "=" initzr (TODO)
 decltr         =   pointer? drct_decltr
 drct_decltr    =   ident
                |   "(" decltr ")"
                |   drct_decltr "[" cnst_expr? "]"
                |   drct_decltr "(" p_type_lst ")"
                |   drct_decltr "(" ident_lst? ")"
 pointer        =   "*"                                         type.c
                |   pointer (TODO)
 p_type_lst     =   p_list
                |   p_list "," "..." (TODO)                     type.c
 p_list         =   p_decltn                                    type.c
                |   p_list "," p_decltn
 p_decltn       =   decltn_spcf decltr                          type.c
                |   decltn_spcf abst_decltr?
 ident_lst      =   ident
                |   ident_lst "," ident
 initzr         =   assign  (TODO)
 initzr_lst     (TODO)
 abst_decltr    =   pointer                                     type.c
                |   pointer? d_abst_decltr
 d_abst_decltr  =   "(" abst_decltr ")"                         type.c
                |   d_abst_decltr? "[" cnst_expr? "]"
                |   d_abst_decltr? "(" p_type_list ")"
 stmt           =   expr_stmt
                |   cmp_stmt
                |   select_stmt
                |   jump_stmt
                |   iter_stmt
 expr_stmt      =   expr? ";"
 cmp_stmt       =   "{" decltn_lst? stmt_lst? "}"
 stmt_lst       =   stmt
                |   stmt_lst stmt
 select_stmt    =   "if(" expr ")" stmt
                |   "if(" expr ")" stmt "else" stmt
 iter_stmt      =   "while(" expr ")" stmt
                |   "for(" expr? ";" expr? ";" expr ")" stmt
 jump_stmt      =   "return" expr? ";"
 expr           =   assign
                |   expr "," assign
 assign         =   cond_expr
                =   unary assign_op assign
 assign_op      =   "="
 cond_expr      =   equality
 cnst_expr      =   cond_expr
 equality       =   relational
                |   equality "==" relational
                |   equality "!=" relational
 relational     =   add
 add            =   mul
                |   add "+" mul
                |   add "-" mul
 mul            =   mul "*" unary
                |   mul "/" unary
 unary          =   postfix
                |   unary_op unary
                |   "sizeof" unary
                |   "sizeof" "(" type_name ")"
 unary_op       =   "&" | "*" | "+" | "-"
 postfix        =   primary
                |   postfix "[" expr "]"
                |   postfix "(" agument_lst? ")"
 primary        =   ident | string | "(" expr ")"
 argument_lst   =   assign
                |   argument_lst "," assign
 */

Function *program(Token **rest) {
    Function *ret = glbl_def(rest);
    ret->literals = literals;
    return ret;
}

Function *glbl_def(Token **rest) {
    Token *tok = *rest; // 先読みのため定義
    if(at_eof(tok)) return NULL;
    while (func_def(&tok) == NULL) {
        decl(rest, true);
        expect(";", rest);
        tok = *rest;
    }
    Function *car = func_def(rest);
    if (car == NULL) {
        return NULL;
    }
    car->globals = globals;
    Function *cdr = glbl_def(rest);
    car->next = cdr;
    return car;
};
// func_def       =   decltn_spcf? decltr decltn_list? cmp_stmt
Function *func_def(Token **rest){
    decltn_spcf(rest);
    locals = globals;
    Function *ret = f_head(rest);
    if (ret == NULL || !equal("{", rest)) {
        return NULL;
    }
    Node head;
    Node *cur = &head;
    stack_size = ret->nparams * 8;
    cur->next = cmp_stmt(rest);
    ret->code = head.next;
    ret->locals = locals;
    ret->stack_size = stack_size;
    return ret;
};
/*
 decltn_spcf    =   type_spcf decltn_spcf? (TODO)
 */
Type *decltn_spcf(Token **rest) {
    return type_spcf(rest, *rest);
};
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
//decltr         =   pointer? drct_decltr

/*
drct_decltr    =   ident
               |   "(" decltr ")"
               |   drct_decltr "[" cnst_expr? "]"
               |   drct_decltr "(" p_type_lst ")"
               |   drct_decltr "(" ident_lst? ")"
 */
Function *f_head(Token **rest){
    Token *tok = *rest;
    if (tok->kind != TK_IDENT) return NULL;
    Function *ret = calloc(1, sizeof(Function));
    ret->name = tok->str;
    consume_token(TK_IDENT, rest);
    tok = *rest;
    if (!consume("(", rest)) {
        return NULL;
    }
    Node *args = p_list(rest, 0);
    ret->args = args;
    if(args != NULL) ret->nparams = args->ireg;
    if (!consume(")", rest)) {
        return NULL;
    }
    return ret;
};
// TODO char*
Type *type_prim(Token **rest){
    Token *tok = *rest;
    Type *type = calloc(1, sizeof(Type));
    if (consume_token(TK_TYPE, rest)) {
        if (strncmp(tok->str, "char", 4) == 0) {
            type->ty = CHAR;
        } else {
            type->ty = INT;
        }
    } else error_tok(tok, "型がありません。");
    return RefType(rest, type);
};

// size_list  = array_sz
//            | array_sz size_list
// array_sz   = "[" "]"
//            | "[" num "]"
// size_list[0] -> size_list[1] -> ... -> ty_recar
Type *size_list(Token **rest){
    Type *car = array_sz(rest);
    if(car == NULL) return NULL;
    Type *cdr = size_list(rest);
    if(cdr == NULL) car->ptr_to = ty_recar;
    else car->ptr_to = cdr;
    return car;
}

Type *array_sz(Token **rest){
    if(!consume("[", rest)) return NULL;
    Type *ret = calloc(1, sizeof(Type));
    ret->array_size = expect_number(rest);
    ret->ty = ARRAY;
    expect("]", rest);
    return ret;
}

void decl(Token **rest, bool global){
    ty_recar = type_prim(rest);
    Token *tok = *rest;
    consume_token(TK_IDENT, rest);
    Type *cdr = size_list(rest);
    if (cdr != NULL) varDecl(tok, cdr, global);
    else varDecl(tok, ty_recar, global);
};

Node *p_list(Token **rest, int depth){
    if(equal(")", rest)) return NULL;
    if(depth) expect(",", rest);
    Token *tok = *rest;
    p_decl(rest);
    Node *car, *cdr;
    cdr = p_list(rest, depth + 1);
    car = p_decl(&tok);
    car->ireg = depth + 1;
    pvarDecl(car, tok);
    if(cdr == NULL) head.next = car;
    else cdr->next = car;
    if(!depth) return head.next;
    return car;
};

Node *p_decl(Token **rest){
    Type *type = type_prim(rest);
    Token *tok = *rest;
    if (tok->kind != TK_IDENT) error_tok(tok, "識別子ではありません。");
    Node *var = calloc(1, sizeof(Node));
    var->name = tok->str;
    var->type = type;
    *rest = tok->next;
    return var;
};

void decl_list(Token **rest, int depth) {
    Token *tok = *rest;
    if (tok->kind != TK_TYPE) return;
    if(at_eof(tok)) error_tok(tok, "';'がありません。");
    decl(rest, false); // TODO global variable
    expect(";", rest);
    decl_list(rest, depth + 1);
    return;
}
