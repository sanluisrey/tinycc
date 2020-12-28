//
//  main.c
//  tinycc
//
//  Created by sanluisrey on 2020/12/28.
//

#include "tinycc.h"

// 入力プログラム
char *user_input;

// 現在着目しているトークン
Token *token;

// ローカル変数リストのヘッダー
LVar locals;

// 抽象構文木のルート
Node *root;

// 抽象構文木の配列
Node *code[100];

int main(int argc, char **argv){
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }
    
    // トークナイズする
    user_input = argv[1];
    token = tokenize(argv[1]);
    
    // 抽象構文木の作成
    program();
    
    // アセンブリの前半部分の出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    
    // プロローグ
    // 変数26個分の領域を確保する
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, 208\n");
    
    //アセンブリを出力
    int i = 0;
    while (code[i] != NULL) {
        gen(code[i++]);
        // スタックから式の評価結果をポップする。
        printf("    pop rax\n");
    }
    
    //エピローグ
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
    
    return 0;
}
