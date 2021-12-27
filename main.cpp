#include <bits/stdc++.h>
using namespace std;

struct TokenInfo {
    const char *loc; // Token location
    int len;   // Token length
    TokenInfo() = default;
    TokenInfo(char* start, char* end){
        loc = start;
        len = end - start;
    }
};

struct TokenNum {
    int val;
    TokenInfo info;
    TokenNum () = default;
    TokenNum (char*& p) {
        char* next;
        auto cur = strtol(p, &next, 10);
        val = cur;
        if (next == nullptr){

        }
        info = TokenInfo(p, next);
        p = next;
    }
};

struct TokenPunct {
    char val;
    TokenPunct () = default;
    TokenPunct (char*& p) {
        val = *p;
        p++;
    }
};

using Token = variant<TokenNum, TokenPunct>;

static void error(const string& s) {
    throw invalid_argument(s);
}

using Tokens = vector<unique_ptr<Token>>;

vector<unique_ptr<Token>> tokenize(char*p){
    vector<unique_ptr<Token>> tokens;
    while(*p){
        if(isspace(*p)){
            p++;
            continue;
        }

        if (isdigit(*p)){
            tokens.emplace_back(make_unique<TokenNum>(p));
            continue;
        }

        if (*p == '+' || *p == '-'){
            tokens.emplace_back(make_unique<TokenPunct>(p));
            continue;
        }

    }
    return tokens;
}

void gen_header(){
    cout << "  .global main\n";
}

void ass_assign_rax(const Token& token){
    auto cur = get<TokenNum>(token).val;
    cout << "mov $" << cur << ", %rax\n";
}

void ass_punct(const TokenPunct& punct, const TokenNum& num){
    cout << (punct.val == '+' ? "add": "sub") << " $" << num.val << ", %rax\n";
}


void gen_assembly(char* s){
    auto cur = strtol(s, &s, 10);
    // cout << "mov $" << cur << ", %rax\n";
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

void gen_assembly(const Tokens& tokens){
    gen_header();
    cout << "main:\n";
    ass_assign_rax(*tokens.at(0));
    int i = 1;
    while(i < tokens.size()){
        if (auto cur_token = get_if<TokenPunct>(tokens.at(i).get())){
            auto& next_token = get<TokenNum>(*tokens.at(i+1));
            ass_punct(*cur_token, next_token);
            i+= 2;
            continue;
        }
        throw invalid_argument("unknown operator "s);
    }
    cout << "  ret\n";
}


int main(int argc, char **argv){
    assert(argc == 2);
    auto tokens = tokenize(argv[1]);
    gen_assembly(tokens);
}
