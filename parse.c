//
//  parser.c
//  tinycc
//
//  Created by sanluisrey on 2020/12/28.
//

#include "tinycc.h"

// エラーを報告する

void error_at(char *loc, char *fmt, ...) {
    fprintf(stderr,"%s\n", user_input);
    int num = loc - user_input;
    fprintf(stderr,"%*s",num,  " ");
    fprintf(stderr, "^ ");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

bool at_eof() {
    return token->kind == TK_EOF;
}

// 次のトークンが期待している記号の時には、トークンを一つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op){
    if (token->kind != TK_RESERVED || strncmp(token->str, op, strlen(op)) != 0) {
        return false;
    }
    token = token->next;
    return true;
}

// 次のトークンが期待している記号のときには、トークンを一つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op){
    if (token->kind != TK_RESERVED || strncmp(token->str, op, strlen(op)) != 0) {
        error_at(token->str,"'%c'ではありません", op);
    }
    token = token->next;
}

// 次のトークンが数値の場合、トークンを一つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number(){
    if (token->kind != TK_NUM) {
        error_at(token->str,"数ではありません");
    }
    int val = token->val;
    token = token->next;
    return val;
}

// ローカル変数リストに追加済みであれば定義された順番を返す。
// それ以外の場合、0を返す。
int exist_in_locals(char *target, int len, LVar *cur) {
    int i = 0;
    while (cur->next != NULL) {
        cur = cur->next;
        i++;
        if (len == cur->len && strncmp(target, cur->str, len) == 0) {
            return i;
        }
    }
    return 0;
}

// 次のトークンが識別子の場合、トークンを一つ読み進めてローカル変数のベースポインタからのオフセットを返す。
// それ以外の場合には-1を返す。
int expect_lvar() {
    if (token->kind == TK_IDENT) {
        LVar *cur = &locals;
        int offset = exist_in_locals(token->str, token->len, cur);
        offset *= 8;
        token = token->next;
        return offset;
    }
    return -1;
}

// ローカル変数リストの更新
void update_loclas(char *target, int len) {
    LVar *cur = &locals;
    int exist = exist_in_locals(target, len, cur);
    if (!exist) {
        LVar *loc = calloc(1, sizeof(LVar));
        loc->len = len;
        loc->str = target;
        loc->next = NULL;
        cur->next = loc;
    }
}

//新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len){
    Token *tok = calloc(1, sizeof(Token));
    tok->str = str;
    tok->kind = kind;
    tok->len = len;
    cur->next = tok;
    return tok;
}

//入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p){
    Token head;
    head.next = NULL;
    Token *cur = &head;
    
    locals.next = NULL;
    
    while (*p) {
        if (*p == '\t' || *p == ' ') {
            p++;
            continue;
        }
        if (!strncmp(p, ">=", 2) || !strncmp(p, "<=", 2) || !strncmp(p, "==", 2) || !strncmp(p, "!=", 2)) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }
        if (strchr("+-*/()><=;", *p)) {
            cur = new_token(TK_RESERVED, cur, p, 1);
            p++;
            continue;
        }
        if (isdigit(*p)) {
            char *q = p;
            int val = strtol(p, &p, 10);
            int len = p - q;
            cur = new_token(TK_NUM, cur, q, len);
            cur->val = val;
            continue;
        }
        if (isalpha(*p) && islower(*p)) {
            char *q = p++;
            while (isalpha(*p) && islower(*p)) {
                p++;
            }
            int len = p - q;
            update_loclas(q, len);
            cur = new_token(TK_IDENT, cur, q, len);
            continue;
        }
        error_at(p, "トークナイズできません");
    }
    new_token(TK_EOF, cur, p, 1);
    return head.next;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
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

// 生成規則
// program    = stmt*
// stmt       = expr ";"
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? primary
// primary    = num | indent | "(" expr ")"

void program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

Node *stmt() {
    Node *ret = expr();
    expect(";");
    return ret;
}

Node *expr() {
    return assign();
};

Node *assign() {
    Node *left = equality();
    if (consume("=")) {
        Node *right = assign();
        return new_node(ND_ASGMT, left, right);
    }
    return left;
}

Node *equality() {
    Node *left = relational();
    Node *ret = left;
    for (; ; ) {
        if (consume("==")) {
            Node *right = relational();
            ret = new_node(ND_EQ, ret, right);
        } else if (consume("!=")) {
            Node *right = relational();
            ret = new_node(ND_NE, ret, right);
        } else {
            return ret;
        }
    }
};
Node *relational() {
    Node *ret = add();
    for (; ; ) {
        if (consume(">=")) {
            Node *left = add();
            ret = new_node(ND_GE, left, ret);
        } else if (consume("<=")) {
            Node *right = add();
            ret = new_node(ND_LE, ret, right);
        } else if (consume(">")){
            Node *left = add();
            ret = new_node(ND_GT, left, ret);
        } else if (consume("<")) {
            Node *right = add();
            ret = new_node(ND_LT, ret, right);
        } else {
            return ret;
        }
    }
};

Node *add() {
    Node *left = mul();
    Node *ret = left;
    for (; ; ) {
        if (consume("+")) {
            Node *right = mul();
            ret = new_node(ND_ADD, ret, right);
        } else if (consume("-")){
            Node *right = mul();
            ret = new_node(ND_SUB, ret, right);
        } else {
            return ret;
        }
    }
}

Node *mul() {
    Node *left = unary();
    Node *ret = left;
    for (; ;) {
        if (consume("*")) {
            Node *right = unary();
            ret = new_node(ND_MUL, ret, right);
        } else if (consume("/")){
            Node *right = unary();
            ret = new_node(ND_DIV, ret, right);
        } else {
            return ret;
        }
    }
}

Node *unary() {
    if (consume("+")) {
        return primary();
    } else if (consume("-")) {
        Node *right = primary();
        return new_node(ND_SUB, new_node_num(0), right);
    }
    return primary();
}

Node *primary() {
    int offset;
    if (consume("(")) {
        Node *ret = expr();
        expect(")");
        return ret;
    }
    if ((offset = expect_lvar()) != -1) {
        return new_node_lvar(offset);
    }
    return new_node_num(expect_number());
}

