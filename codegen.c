//
//  codegen.c
//  tinycc
//
//  Created by sanluisrey on 2020/12/28.
//

#include "tinycc.h"

// エラーを報告する
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 式を左辺値として計算する。
// ノードが変数を指す場合、変数のアドレスを計算し、スタックにプッシュする。
// それ以外の場合、エラーを表示する。
void gen_lval(Node *node){
    if (node->kind != ND_LVAR) {
        error("変数が左辺値ではありません。");
        return;
    }
    printf("    mov rax, rbp\n");
    printf("    sub rax, %d\n", node->offset);
    printf("    push rax\n");
}

void gen(Node *node){
    switch (node->kind) {
        case ND_NUM:
            printf("    push %d\n", node->val);
            return;
        case ND_LVAR:
            gen_lval(node);
            printf("    pop rax\n");
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
            return;
        case ND_ASGMT:
            gen_lval(node->left);
            gen(node->right);
            printf("    pop rdi\n");
            printf("    pop rax\n");
            printf("    mov [rax], rdi\n");
            printf("    push rdi\n");
            return;
        case ND_RETURN:
            gen(node->right);
            printf("    pop rax\n");
            //エピローグ
            printf("    mov rsp, rbp\n");
            printf("    pop rbp\n");
            printf("    ret\n");
            return;
        case ND_IF:
            gen(node->cond);
            // condの結果を0と比較し、0であればL2ラベルへジャンプ
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .L2\n");
            // bodyのコード生成
            gen(node->body);
            printf("    jmp .L3\n");
            // elsのコード生成
            printf(".L2:\n");
            if (node->els != NULL) {
                gen(node->els);
            }
            printf(".L3:\n");
            return;
        case ND_FOR:
            if (node->initialization != NULL) {
                gen(node->initialization);
                // initializationを評価し、.L2ラベルへジャンプ
                printf("    jmp .L2\n");
            }
            
            printf(".L3:\n");
            if (node->body != NULL) {
                gen(node->body);
            }
            if (node->step != NULL) {
                gen(node->step);
            }
            
            printf(".L2:\n");
            if (node->cond != NULL) {
                gen(node->cond);
                // condの結果を0と比較し、0でなければ.L3ラベルへジャンプ
                printf("    pop rax\n");
                printf("    cmp rax, 0\n");
                printf("    jne .L3\n");
            }
            return;
        case ND_WHILE:
            printf("    jmp .WHILE_BODY\n");
            printf(".WHILE_COND:\n");
            gen(node->body);
            printf(".WHILE_BODY:\n");
            gen(node->cond);
            // condの結果を0と比較し、0でなければ.L3ラベルへジャンプ
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    jne .WHILE_COND\n");
            return;
    }
    gen(node->left);
    gen(node->right);
    printf("    pop rdi\n");
    printf("    pop rax\n");
    switch (node->kind) {
        case ND_ADD:
            printf("    add rax, rdi\n");
            break;
        case ND_SUB:
            printf("    sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("    imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("    cqo\n");
            printf("    idiv rdi\n");
            break;
        case ND_EQ:
            printf("    cmp rax, rdi\n");
            printf("    sete al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_NE:
            printf("    cmp rax, rdi\n");
            printf("    setne al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_GE:
        case ND_LE:
            printf("    cmp rax, rdi\n");
            printf("    setle al\n");
            printf("    movzb rax, al\n");
            break;
        case ND_GT:
        case ND_LT:
            printf("    cmp rax, rdi\n");
            printf("    setl al\n");
            printf("    movzb rax, al\n");
            break;
    }
    printf("    push rax\n");
}
