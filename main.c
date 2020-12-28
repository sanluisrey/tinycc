//
//  main.c
//  tinycc
//
//  Created by sanluisrey on 2020/12/28.
//

#include "tinycc.h"

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
