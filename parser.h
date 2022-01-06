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

static bool is_keyword(const vector<Token>& tokens, int pos, const string& keyword){
    if (pos >= tokens.size()){
        verror_at(tokens.back(), "'" + keyword + "' is expected", true);
    }
    return tokens.at(pos).kind == TokenKind::Keyword && tokens.at(pos).ident == keyword;
}


template<class T>
static auto get_node(T&& node){
    return make_unique<Node>(forward<T>(node));
}

static auto get_null_statement(){
    return get_node(NodeCompoundStatement{});
}

using PaserType = function<pair<PtrNode,int>(const vector<Token>& tokens, int start_pos)>;

static pair<unique_ptr<Node>,int> parse_left_joint_binary_operator(const vector<Token>& tokens, int start_pos, const set<string>& operators, PaserType next_perser){
    auto [pNode, pos] = next_perser(tokens, start_pos);
    while (pos < tokens.size()){
        auto token_val = tokens.at(pos).punct;
        if (operators.contains(token_val)){
            auto [pNode2, pos2] = next_perser(tokens, pos+1);
            pNode = get_node(NodePunct{tokens.at(pos), move(pNode), move(pNode2)});
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
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
        return {get_node(NodePunct{tokens.at(start_pos), get_node(NodeNum{0}), move(pNode)}), pos};
    }
    return parse_primary(tokens, start_pos);
}



//  mul     = unary ("*" unary | "/" unary)*
pair<unique_ptr<Node>,int> parse_mul(const vector<Token>& tokens, int pos){
    parse_left_joint_binary_operator(tokens, pos, {"*", "/"}, parse_unary);
}

//  add    = mul ("+" mul | "-" mul)*
pair<unique_ptr<Node>,int> parse_add(const vector<Token>& tokens, int pos){
    parse_left_joint_binary_operator(tokens, pos, {"+", "-"}, parse_mul);
}

//  relational = add ("<" add | "<=" add | ">" add | ">=" add)*
pair<unique_ptr<Node>,int> parse_relational(const vector<Token>& tokens, int pos){
    parse_left_joint_binary_operator(tokens, pos, {"<", "<=", ">", ">="}, parse_mul);
}

//  equality   = relational ("==" relational | "!=" relational)*
pair<unique_ptr<Node>,int> parse_equality(const vector<Token>& tokens, int pos){
    parse_left_joint_binary_operator(tokens, pos, {"==", "!="}, parse_mul);
}

// assign     = equality ("=" assign)?
pair<unique_ptr<Node>,int> parse_assign(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_equality(tokens, start_pos);
    if (is_punct(tokens, pos, "=")){
        auto [pNode2, pos2] = parse_assign(tokens, pos+1);
        pNode = get_node(NodeAssign{tokens.at(pos), move(pNode), move(pNode2)});
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
    return {get_node(NodeCompoundStatement{move(pNodes)}), pos+1};
}

// expr_statement_return = "return" expr ";"
pair<unique_ptr<Node>,int> parse_statement_return(const vector<Token>& tokens, int pos){
    assert(is_keyword(tokens, pos, "return"));
    auto [pNode, pos2] = parse_expr(tokens, pos+1);
    expect_punct(tokens, pos2, ";");
    pNode = get_node(NodeRet{tokens.at(pos), move(pNode)});
    return {move(pNode), pos2+1};
}

// expr_statement = expr? ";"
pair<unique_ptr<Node>,int> parse_expr_statement(const vector<Token>& tokens, int pos){
    if (is_punct(tokens, pos, ";")){
        return {get_null_statement(), pos+1};
    }
    auto [pNode, pos2] = parse_expr(tokens, pos);
    expect_punct(tokens, pos2, ";");
    return {move(pNode), pos2+1};
}

// // statement_for = "for" "(" expr_statement expr_statement expr_statement ")" statement
// pair<unique_ptr<Node>,int> parse_statement_for(const vector<Token>& tokens, int start_pos){
//     assert(is_keyword(tokens, start_pos, "if"));
//     expect_punct(tokens, start_pos, "(");
//     auto [expr, pos] = parse_expr(tokens, start_pos+1);
//     expect_punct(tokens, pos, ")");
//     auto [statement_if, pos2] = parse_statement(tokens, pos+1);
//     if (is_keyword(tokens, pos2, "else")){
//         auto [statement_else, pos3] = parse_statement(tokens, pos2+1);
//         return {get_node(NodeIf(move(expr), move(statement_if), move(statement_else))), pos3};
//     }
//     return {get_node(NodeIf(move(expr), move(statement_if), nullptr)), pos2};
// }

// statement_if = "if" "(" expr ")" statement ("else" statement)?
pair<unique_ptr<Node>,int> parse_statement_if(const vector<Token>& tokens, int pos){
    assert(is_keyword(tokens, pos, "if"));
    expect_punct(tokens, pos+1, "(");
    auto [expr, pos2] = parse_expr(tokens, pos+2);
    expect_punct(tokens, pos2, ")");
    auto [statement_if, pos3] = parse_statement(tokens, pos2+1);
    if (is_keyword(tokens, pos3, "else")){
        auto [statement_else, pos4] = parse_statement(tokens, pos3+1);
        return {get_node(NodeIf(move(expr), move(statement_if), move(statement_else))), pos4};
    }
    return {get_node(NodeIf(move(expr), move(statement_if), nullptr)), pos3};
}

// statement = statement_if | statement_for | "{" compound-statement |  "return" expr ";" |  expr_statement |
pair<unique_ptr<Node>,int> parse_statement(const vector<Token>& tokens, int start_pos){
    if (is_keyword(tokens, start_pos, "if")){
        return parse_statement_if(tokens, start_pos);
    }
    // if (is_keyword(tokens, start_pos, "for")){
    //     return parse_statement_for(tokens, start_pos);
    // }
    if (is_punct(tokens, start_pos, "{")){
        return parse_compound_statement(tokens, start_pos+1);
    }
    if (is_keyword(tokens, start_pos, "return")){
        return parse_statement_return(tokens, start_pos);
    }
    return parse_expr_statement(tokens, start_pos);
}

// program    = "{" compound-statement
pair<unique_ptr<Node>,int> parse_program(const vector<Token>& tokens, int pos){
    expect_punct(tokens, pos++, "{");
    return parse_compound_statement(tokens, pos);
}


