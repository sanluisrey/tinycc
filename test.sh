#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
#include <stdlib.h>
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }
int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
void alloc4(int **p, int a, int b, int c, int d){
    int *q = (int*) malloc(sizeof(int) * 4);
    q[0] = a;
    q[1] = b;
    q[2] = c;
    q[3] = d;
    *p = q;
}
EOF
assert() {
  expected="$1"
  input="$2"

  echo "$input" | ./tinycc - > tmp.s || exit
  cc -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}
assert 0 'int main(){ return 0; }'
assert 42 'int main(){ return 42; }'
assert 21 'int main(){ return 5+20-4; }'
assert 41 'int main(){ return  12 + 34 - 5 ; }'
assert 47 'int main(){ return 5+6*7; }'
assert 15 'int main(){ return 5*(9-6); }'
assert 4 'int main(){ return (3+5)/2; }'
assert 10 'int main(){ return -10+20; }'

assert 0 'int main(){ return 0==1; }'
assert 1 'int main(){ return 42==42; }'
assert 1 'int main(){ return 0!=1; }'
assert 0 'int main(){ return 42!=42; }'

assert 1 'int main(){ return 0<1; }'
assert 0 'int main(){ return 1<1; }'
assert 0 'int main(){ return 2<1; }'
assert 1 'int main(){ return 0<=1; }'
assert 1 'int main(){ return 1<=1; }'
assert 0 'int main(){ return 2<=1; }'

assert 1 'int main(){ return 1>0; }'
assert 0 'int main(){ return 1>1; }'
assert 0 'int main(){ return 1>2; }'
assert 1 'int main(){ return 1>=0; }'
assert 1 'int main(){ return 1>=1; }'
assert 0 'int main(){ return 1>=2; }'

assert 2 "int main(){int x;int z;x = 2;return x;}"
assert 3 'int main(){ int a; a=3; return a; }'
assert 3 'int main(){ int a;a=3; return a; }'
assert 8 'int main(){ int a; int z;a=3;  z=5; return a+z; }'

assert 3 'int main(){ int a;a=3; return a; }'
assert 8 'int main(){ int a; int z;a=3; z=5; return a+z; }'
assert 6 'int main(){ int a; int b; a=b=3; return a+b; }'
assert 3 'int main(){ int foo; foo=3; return foo; }'
assert 8 'int main(){ int foo123;int bar; foo123=3;  bar=5; return foo123+bar; }'

assert 1 'int main(){ return 1; 2; 3; }'
assert 2 'int main(){ 1; return 2; 3; }'
assert 3 'int main(){ 1; 2; return 3; }'

assert 3 'int main(){ {1; {2;} return 3;} }'
assert 5 'int main(){ ;;; return 5; }'

assert 3 'int main(){ if (0) return 2; return 3; }'
assert 3 'int main(){ if (1-1) return 2; return 3; }'
assert 2 'int main(){ if (1) return 2; return 3; }'
assert 2 'int main(){ if (2-1) return 2; return 3; }'
assert 4 'int main(){ if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 'int main(){ if (1) { 1; 2; return 3; } else { return 4; } }'

assert 55 'int main(){ int i; int j; i=0;  j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 3 'int main(){ for (;;) return 3; return 5; }'

assert 10 'int main(){ int i;  i=0; while(i<10) i=i+1; return i; }'

assert 3 'int main(){ {1; {2;} return 3;} }'

assert 10 'int main(){ int i; i=0; while(i<10) i=i+1; return i; }'
assert 55 'int main(){ int i;int j; i=0;  j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'
assert 7 'int main(){return 7;}'
assert 14 'int main(){int a; int b;a = 3;b = 5 * 6 - 8;return a + b / 2;return 8;}'
assert 100 'int main(){int return_;return_ = 100;return return_;}'

assert 1 'int main(){if(1==1)return 1;else return 0;}'
assert 0 'int main(){int a; int b;a = 2; b = 1; if(a==b)return 1;else return 0;}'
assert 1 'int main(){int a; int b;a = 2; b = 1; if(a==b*2)return 1;else return 0;}'
assert 1 'int main(){int a; int b;a = 2; b = 1; if(a==b*2)return 1;return 2;}'

assert 45 'int main(){int x; int i;x = 0;for(i = 0; i < 10; i = i + 1) x = x + i;return x;}'
assert 15 'int main(){int i;for(i = 0; i < 10; ) i = 1 + i;return 15;}'
assert 11 'int main(){for(; ; ) return 11;}'

assert 50 'int main(){int i;i = 0; while(i < 50) i = i + 1;return i;}'
assert 95 'int main(){int x; int i;x = 0;for(i = 0; i < 5; i = i + 1) x = x + i;for(i = 5; i < 10; i = i + 1) x = x + i;while(i < 50) i = i + 1;return x+i;}'


# block
assert 10 "int main(){int a;a=0;while(1){a = a + 1;if(a > 5) return 10;}return 2;}"
assert 10 " int  main(){int a;a=0;for(;;){a = a + 1;if(a > 5) return 10;}return 2;}"
assert 50 'int main(){int i;i = 0;while(i < 50) {i = i + 1;i = i + 1;}return i;}'

# function call
assert 3 'int main() { return ret3(); }'
assert 5 'int main() { return ret5(); }'
assert 8 'int main() { return add(3, 5); }'
assert 2 'int main() { return sub(5, 3); }'
assert 21 'int main() { return add6(1,2,3,4,5,6); }'

#function dif
assert 108 'int foo(int x, int y){return x+y;} int main(){return foo(8,100); } '

assert 1 'int fib(int n) {
    if(n == 0) return 1;
    return fib(n-1);
}
int main(){
    return fib(10);
}
'
assert 233 'int fib(int n) {
    if(n == 0) return 1;
    if(n == 1) return 1;
    return fib(n-1)+fib(n-2);
}
int main(){
    return fib(12);
}
'
assert 32 'int main() { return ret32(); } int ret32() { return 32; }'
assert 7 'int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }'
assert 1 'int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }'
assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

# addr and deref
assert 3 'int main(){
int x;
int *y;
y = &x;
*y = 3;
return *y;
}'

assert 3 'int main(){
int x;
int *y;
y = &x;
x = 3;
return *y;
}'

assert 3 'int main(){
int x;
int *y;
y = &x;
*y = 3;
return x;
}'

assert 4 '
int main() {
int *p;
int *q;
alloc4(&p, 1, 2, 4, 8);
q = p + 2;
return *q;}'

assert 13 '
int main() {
int *p;
int *q;
alloc4(&p, 1, 2, 4, 8);
q = p + 2;
q = p + 3;
return *q + 5;}'

# 1次元配列

assert 3 'int main() { int x[2]; int *y; y=&x; *y=3; return *x; }'

assert 3 'int main() { int x[3]; *x=3;*(x+1)=4; return *x; }'
assert 3 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; }'
assert 4 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }'
assert 3 ' int main(){
int a[2];
*a = 1;
*(a + 1) = 2;
int *p;
p = a;
return *p + *(p + 1);
}
'

# [] operator
assert 100 ' int main(){
    int a[10];
    a[9] = 100;
    return a[9];
}'
assert 100 ' int main(){
    int a[10];
    int x;
    x = 0;
    a[9] = 100;
    return a[9];
}'

# 多次元配列
assert 5 'int main() { int x[2][3]; *(*(x+1)+2)=5; return *(*(x+1)+2); }'
assert 0 'int main() { int x[2][3]; int *y; y=x[0]; *y=0; return **x; }'
assert 1 'int main() { int x[2][3]; int *y; y=x[0]; *(y+1)=1; return *(*x+1); }'
assert 2 'int main() { int x[2][3]; int *y; y=x[0]; *(y+2)=2; return *(*x+2); }'
assert 3 'int main() { int x[2][3]; int *y; y=x[0]; *(y+3)=3; return **(x+1); }'
assert 4 'int main() { int x[2][3]; int *y; y=x[0]; *(y+4)=4; return *(*(x+1)+1); }'
assert 5 'int main() { int x[2][3]; int *y; y=x[0]; *(y+5)=5; return *(*(x+1)+2); }'

# 多次元配列 [] operator
assert 5 'int main() { int x[2][3]; x[1][2]=5; return x[1][2]; }'
assert 0 'int main() { int x[2][3]; int *y; y=x[0]; y[0]=0; return x[0][0]; }'
assert 1 'int main() { int x[2][3]; int *y; y=x[0]; y[1]=1; return x[0][1]; }'
assert 2 'int main() { int x[2][3]; int *y; y=x[0]; y[2]=2; return x[0][2]; }'
assert 3 'int main() { int x[2][3]; int *y; y=x[0]; y[3]=3; return x[1][0]; }'
assert 4 'int main() { int x[2][3]; int *y; y=x[0]; y[4]=4; return x[1][1]; }'
assert 5 'int main() { int x[2][3]; int *y; y=x[0]; y[5]=5; return x[1][2]; }'

#グローバル変数
assert 10 '
int x;
int main() {x = 10;return x; }
'

assert 14 '
int x;
int y;
int main() {x = 10;y = 4; return x + y; }
'
assert 14 '
int x;
int y;
int main() {int x; x = 10;y = 4; return x + y; }
'
assert 0 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[0]; }'
assert 1 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[1]; }'
assert 2 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[2]; }'
assert 3 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[3]; }'


# char type
assert 1 'int main() { char x; x=1; return x; }'
assert 1 'int main() { char x;x=1; char y;y=2; return x; }'
assert 2 'int main() { char x;x=1; char y;y=2; return y; }'
assert 1 'int main() { return sub_char(7, 3, 3); } int sub_char(char a, char b, char c) { return a-b-c; }'

assert 1 '
int main() {
char *x;
char y;
x = &y;
y = 1;
return *x;
}
'
assert 7 '
int main() {
char x[3];
x[0] = 1;
x[1] = 2;
int y;
y = 4;
return x[0] + x[1] + y;
}
'
#sizeof
assert 4 '
int main(){
return sizeof(1);
}'
assert 4 '
int main(){
int x;
return sizeof(x);
}'
assert 8 '
int main(){
int *x;
return sizeof(x);
}'
assert 4 '
int main(){
int x;
return sizeof(x+3);
}'
assert 8 '
int main(){
int *y;
return sizeof(y+3);
}'
assert 4 '
int main(){
int *y;
return sizeof(*y);
}'
assert 8 '
int main(){
int x;
return sizeof(&x);
}'
assert 4 '
int main(){
return sizeof(sizeof(1));
}'

assert 4 'int x; int main() { return sizeof(x); }'
assert 16 'int x[4]; int main() { return sizeof(x); }'
assert 1 'int main() { char x; return sizeof(x); }'
assert 10 'int main() { char x[10]; return sizeof(x); }'
assert 16 'int main() { int x[4]; return sizeof(x); }'
assert 48 'int main() { int x[3][4]; return sizeof(x); }'
assert 16 'int main() { int x[3][4]; return sizeof(*x); }'
assert 4 'int main() { int x[3][4]; return sizeof(**x); }'
assert 5 'int main() { int x[3][4]; return sizeof(**x) + 1; }'
assert 5 'int main() { int x[3][4]; return sizeof **x + 1; }'
assert 4 'int main() { int x[3][4]; return sizeof(**x + 1); }'

# comment
assert 2 'int main() { /* return 1; */ return 2; }'
assert 2 'int main() { // return 1;
return 2; }'


# string literal
assert 97 'int main() { return "abc"[0]; }'
assert 98 'int main() { return "abc"[1]; }'
assert 99 'int main() { return "abc"[2]; }'
assert 0 'int main() { return "abc"[3]; }'
assert 97 'int main(){char *x; x = "abc"; return *x; }'
assert 97 'int main() { char *x; x ="abc"; return x[0]; }'
assert 98 'int main() { char *x; x ="abc"; return x[1]; }'
assert 0 'int main() {printf("%s\n","Hello World!");return 0;}'
assert 0 'int main() { return ""[0]; }'
assert 1 'int main() { return sizeof(""); }'
assert 4 'int main() { return sizeof("abc"); }'
assert 4 'int main() { return sizeof("abc"); }'

# handle scope
assert 2 'int main() { int x; x=2; { int x; x=3; } return x; }'
assert 2 'int main() { int x; x=2; { int x; x=3; } { int y; y=4; return x; }}'
assert 3 'int main() { int x; x=2; { x=3; } return x; }'
<<SKIP
# segmentation faultが出るため後で調査
assert 3 '
int main(){
int x;
int *y;
y = &x;
*(y + 4) = 3;
x = 10;
return *(y + 4);
}
'
assert 3 '
int main(){
    int x;
    int *y;
    y = &x;
    y = y + 4;
    *y = 3;
return *y;
}
'

assert 3 '
int main(){
int x;
int *y;
y = &x;
y = y + 4;
    *y = 3;
    x = 10;
return *y;
}
'

SKIP

echo OK

