#pragma once

#include "common.h"
#include "type.h"
#include "tokenizer.h"

struct INode{
    virtual void generate() = 0;
};
using PtrNode = unique_ptr<INode>;
using OptionalType = optional<Type>;

struct ITyped: virtual INode {
    virtual optional<Type> get_type() = 0;
};

using PtrTyped = unique_ptr<ITyped>;

struct NodeNull: virtual INode{
    void generate() override {};
};

struct NodeNum: ITyped{
    int num;

    NodeNum(int num): num(num){}
    NodeNum(const Token& token): num(token.val){}

    OptionalType get_type() override {return Type{TypeKind::Int, 0}; }
    void generate() override;
};

struct NodeVar: ITyped{
    string name;
    int offset;
    NodeVar(const Token& token): name(token.ident), offset(token.val){}
    OptionalType get_type() override;
    void generate() override;
};

struct NodeAddress: ITyped{
    Token token;
    PtrTyped var;
    NodeAddress(Token token, PtrTyped var): token(token), var(move(var)){}

    OptionalType get_type() override;
    void generate() override;
};

struct NodeDeref: ITyped{
    Token token;
    PtrTyped var;

    NodeDeref(Token token, PtrTyped var)
        : token(token), var(move(var)){}
    OptionalType get_type() override;
    void generate() override;
};

struct NodePunct: ITyped{
    Token token;
    PtrTyped lhs, rhs;

    NodePunct(const Token& token, PtrTyped lhs, PtrTyped rhs)
        : token(token), lhs(move(lhs)), rhs(move(rhs)){}

    OptionalType get_type() override;
    void generate() override;
    void ass_adjust_address_mul();
    void ass_adjust_address_div();
};

struct NodeAssign: ITyped{
    Token token;
    PtrTyped lhs, rhs;

    NodeAssign(Token token, PtrTyped lhs, PtrTyped rhs)
        : token(token), lhs(move(lhs)), rhs(move(rhs)){}
    OptionalType get_type() override;
    void generate() override;
};

struct NodeRet: ITyped{
    Token token;
    PtrTyped pNode;

    NodeRet(Token token, PtrTyped pNode): token(token), pNode(move(pNode)){}
    OptionalType get_type() override;
    void generate() override;
};

struct NodeCompoundStatement: INode{
    vector<PtrNode> pNodes;

    NodeCompoundStatement(vector<PtrNode> pNodes)
        : pNodes(move(pNodes)){}
    void generate() override;
};


struct NodeIf: INode{
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
    void generate() override;
};

struct NodeFor: INode{
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
    void generate() override;
};

struct NodeInitializer: ITyped {
    PtrNode var;
    PtrNode expr;
    Type type;

    NodeInitializer(PtrNode var, PtrNode expr, Type type)
        : var(move(var)), expr(move(expr)), type(type){}

    OptionalType get_type() override { return type; }
    void generate() override;
};

struct NodeDeclaration: INode {
    vector<PtrNode> pNodes;

    NodeDeclaration(vector<PtrNode> pNodes): pNodes(move(pNodes)){}
    void generate() override;
};

OptionalType NodeAddress::get_type() {
    auto type = var->get_type();
    return Type{type->kind, type->ptr+1};
}

OptionalType NodeDeref::get_type(){
    auto type = var->get_type();
    if (type->ptr<=0){
        verror_at(token, "non pointer type cannot be dereferenced");
    }
    return Type{type->kind, type->ptr-1};
}

OptionalType NodePunct::get_type(){
    auto tl = *lhs->get_type();
    auto tr = *rhs->get_type();
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

OptionalType NodeAssign::get_type(){
    // check type of both side of assignement or complement of the type of left hand side
    auto tl = lhs->get_type();
    auto tr = rhs->get_type();
    assert_at(tr.has_value() ,token, "type of right hand side of assignment not defined");
    if (!tl){
        assert_at(dynamic_cast<NodeVar*>(lhs.get()), token, "variable should come at lhs of assignment");
    }
    else {
        assert_at(*tl==*tr, token, "diffrent types for left and right hand side of assignment");
    }
    return *tl;
}

OptionalType NodeRet::get_type(){
    return pNode->get_type();
}

optional<Type> NodeVar::get_type(){
    return var_types[name];
}
