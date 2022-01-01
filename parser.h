#pragma once
#include "common.h"
#include "tokenizer.h"
#include "codegen.h"

pair<unique_ptr<INode>,int> parse_expr(const vector<Token>& tokens, int start_pos);

//  primary = num | "(" expr ")"
pair<unique_ptr<INode>,int> parse_primary(const vector<Token>& tokens, int start_pos){
    auto token_val = tokens.at(start_pos).punct;
    if (token_val == "("){
        auto [pNode, pos] = parse_expr(tokens, start_pos+1);
        if (pos >= tokens.size()){
            verror_at(tokens.at(start_pos), "'(' was not closed");
        }
        if (tokens.at(pos).punct != ")") {
            verror_at(tokens.at(pos), "')' should come here");
        }
        return {move(pNode), pos+1};
    }
    return {make_unique<NodeNum>(tokens.at(start_pos)), start_pos+1};
}

// unary = ("+" | "-") unary | primary
pair<unique_ptr<INode>,int> parse_unary(const vector<Token>& tokens, int start_pos){
    if (tokens.at(start_pos).punct == "+"){
        return parse_unary(tokens, start_pos+1);
    }
    else if (tokens.at(start_pos).punct == "-"){
        auto [pNode, pos] = parse_unary(tokens, start_pos+1);
        return {make_unique<NodePunct>(tokens.at(start_pos), make_unique<NodeNum>(0), move(pNode)), pos};
    }
    return parse_primary(tokens, start_pos);
}

//  mul     = unary ("*" unary | "/" unary)*
pair<unique_ptr<INode>,int> parse_mul(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_unary(tokens, start_pos);
    while (pos < tokens.size()){
        auto token_val = tokens.at(pos).punct;
        if (token_val == "*" || token_val == "/" ){
            auto [pNode2, pos2] = parse_unary(tokens, pos+1);
            pNode = make_unique<NodePunct>(tokens.at(pos), move(pNode), move(pNode2));
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
}

//  add    = mul ("+" mul | "-" mul)*
pair<unique_ptr<INode>,int> parse_add(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_mul(tokens, start_pos);
    while (pos < tokens.size()){
        auto token_val = tokens.at(pos).punct;
        if (token_val == "+" || token_val == "-" ){
            auto [pNode2, pos2] = parse_mul(tokens, pos+1);
            pNode = make_unique<NodePunct>(tokens.at(pos), move(pNode), move(pNode2));
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
}

//  relational = add ("<" add | "<=" add | ">" add | ">=" add)*
pair<unique_ptr<INode>,int> parse_relational(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_add(tokens, start_pos);
    while (pos < tokens.size()){
        auto token_val = tokens.at(pos).punct;
        if (token_val == "<" || token_val == "<=" || token_val == ">" || token_val == ">="){
            auto [pNode2, pos2] = parse_add(tokens, pos+1);
            pNode = make_unique<NodePunct>(tokens.at(pos), move(pNode), move(pNode2));
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
}

//  expr    = relational ("==" relational | "!=" relational)*
pair<unique_ptr<INode>,int> parse_expr(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_relational(tokens, start_pos);
    while (pos < tokens.size()){
        auto& token_val = tokens.at(pos).punct;
        if (token_val == "==" || token_val == "!="){
            auto [pNode2, pos2] = parse_relational(tokens, pos+1);
            pNode = make_unique<NodePunct>(tokens.at(pos), move(pNode), move(pNode2));
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
}

// statement = expr ";"
pair<unique_ptr<INode>,int> parse_statement(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_expr(tokens, start_pos);
    auto& token_val = tokens.at(pos).punct;
    if (token_val == ";"){
        pos += 1;
    }
    else {
        verror_at(tokens.at(pos), "';' is expected");
    }
    return {move(pNode), pos};
}

// program    = statement*
pair<unique_ptr<INode>,int> parse_program(const vector<Token>& tokens, int start_pos){
    vector<unique_ptr<INode>> pNodes;
    int pos = 0;
    while (pos < tokens.size()){
        auto [pNode, pos_] = parse_statement(tokens, pos);
        pos = pos_;
        pNodes.emplace_back(move(pNode));
    }
    return {make_unique<NodeProgram>(move(pNodes)), pos};
}


