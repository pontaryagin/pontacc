#include <bits/stdc++.h>
using namespace std;

void gen_header(){
    cout << "  .global main\n";
}

void parse_formula(char* s){
    auto cur = strtol(s, &s, 10);
    cout << "mov $" << cur << ", %rax\n";
    while(*s) {
        if (*s == '+') {
            s++;
            auto cur = strtol(s, &s, 10);
            cout << "  add $" << cur << ", %rax\n";
        }
        else if (*s == '-'){
            s++;
            auto cur = strtol(s, &s, 10);
            cout << "  sub $" << cur << ", %rax\n"; 
        }
        else {
            throw invalid_argument("unknown operator "s + *s);
        }
    }
}
void gen_main(char* s){
    cout << "main:\n";
    parse_formula(s);
    cout << "  ret\n";
}


int main(int argc, char **argv){
    assert(argc == 2);
    gen_header();
    gen_main(argv[1]);
}
