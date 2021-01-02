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

assert 42 '42;'
assert 21 '5+20-4;'
assert 41 ' 12 + 34 - 5 ;'
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 10 '-(5+5)+20;'

assert 0 '0==1;'
assert 1 '42==42;'
assert 1 '0!=1;'
assert 0 '42!=42;'

assert 1 '0<1;'
assert 0 '1<1;'
assert 0 '2<1;'
assert 1 '0<=1;'
assert 1 '1<=1;'
assert 0 '2<=1;'

assert 1 '1>0;'
assert 0 '1>1;'
assert 0 '1>2;'
assert 1 '1>=0;'
assert 1 '1>=1;'
assert 0 '1>=2;'

assert 0 '(10-5)*2 < 4;'
assert 1 '(10-5)*2 > 4;'
assert 0 '(10-5)*2 <= 4;'
assert 1 '(10-5)*2 >= 4;'
assert 1 '(10-5)*2 != 4;'
assert 1 '(10-5)*2 == 10;'

assert 1 '(10-5)*2 < 4;(10-5)*2 > 4;'
assert 14 'a = 3;b = 5 * 6 - 8;a + b / 2;'

assert 6 'foo = 1;bar = 2 + 3;foo + bar;'
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

