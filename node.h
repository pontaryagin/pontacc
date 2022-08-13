#pragma once

#include "common.h"
#include "type.h"
#include "tokenizer.h"

struct INode{
    virtual optional<Token> get_token() const {return nullopt; };
    virtual void generate() = 0;
    virtual void generate_address() const { 
        auto token = get_token();
        verror_at(token, "Left hand side of assignment should be identifier"); 
    }
    virtual optional<int> get_offset() const { return nullopt; }
    virtual optional<bool> is_global() const { return nullopt; }
    virtual string ass_stack_reg() const { throw; };
    virtual ~INode() {}
};

using PtrNode = unique_ptr<INode>;

struct ITyped: virtual INode {
    virtual Type get_type() const = 0;
    virtual ~ITyped() {}
};

using PtrTyped = unique_ptr<ITyped>;

void generate_main(const PtrNode& node);

struct NodeNull: virtual INode{
    Token token;
    NodeNull(Token token) : token(token) {}
    optional<Token> get_token() const override { return token; }
    void generate() override {};
};

struct NodeNum: ITyped{
    Token token;
    int num;

    NodeNum(int num): num(num){}
    NodeNum(const Token& token): token(token), num(token.val){}

    Type get_type() const override {return TypeInt{}; }
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeVar: ITyped{
    Token token;
    string name;
    int offset;
    Type m_type;
    bool m_is_global;
    NodeVar(const Token& token, int offset, Type type, bool is_global = false)
        : token(token), name(token.ident), offset(offset), m_type(type), m_is_global(is_global){}
    optional<int> get_offset() const override { return offset; }
    optional<bool> is_global() const override { return m_is_global; }
    string ass_stack_reg() const override;
    Type get_type() const override;
    optional<Token> get_token() const override { return token; }
    void generate() override;
    void generate_address() const override;
};

struct NodeFunc: ITyped{
    Token token;
    string name;
    vector<PtrTyped> m_nodes;
    
    NodeFunc(const Token& token, vector<PtrTyped> m_nodes)
        : token(token), name(token.ident), m_nodes(move(m_nodes)){}
    Type get_type() const override;
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeAddress: ITyped{
    Token token;
    PtrTyped var;
    NodeAddress(Token token, PtrTyped var): token(token), var(move(var)){}

    Type get_type() const override;
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeDeref: ITyped{
    Token token;
    PtrTyped var;

    NodeDeref(Token token, PtrTyped var)
        : token(token), var(move(var)){}
    string ass_stack_reg() const override { return var->ass_stack_reg(); };
    optional<int> get_offset() const override { return var->get_offset(); }
    optional<bool> is_global() const override { return var->is_global(); }
    Type get_type() const override;
    optional<Token> get_token() const override { return token; }
    void generate() override;
    void generate_address() const override;
};

struct NodePunct: ITyped{
    Token token;
    PtrTyped lhs, rhs;

    NodePunct(const Token& token, PtrTyped lhs, PtrTyped rhs)
        : token(token), lhs(move(lhs)), rhs(move(rhs)){}

    Type get_type() const override;
    optional<Token> get_token() const override { return token; }
    void generate() override;
    void ass_adjust_address_mul();
    void ass_adjust_address_div();
};

struct NodeAssign: ITyped{
    Token token;
    PtrTyped lhs, rhs;

    NodeAssign(Token token, PtrTyped lhs, PtrTyped rhs)
        : token(token), lhs(move(lhs)), rhs(move(rhs)){}
    Type get_type() const override;
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeRet: ITyped{
    Token token;
    PtrTyped pNode;
    string m_func_name;

    NodeRet(Token token, PtrTyped pNode, string func_name): token(token), pNode(move(pNode)), m_func_name(func_name){}
    Type get_type() const override;
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeCompoundStatement: INode{
    Token token;
    vector<PtrNode> pNodes;

    NodeCompoundStatement(Token token, vector<PtrNode> pNodes)
        : token(token), pNodes(move(pNodes)){}
    optional<Token> get_token() const override { return token; }
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
    optional<Token> get_token() const override { return token; }
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
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeInitializer: ITyped {
    Token token;
    PtrNode var;
    PtrNode expr;
    Type type;

    NodeInitializer(Token token, PtrNode var, PtrNode expr, Type type)
        : token(token), var(move(var)), expr(move(expr)), type(type){}

    string ass_stack_reg() const override { return var->ass_stack_reg(); };
    Type get_type() const override { return type; }
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeDeclaration: INode {
    Token token;
    vector<PtrNode> pNodes;

    NodeDeclaration(Token token, vector<PtrNode> pNodes): token(token), pNodes(move(pNodes)){}
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeFuncDef: INode{
    Token token;
    string m_name;
    PtrNode m_statement;
    Type m_type;
    int m_local_variable_num;
    vector<unique_ptr<NodeVar>> m_param;
    
    NodeFuncDef(const Token& token, string name, PtrNode statement, Type m_type, int local_variable_num, decltype(m_param) param)
        : token(token), m_name(move(name)), m_statement(move(statement)), m_type(m_type), m_local_variable_num(local_variable_num), m_param(move(param)){}
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeProgram: INode {
    vector<PtrNode> pNodes;

    NodeProgram(vector<PtrNode> pNodes): pNodes(move(pNodes)){}
    void generate() override;
};

