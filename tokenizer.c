//
//  tokenizer.c
//  tinycc
//
//  Created by sanluisrey on 2021/01/02.
//

#include "tinycc.h"

// 入力プログラム
static char *user_input;

// エラーを報告する
void error_at(char *loc, int pos, char *fmt, ...) {
    fprintf(stderr,"%s\n", user_input);
    fprintf(stderr,"%*s",pos,  " ");
    fprintf(stderr, "^ ");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// トークンを構成する文字かどうかを返す。
int is_alnum(char c) {
  return ('a' <= c && c <= 'z') ||
         ('A' <= c && c <= 'Z') ||
         ('0' <= c && c <= '9') ||
         (c == '_');
}

//新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len){
    Token *tok = calloc(1, sizeof(Token));
    char *q = calloc(1, sizeof(char));
    strncpy(q, str, len);
    tok->str = q;
    tok->kind = kind;
    tok->len = len;
    tok->pos = str - user_input;
    cur->next = tok;
    return tok;
}

//入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p){
    user_input = p;
    Token head;
    head.next = NULL;
    Token *cur = &head;
    
    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }
        if (!strncmp(p, ">=", 2) || !strncmp(p, "<=", 2) || !strncmp(p, "==", 2) || !strncmp(p, "!=", 2)) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }
        if (strchr("+-*/()><=;{},&", *p)) {
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
        if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_RETURN, cur, p, 6);
            p += 6;
            continue;
        }
        if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
            cur = new_token(TK_IF, cur, p, 2);
            p += 2;
            continue;
        }
        if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4])) {
            cur = new_token(TK_ELSE, cur, p, 4);
            p += 4;
            continue;
        }
        if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3])) {
            cur = new_token(TK_FOR, cur, p, 3);
            p += 3;
            continue;
        }
        if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5])) {
            cur = new_token(TK_WHILE, cur, p, 5);
            p += 5;
            continue;
        }
        if (strncmp(p, "int", 3) == 0 && !is_alnum(p[3])) {
            cur = new_token(TK_TYPE, cur, p, 3);
            p += 3;
            continue;
        }
        if (isalpha(*p)) {
            char *q = p++;
            while (is_alnum(*p)) {
                p++;
            }
            int len = p - q;
            cur = new_token(TK_IDENT, cur, q, len);
            continue;
        }
        error_at(p, p - user_input, "トークナイズできません");
    }
    new_token(TK_EOF, cur, p, 1);
    return head.next;
}
