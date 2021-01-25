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
// raxが指す場所へ値をロードする。
static void load(Type *type){
    if (type->ty == ARRAY) {
        return;
    }
    printf("    mov rax, [rax]\n");
}
// スタックトップが指すアドレスへraxをストアする。
static void store(void) {
    pop("rdi");
    printf("    mov [rdi], rax\n");
}

// 式を左辺値として計算する。
// ノードが変数を指す場合、変数のアドレスを計算し、スタックにプッシュする。
// それ以外の場合、エラーを表示する。
void gen_lval(Node *node){
    switch (node->kind) {
        case ND_LVAR:
            printf("    mov rax, rbp\n");
            printf("    sub rax, %d\n", node->offset);
            return;
            
        case ND_DEREF:
            gen(node->right);
            return;
    }
    error("左辺値ではありません。");
}

void gen(Node *node){
    switch (node->kind) {
        case ND_NUM:
            printf("    mov rax, %d\n",node->val);
            return;
        case ND_LVAR:
            gen_lval(node);
            load(node->type);
            return;
        case ND_ADDR:
            gen_lval(node->right);
            return;
        case ND_DEREF:
            gen(node->right);
            load(node->type);
            return;
        case ND_ASGMT:
            gen_lval(node->left); //rdi
            push();
            gen(node->right); //rax
            store();
            return;
        case ND_FUNCCALL: {
            Node *args = node->args;
            while (args != NULL) {
                if (args->ireg == 0) {
                    args = args->next;
                    continue;
                }
                gen(args);
                printf("    mov %s, rax\n", MREGS[args->ireg - 1]);
                args = args->next;
            }
            printf("    call %s\n", node->name);
            return;
        }
    }
    gen(node->right); //rdi
    push();
    gen(node->left); //rax
    pop("rdi");
    switch (node->kind) {
        case ND_ADD:
            printf("    add rax, rdi\n");
            return;
        case ND_SUB:
            printf("    sub rax, rdi\n");
            return;
        case ND_MUL:
            printf("    imul rax, rdi\n");
            return;
        case ND_DIV:
            printf("    cqo\n");
            printf("    idiv rdi\n");
            return;
        case ND_EQ:
            printf("    cmp rax, rdi\n");
            printf("    sete al\n");
            printf("    movzb rax, al\n");
            return;
        case ND_NE:
            printf("    cmp rax, rdi\n");
            printf("    setne al\n");
            printf("    movzb rax, al\n");
            return;
        case ND_GE:
        case ND_LE:
            printf("    cmp rax, rdi\n");
            printf("    setle al\n");
            printf("    movzb rax, al\n");
            return;
        case ND_GT:
        case ND_LT:
            printf("    cmp rax, rdi\n");
            printf("    setl al\n");
            printf("    movzb rax, al\n");
            return;
    }
    error("不正な式です。");
}

static void gen_stmt(Node *node) {
    switch (node->kind) {
        case ND_RETURN:
            gen(node->right);
            printf("    jmp .%s.return\n", label);
            return;
        case ND_IF: {
            int c = count();
            gen(node->cond);
            // condの結果を0と比較し、0であればLelseラベルへジャンプ
            printf("    cmp rax, 0\n");
            printf("    je .Lelse%d\n", c);
            // bodyのコード生成
            gen_stmt(node->body);
            printf("    jmp .Lend%d\n", c);
            // elsのコード生成
            printf(".Lelse%d:\n", c);
            if (node->els != NULL) {
                gen_stmt(node->els);
            }
            printf(".Lend%d:\n", c);
            return;
        }
        case ND_FOR: {
            int c = count();
            if (node->initialization != NULL) {
                gen_stmt(node->initialization);
            }
            printf(".Lbegin%d:\n", c);
            
            if (node->cond != NULL) {
                gen(node->cond);
                // condの結果を0と比較し、0でなければ.Lendラベルへジャンプ
                printf("    cmp rax, 0\n");
                printf("    je .Lend%d\n", c);
            }
            gen_stmt(node->body);
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
            printf("    cmp rax, 0\n");
            printf("    je .Lend%d\n", c);
            
            gen_stmt(node->body);
            printf("    jmp .Lbegin%d\n", c);
            printf(".Lend%d:\n", c);
            
            return;
        }
        case ND_BLOCK: {
            node = node->right;
            while (node != NULL) {
                gen_stmt(node);
                node = node->next;
                // TODO pop
                /*if (node != NULL) {
                    pop("rax");
                }*/
            }
            return;
        }
        case ND_EXPR_STMT:{
            gen(node->right);
            return;
        }
    }
    error("不正なステートメントです。");
}

void codegen(Function *prog) {
    // アセンブリの前半部分の出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    for (Function *func = prog; func != NULL; func = func->next) {
        printf("%s:\n", func->name);
        printf("    mov rax, rbp\n");
        push();
        printf("    mov rbp, rsp\n");
        for (Node *arg = func->args; arg; arg = arg->next) {
            printf("    mov rax, rbp\n");
            printf("    sub rax, %d\n", (arg->ireg)*8);
            printf("    mov [rax], %s\n", MREGS[arg->ireg - 1]);
        }
        // ローカル変数領域の確保
        printf("    sub rsp, %d\n", func->stack_size);
        
        // コードの本体部分の出力
        label = func->name;
        for (Node *body = func->code; body != NULL; body = body->next) {
            gen_stmt(body);
        }
        //エピローグ
        printf(".%s.return:\n", label);
        printf("    mov rsp, rbp\n");
        pop("rbp");
        printf("    ret\n");
    }
}
