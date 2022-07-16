#!/bin/bash -e

cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }

int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
EOF

assert() {
    set +e
    expected="$1"
    input="$2"

    ./pontacc "$input" > tmp.s || exit
    gcc -static -o tmp tmp.s tmp2.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 7 'int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }'
# assert 1 'int main() { return sub2(4,3,1); } int sub2(int x, int y) { return x-y; }'
# assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

echo OK
