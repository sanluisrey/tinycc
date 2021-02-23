//
//  stmt.c
//  tinycc
//
//  Created by sanluisrey on 2021/02/10.
//

#include "tinycc.h"


Node *new_node_if(Node *cond, Node *body, Node *els){
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_IF;
    ret->cond = cond;
    ret->body = body;
    ret->els = els;
    return ret;
}

Node *new_node_for(Node *initialization, Node *cond, Node *step, Node *body){
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_FOR;
    ret->initialization = initialization;
    ret->cond = cond;
    ret->step = step;
    ret->body = body;
    return ret;
}

Node *new_node_while(Node *cond, Node *body){
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_WHILE;
    ret->cond = cond;
    ret->body = body;
    return ret;
}

Node *new_node_expr(Node *expr_stmt) {
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_EXPR_STMT;
    ret->right = expr_stmt;
    return ret;
}

Node *new_node_null() {
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_NULL;
    return ret;
}

Node *new_node_return(Node *expr) {
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_RETURN;
    ret->right = expr;
    return ret;
}

Node *new_node_block(Node *list) {
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_BLOCK;
    ret->right = list;
    return ret;
}

/*
 stmt           =   expr_stmt
                |   cmp_stmt
                |   select_stmt
                |   jump_stmt
                |   iter_stmt
 */
Node *stmt(Token **rest){
    if (equal("{", rest)) {
        return cmp_stmt(rest);
    } else if(equal_tk(TK_IF, rest)) {
        return select_stmt(rest);
    } else if (equal_tk(TK_RETURN, rest)) {
        return jump_stmt(rest);
    } else if (equal_tk(TK_WHILE, rest) || equal_tk(TK_FOR, rest)) {
        return iter_stmt(rest);
    } else {
        return expr_stmt(rest);
    }
}
// expr_stmt      =   expr? ";"
Node *expr_stmt(Token **rest) {
    if (consume(";", rest)) {
        return new_node_expr(new_node_null());
    } else {
        Node *ret = expr(rest);
        expect(";", rest);
        return new_node_expr(ret);
    }
}
// cmp_stmt       =   "{" decltn_lst? stmt_lst? "}"
Node *cmp_stmt(Token **rest) {
    consume("{", rest);
    enterscope();
    if (equal_tk(TK_TYPE, rest)) {
        ex_decltn(rest);
    }
    if (equal("}", rest)) {
        if (getlevel() >= LOCAL) {
            exitscope();
        }
        return new_node_block(new_node_null());
    } else {
        Node *ret = stmt_lst(rest, *rest);
        expect("}", rest);
        if (getlevel() >= LOCAL) {
            exitscope();
        }
        return new_node_block(ret);
    }
}
// stmt_lst       =   stmt
//                |   stmt_lst stmt
Node *stmt_lst(Token **rest, Token *tok) {
    if (at_eof(tok) || !strncmp("}", tok->str, 1)) return NULL;
    if (tok->kind == TK_TYPE) ex_decltn(rest);
    Node *car = stmt(rest);
    Node *cdr = stmt_lst(rest, *rest);
    if(cdr != NULL) car->next = cdr;
    return car;
}
//select_stmt    =   "if(" expr ")" stmt
//               |   "if(" expr ")" stmt "else" stmt
Node *select_stmt(Token **rest) {
    if (consume_token(TK_IF, rest)) {
        expect("(", rest);
        Node *cond = expr(rest);
        expect(")", rest);
        Node *body = stmt(rest);
        Node *els = NULL;
        if (consume_token(TK_ELSE, rest)) {
            els = stmt(rest);
        }
        return new_node_if(cond, body, els);
    } else {
        return NULL;
    }
}
// iter_stmt      =   "while(" expr ")" stmt
//                |   "for(" expr? ";" expr? ";" expr ")" stmt
Node *iter_stmt(Token **rest) {
    if (consume_token(TK_WHILE, rest)) {
        expect("(", rest);
        Node *cond = expr(rest);
        expect(")", rest);
        Node *body = stmt(rest);
        return new_node_while(cond, body);
    } else if(consume_token(TK_FOR, rest)) {
        Node *initialization = NULL;
        Node *cond = NULL;
        Node *step = NULL;
        Node *body;
        expect("(", rest);
        if (!consume(";", rest)) {
            initialization = new_node_expr(expr(rest));
            expect(";", rest);
        }
        if (!consume(";", rest)) {
            cond = expr(rest);
            expect(";", rest);
        }
        if (!consume(")", rest)) {
            step = expr(rest);
            expect(")", rest);
        }
        body = stmt(rest);
        return new_node_for(initialization, cond, step, body);
    } else {
        return NULL;
    }
}
// jump_stmt      =   "return" expr? ";"
Node *jump_stmt(Token **rest) {
    if (consume_token(TK_RETURN, rest)) {
        if (consume(";", rest)) {
            return new_node_return(new_node_null());
        }
        Node *ret = new_node_return(expr(rest));
        expect(";", rest);
        return ret;
    } else {
        return NULL;
    }
}
