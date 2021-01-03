//
//  parser.c
//  tinycc
//
//  Created by sanluisrey on 2020/12/28.
//

#include "tinycc.h"

// ローカル変数リスト
static LVar *locals;

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
        error_at(token->str,"'%s'ではありません", op);
    }
    *rest = token->next;
}

// 次のトークンが数値の場合、トークンを一つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number(Token **rest){
    Token *token = *rest;
    if (token->kind != TK_NUM) {
        error_at(token->str,"数ではありません");
    }
    int val = token->val;
    *rest = token->next;
    return val;
}

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar *find_lvar(Token *tok) {
    for (LVar *cur = locals; cur != NULL; cur = cur->next) {
        if (tok->len == cur->len && strncmp(tok->str, cur->str, tok->len) == 0) {
            return cur;
        }
    }
    return NULL;
}

// 次のトークンの種類がTK_IDENTの時には、ローカル変数リストを更新後、トークンを一つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume_ident(Token **rest, int *offset) {
    Token *token = *rest;
    if (token->kind != TK_IDENT) {
        return false;
    }
    LVar *lvar = find_lvar(token);
    if (lvar == NULL) {
        lvar = calloc(1, sizeof(LVar));
        lvar->str = token->str;
        lvar->len = token->len;
        int base = locals != NULL ? locals->offset : 0;
        lvar->offset = base + 8;
        lvar->next = locals;
        locals = lvar;
    }
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

// 生成規則
// program    = stmt*
// stmt       = expr ";"
//            | "{" stmt* "}"
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
// unary      = ("+" | "-")? primary
// primary    = num | ident ("("(args)? ")")? | "(" expr ")"
// args       = expr ("," args )?

Function *program(Token **rest) {
    Node head = {};
    Node *cur = &head;
    while (!at_eof(*rest)) {
        cur = cur->next = stmt(rest);
    }
    Function *prog = calloc(1, sizeof(Function));
    prog->code = head.next;
    prog->locals = locals;
    return prog;
}

Node *stmt(Token **rest) {
    Node *ret;
    if (consume_token(TK_RETURN, rest)) {
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
    } else if(consume("{", rest)) {
        Node *ret = calloc(1, sizeof(Node));
        Node *cur = calloc(1, sizeof(Node));
        ret->next = NULL;
        ret->kind = ND_BLOCK;
        ret->next = stmt(rest);
        cur = ret->next;
        while (!consume("}", rest)) {
            if (at_eof(*rest)) {
                expect("}", rest);
            }
            cur->next = stmt(rest);
            cur = cur->next;
        }
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
            ret->args = args(rest);
            if(ret->args->ireg > 6) {
                error_at((*rest)->str, "引数が6個より多いです。\n");
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

Node *args(Token **rest) {
    Token *tok = *rest;
    int ireg = 0;
    if (tok->len == 1 && strncmp(tok->str, ")", 1) == 0) {
        return NULL;
    }
    Node head;
    Node *args = &head;
    args->next = expr(rest);
    args = args->next;
    args->ireg = ireg;
    while (consume(",", rest)) {
        Node *new = expr(rest);
        new->next = head.next;
        head.next = new;
        new->ireg = ++ireg;
    }
    return head.next;
}
