//
//  main.c
//  tinycc
//
//  Created by sanluisrey on 2020/12/28.
//

#include "tinycc.h"

// ローカル変数リストのヘッダー
LVar locals;

// 抽象構文木の配列
Node *code[100];

int main(int argc, char **argv){
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }
    // 現在着目しているトークン
    static Token *token;
    // トークナイズする
    token = tokenize(argv[1]);
    
    // 抽象構文木の作成
    program(&token);
    
    // アセンブリの前半部分の出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    
    // プロローグ
    // ローカル変数の領域を確保する
    int cnt = 0;
    for (LVar* it = &locals; it->next != NULL ; it = it->next) {
        cnt++;
    }
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    if (cnt) {
        cnt *= 8;
        printf("    sub rsp, %d\n", cnt);
    }
    
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
