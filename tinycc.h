//
//  tinycc.h
//  tinycc
//
//  Created by sanluisrey on 2020/12/28.
//

#ifndef tinycc_h
#define tinycc_h
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

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
    TK_TYPE,        // 型
    TK_SIZEOF,      // sizeof演算子
    TK_STR,     // string literal
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン
    int val;        // kindがTK_NUMの場合、その数値
    char *str;      // トークン文字列
    int len;        // トークン文字列の長さ
    int pos;        // 入力文字列でのトークン文字列の位置
    char *loc;
};

// 入力文字列pをトークナイズしてそれを返す
// tokenizer.c
Token *tokenize_file(char *path);

// 型
typedef struct Type Type;

struct Type {
    enum { INT, PTR, ARRAY, CHAR} ty;
    struct Type *ptr_to;    // 型リスト
    size_t array_size;
};

typedef struct Var Var;

struct Var {
    char *str;      // 変数名
    int len;        // 変数名の長さ
    int offset;     // ローカル変数のRBPからのオフセット
    bool used;      // 定義されているかどうか
    Var *next;     // 次に定義された変数へのポインタ
    Type *type;      // 変数の型
    bool global;    // グローバル変数かどうか
    bool generated;   // コード生成済みかどうか
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
    ND_GVAR,    // グローバル変数
    ND_RETURN,  // return
    ND_IF,      // if
    ND_FOR,     // for loop
    ND_WHILE,   // while
    ND_BLOCK,   // {}
    ND_FUNCCALL,// 関数呼び出し
    ND_EXPR_STMT, // expr;単体
    ND_ADDR,    // 単項&
    ND_DEREF,   // 単項*
    ND_STR, // string literal
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind;  // ノードの型
    Node *left;     // 左辺
    Node *right;    // 右辺
    int val;        // kindがND_NUMのときのみ扱う
    int offset;     // kindがND_IDENTのときのみ扱う
    char *name;     // kindがND_FUNCTIONのときの識別子の名前
    int len;        // kindがND_FUNCTIONのときの識別子の名前の長さ
    
    Node *cond;     // if, for, while文の条件式
    Node *body;     // if文の真の場合の本体, for, while loopの本体
    Node *els;      // if文の偽の場合の本体
    
    Node *initialization;     // for(式1;式2;式3) 式1を表す
    Node *step;           // for(式1;式2;式3) 式3を表す
    
    Node *next;     // block内の構文の連結リスト
    Node *args;     // 関数呼び出しの引数
    int ireg;       // 関数呼び出しの引数の順序
    
    Type *type;     // 型
};

// プログラム全体を表す
typedef struct Function Function;

struct Function {
    Node *code;
    Var *locals;
    int stack_size;
    char *name;
    Function *next;
    Node *args;
    int nparams;
    Var *globals;
    Token *literals;
};

// パーサー
// parse.c
Function *parse(Token **rest);
Function *program(Token **rest);

// 抽象構文木の配列
extern Node *code[100];

// アセンブリの出力
// codegen.c
void gen(Node *node);
void codegen(Function *prog);

// エラーメッセージ(汎用)出力
// tokenizer.c
void error_tok(Token *tok, char *fmt, ...);
void error(char *fmt, ...);

#endif /* tinycc_h */
