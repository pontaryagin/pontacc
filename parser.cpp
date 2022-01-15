#include "tokenizer.h"
#include "parser.h"

pair<PtrTyped,int> parse_left_joint_binary_operator(const vector<Token>& tokens, int start_pos, const set<string>& operators, PaserType next_perser){
    auto [pNode, pos] = next_perser(tokens, start_pos);
    while (pos < tokens.size()){
        auto token_val = tokens.at(pos).punct;
        if (operators.contains(token_val)){
            auto [pNode2, pos2] = next_perser(tokens, pos+1);
            pNode = make_unique<NodePunct>(tokens.at(pos), move(pNode), move(pNode2));
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
}

pair<PtrTyped,int> parse_expr(const vector<Token>& tokens, int start_pos);

// declspec = "int"
tuple<bool, Type, int> try_parse_declspec(const vector<Token>& tokens, int pos) {
    if(is_keyword(tokens, pos, "int")){
        return {true, Type{TypeKind::Int, 0}, pos+1};
    }
    return {false, Type{}, pos};
}

// declarator = "*"* ident
pair<PtrNode, int> parse_declarator(const vector<Token>& tokens, int pos, Type type) {
    while(is_punct(tokens, pos, "*")){
        ++pos;
        ++type.ptr;
    }
    expect_kind(tokens, pos, TokenKind::Ident);
    var_types[tokens.at(pos).ident] = type;
    auto pNode = make_unique<NodeVar>(tokens.at(pos));
    return {move(pNode), pos+1};
}

// initializer = declarator ("=" expr)?
pair<PtrNode,int> parse_initializer(const vector<Token>& tokens, int pos, Type type) {
    auto [pNode, pos_decl] = parse_declarator(tokens, pos, type);
    if (is_punct(tokens, pos_decl, "=")){
        auto [expr, pos_expr] = parse_expr(tokens, pos_decl+1);
        return {make_unique<NodeInitializer>(move(pNode), move(expr), type), pos_expr};
    }
    return {make_unique<NodeInitializer>(move(pNode), nullptr, type), pos_decl};
}


// declaration = declspec (initializer ("," initializer)*)? ";"
tuple<bool, PtrNode,int> try_parse_declaration(const vector<Token>& tokens, int pos) {
    auto [ok, type, pos1] = try_parse_declspec(tokens, pos);
    if (!ok){
        return {false, nullptr, pos1};
    }
    vector<PtrNode> pNodes;
    PtrNode pNode;
    do {
        tie(pNode, pos1) = parse_initializer(tokens, pos1, type);
        pNodes.emplace_back(move(pNode));
    } while(is_punct(tokens, pos1++, ","));
    expect_punct(tokens, pos1-1, ";");
    return {true, make_unique<NodeDeclaration>(move(pNodes)), pos1};
}

//  primary = num | ident | "(" expr ")"
pair<PtrTyped,int> parse_primary(const vector<Token>& tokens, int start_pos){
    auto token_val = tokens.at(start_pos).punct;
    if (is_punct(tokens, start_pos, "(")){
        auto [pNode, pos] = parse_expr(tokens, start_pos+1);
        expect_punct(tokens, pos, ")");
        return {move(pNode), pos+1};
    }
    else if (tokens.at(start_pos).kind == TokenKind::Ident){
        return {make_unique<NodeVar>(tokens.at(start_pos)), start_pos+1};
    }
    return {make_unique<NodeNum>(tokens.at(start_pos)), start_pos+1};
}

// unary = ("+" | "-" | "*" | "&") unary | primary
pair<PtrTyped,int> parse_unary(const vector<Token>& tokens, int pos){
    if (is_punct(tokens, pos, "+")){
        return parse_unary(tokens, pos+1);
    }
    else if (is_punct(tokens, pos, "-")){
        auto [pNode, pos_end] = parse_unary(tokens, pos+1);
        return {make_unique<NodePunct>(tokens.at(pos), make_unique<NodeNum>(0), move(pNode)), pos_end};
    }
    else if (is_punct(tokens, pos, "*")){
        auto [pNode, pos_end] = parse_unary(tokens, pos+1);
        return {make_unique<NodeDeref>(tokens.at(pos), move(pNode)), pos_end};
    }
    else if (is_punct(tokens, pos, "&")){
        auto [pNode, pos_end] = parse_unary(tokens, pos+1);
        return {make_unique<NodeAddress>(tokens.at(pos), move(pNode)), pos_end};
    }
    return parse_primary(tokens, pos);
}

//  mul     = unary ("*" unary | "/" unary)*
pair<PtrTyped,int> parse_mul(const vector<Token>& tokens, int pos){
    return parse_left_joint_binary_operator(tokens, pos, {"*", "/"}, parse_unary);
}

//  add    = mul ("+" mul | "-" mul)*
pair<PtrTyped,int> parse_add(const vector<Token>& tokens, int pos){
    return parse_left_joint_binary_operator(tokens, pos, {"+", "-"}, parse_mul);
}

//  relational = add ("<" add | "<=" add | ">" add | ">=" add)*
pair<PtrTyped,int> parse_relational(const vector<Token>& tokens, int pos){
    return parse_left_joint_binary_operator(tokens, pos, {"<", "<=", ">", ">="}, parse_add);
}

//  equality   = relational ("==" relational | "!=" relational)*
pair<PtrTyped,int> parse_equality(const vector<Token>& tokens, int pos){
    return parse_left_joint_binary_operator(tokens, pos, {"==", "!="}, parse_relational);
}

// assign     = equality ("=" assign)?
pair<PtrTyped,int> parse_assign(const vector<Token>& tokens, int start_pos){
    auto [pNode, pos] = parse_equality(tokens, start_pos);
    if (is_punct(tokens, pos, "=")){
        auto [pNode2, pos2] = parse_assign(tokens, pos+1);
        pNode = make_unique<NodeAssign>(tokens.at(pos), move(pNode), move(pNode2));
        pos = pos2;
    }
    return {move(pNode), pos};
}

// expr = assign
pair<PtrTyped,int> parse_expr(const vector<Token>& tokens, int start_pos){
    return parse_assign(tokens, start_pos);
}

pair<PtrNode,int> parse_statement(const vector<Token>& tokens, int pos);

// compound-statement = (declaration | statement)* "}"
pair<PtrNode,int> parse_compound_statement(const vector<Token>& tokens, int pos){
    vector<PtrNode> pNodes;
    while (!is_punct(tokens, pos, "}")){
        bool ok;
        PtrNode pNode;
        tie(ok, pNode, pos) = try_parse_declaration(tokens, pos);
        if(!ok){
            tie(pNode, pos) = parse_statement(tokens, pos);
        }
        pNodes.emplace_back(move(pNode));
    }
    return {make_unique<NodeCompoundStatement>(move(pNodes)), pos+1};
}

// expr_statement_return = "return" expr ";"
pair<PtrNode,int> parse_statement_return(const vector<Token>& tokens, int pos){
    assert(is_keyword(tokens, pos, "return"));
    auto [pNode, pos2] = parse_expr(tokens, pos+1);
    expect_punct(tokens, pos2, ";");
    return {make_unique<NodeRet>(tokens.at(pos), move(pNode)), pos2+1};
}

// expr_statement = expr? ";"
pair<PtrNode,int> parse_expr_statement(const vector<Token>& tokens, int pos){
    if (is_punct(tokens, pos, ";")){
        return {get_null_statement(), pos+1};
    }
    auto [pNode, pos2] = parse_expr(tokens, pos);
    expect_punct(tokens, pos2, ";");
    return {move(pNode), pos2+1};
}

// statement_for = "for" "(" expr_statement expr_statement expr? ")" statement
pair<PtrNode,int> parse_statement_for(const vector<Token>& tokens, int pos){
    assert(is_keyword(tokens, pos, "for"));
    expect_punct(tokens, pos+1, "(");
    auto [expr1, pos1] = parse_expr_statement(tokens, pos+2);
    auto [expr2, pos2] = parse_expr_statement(tokens, pos1);
    PtrNode expr3 = get_null_statement();
    int pos3 = pos2; 
    if (!is_punct(tokens, pos2, ")")) {
        tie(expr3, pos3) = parse_expr(tokens, pos2);
        expect_punct(tokens, pos3, ")");
    }
    auto [statement, pos_end] = parse_statement(tokens, pos3+1);
    return {make_unique<NodeFor>(move(expr1), move(expr2), move(expr3), move(statement)), pos_end};
}

// statement_while = "while" "(" expr ")" statement
pair<PtrNode,int> parse_statement_while(const vector<Token>& tokens, int pos){
    assert(is_keyword(tokens, pos, "while"));
    expect_punct(tokens, pos+1, "(");
    auto [expr, pos_expr] = parse_expr(tokens, pos+2);
    expect_punct(tokens, pos_expr, ")");
    auto [statement, pos_statement] = parse_statement(tokens, pos_expr+1);
    return {make_unique<NodeFor>(get_null_statement(), move(expr), get_null_statement(), move(statement)), pos_statement};
}

// statement_if = "if" "(" expr ")" statement ("else" statement)?
pair<PtrNode,int> parse_statement_if(const vector<Token>& tokens, int pos){
    assert(is_keyword(tokens, pos, "if"));
    expect_punct(tokens, pos+1, "(");
    auto [expr, pos2] = parse_expr(tokens, pos+2);
    expect_punct(tokens, pos2, ")");
    auto [statement_if, pos3] = parse_statement(tokens, pos2+1);
    if (is_keyword(tokens, pos3, "else")){
        auto [statement_else, pos4] = parse_statement(tokens, pos3+1);
        return {make_unique<NodeIf>(move(expr), move(statement_if), move(statement_else)), pos4};
    }
    return {make_unique<NodeIf>(move(expr), move(statement_if), nullptr), pos3};
}

// statement = statement_if | statement_for | statement_while | "{" compound-statement |  "return" expr ";" |  expr_statement |
pair<PtrNode,int> parse_statement(const vector<Token>& tokens, int pos){
    if (is_keyword(tokens, pos, "if")){
        return parse_statement_if(tokens, pos);
    }
    if (is_keyword(tokens, pos, "for")){
        return parse_statement_for(tokens, pos);
    }
    if (is_keyword(tokens, pos, "while")){
        return parse_statement_while(tokens, pos);
    }
    if (is_punct(tokens, pos, "{")){
        return parse_compound_statement(tokens, pos+1);
    }
    if (is_keyword(tokens, pos, "return")){
        return parse_statement_return(tokens, pos);
    }
    return parse_expr_statement(tokens, pos);
}

// program    = "{" compound-statement
pair<PtrNode,int> parse_program(const vector<Token>& tokens, int pos){
    expect_punct(tokens, pos++, "{");
    return parse_compound_statement(tokens, pos);
}


