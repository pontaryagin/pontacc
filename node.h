#pragma once

#include "common.h"
#include "type.h"
#include "tokenizer.h"

static inline map<string, Type> var_types;

struct INode{
    virtual Token get_token() const = 0;
    virtual void generate() = 0;
    virtual void generate_address() const { verror_at(get_token(), "Left hand side of assignment should be identifier"); }
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
    Token token;
    NodeNull(Token token) : token(token) {}
    Token get_token() const override { return token; }
    void generate() override {};
};

struct NodeNum: ITyped{
    Token token;
    int num;

    NodeNum(int num): num(num){}
    NodeNum(const Token& token): token(token), num(token.val){}

    Type get_type() override {return Type{TypeKind::Int, 0}; }
    Token get_token() const override { return token; }
    void generate() override;
};

struct NodeVar: ITyped{
    Token token;
    string name;
    int offset;
    NodeVar(const Token& token): token(token), name(token.ident), offset(token.val){}
    optional<int> get_offset() const override { return offset; }
    Type get_type() override;
    Token get_token() const override { return token; }
    void generate() override;
    void generate_address() const override;
};

struct NodeFunc: ITyped{
    Token token;
    string name;
    vector<PtrTyped> m_nodes;
    
    NodeFunc(const Token& token, vector<PtrTyped> m_nodes)
        : token(token), name(token.ident), m_nodes(move(m_nodes)){}
    Type get_type() override;
    Token get_token() const override { return token; }
    void generate() override;
};

struct NodeAddress: ITyped{
    Token token;
    PtrTyped var;
    NodeAddress(Token token, PtrTyped var): token(token), var(move(var)){}

    Type get_type() override;
    Token get_token() const override { return token; }
    void generate() override;
};

struct NodeDeref: ITyped{
    Token token;
    PtrTyped var;

    NodeDeref(Token token, PtrTyped var)
        : token(token), var(move(var)){}
    optional<int> get_offset() const override { return var->get_offset(); }
    Type get_type() override;
    Token get_token() const override { return token; }
    void generate() override;
    void generate_address() const override;
};

struct NodePunct: ITyped{
    Token token;
    PtrTyped lhs, rhs;

    NodePunct(const Token& token, PtrTyped lhs, PtrTyped rhs)
        : token(token), lhs(move(lhs)), rhs(move(rhs)){}

    Type get_type() override;
    Token get_token() const override { return token; }
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
    Token get_token() const override { return token; }
    void generate() override;
};

struct NodeRet: ITyped{
    Token token;
    PtrTyped pNode;

    NodeRet(Token token, PtrTyped pNode): token(token), pNode(move(pNode)){}
    Type get_type() override;
    Token get_token() const override { return token; }
    void generate() override;
};

struct NodeCompoundStatement: INode{
    Token token;
    vector<PtrNode> pNodes;

    NodeCompoundStatement(Token token, vector<PtrNode> pNodes)
        : token(token), pNodes(move(pNodes)){}
    Token get_token() const override { return token; }
    void generate() override;
};


struct NodeIf: INode{
    Token token;
    PtrNode expr;
    PtrNode statement_if;
    PtrNode statement_else;
    int count;
private:
    static inline int curr_count = 1;
public:
    NodeIf(Token token, PtrNode expr, PtrNode statement_if, PtrNode statement_else)
        : token(token), expr(move(expr)), statement_if(move(statement_if)), statement_else(move(statement_else)), count(curr_count++)
    {}
    Token get_token() const override { return token; }
    void generate() override;
};

struct NodeFor: INode{
    Token token;
    PtrNode expr_init;
    PtrNode expr_condition;
    PtrNode expr_increment;
    PtrNode statement;
    int count;
private:
    static inline int curr_count = 1;
public:
    NodeFor(Token token, PtrNode expr_init, PtrNode expr_condition, PtrNode expr_increment, PtrNode statement)
        : token(token), expr_init(move(expr_init)), expr_condition(move(expr_condition)), expr_increment(move(expr_increment)), 
          statement(move(statement)), count(curr_count++)
    {}
    Token get_token() const override { return token; }
    void generate() override;
};

struct NodeInitializer: ITyped {
    Token token;
    PtrNode var;
    PtrNode expr;
    Type type;

    NodeInitializer(Token token, PtrNode var, PtrNode expr, Type type)
        : token(token), var(move(var)), expr(move(expr)), type(type){}

    Type get_type() override { return type; }
    Token get_token() const override { return token; }
    void generate() override;
};

struct NodeDeclaration: INode {
    Token token;
    vector<PtrNode> pNodes;

    NodeDeclaration(Token token, vector<PtrNode> pNodes): token(token), pNodes(move(pNodes)){}
    Token get_token() const override { return token; }
    void generate() override;
};
