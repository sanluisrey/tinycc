#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./tinycc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 '{ return 0; }'
assert 42 '{ return 42; }'
assert 21 '{ return 5+20-4; }'
assert 41 '{ return  12 + 34 - 5 ; }'
assert 47 '{ return 5+6*7; }'
assert 15 '{ return 5*(9-6); }'
assert 4 '{ return (3+5)/2; }'
assert 10 '{ return -10+20; }'

assert 0 '{ return 0==1; }'
assert 1 '{ return 42==42; }'
assert 1 '{ return 0!=1; }'
assert 0 '{ return 42!=42; }'

assert 1 '{ return 0<1; }'
assert 0 '{ return 1<1; }'
assert 0 '{ return 2<1; }'
assert 1 '{ return 0<=1; }'
assert 1 '{ return 1<=1; }'
assert 0 '{ return 2<=1; }'

assert 1 '{ return 1>0; }'
assert 0 '{ return 1>1; }'
assert 0 '{ return 1>2; }'
assert 1 '{ return 1>=0; }'
assert 1 '{ return 1>=1; }'
assert 0 '{ return 1>=2; }'

assert 3 '{ a; a=3; return a; }'
assert 3 '{ a=3; return a; }'
assert 8 '{ a=3;  z=5; return a+z; }'

assert 3 '{ a=3; return a; }'
assert 8 '{ a=3; z=5; return a+z; }'
assert 6 '{  a;  b; a=b=3; return a+b; }'
assert 3 '{  foo=3; return foo; }'
assert 8 '{  foo123=3;  bar=5; return foo123+bar; }'

assert 1 '{ return 1; 2; 3; }'
assert 2 '{ 1; return 2; 3; }'
assert 3 '{ 1; 2; return 3; }'

assert 3 '{ {1; {2;} return 3;} }'
assert 5 '{ ;;; return 5; }'

assert 3 '{ if (0) return 2; return 3; }'
assert 3 '{ if (1-1) return 2; return 3; }'
assert 2 '{ if (1) return 2; return 3; }'
assert 2 '{ if (2-1) return 2; return 3; }'
assert 4 '{ if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 '{ if (1) { 1; 2; return 3; } else { return 4; } }'

assert 55 '{  i=0;  j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 3 '{ for (;;) return 3; return 5; }'

assert 10 '{  i=0; while(i<10) i=i+1; return i; }'

assert 3 '{ {1; {2;} return 3;} }'

assert 10 '{  i=0; while(i<10) i=i+1; return i; }'
assert 55 '{  i=0;  j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'
assert 7 'return 7;'
assert 14 'a = 3;b = 5 * 6 - 8;return a + b / 2;return 8;'
assert 100 'return_ = 100;'

assert 1 'if(1==1)return 1;else return 0;'
assert 0 'a = 2; b = 1; if(a==b)return 1;else return 0;'
assert 1 'a = 2; b = 1; if(a==b*2)return 1;else return 0;'
assert 1 'a = 2; b = 1; if(a==b*2)return 1;return 2;'

assert 45 'x = 0;for(i = 0; i < 10; i = i + 1) x = x + i;return x;'
assert 15 'for(i = 0; i < 10; ) i = 1 + i;return 15;'
assert 11 'for(; ; ) return 11;'

assert 50 'i = 0; while(i < 50) i = i + 1;return i;'
assert 95 'x = 0;for(i = 0; i < 5; i = i + 1) x = x + i;for(i = 5; i < 10; i = i + 1) x = x + i;while(i < 50) i = i + 1;return x+i;'

# block
assert 10 "  a=0;while(1){a = a + 1;if(a > 5) return 10;}return 2;"
assert 10 "  a=0;for(;;){a = a + 1;if(a > 5) return 10;}return 2;"
assert 50 'i = 0;
           while(i < 50) {
                i = i + 1;
                i = i + 1;
           }
           return i;'

echo OK

