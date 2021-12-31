#include <bits/stdc++.h>
using namespace std;
#include <span>

enum class TokenKind {
    Num,
    Punct,
};

// Reports an error location and exit.
void verror_at(const char * current_input, const char *loc, string_view fmt) {
    int pos = loc - current_input;
    fprintf(stderr, "%s\n", current_input);
    fprintf(stderr, "%*s", pos, ""); // print pos spaces.
    fprintf(stderr, "^ ");
    cerr << fmt << endl;
    fprintf(stderr, "\n");
    exit(1);
}

struct Token {
    int val;
    string punct;
    TokenKind kind;
    // info 
    const char *start_point;
    const char *loc; // Token location
    int len;   // Token length
    Token () = default;
    void from_num(const char*& p) {
        kind = TokenKind::Num;
        char* next;
        val = strtol(p, &next, 10);
        loc = p;
        len = next - p;
        p = next;
    }
    void from_punct(const char*& p) {
        kind = TokenKind::Punct;
        punct = string(1, *p);
        loc = p;
        len = 1;
        p++;
    }
    Token(const char * start_point, const char*& p)
        : start_point(start_point)
    {
        if (isdigit(*p)){
            from_num(p);
            return;
        }
        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')'){
            from_punct(p);
            return;
        }
        verror_at(start_point, p, "Unknown operator\n");
    }
};


void verror_at(const Token& token, string_view fmt){
    verror_at(token.start_point, token.loc, fmt);
}

using Tokens = vector<Token>;

Tokens tokenize(const char*p){
    const char* start_point = p;
    Tokens tokens;
    while(*p){
        if(isspace(*p)){
            p++;
            continue;
        }
        tokens.emplace_back(start_point, p);
    }
    return tokens;
}

void gen_header(){
    cout << "  .global main\n";
}

struct INode{
    virtual void generate() const = 0;
};

static void ass_pop(string_view reg){
    cout << "  pop %" << reg << endl;
}

static void ass_push(string_view reg){
    cout << "  push %" << reg << endl;
}

static void ass_push(int num){
    cout << "  push $" << num << endl;
}

enum class PunctType {
    Add,
    Sub,
    Mul,
    Div
};

struct NodeNum: INode {
    int num;
    NodeNum(const Token& token){
        if (token.kind != TokenKind::Num){
            verror_at(token, "Number is expected");
        }
        num = token.val;
    }
    void generate() const override{
        ass_push(num);
    }
};

struct NodePunct: INode{
    unique_ptr<INode> lhs, rhs;
    Token token;
    NodePunct(Token token, unique_ptr<INode> lhs, unique_ptr<INode> rhs)
        : token(token), lhs(move(lhs)), rhs(move(rhs))
    {}
    void generate() const override{
        lhs->generate();
        rhs->generate();
        ass_pop("rdi");
        ass_pop("rax");
        auto& type = token.punct;
        if(type == "+"){
            cout << "  add %rdi, %rax" << endl;
        }
        else if(type == "-"){
            cout << "  sub %rdi, %rax" << endl;
        }
        else if(type == "*"){
            cout << "  imul %rdi, %rax" << endl;
        }
        else if(type == "/"){
            cout << "  cqo" << endl;
            cout << "  idiv %rdi" << endl;
        }
        else{
            abort();
        }
        ass_push("rax");
    }
};

void gen_assembly(const INode& node){
    gen_header();
    cout << "main:\n";
    node.generate();
    ass_pop("rax");
    cout << "  ret\n";
}
pair<unique_ptr<INode>,int> parse_expr(const vector<Token>& tokens, int start_pos);
pair<unique_ptr<INode>,int> parse_primary(const vector<Token>& tokens, int start_pos){
    //  primary = num | "(" expr ")"
    auto token_val = tokens.at(start_pos).punct;
    if (token_val == "("){
        auto [pNode, pos] = parse_expr(tokens, start_pos+1);
        if (pos >= tokens.size()){
            verror_at(tokens.at(start_pos), "'(' was not closed");
        }
        if (tokens.at(pos).punct != ")") {
            verror_at(tokens.at(pos), "')' should come here");
        }
        return {move(pNode), pos+1};
    }
    return {make_unique<NodeNum>(tokens.at(start_pos)), start_pos+1};
}
pair<unique_ptr<INode>,int> parse_mul(const vector<Token>& tokens, int start_pos){
    //  mul     = primary ("*" primary | "/" primary)*
    auto [pNode, pos] = parse_primary(tokens, start_pos);
    while (pos < tokens.size()){
        auto token_val = tokens.at(pos).punct;
        if (token_val == "*" || token_val == "/" ){
            auto [pNode2, pos2] = parse_primary(tokens, pos+1);
            pNode = make_unique<NodePunct>(tokens.at(pos), move(pNode), move(pNode2));
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
}


pair<unique_ptr<INode>,int> parse_expr(const vector<Token>& tokens, int start_pos){
    //  expr    = mul ("+" mul | "-" mul)*
    auto [pNode, pos] = parse_mul(tokens, start_pos);
    while (pos < tokens.size()){
        auto token_val = tokens.at(pos).punct;
        if (token_val == "+" || token_val == "-" ){
            auto [pNode2, pos2] = parse_mul(tokens, pos+1);
            pNode = make_unique<NodePunct>(tokens.at(pos), move(pNode), move(pNode2));
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
}

string read_all_lines(){
    string text;
    string line;
    while (getline(cin, line))
    {
        if (text.size() > 0){
            text += "\n";
        }
        text += line;
    }
    return text;
}

int main(int argc, char **argv){
    string program = argc == 2 ? argv[1]: read_all_lines();
    auto tokens = tokenize(program.c_str());
    auto [node, pos] = parse_expr(tokens, 0);
    if (pos != tokens.size()) {
        verror_at(tokens.at(pos), "Not parsed");
    }
    gen_assembly(*node);
}
