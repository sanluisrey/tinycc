//
//  tokenizer.c
//  tinycc
//
//  Created by sanluisrey on 2021/01/02.
//

#include "tinycc.h"

// 入力プログラム
static char *user_input;

// 入力ファイル名
static char *current_filename;

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}
char test[] = "int main(){return 0;}";
// エラーの起きた場所を報告するための関数
// 下のようなフォーマットでエラーメッセージを表示する
//
// foo.c:10: x = y + + 5;
//                   ^ 式ではありません
void verror_at(char *loc , char *fmt, va_list ap) {
    char *line = loc;
    while (user_input < line && line[-1] != '\n') {
        line--;
    }
    char *end = loc;
    while (*end != '\n') {
        end++;
    }
    
    int line_num = 1;
    for (char *p = user_input; p < line; p++) {
        if (*p == '\n') {
            line_num++;
        }
    }
    
    int indent = fprintf(stderr, "%s:%d: ",current_filename, line_num);
    fprintf(stderr, "%.*s\n",(int)(end - line), line);
    
    int pos = loc - line + indent;
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(tok->loc, fmt, ap);
}

static char *read_file(char *path) {
    FILE *fp;
    if (strcmp(path, "-") == 0) {
        fp = stdin;
    } else {
        fp = fopen(path, "r");
        if (!fp) {
            error("cannot open %s: %s", path, strerror(errno));
        }
    }
    char *buf;
    size_t buffer_size;
    FILE *out = open_memstream(&buf, &buffer_size);
    
    for (;;) {
        char buf2[4096];
        int n = fread(buf2, 1, sizeof(buf2), fp);
        if (n == 0)
          break;
        fwrite(buf2, 1, n, out);
    }
    fflush(out);
    if (buffer_size == 0 || buf[buffer_size - 1] != '\n') {
        fputc('\n', out);
    }
    fputc('\0', out);
    if(fp != stdin) fclose(fp);
    fclose(out);
    return buf;
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
    tok->loc = str;
    cur->next = tok;
    return tok;
}

//入力文字列pをトークナイズしてそれを返す
static Token *tokenize(char *filename, char *p){
    current_filename = filename;
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
        if (strchr("+-*/()><=;{},&[]", *p)) {
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
        if (strncmp(p, "sizeof", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_SIZEOF, cur, p, 6);
            p += 6;
            continue;
        }
        if (strncmp(p, "char", 4) == 0 && !is_alnum(p[4])) {
            cur = new_token(TK_TYPE, cur, p, 4);
            p += 4;
            continue;
        }
        if (strncmp(p, "\"", 1) == 0) {
            char *q = ++p;
            while (strncmp(p, "\"", 1) != 0) {
                if(*p == EOF || strncmp(p, "\n", 1) == 0) error_at(p, "\"がありません。");
                p++;
            }
            int len = p - q;
            cur = new_token(TK_STR, cur, q, len);
            p++;
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
        error_at(p, "トークナイズできません");
    }
    new_token(TK_EOF, cur, p, 1);
    return head.next;
}

Token *tokenize_file(char *path) {
  return tokenize(path, read_file(path));
}
