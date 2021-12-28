#include <bits/stdc++.h>
using namespace std;

struct TokenInfo {
    const char *start_point;
    const char *loc; // Token location
    int len;   // Token length
    TokenInfo() = default;
    TokenInfo(const char *start_point, char* start, char* end)
        : start_point(start_point), loc(start), len(end - start)
    {}
};

struct TokenNum {
    int val;
    TokenInfo info;
    TokenNum () = default;
    TokenNum (const char * start_point, char*& p) {
        char* next;
        auto cur = strtol(p, &next, 10);
        val = cur;
        if (next == nullptr){

        }
        info = TokenInfo(start_point, p, next);
        p = next;
    }
};

struct TokenPunct {
    char val;
    TokenInfo info;
    TokenPunct () = default;
    TokenPunct (const char * start_point, char*& p) {
        val = *p;
        info = TokenInfo(start_point, p, p+1);
        p++;
    }
};

using Token = variant<TokenNum, TokenPunct>;

// Reports an error location and exit.
void verror_at(const char * current_input, const char *loc, const string& fmt) {
    int pos = loc - current_input;
    fprintf(stderr, "%s\n", current_input);
    fprintf(stderr, "%*s", pos, ""); // print pos spaces.
    fprintf(stderr, "^ ");
    cerr << fmt << endl;
    fprintf(stderr, "\n");
    exit(1);
}

void verror_at(const TokenInfo& info){
    verror_at(info.start_point, info.loc, "Unknown operator\n");
}

void verror_at(const Token& token){
    visit([&](const auto& token){
        const TokenInfo& info = token.info;
        verror_at(info);
    }, token);
}

using Tokens = vector<Token>;

Tokens tokenize(char*p){
    const char* start_point = p;
    Tokens tokens;
    while(*p){
        if(isspace(*p)){
            p++;
            continue;
        }

        if (isdigit(*p)){
            tokens.emplace_back(TokenNum(start_point, p));
            continue;
        }

        if (*p == '+' || *p == '-'){
            tokens.emplace_back(TokenPunct(start_point, p));
            continue;
        }
        verror_at(TokenInfo(start_point, p, p+1));
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

void gen_assembly(const Tokens& tokens){
    gen_header();
    cout << "main:\n";
    ass_assign_rax(tokens.at(0));
    int i = 1;
    while(i < tokens.size()){
        if (auto cur_token = get_if<TokenPunct>(&tokens.at(i))){
            auto& next_token = get<TokenNum>(tokens.at(i+1));
            ass_punct(*cur_token, next_token);
            i+= 2;
            continue;
        }
        verror_at(tokens.at(i));
    }
    cout << "  ret\n";
}

int main(int argc, char **argv){
    assert(argc == 2);
    auto tokens = tokenize(argv[1]);
    gen_assembly(tokens);
}
