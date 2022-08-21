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
    virtual string ass_stack_reg() const { throw; }
    virtual ~INode() = default;
    virtual void assign_stack_offset() const {}
};

using PINode = unique_ptr<INode>;

struct ITyped: virtual INode {
    virtual Type get_type() const = 0;
    virtual ~ITyped() = default;
};

using PITyped = unique_ptr<ITyped>;

void generate_main(const PINode& node);

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
    int offset = -1;
    Type m_type;
    bool m_is_global;
    NodeVar* m_base = nullptr;
    NodeVar(Token token, Type type, string name, bool is_global = false, NodeVar* base = nullptr)
        : token(move(token)), name(name), m_type(move(type)), m_is_global(is_global), m_base(base){}
    NodeVar(const Token& token, Type type, bool is_global = false, NodeVar* base = nullptr)
        : NodeVar(token, move(type), token.ident, is_global, base) {}
    optional<int> get_offset() const override { return offset; }
    optional<bool> is_global() const override { return m_is_global; }
    string ass_stack_reg() const override;
    Type get_type() const override;
    optional<Token> get_token() const override { return token; }
    void generate() override;
    void generate_address() const override;
    // void assign_stack_offset(int offset) const override {
    // }
};

// GNU extension
struct NodeExpressVar: ITyped{
    Token m_token;
    string name;
    int offset;
    unique_ptr<struct NodeCompoundStatement> m_statement;
    reference_wrapper<ITyped> m_typed;
    ITyped& get_last_expr();
    NodeExpressVar(Token token, decltype(m_statement) statement)
        : m_token(move(token)), m_statement(move(statement)), m_typed(get_last_expr()){}
    Type get_type() const override { return m_typed.get().get_type();};
    optional<Token> get_token() const override { return m_token; }
    void generate() override;
};

struct NodeFunc: ITyped{
    Token token;
    string name;
    vector<PITyped> m_nodes;
    
    NodeFunc(const Token& token, vector<PITyped> m_nodes)
        : token(token), name(token.ident), m_nodes(move(m_nodes)){}
    Type get_type() const override;
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeAddress: ITyped{
    Token token;
    PITyped var;
    NodeAddress(Token token, PITyped var): token(token), var(move(var)){}

    Type get_type() const override;
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeDeref: ITyped{
    Token token;
    PITyped var;

    NodeDeref(Token token, PITyped var)
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
    PITyped lhs, rhs;

    NodePunct(const Token& token, PITyped lhs, PITyped rhs)
        : token(token), lhs(move(lhs)), rhs(move(rhs)){}

    Type get_type() const override;
    optional<Token> get_token() const override { return token; }
    void generate() override;
    void ass_adjust_address_mul();
    void ass_adjust_address_div();
};

struct NodeAssign: ITyped{
    Token token;
    PITyped lhs, rhs;

    NodeAssign(Token token, PITyped lhs, PITyped rhs)
        : token(token), lhs(move(lhs)), rhs(move(rhs)){}
    Type get_type() const override;
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeRet: ITyped{
    Token token;
    PITyped pNode;
    string m_func_name;

    NodeRet(Token token, PITyped pNode, string func_name): token(token), pNode(move(pNode)), m_func_name(func_name){}
    Type get_type() const override;
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeCompoundStatement: INode{
    Token token;
    vector<PINode> pNodes;

    NodeCompoundStatement(Token token, vector<PINode> pNodes)
        : token(token), pNodes(move(pNodes)){}
    optional<Token> get_token() const override { return token; }
    void generate() override;
};


struct NodeIf: INode{
    Token token;
    PINode expr;
    PINode statement_if;
    PINode statement_else;
    int count;
private:
    static inline int curr_count = 1;
public:
    NodeIf(Token token, PINode expr, PINode statement_if, PINode statement_else)
        : token(token), expr(move(expr)), statement_if(move(statement_if)), statement_else(move(statement_else)), count(curr_count++)
    {}
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeFor: INode{
    Token token;
    PINode expr_init;
    PINode expr_condition;
    PINode expr_increment;
    PINode statement;
    int count;
private:
    static inline int curr_count = 1;
public:
    NodeFor(Token token, PINode expr_init, PINode expr_condition, PINode expr_increment, PINode statement)
        : token(token), expr_init(move(expr_init)), expr_condition(move(expr_condition)), expr_increment(move(expr_increment)), 
          statement(move(statement)), count(curr_count++)
    {}
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeInitializer: ITyped {
    Token token;
    PITyped var;
    PINode expr;
    Type type;

    NodeInitializer(Token token, PITyped var, PINode expr, Type type)
        : token(token), var(move(var)), expr(move(expr)), type(type){}

    string ass_stack_reg() const override { return var->ass_stack_reg(); };
    Type get_type() const override { return type; }
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeDeclaration: INode {
    Token token;
    vector<PINode> pNodes;

    NodeDeclaration(Token token, vector<PINode> pNodes): token(token), pNodes(move(pNodes)){}
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeFuncDef: INode{
    Token token;
    string m_name;
    PINode m_statement;
    Type m_type;
    vector<unique_ptr<NodeVar>> m_param;
    int m_stack_size = 0;
    
    NodeFuncDef(const Token& token, string name, PINode statement, Type m_type, decltype(m_param) param, int stack_size)
        : token(token), m_name(move(name)), m_statement(move(statement)), m_type(m_type), 
        m_param(move(param)), m_stack_size(stack_size){}
    optional<Token> get_token() const override { return token; }
    void generate() override;
};

struct NodeProgram: INode {
    vector<PINode> pNodes;
    map<string, shared_ptr<const string>> m_string_literals;

    NodeProgram(vector<PINode> pNodes, decltype(m_string_literals) m_string_literals)
        : pNodes(move(pNodes)), m_string_literals(move(m_string_literals)){}
    void generate() override;
};

