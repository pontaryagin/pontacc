#pragma once
#include "common.h"
#include "tokenizer.h"

struct Node;

using PtrNode = unique_ptr<Node>;

struct NodeNull {};

struct NodeNum {
    int num;
};

struct NodeVar {
    string name;
    int offset;
};

struct NodeAddress {
    Token token;
    PtrNode var;
};

struct NodeDeref {
    Token token;
    PtrNode var;
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
    {}
};

struct NodeFor {
    PtrNode expr_init;
    PtrNode expr_condition;
    PtrNode expr_increment;
    PtrNode statement;
    int count;
private:
    static inline int curr_count = 1;
public:
    NodeFor(PtrNode expr_init, PtrNode expr_condition, PtrNode expr_increment, PtrNode statement)
        : expr_init(move(expr_init)), expr_condition(move(expr_condition)), expr_increment(move(expr_increment)), 
          statement(move(statement)), count(curr_count++)
    {}
};

struct Node
{
    variant<
        NodeNull,
        NodeNum,
        NodeVar,
        NodeAddress,
        NodeDeref,
        NodePunct,
        NodeAssign,
        NodeRet,
        NodeCompoundStatement,
        NodeIf,
        NodeFor> val;
};

void gen_header(){
    cout << "  .global main\n";
}

static void ass_pop(string_view reg){
    cout << "  pop " << reg << endl;
}

static void ass_push(string_view reg){
    cout << "  push " << reg << endl;
}

static void ass_push(int num){
    cout << "  push $" << num << endl;
}

static void ass_mov(string from, string to){
    cout << "  mov " << from << ", " << to << endl;
}

static void ass_mov(int from, string to){
    cout << "  mov " << "$" << from << ", " << to << endl;
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

static string ass_stack_reg(const NodeVar& node){
    return to_string(-8*(node.offset)) + "(%rbp)";
}

static optional<string> ass_stack_reg(const Node& node){
    if (auto pvar = get_if<NodeVar>(&node.val)){
        return ass_stack_reg(*pvar);
    }
    else if (auto pvar = get_if<NodeDeref>(&node.val)){
        return ass_stack_reg(*pvar->var);
    }
    return nullopt;
}


static void generate(const Node& node);

static void generate([[maybe_unused]]const NodeNull& node){
}

static void generate(const NodeIf& node){
    generate(*node.expr);
    auto count = to_string(node.count);
    cout << "cmp $0" << ", %rax" << endl;
    cout << "je " << ".L.else." << count << endl;
    generate(*node.statement_if);
    cout << "jmp " << ".L.end." << count << endl;
    ass_label(".L.else." + count);
    if (node.statement_else){
        generate(*node.statement_else);
    }
    ass_label(".L.end." + count);
}

static void generate(const NodeFor& node){
    auto count = to_string(node.count);
    generate(*node.expr_init);
    ass_label(".L.for." + count);
    if (get_if<NodeNull>(&node.expr_condition->val)){
        ass_mov(1, "%rax");
    }
    else {
        generate(*node.expr_condition);
    }
    cout << "cmp $0" << ", %rax" << endl;
    cout << "je " << ".L.end." + count << endl;
    generate(*node.statement);
    generate(*node.expr_increment);
    cout << "jmp " << ".L.for." + count << endl;
    ass_label(".L.end." + count);
}

static void generate(const NodeNum& node){
    ass_mov(node.num, "%rax");
}
static void generate(const NodeCompoundStatement& node){
    for (auto& pNode: node.pNodes){
        generate(*pNode);
    }
}

static void generate(const NodeVar& node){
    ass_mov(ass_stack_reg(node), "%rax");
}

static void generate(const NodeAddress& node){
    auto var = ass_stack_reg(*node.var);
    if (!var){
        verror_at(node.token, " cannot take address of lvalue");
    }
    cout << "  lea " << *var << ", %rax" << endl;
}

static void generate(const NodeDeref& node){
    generate(*node.var);
    ass_mov("(%rax)", "%rax");
}

static void generate(const NodePunct& node){
    generate(*node.lhs);
    ass_push("%rax");
    generate(*node.rhs);
    ass_mov("%rax", "%rdi");
    ass_pop("%rax");

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
        verror_at(node.token, "Unknown token in generate for NodePunct");
    }
}

static void generate(const NodeAssign& node){
    generate(*node.rhs);
    if (auto ident = get_if<NodeVar>(&node.lhs->val)){
        cout << "  mov %rax, " << ass_stack_reg(*ident) << endl;

    }
    else if (auto ident = get_if<NodeDeref>(&node.lhs->val)) {
        ass_push("%rax");
        generate(*ident->var);
        ass_pop("%rdi");
        ass_mov("%rdi", "(%rax)");
    }
    else {
        verror_at(node.token, "Left hand side of assignment should be identifier");
    }
}

static void generate(const NodeRet& node){
    generate(*node.pNode);
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

