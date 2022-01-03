#pragma once
#include "common.h"
#include "tokenizer.h"

void gen_header(){
    cout << "  .global main\n";
}

static void ass_pop(string_view reg){
    cout << "  pop %" << reg << endl;
}

static void ass_push(string_view reg){
    cout << "  push %" << reg << endl;
}

static void ass_push(int num){
    cout << "  push $" << num << endl;
}

static void ass_label(string s){
    cout << s << ":" << endl;
}

static void ass_prologue(int indent_count){
    indent_count = round_up(indent_count, 2); // Some function requires 16-byte alignment for rsp register
    cout << "  push %rbp" << endl;
    cout << "  mov %rsp, %rbp" << endl;
    cout << "  sub $" << indent_count*8 << ", %rsp" << endl;
}

static void ass_epilogue(){
    cout << ".L.return:" << endl;
    cout << "  mov %rbp, %rsp" << endl;
    cout << "  pop %rbp" << endl;
}

static string ass_stack_reg(int stack_num){
    return to_string(-8*(stack_num)) + "(%rbp)";
}

struct Node;

using PtrNode = unique_ptr<Node>;

struct NodeNum {
    int num;
};

struct NodeIdent {
    string name;
    int stack_num;
};

struct NodePunct{
    Token token;
    unique_ptr<Node> lhs, rhs;
};


struct NodeAssign {
    Token token;
    unique_ptr<Node> lhs, rhs;
};

struct NodeRet {
    Token token;
    unique_ptr<Node> pNode;
};

struct NodeCompoundStatement {
    vector<unique_ptr<Node>> pNodes;
};


struct NodeIf {
    PtrNode expr;
    PtrNode statement_if;
    PtrNode statement_else;
    int count;
private:
    static inline int curr_count = 1;
public:
    NodeIf(PtrNode expr, PtrNode statement_if, PtrNode statement_else)
        : expr(move(expr)), statement_if(move(statement_if)), statement_else(move(statement_else)), count(curr_count++)
    {

    }
};

struct Node
{
    variant<
        NodeNum,
        NodeIdent,
        NodePunct,
        NodeAssign,
        NodeRet,
        NodeCompoundStatement,
        NodeIf> val;
};

static void generate(const Node& node);

static void generate(const NodeIf& node){
    generate(*node.expr);
    auto count = to_string(node.count);
    ass_pop("rax");
    cout << "cmp $0" << ", %rax" << endl;
    cout << "je " << ".L.else." << count << endl;
    generate(*node.statement_if);
    ass_pop("rax");
    cout << "jmp " << ".L.end." << count << endl;
    ass_label(".L.else." + count);
    if (node.statement_else){
        generate(*node.statement_else);
        ass_pop("rax");
    }
    ass_label(".L.end." + count);
}


static void generate(const NodeNum& node){
    ass_push(node.num);
}
static void generate(const NodeCompoundStatement& node){
    for (auto& pNode: node.pNodes){
        generate(*pNode);
        ass_pop("rax");
    }
}
static void generate(const NodeIdent& node){
    cout << "  mov " << ass_stack_reg(node.stack_num) << ", %rax" << endl;
    ass_push("rax"); 
}

static void generate(const NodePunct& node){
    generate(*node.lhs);
    generate(*node.rhs);
    ass_pop("rdi");
    ass_pop("rax");
    auto& type = node.token.punct;
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
        cout << "  cmp %rax, %rdi" << endl;
        cout << "  setle %al" << endl;
        cout << "  movzb %al, %rax" << endl;
    }
    else if(type == ">"){
        cout << "  cmp %rax, %rdi" << endl;
        cout << "  setl %al" << endl;
        cout << "  movzb %al, %rax" << endl;
    }
    else{
        abort();
    }
    ass_push("rax");
}

static void generate(const NodeAssign& node){
    generate(*node.rhs);
    ass_pop("rax");
    auto& ident = get<NodeIdent>(node.lhs->val);
    cout << "  mov %rax, " << ass_stack_reg(ident.stack_num) << endl;
    ass_push("rax");
}

static void generate(const NodeRet& node){
    generate(*node.pNode);
    ass_pop("rax");
    cout << "  jmp .L.return" << endl;
}

static void generate(const Node& node){
    visit([](const auto& node){
        generate(node);
    }, node.val);
}

void gen_assembly(const Node& node){
    gen_header();
    cout << "main:\n";
    ass_prologue(Token::indents.size()+1);
    generate(node);
    ass_epilogue();
    cout << "  ret\n";
}

static unique_ptr<Node> token_to_node(const Token& token){
    if (token.kind == TokenKind::Num){
        return make_unique<Node>(NodeNum{token.val});
    }
    if (token.kind == TokenKind::Ident){
        return make_unique<Node>(NodeIdent{token.ident, token.val});
    }
    abort();
}

