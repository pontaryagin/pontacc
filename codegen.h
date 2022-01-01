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

void gen_assembly(const INode& node){
    gen_header();
    cout << "main:\n";
    node.generate();
    ass_pop("rax");
    cout << "  ret\n";
}

