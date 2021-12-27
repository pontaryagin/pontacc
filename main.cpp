#include <bits/stdc++.h>
using namespace std;

void gen_header(){
    cout << "  .global main\n";
}

void gen_main(int ret){
    cout << "main:\n";
    cout << "  mov $"s << ret << ", %rax\n";
    cout << "  ret\n";
}

int main(int argc, char **argv){
    assert(argc == 2);
    const auto ret = stoi(string(argv[1]));
    gen_header();
    gen_main(ret);
}
