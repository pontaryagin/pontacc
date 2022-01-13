#pragma once

#include "common.h"
#include "type.h"
#include "tokenizer.h"

struct Node;
using PtrNode = unique_ptr<Node>;

struct NodeNull{};

struct NodeNum{
    int num;
    Type get_type(){return Type{TypeKind::Int, 0}; }
    NodeNum(int num): num(num){}
    NodeNum(const Token& token): num(token.val){}
};

struct NodeVar{
    string name;
    int offset;
    optional<Type> get_type();
    NodeVar(const Token& token): name(token.ident), offset(token.val){}
};

struct NodeAddress{
    Token token;
    PtrNode var;

    Type get_type();
};

struct NodeDeref{
    Token token;
    PtrNode var;

    Type get_type();
};

struct NodePunct{
    Token token;
    PtrNode lhs, rhs;

    Type get_type();
};

struct NodeAssign{
    Token token;
    unique_ptr<Node> lhs, rhs;

    Type get_type();
};

struct NodeRet{
    Token token;
    unique_ptr<Node> pNode;
    Type get_type();
};

struct NodeCompoundStatement{
    vector<PtrNode> pNodes;
};


struct NodeIf{
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

struct NodeFor{
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

struct NodeInitializer {
    PtrNode var;
    PtrNode expr;
};

struct NodeDeclaration {
    vector<PtrNode> pNodes;
};

struct Node
{
    using NodeUnderlying = variant<
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
        NodeFor,
        NodeInitializer,
        NodeDeclaration>;
    NodeUnderlying val;
    optional<Type> type;
};

Type NodeAddress::get_type() {
    return Type{var->type->kind, var->type->ptr+1};
}

Type NodeDeref::get_type(){
    if (var->type->ptr<=0){
        verror_at(token, "non pointer type cannot be dereferenced");
    }
    return Type{var->type->kind, var->type->ptr-1};
}

Type NodePunct::get_type(){
    auto tl = *lhs->type;
    auto tr = *rhs->type;
    if ((tl.ptr > 0 && tr.ptr == 0) || (tl.ptr ==0 && tr.ptr > 0)){
        return tl.ptr > 0 ? tl: tr;
    }
    else if (tl.ptr > 0 && tr.ptr > 0){
        if (tl!= tr){
            verror_at(token, "diffrent types passed to operator");
        }
        return Type{TypeKind::Int, 0};
    }
    return tl;
}

static inline map<string, optional<Type>> var_types;

Type NodeAssign::get_type(){
    // check type of both side of assignement or complement of the type of left hand side
    auto& tl = lhs->type;
    auto& tr = rhs->type;
    if(!tr){
        verror_at(token, "type of right hand side of assignment not defined");
    }
    if (!tl){
        auto node_var = get_if<NodeVar>(&lhs->val);
        if(!node_var){
            verror_at(token, "variable should come at lhs of assignment");
        }
    }
    else {
        if(*tl!=*tr){
            verror_at(token, "diffrent types for left and right hand side of assignment");
        }
    }
    return *tl;
}

Type NodeRet::get_type(){
    return *pNode->type;
}

optional<Type> NodeVar::get_type(){
    return var_types[name];
}
