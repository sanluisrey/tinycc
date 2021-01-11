//
//  codegen.c
//  tinycc
//
//  Created by sanluisrey on 2020/12/28.
//

#include "tinycc.h"

// 関数呼び出しの引数に用いるレジスタ
static char* MREGS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
// スタックの深さ
static int stackpos = 0;
// 関数の戻り先のラベル
static char *label;

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
// raxレジスタからRSPへのプッシュのコード生成
static void push() {
    printf("    push rax\n");
    stackpos += 8;
}
// 即値からRSPへのプッシュのコード生成
static void push_imm(int n) {
    printf("    push %d\n", n);
    stackpos += 8;
}
// RSPからのポップのコード生成
static void pop(char *reg) {
    printf("    pop %s\n", reg);
    stackpos -= 8;
    assert(stackpos >= 0);
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
    push();
}

void gen(Node *node){
    switch (node->kind) {
        case ND_NUM:
            push_imm(node->val);
            return;
        case ND_LVAR:
            gen_lval(node);
            pop("rax");
            printf("    mov rax, [rax]\n");
            push();
            return;
        case ND_ADDR:
            gen_lval(node->right);
            return;
        case ND_DEREF:
            gen(node->right);
            pop("rax");
            printf("    mov rax, [rax]\n");
            push();
            return;
        case ND_ASGMT:
            gen_lval(node->left);
            gen(node->right);
            pop("rdi");
            pop("rax");
            printf("    mov [rax], rdi\n");
            printf("    mov rax, rdi\n");
            push();
            return;
        case ND_RETURN:
            gen(node->right);
            pop("rax");
            push();
            printf("    jmp .%s.return\n", label);
            return;
        case ND_IF: {
            int c = count();
            gen(node->cond);
            // condの結果を0と比較し、0であればLelseラベルへジャンプ
            pop("rax");
            printf("    cmp rax, 0\n");
            push();
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
                pop("rax");
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
            pop("rax");
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
                    pop("rax");
                }
            }
            return;
        }
        case ND_FUNCCALL: {
            Node *args = node->args;
            while (args != NULL) {
                if (args->ireg == 0) {
                    args = args->next;
                    continue;
                }
                gen(args);
                pop("rax");
                printf("    mov %s, rax\n", MREGS[args->ireg - 1]);
                args = args->next;
            }
            printf("    call %s\n", node->name);
            push();
            return;
        }
    }
    gen(node->left);
    gen(node->right);
    pop("rdi");
    pop("rax");
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
    push();
}

void codegen(Function *prog) {
    // アセンブリの前半部分の出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    for (Function *func = prog; func != NULL; func = func->next) {
        int ireg = func->args->ireg;
        printf("%s:\n", func->name);
        printf("    mov rax, rbp\n");
        push();
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, %d\n", ireg*8);
        for (Node *arg = func->args; arg; arg = arg->next) {
            if (!arg->ireg) {
                continue;
            }
            printf("    mov rax, rbp\n");
            printf("    sub rax, %d\n", (arg->ireg)*8);
            printf("    mov [rax], %s\n", MREGS[arg->ireg - 1]);
        }
        // ローカル変数領域の確保
        for (LVar *lvar = func->locals; lvar != NULL; lvar = lvar->next) {
            Node *loc = NULL;
            for (Node *arg = func->args; arg; arg = arg->next) {
                if (arg->name == NULL) {
                    continue;
                }
                if (!strncmp(arg->name, lvar->str, lvar->len) && strlen(arg->name) == lvar->len) {
                    loc = arg;
                    break;
                }
            }
            if (loc != NULL) {
                lvar->offset = (loc->ireg)*8;
            } else {
                lvar->offset += ireg * 8;
                func->stack_size += 8;
            }
        }
        printf("    sub rsp, %d\n", func->stack_size);
        
        // コードの本体部分の出力
        label = func->name;
        for (Node *body = func->code; body != NULL; body = body->next) {
            gen(body);
            // スタックから式の評価結果をポップする。
            pop("rax");
        }
        //エピローグ
        printf(".%s.return:\n", label);
        printf("    mov rsp, rbp\n");
        pop("rbp");
        printf("    ret\n");
    }
}
