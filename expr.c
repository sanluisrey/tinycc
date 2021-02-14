//
//  expr.c
//  tinycc
//
//  Created by sanluisrey on 2021/02/10.
//

#include "tinycc.h"

Node *new_node_num(int val){
    Node *ret = calloc(1, sizeof(Node));
    Type *type = calloc(1, sizeof(Type));
    ret->kind = ND_NUM;
    ret->val = val;
    ret->type = type;
    return ret;
}

Node *new_node_var(Var *var){
    Node *ret = calloc(1, sizeof(Node));
    if (var->global) {
        ret->kind = ND_GVAR;
        ret->name = var->str;
    } else {
        ret->kind = ND_LVAR;
        ret->offset = var->offset;
    }
    ret->type = var->type;
    return ret;
}

Node *new_node(Token *tok) {
    Node *ret = calloc(1, sizeof(Node));
    ret->name = tok->str;
    ret->len = tok->len;
    return ret;
}

Node *new_node_binary(NodeKind kind, Type *ty, Node *lhs, Node *rhs) {
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = kind;
    ret->left = lhs;
    ret->right = rhs;
    ret->type = ty;
    return ret;
}

Node *new_node_funcall(Node *f_head, Node *argument) {
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_FUNCCALL;
    ret->name = f_head->name;
    ret->len = argument ? argument->len : 0;
    ret->args = argument;
    ret->type = calloc(1, sizeof(Type));
    return ret;
}

Node *new_node_str(Token *tok) {
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_STR;
    ret->name = tok->str;
    ret->offset = strDecl(tok)->pos;
    Type *type = calloc(1, sizeof(Type));
    Type *ptr_to = calloc(1, sizeof(Type));
    ptr_to->ty = CHAR;
    type->ty = ARRAY;
    type->ptr_to = ptr_to;
    type->array_size = tok->len;
    ret->type = type;
    return ret;
}

Node *node(int op, Type *type, Node *lhs, Node *rhs) {
    Node *p = calloc(1, sizeof(Node));
    p->kind = op;
    p->type = type;
    p->left = lhs;
    p->right = rhs;
    return p;
}

Node *retype(Node *p, Type *type) {
    Node *q;

    if (p->type->ty == type->ty)
        return p;
    q = node(p->kind, type, p->left, p->right);
    q->offset = p->offset;
    return q;
}
// 配列型のノードをポインター型に変える
Node *pointer(Node *p) {
    if (isarray(p->type)) {
        p = retype(p, atop(p->type));
    }
    return p;
}
// intとポインタの加算減算の実装
Node *add_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *p = pointer(lhs);
    Node *q = pointer(rhs);
    if(iscint(p->type) && iscint(q->type)) return node(kind, p->type, p, q);
    if (isptr(q->type) && iscint(p->type)) return add_node(kind, rhs, lhs);
    if(!isptr(p->type) || (!iscint(q->type))) {
        error("type error: cannot add/substract operand");
        return NULL;
    }
    // lhsがポインタまたは配列、rhsがint型
    // pがポインタ、qがint型
    Type *next = p->type->ptr_to;
    switch (next->ty) {
        case INT:
        case ARRAY: // TODO int配列以外の扱い
            q = new_node_binary(ND_MUL, q->type, q, new_node_num(4));
            break;
            
        case CHAR:
            break;
            
        case PTR:
            q = new_node_binary(ND_MUL, q->type, q, new_node_num(8));
            break;
            
    }
    if (isarray(lhs->type)) {
        Type *t = lhs->type->ptr_to;
        if(t->ty == ARRAY) {
            q = new_node_binary(ND_MUL, q->type, q, new_node_num((int) t->array_size));
        }
        
    }
    return new_node_binary(kind, lhs->type, lhs, q);
    
}

/*
 expr           =   assign
                |   expr "," assign (TODO)
 assign         =   cond_expr
                =   unary assign_op assign
 assign_op      =   "="
 cond_expr      =   equality
 cnst_expr      =   cond_expr
 equality       =   relational
                |   equality "==" relational
                |   equality "!=" relational
 relational     =   add
 add            =   mul
                |   add "+" mul
                |   add "-" mul
 mul            =   mul "*" unary
                |   mul "/" unary
 unary          =   postfix
                |   unary_op unary
                |   "sizeof" unary
                |   "sizeof" "(" type_name ")"
 unary_op       =   "&" | "*" | "+" | "-"
 postfix        =   primary
                |   postfix "[" expr "]"
                |   postfix "(" agument_lst? ")"
 primary        =   ident | string | "(" expr ")"
 argument_lst   =   assign
                |   argument_lst "," assign
 */

Node *expr(Token **rest) {
    return assign(rest);
};

Node *assign(Token **rest) {
    Node *ret;
    Node *left = equality(rest);
    if (consume("=",rest)) {
        Node *right = assign(rest);
        ret = new_node_binary(ND_ASGMT,left->type, left, right); // TODO left, rightの型チェック
        ret->type = left->type;
        return ret;
    }
    return left;
}

Node *equality(Token **rest) {
    Node *left = relational(rest);
    Node *ret = left;
    for (; ; ) {
        if (consume("==", rest)) {
            Node *right = relational(rest);
            ret = new_node_binary(ND_EQ, type(INT, NULL), ret, right);
            ret->type = calloc(1, sizeof(Type));
        } else if (consume("!=", rest)) {
            Node *right = relational(rest);
            ret = new_node_binary(ND_NE, type(INT, NULL), ret, right);
            ret->type = calloc(1, sizeof(Type));
        } else {
            return ret;
        }
    }
};
Node *relational(Token **rest) {
    Node *ret = add(rest);
    for (; ; ) {
        if (consume(">=", rest)) {
            Node *left = add(rest);
            ret = new_node_binary(ND_GE,type(INT, NULL), left, ret);
            ret->type = calloc(1, sizeof(Type));
        } else if (consume("<=", rest)) {
            Node *right = add(rest);
            ret = new_node_binary(ND_LE,type(INT, NULL), ret, right);
            ret->type = calloc(1, sizeof(Type));
        } else if (consume(">", rest)){
            Node *left = add(rest);
            ret = new_node_binary(ND_GT,type(INT, NULL), left, ret);
            ret->type = calloc(1, sizeof(Type));
        } else if (consume("<", rest)) {
            Node *right = add(rest);
            ret = new_node_binary(ND_LT,type(INT, NULL), ret, right);
            ret->type = calloc(1, sizeof(Type));
        } else {
            return ret;
        }
    }
};

Node *add(Token **rest) {
    Node *left = mul(rest);
    Node *ret = left;
    for (; ; ) {
        if (consume("+", rest)) {
            Node *right = mul(rest);
            ret = add_node(ND_ADD, ret, right);
        } else if (consume("-", rest)){
            Node *right = mul(rest);
            ret = add_node(ND_SUB, ret, right);
        } else {
            return ret;
        }
    }
}

Node *mul(Token **rest) {
    Node *left = unary(rest);
    Node *ret = left;
    for (; ;) {
        if (consume("*", rest)) {
            Node *right = unary(rest);
            ret = new_node_binary(ND_MUL,ret->type, ret, right); //TODO type check
        } else if (consume("/", rest)){
            Node *right = unary(rest);
            ret = new_node_binary(ND_DIV,ret->type, ret, right); //TODO type check
        } else {
            return ret;
        }
    }
}
/*
 unary          =   postfix
                |   unary_op unary
                |   "sizeof" unary
                |   "sizeof" "(" type_name ")"
 */
Node *unary(Token **rest) {
    Node *ret;
    if (consume_token(TK_SIZEOF, rest)) {
        // TODO type->sizeで求める
        Node *right = unary(rest);
        int size = type_size(right->type);
        if (right->kind == ND_MUL) {
            size *= right->val;
        }
        if (right->kind == ND_DIV) {
            size /= right->val;
        }
        ret = new_node_num(size);
        return ret;
    } else if (consume("*",rest)) {
        Node *right = unary(rest);
        ret = new_node_binary(ND_DEREF,deref(right->type), NULL, right);
        return ret;
    } else if (consume("&",rest)) {
        Node *right = unary(rest);
        ret = new_node_binary(ND_ADDR, ptr(right->type), NULL, right);
        return ret;
    }
    if (consume("+",rest)) {
        return primary(rest);
    } else if (consume("-",rest)) {
        Node *right = primary(rest);
        if(!isint(right->type)) error_tok(*rest, "int型ではありません。");
        ret = new_node_binary(ND_SUB, right->type, new_node_num(0), right);
        return ret;
    }
    return postfix(rest);
}
/*
 postfix        =   primary
                |   postfix "[" expr "]"
                |   postfix "(" argument_lst? ")"
 */
Node *postfix(Token **rest) {
    Node *p = primary(rest);
    for (; ; ) {
        if (consume("[", rest)) {
            Node *q = expr(rest);
            expect("]", rest);
            p = add_node(ND_ADD, p, q);
            p = new_node_binary(ND_DEREF, p->type->ptr_to, NULL, p); // TODO p->type->ptr_toが配列のときの検証
        } else if(consume("(", rest)) {
            // TODO 関数の型、定義チェック
            Node *args = arg_list(rest, 0);
            if(args != NULL && args->ireg > 6) {
                error_tok(*rest, "引数が6個より多いです。\n");
            }
            expect(")", rest);
            return new_node_funcall(p, args);
        } else {
            return p;
        }
    }
}

//primary        =   ident | string | "(" expr ")"
Node *primary(Token **rest) {
    Token *tok = *rest;
    Node *ret;
    if (consume("(",rest)) {
        ret = expr(rest);
        expect(")",rest);
    } else if(tok->kind == TK_STR) {
        *rest = tok->next;
        ret = new_node_str(tok);
    } else if (tok->kind == TK_IDENT) {
        Var *var = find_var(*rest);
        if (var == NULL) {
            ret = new_node(*rest);
        } else {
            ret = new_node_var(var);
        }
        consume_token(TK_IDENT, rest);
    } else {
        ret = new_node_num(expect_number(rest));
    }
    return ret;
}
/*
 argument_lst   =   assign
                |   argument_lst "," assign
 */
Node *arg_list(Token **rest, int depth){
    Token *tok = *rest;
    if (tok->len == 1 && strncmp(tok->str, ")", 1) == 0) return NULL;
    expr(rest);  //トークンを1単位進める
    char *name = tok->str;
    if (!equal(",", rest)) {
        head.next = expr(&tok); // 前のトークン
        Node *car = head.next;
        car->ireg = ++depth;
        car->name = name;
        return car;
    }
    expect(",", rest);
    Node *cdr = arg_list(rest, depth + 1);
    Node *car = expr(&tok); // 前のトークン
    car->ireg = depth + 1;
    car->name = name;
    cdr->next = car;
    if(depth) return car;
    return head.next;
}
