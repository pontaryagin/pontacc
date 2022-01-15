#pragma once
#include "common.h"
#include "node.h"

inline auto get_null_statement(){
    return make_unique<NodeNull>();
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


using PaserType = function<pair<PtrTyped,int>(const vector<Token>& tokens, int start_pos)>;

pair<PtrTyped,int> parse_left_joint_binary_operator(const vector<Token>& tokens, int start_pos, const set<string>& operators, PaserType next_perser);

pair<PtrTyped,int> parse_expr(const vector<Token>& tokens, int start_pos);

// declspec = "int"
tuple<bool, Type, int> try_parse_declspec(const vector<Token>& tokens, int pos);

// declarator = "*"* ident
pair<PtrNode, int> parse_declarator(const vector<Token>& tokens, int pos, Type type);

// initializer = declarator ("=" expr)?
pair<PtrNode,int> parse_initializer(const vector<Token>& tokens, int pos, Type type);

// declaration = declspec (initializer ("," initializer)*)? ";"
tuple<bool, PtrNode,int> try_parse_declaration(const vector<Token>& tokens, int pos);

//  primary = num | ident | "(" expr ")"
pair<PtrTyped,int> parse_primary(const vector<Token>& tokens, int start_pos);

// unary = ("+" | "-" | "*" | "&") unary | primary
pair<PtrTyped,int> parse_unary(const vector<Token>& tokens, int pos);

//  mul     = unary ("*" unary | "/" unary)*
pair<PtrTyped,int> parse_mul(const vector<Token>& tokens, int pos);

//  add    = mul ("+" mul | "-" mul)*
pair<PtrTyped,int> parse_add(const vector<Token>& tokens, int pos);

//  relational = add ("<" add | "<=" add | ">" add | ">=" add)*
pair<PtrTyped,int> parse_relational(const vector<Token>& tokens, int pos);

//  equality   = relational ("==" relational | "!=" relational)*
pair<PtrTyped,int> parse_equality(const vector<Token>& tokens, int pos);

// assign     = equality ("=" assign)?
pair<PtrTyped,int> parse_assign(const vector<Token>& tokens, int start_pos);

// expr = assign
pair<PtrTyped,int> parse_expr(const vector<Token>& tokens, int start_pos);

pair<PtrNode,int> parse_statement(const vector<Token>& tokens, int pos);

// compound-statement = (declaration | statement)* "}"
pair<PtrNode,int> parse_compound_statement(const vector<Token>& tokens, int pos);

// expr_statement_return = "return" expr ";"
pair<PtrNode,int> parse_statement_return(const vector<Token>& tokens, int pos);

// expr_statement = expr? ";"
pair<PtrNode,int> parse_expr_statement(const vector<Token>& tokens, int pos);

// statement_for = "for" "(" expr_statement expr_statement expr? ")" statement
pair<PtrNode,int> parse_statement_for(const vector<Token>& tokens, int pos);

// statement_while = "while" "(" expr ")" statement
pair<PtrNode,int> parse_statement_while(const vector<Token>& tokens, int pos);

// statement_if = "if" "(" expr ")" statement ("else" statement)?
pair<PtrNode,int> parse_statement_if(const vector<Token>& tokens, int pos);

// statement = statement_if | statement_for | statement_while | "{" compound-statement |  "return" expr ";" |  expr_statement |
pair<PtrNode,int> parse_statement(const vector<Token>& tokens, int pos);

// program    = "{" compound-statement
pair<PtrNode,int> parse_program(const vector<Token>& tokens, int pos);
