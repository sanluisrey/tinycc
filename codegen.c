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

// 制御構文のラベルの番号づけ
int count(void) {
    static int i = 1;
    return i++;
}

void assign_lvar_offset(Function *prog) {
    prog->stack_size = 0;
    if (prog->locals != NULL) {
        prog->stack_size = prog->locals->offset;
    }
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
        case ND_IF: {
            int c = count();
            gen(node->cond);
            // condの結果を0と比較し、0であればLelseラベルへジャンプ
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .Lelse%d\n", c);
            // bodyのコード生成
            gen(node->body);
            printf("    jmp .Lend%d\n", c);
            // elsのコード生成
            printf(".Lelse%d:\n", c);
            if (node->els != NULL) {
                gen(node->els);
            }
            printf(".Lend%d:\n", c);
            return;
        }
        case ND_FOR: {
            int c = count();
            if (node->initialization != NULL) {
                gen(node->initialization);
            }
            printf(".Lbegin%d:\n", c);
            
            if (node->cond != NULL) {
                gen(node->cond);
                // condの結果を0と比較し、0でなければ.Lendラベルへジャンプ
                printf("    pop rax\n");
                printf("    cmp rax, 0\n");
                printf("    je .Lend%d\n", c);
            }
            gen(node->body);
            if (node->step != NULL) {
                gen(node->step);
            }
            printf("    jmp .Lbegin%d\n", c);
            printf(".Lend%d:\n", c);
            return;
        }
        case ND_WHILE: {
            int c = count();
            printf(".Lbegin%d:\n", c);
            gen(node->cond);
            // condの結果を0と比較し、0でなければ.L3ラベルへジャンプ
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .Lend%d\n", c);
            
            gen(node->body);
            printf("    jmp .Lbegin%d\n", c);
            printf(".Lend%d:\n", c);
            
            return;
        }
        case ND_BLOCK: {
            while (node->next != NULL) {
                gen(node->next);
                node = node->next;
                // TODO pop
                if (node->next != NULL) {
                    printf("    pop rax\n");
                }
            }
            return;
        }
        case ND_FUNCTION: {
            printf("    call %s\n", node->name);
            return;
        }
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

void codegen(Function *prog) {
    // アセンブリの前半部分の出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    // ローカル変数領域の確保
    assign_lvar_offset(prog);
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", prog->stack_size);
    // コードの本体部分の出力
    for (Node *body = prog->code; body != NULL; body = body->next) {
        gen(body);
        // スタックから式の評価結果をポップする。
        printf("    pop rax\n");
    }
    //エピローグ
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
}
