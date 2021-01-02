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
    TK_RESERVED,    // 記号
    TK_IDENT,       // 識別子
    TK_NUM,         // 整数のトークン
    TK_EOF,         // 入力の終わりを表すトークン
    TK_RETURN,      // return
    TK_IF,          // if
    TK_ELSE,        // else
    TK_FOR,         // for loop
    TK_WHILE,       // while
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

typedef struct LVar LVar;

struct LVar {
    char *str;      // ローカル変数の文字列
    int len;        // 文字列の長さ
    int offset; // RBPからのオフセット
    LVar *next;     // 次に定義されたローカル変数へのポインタ
};

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD,     // +
    ND_SUB,     // -
    ND_MUL,     // *
    ND_DIV,     // /
    ND_NUM,     // 整数
    ND_EQ,      // ==
    ND_NE,      // !=
    ND_GT,      // >
    ND_GE,      // >=
    ND_LT,      // <
    ND_LE,      // <=
    ND_ASGMT,   // = 代入
    ND_LVAR,    // ローカル変数
    ND_RETURN,  // return
    ND_IF,      // if
    ND_FOR,     // for loop
    ND_WHILE,   // while
    ND_BLOCK,   // {}
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind;  // ノードの型
    Node *left;     // 左辺
    Node *right;    // 右辺
    int val;        // kindがND_NUMのときのみ扱う
    int offset;     // kindがND_IDENTのときのみ扱う
    
    Node *cond;     // if, for, while文の条件式
    Node *body;     // if文の真の場合の本体, for, while loopの本体
    Node *els;      // if文の偽の場合の本体
    
    Node *initialization;     // for(式1;式2;式3) 式1を表す
    Node *step;           // for(式1;式2;式3) 式3を表す
    
    Node *next;     // block内の構文の連結リスト
};

// プログラム全体を表す
typedef struct Function Function;

struct Function {
    Node *code;
    LVar *locals;
    int stack_size;
};


// 生成規則
// 定義はparse.c参照
Function *program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// 抽象構文木のルート
extern Node *root;

// 抽象構文木の配列
extern Node *code[100];

// アセンブリの出力
void gen(Node *node);
void codegen(Function *prog);

// エラーメッセージ(汎用)出力
// 定義はparse.c内
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

#endif /* tinycc_h */
