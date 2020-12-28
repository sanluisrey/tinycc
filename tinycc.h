//
//  tinycc.h
//  tinycc
//
//  Created by sanluisrey on 2020/12/28.
//

#ifndef tinycc_h
#define tinycc_h

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_NUM,      // 整数のトークン
    TK_EOF       // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン
    int val;        // kindがTK_NUMの場合、その数値
    char *str;      // トークン文字列
    int len;        // トークン文字列の長さ
};

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p);

// 抽象構文木のノードの種類
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

// 生成規則
// expr       = equality
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? primary
// primary    = num | "(" expr ")"
Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// 入力プログラム
extern char *user_input;

// 現在着目しているトークン
extern Token *token;

// 抽象構文木のルート
extern Node *root;

// アセンブリの出力
void gen(Node *node);

#endif /* tinycc_h */
