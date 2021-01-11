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
    // 現在着目しているトークン
    Token *token;
    // トークナイズ
    token = tokenize(argv[1]);
    
    // 抽象構文木の作成
    Function *prog = parse(&token);
    
    // コード生成
    codegen(prog);
    
    
    return 0;
}
