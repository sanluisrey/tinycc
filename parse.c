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
static Node head;

// 抽象構文木の部分木の末尾
static Node *tail;

// 配列型のポインタレコードの末尾
static Type *ty_recar;

// 文字列リテラルのリスト
static Token *literals;

// パーサーの宣言
static Function *glbl_def(Token **rest);
//static Type *type_expr(Token **rest);
static Type *size_list(Token **rest);
static Type *array_sz(Token **rest);
static Type *type_prim(Token **rest);
static void decl(Token **rest, bool global);
static Function *f_head(Token **rest);
static Node *p_list(Token **rest, int depth);
static Node *p_decl(Token **rest);
static Function *func_def(Token **rest);
static void decl_list(Token **rest, int depth);
static Node *stmt_list(Token **rest);
static Node *stmt(Token **rest);
static Node *expr(Token **rest);
static Node *assign(Token **rest);
static Node *equality(Token **rest);
static Node *relational(Token **rest);
static Node *add(Token **rest);
static Node *mul(Token **rest);
static Node *unary(Token **rest);
static Node *primary(Token **rest);
static Node *arg_list(Token **rest, int depth);

// 次のトークンが引数の記号と等しいかどうか真偽を返す。
bool equal(char *op, Token **rest) {
    Token *token = *rest;
    if(token->len != strlen(op)) return false;
    return strncmp(token->str, op, strlen(op)) == 0;
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
        error_tok(tok, "Undeclared identifier used");
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

Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs) {
    if (kind == ND_ADD || kind == ND_SUB) {
        if (lhs->type && (lhs->type->ty == PTR || lhs->type->ty == ARRAY)) {
            Type *ty = lhs->type->ptr_to;
            if (ty->ty == PTR) {
                rhs->val *= 8;
            } else if(ty->ty == CHAR){
                rhs->val *= 1;
            } else {
                rhs->val *= 4;
            }
            if (lhs->type->ty == ARRAY) {
                Type *next = lhs->type->ptr_to;
                if(next->ty == ARRAY) rhs->val *= next->array_size;
            }
        } else if (rhs->type && (rhs->type->ty == PTR || rhs->type->ty == ARRAY)) {
            Type *ty = rhs->type->ptr_to;
            if (ty->ty == PTR) {
                lhs->val *= 8;
            } else if(ty->ty == CHAR){
                lhs->val *= 1;
            } else {
                lhs->val *= 4;
            }
            Type *next = rhs->type->ptr_to;
            if(next->ty == ARRAY) lhs->val *= next->array_size;
        }
    }
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = kind;
    ret->left = lhs;
    ret->right = rhs;
    ret->type = calloc(1, sizeof(Type));
    return ret;
}

Node *new_node_num(int val){
    Node *ret = calloc(1, sizeof(Node));
    Type *type = calloc(1, sizeof(Type));
    ret->kind = ND_NUM;
    ret->val = val;
    ret->type = type;
    return ret;
}

Node *new_node_var(Var *var){
    Node *ret = calloc(1, sizeof(Node));
    if (var->global) {
        ret->kind = ND_GVAR;
        ret->name = var->str;
    } else {
        ret->kind = ND_LVAR;
        ret->offset = var->offset;
    }
    ret->type = var->type;
    return ret;
}

Node *new_node_if(Node *cond, Node *body, Node *els){
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_IF;
    ret->cond = cond;
    ret->body = body;
    ret->els = els;
    return ret;
}

Node *new_node_for(Node *initialization, Node *cond, Node *step, Node *body){
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_FOR;
    ret->initialization = initialization;
    ret->cond = cond;
    ret->step = step;
    ret->body = body;
    return ret;
}

Node *new_node_while(Node *cond, Node *body){
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_WHILE;
    ret->cond = cond;
    ret->body = body;
    return ret;
}

Node *new_node_funccall(Token *tok) {
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_FUNCCALL;
    ret->name = tok->str;
    ret->len = tok->len;
    return ret;
}

Node *new_node_expr(Node *expr_stmt) {
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_EXPR_STMT;
    ret->right = expr_stmt;
    return ret;
}

Node *new_node_str(Token *tok) {
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_STR;
    ret->name = tok->str;
    ret->offset = strDecl(tok)->pos;
    Type *type = calloc(1, sizeof(Type));
    Type *ptr_to = calloc(1, sizeof(Type));
    ptr_to->ty = CHAR;
    type->ty = ARRAY;
    type->ptr_to = ptr_to;
    type->array_size = tok->len;
    ret->type = type;
    return ret;
}

// 生成規則(TODO)
// program    = glbl_decl
// glbl_def   =
//            = glbl_def func_def
//            = glbl_def decl ";"
// type_expr  = type_prim
//            | type_prim size_list
// type_prim  = type
//            = type_prim '*'
// size_list  = array_sz
//            | array_sz size_list
// array_sz   = "[" "]"
//            | "[" num "]"
// decl       = type_prim ident
//            | type_prim ident size_list
//            | type f_head (TODO)
//            | decl "," ident (TODO)
//            | decl "," f_head (TODO)
// f_head     = ident "(" p_list ")"
// p_list     =
//            | p_decl
//            | p_list ',' p_decl
// p_decl     = type_expr ident ";"
// func_def   = type f_head "{"     //TODO 型 全てint
//                  decl_list
//                  st_list "}"
// block      = "{" decl_list st_list "}"
// decl_list  =
//            | decl_list decl ';'
// st_list    = stmt*
// stmt       = block
//            | expr ";"
//            | "if" "(" expr ")" stmt ("else" stmt)?
//            | "while" "(" expr ")" stmt
//            | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//            | "return" expr ";"
// expr       = assign  // TODO 型
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = "sizeof" unary
//            | "+"? primary
//            | "-"? primary
//            | "*" unary
//            | "&" unary
// primary    = "(" expr ")" | ident "(" arg_list ")" | num | primary "[" expr "]"
//            | string
// arg_list   =
//            | expr
//            | arg_list "," expr


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

Function *func_def(Token **rest){
    Token *tok = *rest;
    if (tok->kind != TK_TYPE) return NULL;;
    consume_token(TK_TYPE, rest);
    locals = globals;
    Function *ret = f_head(rest);
    if (ret == NULL || !consume("{", rest)) {
        return NULL;
    }
    Node head;
    Node *cur = &head;
    stack_size = ret->nparams * 8;
    while (!consume("}", rest)) {
        Token *tok = *rest;
        if(at_eof(tok)) error_tok(tok, "'}'がありません。");
        decl_list(rest, 0);
        cur->next = stmt_list(rest);
        cur = tail;
    }
    ret->code = head.next;
    ret->locals = locals;
    ret->stack_size = stack_size;
    return ret;
};

Node *block(Token **rest){
    expect("{", rest);
    Node *cur = &head;
    while (!consume("}", rest)) {
        Token *tok = *rest;
        if(at_eof(tok)) error_tok(tok, "'}'がありません。");
        decl_list(rest, 0);
        cur->next = stmt_list(rest);
        cur = tail;
    }
    return head.next;
}

void decl_list(Token **rest, int depth) {
    Token *tok = *rest;
    if (tok->kind != TK_TYPE) return;
    if(at_eof(tok)) error_tok(tok, "';'がありません。");
    decl(rest, false); // TODO global variable
    expect(";", rest);
    decl_list(rest, depth + 1);
    return;
}

Node *stmt_list(Token **rest) {
    Token *tok = *rest;
    if (at_eof(tok) || tok->kind == TK_TYPE || equal("}", rest)) {
        return NULL;
    }
    Node *car = stmt(rest);
    Node *cdr = stmt_list(rest);
    if(cdr == NULL) tail = car;
    car->next = cdr;
    return car;
}

Node *stmt(Token **rest) {
    Node *ret;
    while (consume(";", rest)) {
        continue;
    }
    if(equal("{", rest)) {
        ret = calloc(1, sizeof(Node));
        ret->kind = ND_BLOCK;
        ret->right = block(rest);
        return ret;
    } else if (consume_token(TK_RETURN, rest)) {
        Node *right = expr(rest);
        ret = new_node_binary(ND_RETURN, NULL, right);
    } else if (consume_token(TK_IF, rest)) {
        expect("(", rest);
        Node *cond = expr(rest);
        expect(")", rest);
        Node *body = stmt(rest);
        Node *els = NULL;
        if (consume_token(TK_ELSE, rest)) {
            els = stmt(rest);
        }
        ret = new_node_if(cond, body, els);
        return ret;
    } else if (consume_token(TK_FOR, rest)) {
        Node *initialization = NULL;
        Node *cond = NULL;
        Node *step = NULL;
        Node *body;
        expect("(", rest);
        if (!consume(";", rest)) {
            initialization = new_node_expr(expr(rest));
            expect(";", rest);
        }
        if (!consume(";", rest)) {
            cond = expr(rest);
            expect(";", rest);
        }
        if (!consume(")", rest)) {
            step = expr(rest);
            expect(")", rest);
        }
        body = stmt(rest);
        ret = new_node_for(initialization, cond, step, body);
        return ret;
    } else if (consume_token(TK_WHILE, rest)) {
        expect("(", rest);
        Node *cond = expr(rest);
        expect(")", rest);
        Node *body = stmt(rest);
        ret = new_node_while(cond, body);
        return ret;
    } else {
        ret = new_node_expr(expr(rest));
    }
    expect(";", rest);
    return ret;
}

Node *expr(Token **rest) {
    return assign(rest);
};

Node *assign(Token **rest) {
    Node *ret;
    Node *left = equality(rest);
    if (consume("=",rest)) {
        Node *right = assign(rest);
        ret = new_node_binary(ND_ASGMT, left, right);
        ret->type = left->type;
        return ret;
    }
    return left;
}

Node *equality(Token **rest) {
    Node *left = relational(rest);
    Node *ret = left;
    for (; ; ) {
        if (consume("==", rest)) {
            Node *right = relational(rest);
            ret = new_node_binary(ND_EQ, ret, right);
            ret->type = calloc(1, sizeof(Type));
        } else if (consume("!=", rest)) {
            Node *right = relational(rest);
            ret = new_node_binary(ND_NE, ret, right);
            ret->type = calloc(1, sizeof(Type));
        } else {
            return ret;
        }
    }
};
Node *relational(Token **rest) {
    Node *ret = add(rest);
    for (; ; ) {
        if (consume(">=", rest)) {
            Node *left = add(rest);
            ret = new_node_binary(ND_GE, left, ret);
            ret->type = calloc(1, sizeof(Type));
        } else if (consume("<=", rest)) {
            Node *right = add(rest);
            ret = new_node_binary(ND_LE, ret, right);
            ret->type = calloc(1, sizeof(Type));
        } else if (consume(">", rest)){
            Node *left = add(rest);
            ret = new_node_binary(ND_GT, left, ret);
            ret->type = calloc(1, sizeof(Type));
        } else if (consume("<", rest)) {
            Node *right = add(rest);
            ret = new_node_binary(ND_LT, ret, right);
            ret->type = calloc(1, sizeof(Type));
        } else {
            return ret;
        }
    }
};

Node *add(Token **rest) {
    Node *left = mul(rest);
    Node *ret = left;
    Type *type = ret->type;
    for (; ; ) {
        if (consume("+", rest)) {
            Node *right = mul(rest);
            ret = new_node_binary(ND_ADD, ret, right);
            ret->type = type->ty ? type : right->type;      //TODO type check
        } else if (consume("-", rest)){
            Node *right = mul(rest);
            ret = new_node_binary(ND_SUB, ret, right);
            ret->type = type->ty ? type : right->type;      //TODO type check
        } else {
            return ret;
        }
    }
}

Node *mul(Token **rest) {
    Node *left = unary(rest);
    Node *ret = left;
    for (; ;) {
        if (consume("*", rest)) {
            Node *right = unary(rest);
            ret = new_node_binary(ND_MUL, ret, right);
            ret->type = right->type;                        //TODO type check
        } else if (consume("/", rest)){
            Node *right = unary(rest);
            ret = new_node_binary(ND_DIV, ret, right);
            ret->type = right->type;                        //TODO type check
        } else {
            return ret;
        }
    }
}

Node *unary(Token **rest) {
    Node *ret;
    if (consume_token(TK_SIZEOF, rest)) {
        Node *right = unary(rest);
        int size = type_size(right->type);
        if (right->kind == ND_MUL) {
            size *= right->val;
        }
        if (right->kind == ND_DIV) {
            size /= right->val;
        }
        ret = new_node_num(size);
        return ret;
    } else if (consume("*",rest)) {
        Node *right = unary(rest);
        ret = new_node_binary(ND_DEREF, NULL, right);
        if(!right->type->ty) error_tok(*rest, "ポインタ型ではありません。");
        ret->type = right->type->ptr_to;
        return ret;
    } else if (consume("&",rest)) {
        Node *right = unary(rest);
        ret = new_node_binary(ND_ADDR, NULL, right);
        ret->type->ty = PTR;
        ret->type->ptr_to = right->type;
        return ret;
    }
    if (consume("+",rest)) {
        return primary(rest);
    } else if (consume("-",rest)) {
        Node *right = primary(rest);
        ret = new_node_binary(ND_SUB, new_node_num(0), right);
        if(right->type->ty) error_tok(*rest, "int型ではありません。");
        ret->type = right->type;
        return ret;
    }
    return primary(rest);
}
//primary    = "(" expr ")" | ident "(" arg_list ")" | num | primary "[" expr "]" | string
Node *primary(Token **rest) {
    Token *tok = *rest;
    Node *ret;
    if (consume("(",rest)) {
        ret = expr(rest);
        expect(")",rest);
    } else if(tok->kind == TK_STR) {
        *rest = tok->next;
        ret = new_node_str(tok);
    } else if (tok->kind == TK_IDENT) {
        tok = tok->next;
        // 関数呼び出しのパース
        if (tok->len == 1 && strncmp(tok->str, "(", 1) == 0) {
            Node *ret = new_node_funccall(*rest);       // TODO register return type
            ret->type = calloc(1, sizeof(Type));
            consume_token(TK_IDENT, rest);
            consume("(", rest);
            ret->args = arg_list(rest, 0);
            if(ret->args != NULL && ret->args->ireg > 6) {
                error_tok(*rest, "引数が6個より多いです。\n");
            }
            expect(")", rest);
            return ret;
        } else {
            Var *var = find_var(*rest);
            ret = new_node_var(var);
            consume_token(TK_IDENT, rest);
        }
    } else {
        ret = new_node_num(expect_number(rest));
    }
    // 配列のパース TODO 多次元配列 型登録
    tok = *rest;
    while (!strncmp("[", tok->str, 1)) {
        Node *base = ret;
        expect("[", rest);
        Node *offset = expr(rest);
        expect("]", rest);
        if (base->type->ptr_to == NULL && offset->type->ptr_to == NULL) {
            error_tok(*rest, "配列型ではありません。");
        }
        Node *right = new_node_binary(ND_ADD, base, offset);
        ret = new_node_binary(ND_DEREF, NULL, right);
        if(base->type->ptr_to != NULL) {
            right->type = base->type;
            ret->type = base->type->ptr_to;
        } else {
            right->type = offset->type;
            ret->type = offset->type->ptr_to;
        }
        tok = *rest;
    }
    return ret;
}

Node *arg_list(Token **rest, int depth){
    Token *tok = *rest;
    if (tok->len == 1 && strncmp(tok->str, ")", 1) == 0) return NULL;
    expr(rest);  //トークンを1単位進める
    char *name = tok->str;
    if (!equal(",", rest)) {
        head.next = expr(&tok); // 前のトークン
        Node *car = head.next;
        car->ireg = ++depth;
        car->name = name;
        return car;
    }
    expect(",", rest);
    Node *cdr = arg_list(rest, depth + 1);
    Node *car = expr(&tok); // 前のトークン
    car->ireg = depth + 1;
    car->name = name;
    cdr->next = car;
    if(depth) return car;
    return head.next;
}
