#pragma once
#include "common.h"

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

struct INode;

struct NodeNum {
    int num;
};

struct NodeIdent {
    string name;
    int stack_num;
};

struct NodePunct{
    unique_ptr<INode> lhs, rhs;
    Token token;
};


struct NodeAssign {
    unique_ptr<INode> lhs, rhs;
    Token token;
};

struct NodeRet {
    unique_ptr<INode> pNode;
    Token token;
};

struct NodeCompoundStatement {
    vector<unique_ptr<INode>> pNodes;
};

using INode = variant<
    NodeNum,
    NodeIdent,
    NodePunct,
    NodeAssign,
    NodeRet,
    NodeCompoundStatement>;

static void generate(const INode& node);

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
    auto& ident = get<NodeIdent>(*node.lhs);
    cout << "  mov %rax, " << ass_stack_reg(ident.stack_num) << endl;
    ass_push("rax");
}

static void generate(const NodeRet& node){
    generate(*node.pNode);
    ass_pop("rax");
    cout << "  jmp .L.return" << endl;
}

static void generate(const INode& node){
    visit(node, [](const auto& node){
        generate(node);
    });
}

void gen_assembly(const INode& node){
    gen_header();
    cout << "main:\n";
    ass_prologue(Token::indents.size()+1);
    generate(node);
    ass_epilogue();
    cout << "  ret\n";
}

static unique_ptr<INode> token_to_node(const Token& token){
    if (token.kind == TokenKind::Num){
        return make_unique<INode>(NodeNum{token.val});
    }
    if (token.kind == TokenKind::Ident){
        return make_unique<INode>(NodeIdent{token.ident, token.val});
    }
    abort();
}

