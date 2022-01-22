#pragma once

#include "common.h"
#include "type.h"
#include "tokenizer.h"

static inline map<string, Type> var_types;

struct INode{
    virtual void generate() = 0;
    virtual optional<int> get_offset() const { return nullopt; }
    virtual ~INode() {}
};
using PtrNode = unique_ptr<INode>;

struct ITyped: virtual INode {
    virtual Type get_type() = 0;
    virtual ~ITyped() {}
};

using PtrTyped = unique_ptr<ITyped>;

void generate_main(const PtrNode& node);

struct NodeNull: virtual INode{
    void generate() override {};
};

struct NodeNum: ITyped{
    int num;

    NodeNum(int num): num(num){}
    NodeNum(const Token& token): num(token.val){}

    Type get_type() override {return Type{TypeKind::Int, 0}; }
    void generate() override;
};

struct NodeVar: ITyped{
    string name;
    int offset;
    NodeVar(const Token& token): name(token.ident), offset(token.val){}
    optional<int> get_offset() const override { return offset; }
    Type get_type() override;
    void generate() override;
};

struct NodeFunc: ITyped{
    Token token;
    string name;
    vector<PtrTyped> m_nodes;
    
    NodeFunc(const Token& token, vector<PtrTyped> m_nodes)
        : token(token), name(token.ident), m_nodes(move(m_nodes)){}
    Type get_type() override;
    void generate() override;
};

struct NodeAddress: ITyped{
    Token token;
    PtrTyped var;
    NodeAddress(Token token, PtrTyped var): token(token), var(move(var)){}

    Type get_type() override;
    void generate() override;
};

struct NodeDeref: ITyped{
    Token token;
    PtrTyped var;

    NodeDeref(Token token, PtrTyped var)
        : token(token), var(move(var)){}
    optional<int> get_offset() const override { return var->get_offset(); }
    Type get_type() override;
    void generate() override;
};

struct NodePunct: ITyped{
    Token token;
    PtrTyped lhs, rhs;

    NodePunct(const Token& token, PtrTyped lhs, PtrTyped rhs)
        : token(token), lhs(move(lhs)), rhs(move(rhs)){}

    Type get_type() override;
    void generate() override;
    void ass_adjust_address_mul();
    void ass_adjust_address_div();
};

struct NodeAssign: ITyped{
    Token token;
    PtrTyped lhs, rhs;

    NodeAssign(Token token, PtrTyped lhs, PtrTyped rhs)
        : token(token), lhs(move(lhs)), rhs(move(rhs)){}
    Type get_type() override;
    void generate() override;
};

struct NodeRet: ITyped{
    Token token;
    PtrTyped pNode;

    NodeRet(Token token, PtrTyped pNode): token(token), pNode(move(pNode)){}
    Type get_type() override;
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

    Type get_type() override { return type; }
    void generate() override;
};

struct NodeDeclaration: INode {
    vector<PtrNode> pNodes;

    NodeDeclaration(vector<PtrNode> pNodes): pNodes(move(pNodes)){}
    void generate() override;
};
