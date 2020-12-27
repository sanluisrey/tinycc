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
};

//現在着目しているトークン
Token *token;

//入力プログラム
char *user_input;
// エラーを報告する
void error_at(char *loc, char *fmt, ...) {
    fprintf(stderr, user_input);
    fprintf(stderr, "\n");
    int num = loc - user_input;
    while (num--) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "^ ");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 次のトークンが期待している記号の時には、トークンを一つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char op){
    if (token->kind != TK_RESERVED || token->str[0] != op) {
        return false;
    }
    token = token->next;
    return true;
}

// 次のトークンが期待している記号のときには、トークンを一つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char op){
    if (token->kind != TK_RESERVED || token->str[0] != op) {
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
Token *new_token(TokenKind kind, Token *cur, char *str){
    Token *tok = calloc(1, sizeof(Token));
    tok->str = str;
    tok->kind = kind;
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
        if (*p == '+' || *p == '-') {
            cur = new_token(TK_RESERVED, cur, p);
            p++;
            continue;
        }
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            val = strtol(p, &p, 10);
            cur->val = val;
            continue;
        }
        error_at(p, "トークナイズできません");
    }
    new_token(TK_EOF, cur, p);
    return head.next;
}

int main(int argc, char **argv){
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }
    
    //トークナイズする
    user_input = argv[1];
    token = tokenize(argv[1]);
    
    //アセンブリの前半部分の出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    
    //式の最初は数でなければならないので、それをチェックして
    //最初のmov命令を出力
    printf("    mov rax, %ld\n", expect_number());
    
    // '+ <数>' あるいは '- <数>'というトークンの並びを消費しつつ
    //アセンブリを出力
    while (!at_eof()) {
        if (consume('+')) {
            printf("    add rax, %ld\n", expect_number());
            continue;
        }
        consume('-');
        printf("    sub rax, %ld\n", expect_number());
    }
    printf("    ret\n");
    return 0;
}
