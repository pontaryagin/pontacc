#pragma once
#include "common.h"
#include "node.h"

class Context {
    string m_func_name;
    map<string, Type> m_var_types;
    map<string, Type> m_var_types_global;
    map<string, int> m_idents_offset;
    int m_idents_index_max;
    shared_ptr<map<string, shared_ptr<const string>>> m_string_literal 
        = make_shared<map<string, shared_ptr<const string>>>();
    Context* m_parent_context;
    
    optref<const Type> global(const string& name) const{
        if (m_parent_context){
            return m_parent_context->global(name);
        }
        auto it = m_var_types_global.find(name);
        return it == m_var_types_global.end() ? nullopt : optref<const Type>(it->second);
    }
    optref<const Type> local(const string& name) const{
        auto it = m_var_types.find(name);
        if (it != m_var_types.end()){
            return optref<const Type>(it->second);
        }
        if (m_parent_context){
            return m_parent_context->local(name);
        }
        return nullopt;
    }
public:
    Context() = default;
    Context(Context* parent_context) 
        : m_idents_index_max(parent_context->m_idents_index_max),
        m_string_literal(parent_context->m_string_literal),
        m_parent_context(parent_context)
        {}
    int variable_offset(const Token& token);
    optref<const Type> variable_type(const string& name, bool is_global) const{
        return is_global ? global(name) : local(name);
    }
    optref<const Type> variable_type(const string& name) const{
        const auto& l = local(name);
        return l ? l : global(name);
    }
    void set_variable_type(const string& name, bool is_global, Type type){
        if (is_global){
            if (m_parent_context){
                m_parent_context->set_variable_type(name, is_global, move(type));
            }
            else {
                m_var_types_global[name] = move(type);
            }
        }
        else {
            m_var_types[name] = move(type);
        }
    }
    void string_literal(const string& name, shared_ptr<const string> val){
        m_string_literal->emplace(name, move(val));
    }
    void set_func_name(optref<const string> func_name_in){
        m_func_name = *func_name_in;
    }
    const string& func_name() const{
        return m_func_name != "" ? m_func_name : m_parent_context->func_name();
    }
    int idents_index_max() const{
        return m_idents_index_max;
    }
    const map<string, shared_ptr<const string>>& string_literal(){
        return *m_string_literal;
    }
};

inline auto get_null_statement(Token token){
    return make_unique<NodeNull>(token);
}

inline bool is_kind(const vector<Token>& tokens, int pos, TokenKind kind){
    if (pos >= tokens.size()){
        verror_at(tokens.back(), to_string(pos) + " is out of range of tokens", true);
    }
    return tokens.at(pos).kind == kind;
}

inline void expect_kind(const vector<Token>& tokens, int pos, TokenKind kind){
    if (!is_kind(tokens, pos, kind)){
        verror_at(tokens.at(pos), "TokenKind " + to_string((int)kind) + " is expected", false);
    }
}

inline bool is_punct(const vector<Token>& tokens, int pos, const string& op){
    return is_kind(tokens, pos, TokenKind::Punct) && tokens.at(pos).punct == op;
}

inline void expect_punct(const vector<Token>& tokens, int pos, const string& op){
    if (!is_punct(tokens, pos, op)){
        verror_at(tokens.at(pos), "'" + op + "' is expected");
    }
}

inline bool is_keyword(const vector<Token>& tokens, int pos, const string& keyword){
    if (pos >= tokens.size()){
        verror_at(tokens.back(), "'" + keyword + "' is expected", true);
    }
    return tokens.at(pos).kind == TokenKind::Keyword && tokens.at(pos).ident == keyword;
}

inline void expect_keyword(const vector<Token>& tokens, int pos, const string& keyword){
    if (!is_keyword(tokens, pos, keyword)){
        verror_at(tokens.at(pos), "'" + keyword + "' is expected");
    }
}

template<class ...T>
using PosRet = tuple<T..., int>;

using PaserType = function<PosRet<PITyped>(const vector<Token>& tokens, int start_pos, Context& context)>;

PosRet<PITyped> parse_left_joint_binary_operator(const vector<Token>& tokens, int start_pos, const set<string>& operators, Context& context, PaserType next_perser);

PosRet<PITyped> parse_expr(const vector<Token>& tokens, int start_pos, Context& context);

PosRet<optional<Type>> try_parse_declspec(const vector<Token>& tokens, int pos);

PosRet<unique_ptr<NodeVar>, vector<unique_ptr<NodeVar>>>
parse_declarator(const vector<Token>& tokens, int pos, Type type, bool is_global, Context& context);

PosRet<PINode> parse_initializer(const vector<Token>& tokens, int pos, Type type);

PosRet<PINode> try_parse_declaration(const vector<Token>& tokens, int pos, Context& context);

PosRet<PITyped> parse_primary(const vector<Token>& tokens, int start_pos, Context& context);

PosRet<PITyped> parse_unary(const vector<Token>& tokens, int pos, Context& context);

PosRet<PITyped> parse_mul(const vector<Token>& tokens, int pos, Context& context);

PosRet<PITyped> parse_add(const vector<Token>& tokens, int pos, Context& context);

PosRet<PITyped> parse_relational(const vector<Token>& tokens, int pos, Context& context);

PosRet<PITyped> parse_equality(const vector<Token>& tokens, int pos, Context& context);

PosRet<PITyped> parse_assign(const vector<Token>& tokens, int start_pos, Context& context);

PosRet<PITyped> parse_expr(const vector<Token>& tokens, int start_pos, Context& context);

PosRet<PINode> parse_statement(const vector<Token>& tokens, int pos, Context& context);

PosRet<unique_ptr<NodeCompoundStatement>> parse_compound_statement(const vector<Token>& tokens, int pos, Context& context);

PosRet<PINode> parse_statement_return(const vector<Token>& tokens, int pos, Context& context);

PosRet<PINode> parse_expr_statement(const vector<Token>& tokens, int pos, Context& context);

PosRet<PINode> parse_statement_for(const vector<Token>& tokens, int pos, Context& context);

PosRet<PINode> parse_statement_while(const vector<Token>& tokens, int pos, Context& context);

PosRet<PINode> parse_statement_if(const vector<Token>& tokens, int pos, Context& context);

PosRet<PINode> parse_statement(const vector<Token>& tokens, int pos, Context& context);

PosRet<PINode> parse_func_def(const vector<Token>& tokens, int pos);

PosRet<PINode> parse_program(const vector<Token>& tokens, int pos);
