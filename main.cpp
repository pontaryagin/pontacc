#include <bits/stdc++.h>
using namespace std;
#include <span>

enum class TokenKind {
    Num,
    Punct,
};

// Reports an error location and exit.
void verror_at(string_view current_input, int pos, string_view fmt) {
    cerr << current_input << endl;
    for (int i = 0; i < pos; ++i){
        cerr << " "; // print pos spaces.
    }
    cerr <<  "^ ";
    cerr << fmt << endl;
    exit(1);
}

struct Token {
    int val;
    string punct;
    TokenKind kind;
    // info 
    string_view statement;
    int loc; // Token location
    int len;   // Token length
    Token () = default;

    int from_num(string_view statement) {
        kind = TokenKind::Num;
        char* next;
        auto p = statement.data();
        val = strtol(p, &next, 10);
        len = next - p;
        return len;
    }

    int from_punct(string_view punct_) {
        kind = TokenKind::Punct;
        punct = punct_;
        len = punct_.size();
        return len;
    }

    Token(string_view statement, int& pos)
        : statement(statement), loc(pos)
    {
        if (isdigit(statement[pos])){
            pos += from_num(statement.substr(pos));
            return;
        }
        auto curr = statement.substr(pos);
        for (auto op : {"<=", ">=", "==", "!=", "+", "-", "*", "/", "(", ")", "<", ">"}){
            if (curr.starts_with(op)){
                pos += from_punct(op);
                return;
            }
        }
        verror_at(statement, loc, "Unknown operator\n");
    }
};


void verror_at(const Token& token, string_view fmt){
    verror_at(token.statement, token.loc, fmt);
}

using Tokens = vector<Token>;

Tokens tokenize(string_view text){
    Tokens tokens;
    int p = 0;
    while(p < text.size()){
        if(isspace(text[p])){
            p++;
            continue;
        }
        tokens.emplace_back(text, p);
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
    NodeNum(int num)
        : num(num)
    {}
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
        else if(type == "=="){
            cout << "  cmp %rdi, %rax" << endl;
            cout << "  sete %al" << endl;
            cout << "  movzb %al, %rax" << endl;
        }
        else if(type == "!="){
            cout << "  cmp %rdi, %rax" << endl;
            cout << "  setne %al" << endl;
            cout << "  movzb %al, %rax" << endl;
        }
        else if(type == "<="){
            cout << "  cmp %rdi, %rax" << endl;
            cout << "  setle %al" << endl;
            cout << "  movzb %al, %rax" << endl;
        }
        else if(type == "<"){
            cout << "  cmp %rdi, %rax" << endl;
            cout << "  setl %al" << endl;
            cout << "  movzb %al, %rax" << endl;
        }
        else if(type == ">="){
            cout << "  cmp %rdi, %rax" << endl;
            cout << "  setge %al" << endl;
            cout << "  movzb %al, %rax" << endl;
        }
        else if(type == ">"){
            cout << "  cmp %rdi, %rax" << endl;
            cout << "  setg %al" << endl;
            cout << "  movzb %al, %rax" << endl;
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

//  primary = num | "(" expr ")"
pair<unique_ptr<INode>,int> parse_primary(const vector<Token>& tokens, int start_pos){
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

// unary = ("+" | "-") unary | primary
pair<unique_ptr<INode>,int> parse_unary(const vector<Token>& tokens, int start_pos){
    if (tokens.at(start_pos).punct == "+"){
        return parse_unary(tokens, start_pos+1);
    }
    else if (tokens.at(start_pos).punct == "-"){
        auto [pNode, pos] = parse_unary(tokens, start_pos+1);
        return {make_unique<NodePunct>(tokens.at(start_pos), make_unique<NodeNum>(0), move(pNode)), pos};
    }
    return parse_primary(tokens, start_pos);
}

//  mul     = unary ("*" unary | "/" unary)*
pair<unique_ptr<INode>,int> parse_mul(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_unary(tokens, start_pos);
    while (pos < tokens.size()){
        auto token_val = tokens.at(pos).punct;
        if (token_val == "*" || token_val == "/" ){
            auto [pNode2, pos2] = parse_unary(tokens, pos+1);
            pNode = make_unique<NodePunct>(tokens.at(pos), move(pNode), move(pNode2));
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
}

//  add    = mul ("+" mul | "-" mul)*
pair<unique_ptr<INode>,int> parse_add(const vector<Token>& tokens, int start_pos){
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

//  relational = add ("<" add | "<=" add | ">" add | ">=" add)*
pair<unique_ptr<INode>,int> parse_relational(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_add(tokens, start_pos);
    while (pos < tokens.size()){
        auto token_val = tokens.at(pos).punct;
        if (token_val == "<" || token_val == "<=" || token_val == ">" || token_val == ">="){
            auto [pNode2, pos2] = parse_add(tokens, pos+1);
            pNode = make_unique<NodePunct>(tokens.at(pos), move(pNode), move(pNode2));
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
}

//  expr    = relational ("==" relational | "!=" relational)*
pair<unique_ptr<INode>,int> parse_expr(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_relational(tokens, start_pos);
    while (pos < tokens.size()){
        auto& token_val = tokens.at(pos).punct;
        if (token_val == "==" || token_val == "!="){
            auto [pNode2, pos2] = parse_relational(tokens, pos+1);
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
