//
//  parser.c
//  tinycc
//
//  Created by sanluisrey on 2020/12/28.
//

#include "tinycc.h"

// 現在のパースしている関数
static Function *prog;
// ローカル変数リスト
static LVar *locals;

// 関数のスタック領域のサイズ
static int stack_size;

// 抽象構文木の部分木のヘッダー
static Node head;

// 抽象構文木の部分木の末尾
static Node* tail;

// パーサーの宣言
static Node *glbl_decl(Token **rest);
static Type *type_prim(Token **rest);
static void decl(Token **rest);
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
static Node *args(Token **rest);
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
        error_at(token->str, token->pos, "'%s'ではありません", op);
    }
    *rest = token->next;
}

// 次のトークンが数値の場合、トークンを一つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number(Token **rest){
    Token *token = *rest;
    if (token->kind != TK_NUM) {
        error_at(token->str,token->pos, "数ではありません");
    }
    int val = token->val;
    *rest = token->next;
    return val;
}

// 変数を名前で検索する。見つからなかった場合はエラー処理。
LVar *find_lvar(Token *tok) {
    LVar *cur = NULL;
    int i = 0;
    for (cur = locals; cur != NULL; cur = cur->next, i++) {
        if (tok->len == cur->len && strncmp(tok->str, cur->str, tok->len) == 0) {
            break;
        }
    }
    if (cur == NULL) {
        error_at(tok->str, tok->pos, "Undeclared identifier used");
    }
    if (!cur->used) {
        stack_size += 8;
        cur->offset = stack_size;
        cur->used = true;
    }
    return cur;
}

// 変数リストへの登録
void lvarDecl(Token *tok, Type *type){
    LVar *cur = NULL;
    for (cur = locals; cur != NULL; cur = cur->next) {
        if (tok->len == cur->len && strncmp(tok->str, cur->str, tok->len) == 0) {
            error_at(tok->str, tok->pos, "Duplicated declaration");
        }
    }
    if (locals == NULL) {
        locals = calloc(1, sizeof(LVar));
        locals->str = tok->str;
        locals->len = tok->len;
        locals->type = type;
    } else {
        LVar *new = calloc(1, sizeof(LVar));
        new->len = tok->len;
        new->str = tok->str;
        new->next = locals;
        new->type = type;
        locals = new;
    }
}

// 1次型式のパース
Type *RefType(Token **rest) {
    Token *tok = *rest;
    Type *ret = calloc(1, sizeof(Type));
    if(strncmp(tok->str, "*", 1) == 0) {
        ret->ty = PTR;
        *rest = tok->next;
        ret->ptr_to = RefType(rest);
    }
    return ret;
}

// 仮引数の変数リストへの登録
void pvarDecl(Node *args, Token *tok) {
    LVar *cur = NULL;
    for (cur = locals; cur != NULL; cur = cur->next) {
        if (strlen(args->name) == cur->len && strncmp(args->name, cur->str, cur->len) == 0) {
            error_at(tok->str, tok->pos, "Duplicated declaration");
        }
    }
    if (locals == NULL) {
        locals = calloc(1, sizeof(LVar));
        locals->str = args->name;
        locals->len = strlen(args->name);
        locals->offset = args->ireg * 8;
        locals->used = true;
        locals->type = args->type;
    } else {
        LVar *new = calloc(1, sizeof(LVar));
        new->len = strlen(args->name);
        new->str = args->name;
        new->offset = args->ireg * 8;
        new->used = true;
        new->next = locals;
        new->type = args->type;
        locals = new;
    }
}

// 次のトークンの種類がTK_IDENTの時には、ローカル変数リストを検索し、トークンを一つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume_ident(Token **rest, int *offset) {
    Token *token = *rest;
    if (token->kind != TK_IDENT) {
        return false;
    }
    LVar *lvar = find_lvar(token);
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

Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = kind;
    ret->left = lhs;
    ret->right = rhs;
    return ret;
}

Node *new_node_num(int val){
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_NUM;
    ret->val = val;
    return ret;
}

Node *new_node_lvar(int offset){
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_LVAR;
    ret->offset = offset;
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

// 生成規則(TODO)
// program    = glbl_decl
// glbl_decl  =
//            = glbl_decl func_def
// type_expr  = type_prim
// type_prim  = type
//            = type_prim '*'
// decl       = type_expr ident
//            | type f_head (TODO)
//            | decl "," ident (TODO)
//            | decl "," f_head (TODO)
// f_head     = ident "(" p_list ")"
// p_list     =
//            | p_decl
//            | p_list ',' p_decl
// p_decl     = type_expr ident ";"
// func_def   = type f_head "{"
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
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = "+"? primary
//            | "-"? primary
//            | "*" unary
//            | "&" unary
// primary    = "(" expr ")" | ident "(" arg_list ")" | num
// arg_list   =
//            | expr
//            | arg_list "," expr


Function *program(Token **rest) {
    Function head;
    prog = &head;
    while (!at_eof(*rest)) {
        prog = prog->next = func_def(rest);
    }
    return head.next;
}

Type *type_prim(Token **rest){
    return RefType(rest);
};

void decl(Token **rest){
    consume_token(TK_TYPE,rest);
    Type *type = type_prim(rest);
    Token *tok = *rest;
    lvarDecl(tok, type);
    *rest = tok->next;
};

Function *f_head(Token **rest){
    Token *tok = *rest;
    if (tok->kind != TK_IDENT) error_at(tok->str, tok->pos, "識別子ではありません。");
    Function *ret = calloc(1, sizeof(Function));
    ret->name = tok->str;
    consume_token(TK_IDENT, rest);
    tok = *rest;
    expect("(", rest);
    Node *args = p_list(rest, 0);
    ret->args = args;
    if(args != NULL) ret->nparams = args->ireg;
    expect(")", rest);
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
    consume_token(TK_TYPE, rest);
    Type *type = type_prim(rest);
    Token *tok = *rest;
    if (tok->kind != TK_IDENT) error_at(tok->str, tok->pos, "識別子ではありません。");
    Node *var = calloc(1, sizeof(Node));
    var->name = tok->str;
    var->type = type;
    *rest = tok->next;
    return var;
};

Function *func_def(Token **rest){
    Token *tok = *rest;
    if (tok->kind != TK_TYPE) error_at(tok->str, tok->pos, "型がありません。");
    consume_token(TK_TYPE, rest);
    locals = NULL;
    Function *ret = f_head(rest);
    expect("{", rest);
    Node head;
    Node *cur = &head;
    stack_size = ret->nparams * 8;
    while (!consume("}", rest)) {
        Token *tok = *rest;
        if(at_eof(tok)) error_at(tok->str, tok->pos, "'}'がありません。");
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
        if(at_eof(tok)) error_at(tok->str, tok->pos, "'}'がありません。");
        decl_list(rest, 0);
        cur->next = stmt_list(rest);
        cur = tail;
    }
    return head.next;
}

void decl_list(Token **rest, int depth) {
    Token *tok = *rest;
    if (tok->kind != TK_TYPE) return;
    if(at_eof(tok)) error_at(tok->str, tok->pos, "';'がありません。");
    decl(rest);
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
            initialization = expr(rest);
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
        ret = expr(rest);
    }
    expect(";", rest);
    return ret;
}

Node *expr(Token **rest) {
    return assign(rest);
};

Node *assign(Token **rest) {
    Node *left = equality(rest);
    if (consume("=",rest)) {
        Node *right = assign(rest);
        return new_node_binary(ND_ASGMT, left, right);
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
        } else if (consume("!=", rest)) {
            Node *right = relational(rest);
            ret = new_node_binary(ND_NE, ret, right);
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
        } else if (consume("<=", rest)) {
            Node *right = add(rest);
            ret = new_node_binary(ND_LE, ret, right);
        } else if (consume(">", rest)){
            Node *left = add(rest);
            ret = new_node_binary(ND_GT, left, ret);
        } else if (consume("<", rest)) {
            Node *right = add(rest);
            ret = new_node_binary(ND_LT, ret, right);
        } else {
            return ret;
        }
    }
};

Node *add(Token **rest) {
    Node *left = mul(rest);
    Node *ret = left;
    for (; ; ) {
        if (consume("+", rest)) {
            Node *right = mul(rest);
            ret = new_node_binary(ND_ADD, ret, right);
        } else if (consume("-", rest)){
            Node *right = mul(rest);
            ret = new_node_binary(ND_SUB, ret, right);
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
        } else if (consume("/", rest)){
            Node *right = unary(rest);
            ret = new_node_binary(ND_DIV, ret, right);
        } else {
            return ret;
        }
    }
}

Node *unary(Token **rest) {
    if (consume("*",rest)) {
        Node *right = unary(rest);
        return new_node_binary(ND_DEREF, NULL, right);
    } else if (consume("&",rest)) {
        Node *right = unary(rest);
        return new_node_binary(ND_ADDR, NULL, right);
    }
    if (consume("+",rest)) {
        return primary(rest);
    } else if (consume("-",rest)) {
        Node *right = primary(rest);
        return new_node_binary(ND_SUB, new_node_num(0), right);
    }
    return primary(rest);
}

Node *primary(Token **rest) {
    if (consume("(",rest)) {
        Node *ret = expr(rest);
        expect(")",rest);
        return ret;
    }
    Token *tok;
    tok = *rest;
    if (tok->kind == TK_IDENT) {
        tok = tok->next;
        if (tok->len == 1 && strncmp(tok->str, "(", 1) == 0) {
            Node *ret = new_node_funccall(*rest);
            consume_token(TK_IDENT, rest);
            consume("(", rest);
            ret->args = arg_list(rest, 0);
            //ret->args = args(rest);
            if(ret->args != NULL && ret->args->ireg > 6) {
                error_at((*rest)->str, (*rest)->pos, "引数が6個より多いです。\n");
            }
            expect(")", rest);
            return ret;
        }
        int *offset = calloc(1, sizeof(int));
        if (consume_ident(rest, offset)) {
            return new_node_lvar(*offset);
        }
    }
    return new_node_num(expect_number(rest));
}

Node *arg_list(Token **rest, int depth){
    Token *tok = *rest;
    if (tok->len == 1 && strncmp(tok->str, ")", 1) == 0) return NULL;
    if (strncmp(tok->next->str, ",", 1)) {
        Node *ret = &head;
        ret->next = expr(rest);
        ret = ret->next;
        ret->ireg = ++depth;
        ret->name = tok->str;
        return ret;
    }
    Node *pre = expr(rest);
    pre->ireg = depth + 1;
    pre->name = tok->str;
    expect(",", rest);
    Node *post = arg_list(rest, depth + 1);
    post->next = pre;
    if (depth) {
        return pre;
    }
    return head.next;
}
