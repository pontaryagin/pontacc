#pragma once
#include "common.h"
#include "tokenizer.h"
#include "codegen.h"

static bool is_punct(const vector<Token>& tokens, int pos, const string& op){
    if (pos >= tokens.size()){
        verror_at(tokens.back(), "'" + op + "' is expected", true);
    }
    return tokens.at(pos).kind == TokenKind::Punct && tokens.at(pos).punct == op;
}

static void expect_punct(const vector<Token>& tokens, int pos, const string& op){
    if (!is_punct(tokens, pos, op)){
        verror_at(tokens.at(pos), "'" + op + "' is expected");
        return;
    }
}


pair<unique_ptr<Node>,int> parse_expr(const vector<Token>& tokens, int start_pos);

//  primary = num | ident | "(" expr ")"
pair<unique_ptr<Node>,int> parse_primary(const vector<Token>& tokens, int start_pos){
    auto token_val = tokens.at(start_pos).punct;
    if (is_punct(tokens, start_pos, "(")){
        auto [pNode, pos] = parse_expr(tokens, start_pos+1);
        expect_punct(tokens, pos, ")");
        return {move(pNode), pos+1};
    }
    else if (tokens.at(start_pos).kind == TokenKind::Ident){
        return {token_to_node(tokens.at(start_pos)), start_pos+1};
    }
    return {token_to_node(tokens.at(start_pos)), start_pos+1};
}

// unary = ("+" | "-") unary | primary
pair<unique_ptr<Node>,int> parse_unary(const vector<Token>& tokens, int start_pos){
    if (tokens.at(start_pos).punct == "+"){
        return parse_unary(tokens, start_pos+1);
    }
    else if (tokens.at(start_pos).punct == "-"){
        auto [pNode, pos] = parse_unary(tokens, start_pos+1);
        return {make_unique<Node>(NodePunct{tokens.at(start_pos), make_unique<Node>(NodeNum{0}), move(pNode)}), pos};
    }
    return parse_primary(tokens, start_pos);
}

//  mul     = unary ("*" unary | "/" unary)*
pair<unique_ptr<Node>,int> parse_mul(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_unary(tokens, start_pos);
    while (pos < tokens.size()){
        auto token_val = tokens.at(pos).punct;
        if (token_val == "*" || token_val == "/" ){
            auto [pNode2, pos2] = parse_unary(tokens, pos+1);
            pNode = make_unique<Node>(NodePunct{tokens.at(pos), move(pNode), move(pNode2)});
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
}

//  add    = mul ("+" mul | "-" mul)*
pair<unique_ptr<Node>,int> parse_add(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_mul(tokens, start_pos);
    while (pos < tokens.size()){
        auto token_val = tokens.at(pos).punct;
        if (token_val == "+" || token_val == "-" ){
            auto [pNode2, pos2] = parse_mul(tokens, pos+1);
            pNode = make_unique<Node>(NodePunct{tokens.at(pos), move(pNode), move(pNode2)});
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
}

//  relational = add ("<" add | "<=" add | ">" add | ">=" add)*
pair<unique_ptr<Node>,int> parse_relational(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_add(tokens, start_pos);
    while (pos < tokens.size()){
        auto token_val = tokens.at(pos).punct;
        if (token_val == "<" || token_val == "<=" || token_val == ">" || token_val == ">="){
            auto [pNode2, pos2] = parse_add(tokens, pos+1);
            pNode = make_unique<Node>(NodePunct{tokens.at(pos), move(pNode), move(pNode2)});
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
}

//  equality   = relational ("==" relational | "!=" relational)*
pair<unique_ptr<Node>,int> parse_equality(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_relational(tokens, start_pos);
    while (pos < tokens.size()){
        auto& token_val = tokens.at(pos).punct;
        if (token_val == "==" || token_val == "!="){
            auto [pNode2, pos2] = parse_relational(tokens, pos+1);
            pNode = make_unique<Node>(NodePunct{tokens.at(pos), move(pNode), move(pNode2)});
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
}


// assign     = equality ("=" assign)?
pair<unique_ptr<Node>,int> parse_assign(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_equality(tokens, start_pos);
    if (is_punct(tokens, pos, "=")){
        auto [pNode2, pos2] = parse_assign(tokens, pos+1);
        pNode = make_unique<Node>(NodeAssign{tokens.at(pos), move(pNode), move(pNode2)});
        pos = pos2;
    }
    return {move(pNode), pos};
}

// expr = assign
pair<unique_ptr<Node>,int> parse_expr(const vector<Token>& tokens, int start_pos){
    return parse_assign(tokens, start_pos);
}

pair<unique_ptr<Node>,int> parse_statement(const vector<Token>& tokens, int pos);
// compound-statement = statement* "}"
pair<unique_ptr<Node>,int> parse_compound_statement(const vector<Token>& tokens, int pos){
    vector<unique_ptr<Node>> pNodes;
    while (!is_punct(tokens, pos, "}")){
        auto [pNode, pos_] = parse_statement(tokens, pos);
        pos = pos_;
        pNodes.emplace_back(move(pNode));
    }
    return {make_unique<Node>(NodeCompoundStatement{move(pNodes)}), pos+1};
}

// statement = "{" compound-statement | expr ";" | "return" expr ";"
pair<unique_ptr<Node>,int> parse_statement(const vector<Token>& tokens, int start_pos){
    optional<int> ret_pos;
    if (tokens.at(start_pos).kind == TokenKind::Keyword){
        ret_pos = start_pos;
        start_pos += 1;
    }
    else if (is_punct(tokens, start_pos, "{")){
        return parse_compound_statement(tokens, start_pos+1);
    }
    auto [pNode, pos] = parse_expr(tokens, start_pos);
    expect_punct(tokens, pos, ";");
    if (ret_pos){
        pNode = make_unique<Node>(NodeRet{tokens.at(*ret_pos), move(pNode)});
    }
    pos += 1;
    return {move(pNode), pos};
}

// program    = "{" compound-statement
pair<unique_ptr<Node>,int> parse_program(const vector<Token>& tokens, int pos){
    expect_punct(tokens, pos++, "{");
    return parse_compound_statement(tokens, pos);
}


