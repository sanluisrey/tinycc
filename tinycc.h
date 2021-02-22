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

#define isarray(t)    (t->ty == ARRAY)
#define isfunc(t)     (t->ty == FUNCTION)
#define isptr(t)      (t->ty == PTR)
#define isint(t)      (t->ty == INT)
#define ischar(t)     (t->ty == CHAR)
#define iscint(t)     ((t->ty == CHAR) || (t->ty == INT))
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))
// スコープ
enum {CONST, GLOBAL, PARAM, LOCAL};
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

typedef enum {
    INT,
    PTR,
    ARRAY,
    CHAR,
    FUNCTION,
} TypeKind;

struct Type {
    TypeKind ty;
    int size;            // 型のサイズ
    Type *ptr_to;        // 型リスト
    int array_size;      // 配列の要素数
    Type *base;          // 配列の最後の参照先の型
    
    Token *name;
    
    Type *return_ty;
    Type *p_list;
    Type *next;
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
    int scope;
    char *name;
    int defined;
};

typedef struct Scope Scope;
struct Scope {
   int level;  // ネストの深さ
   Scope *previous;
   Var *entry;
};

//sym.c
extern Scope *scope(Scope *p, int level);
extern Var *install(char *name, Scope **spp, int level, Type *ty);
extern Var *lookup(char *name, Scope *sp);
extern void enterscope(void);
extern void exitscope(void);
extern void initscope(void);
extern int getlevel(void);
extern int getstacksz(Scope *spp, int level);
//extern Scope ids;
extern Scope *globals;
extern Scope *identifiers;
extern Scope *strings;

// 抽象構文木のノードの種類
typedef enum {
    ND_NULL,    // 式なし
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
    Var **params;
};

// プログラム全体を表す
typedef struct Code Code;
struct Code {
    Function **function;
    int n;
};


// パーサー
// parse.c
Function *parse(Token **rest);
Function *program(Token **rest);
extern void decltn_lst(Token **rest);
extern Type *decltn_spcf(Token **rest);
extern Type *type_spcf(Token **rest, Token *tok);
extern Var *dclparam(Token **rest, char *id, Type *ty);
// 抽象構文木の部分木のヘッダー
extern Node head;

// 抽象構文木の部分木の末尾
extern Node *tail;

extern bool equal_tk(TokenKind kind, Token **rest);
extern bool equal(char *op, Token **rest);
extern bool consume(char *op, Token **rest);
extern void expect(char *op, Token **rest);
extern int expect_number(Token **rest);

//extern Var *find_var(Token *tok);
extern void lvarDecl(Token *tok, Type *type);
extern void glvarDecl(Token *tok, Type *type);
extern void varDecl(Token *tok, Type *type, bool global);
extern Token *strDecl(Token *tok);
extern Type *RefType(Token **rest, Type *type);
extern void pvarDecl(Node *args, Token *tok);
extern bool consume_ident(Token **rest, int *offset);
extern bool at_eof(Token *token);
extern bool consume_token(TokenKind kind, Token **rest);
extern void expect_token(TokenKind kind, Token **rest);

extern void decl_list(Token **rest, int depth);
//decl.c
extern Code *trns_unit(Token **);
extern Function *ex_decltn(Token **);
//stmt.c
extern Node *stmt(Token **rest);
extern Node *expr_stmt(Token **rest);
extern Node *cmp_stmt(Token **rest);
extern Node *stmt_lst(Token **rest, Token *tok);
extern Node *select_stmt(Token **rest);
extern Node *iter_stmt(Token **rest);
extern Node *jump_stmt(Token **rest);

//expr.c
extern Node *new_node_binary(NodeKind kind,Type *ty, Node *lhs, Node *rhs);
extern Node *new_node_num(int val);
extern Node *new_node_var(Var *var);

extern Node *expr(Token **rest);
extern Node *assign(Token **rest);
extern Node *equality(Token **rest);
extern Node *relational(Token **rest);
extern Node *add(Token **rest);
extern Node *mul(Token **rest);
extern Node *unary(Token **rest);
extern Node *postfix(Token **rest);
extern Node *primary(Token **rest);
extern Node *arg_list(Token **rest, int depth);

// type.c
extern Type *ptr(Type *ty);
extern Type *deref(Type *ty);
extern Type *array(Type *ty, int n);
extern Type *atop(Type *ty);    // 配列をポインタに変換
extern int type_size(Type *type);
extern Type *type(int op, Type *operand);
extern Type *get_return_ty(Type *ty);
extern Type *func(Type *ty, Type *proto);
extern Type *IntType;
extern Type *CharType;

typedef struct List List;
struct List {
    void *x;
    List *next;
    List *prev;
};
//list.c
extern List *append(void *x, List *list);
extern void *ltov(List **list);
extern int list_length(List *list);
// アセンブリの出力
// codegen.c
void gen(Node *node);
void codegen(Code *prog);

// エラーメッセージ(汎用)出力
// tokenizer.c
void error_tok(Token *tok, char *fmt, ...);
void error(char *fmt, ...);

#endif /* tinycc_h */
