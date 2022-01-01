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
    cout << "  push %rbp" << endl;
    cout << "  mov %rsp, %rbp" << endl;
    cout << "  sub $" << indent_count*8 << ", %rsp" << endl;
}

static void ass_epilogue(int indent_count){
    cout << "  add $" << indent_count*8 << ", %rsp" << endl;
    cout << "  pop %rbp" << endl;
}

struct INode{
    virtual void generate() const = 0;
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

struct NodeIdent: INode {
    string name;
    int stack_num;
    NodeIdent(string name, int stack_num)
        : name(name), stack_num(stack_num)
    {}
    NodeIdent(const Token& token){
        if (token.kind != TokenKind::Ident){
            verror_at(token, "Identifier is expected");
        }
        name = token.ident;
        stack_num = token.val;
    }
    void generate() const override {
        string var = to_string(-8*(stack_num)) + "(%rbp)";
        cout << "  mov " << var << ", %rax" << endl;
        ass_push("rax"); 
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
};


struct NodeAssign: INode {
    unique_ptr<INode> lhs, rhs;
    Token token;
    NodeAssign(Token token, unique_ptr<INode> lhs, unique_ptr<INode> rhs)
        : token(token), lhs(move(lhs)), rhs(move(rhs))
    {}

    void generate() const override{
        rhs->generate();
        ass_pop("rax");
        auto& ident = dynamic_cast<const NodeIdent&>(*lhs);
        string var = to_string(-8*(ident.stack_num)) + "(%rbp)";
        cout << "  mov %rax, " << var << endl;
        ass_push("rax");
    }
};

struct NodeProgram: INode {
    vector<unique_ptr<INode>> pNodes;

    NodeProgram(vector<unique_ptr<INode>> pNodes)
        : pNodes(move(pNodes))
    {}

    void generate() const override{
        ass_prologue(Token::indents.size()+1);
        for (auto& pNode: pNodes){
            pNode->generate();
            ass_pop("rax");
        }
        ass_epilogue(Token::indents.size()+1);
    }
};

void gen_assembly(const INode& node){
    gen_header();
    cout << "main:\n";
    node.generate();
    cout << "  ret\n";
}

