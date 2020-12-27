#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_NUM,      // 整数のトークン
    TK_EOF       // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

//トークン型
struct Token {
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン
    int val;        // kindがTK_NUMの場合、その数値
    char *str;      // トークン文字列
    int len;        // トークン文字列の長さ
};

//現在着目しているトークン
Token *token;

//抽象構文木のノードの種類
typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NUM, // 整数
    ND_EQ,  // ==
    ND_NE, // !=
    ND_GT, // >
    ND_GE, // >=
    ND_LT, // <
    ND_LE  // <=
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind;  // ノードの型
    Node *left;     // 左辺
    Node *right;    // 右辺
    int val;        // kindがND_NUMのときのみ扱う
};

Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

//抽象構文木のルート
Node *root;

//入力プログラム
char *user_input;
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

bool at_eof(){
    return token->kind == TK_EOF;
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
    int val;
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
        if (strchr("+-*/()><", *p)) {
            cur = new_token(TK_RESERVED, cur, p, 1);
            p++;
            continue;
        }
        if (isdigit(*p)) {
            char *pp = p;
            val = strtol(p, &p, 10);
            int len = strlen(pp) - strlen(p);
            cur = new_token(TK_NUM, cur, pp, len);
            cur->val = val;
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

// 生成規則
// expr       = equality
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? primary
// primary    = num | "(" expr ")"

Node *expr() {
    return equality();
};

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
    if (consume("(")) {
        Node *ret = expr();
        expect(")");
        return ret;
    }
    return new_node_num(expect_number());
}

void gen(Node *node){
    if (node->kind == ND_NUM) {
        printf("    push %d\n", node->val);
        return;
    }
    gen(node->left);
    gen(node->right);
    printf("    pop rdi\n");
    printf("    pop rax\n");
    switch (node->kind) {
        case ND_ADD:
            printf("    add rax, rdi\n");
            break;
        case ND_SUB:
            printf("    sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("    imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("    cqo\n");
            printf("    idiv rdi\n");
            break;
        case ND_EQ:
            printf("    cmp rax, rdi\n");
            printf("    sete al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_NE:
            printf("    cmp rax, rdi\n");
            printf("    setne al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_GE:
        case ND_LE:
            printf("    cmp rax, rdi\n");
            printf("    setle al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_GT:
        case ND_LT:
            printf("    cmp rax, rdi\n");
            printf("    setl al\n");
            printf("    movzb rax, al\n");
            break;
    }
    printf("    push rax\n");
}

int main(int argc, char **argv){
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }
    
    // トークナイズする
    user_input = argv[1];
    token = tokenize(argv[1]);
    
    // 抽象構文木の作成
    root = expr();
    
    // アセンブリの前半部分の出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    
    //アセンブリを出力
    gen(root);
    printf("    pop rax\n");
    printf("    ret\n");
    
    return 0;
}
