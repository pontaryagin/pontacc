#include "tokenizer.h"
#include "parser.h"

static int get_variable_offset(Context& context, const Token& token)
{
    if (context.m_idents.contains(token.ident))
    {
        return context.m_idents[token.ident];
    }
    return context.m_idents[token.ident] = context.m_idents_index_max++;
}

pair<PtrTyped,int> parse_left_joint_binary_operator(const vector<Token>& tokens, int start_pos, const set<string>& operators, Context& context, PaserType next_perser){
    auto [pNode, pos] = next_perser(tokens, start_pos, context);
    while (pos < tokens.size()){
        auto token_val = tokens.at(pos).punct;
        if (operators.contains(token_val)){
            auto [pNode2, pos2] = next_perser(tokens, pos+1, context);
            pNode = make_unique<NodePunct>(tokens.at(pos), move(pNode), move(pNode2));
            pos = pos2;
        }
        else {
            break;
        }
    }
    return {move(pNode), pos};
}

pair<PtrTyped,int> parse_expr(const vector<Token>& tokens, int start_pos, Context& context);

// declspec = "int"
tuple<bool, Type, int> try_parse_declspec(const vector<Token>& tokens, int pos) {
    if(is_keyword(tokens, pos, "int")){
        return {true, TypeInt{}, pos+1};
    }
    return {false, Type{}, pos};
}

// param       = declspec declarator
optional<pair<unique_ptr<NodeVar>, int>> try_parse_param(const vector<Token>& tokens, int pos, Context& context)
{
    auto [ok, type, pos1] = try_parse_declspec(tokens, pos);
    if (!ok) {
        return nullopt;
    }
    auto [node, param, pos2] = parse_declarator(tokens, pos1, type, context);
    assert(param.size() == 0);
    return make_pair(move(node), pos2);
}

// type-suffix = func-params? ")"
// func-params = param ("," param)*
pair<vector<unique_ptr<NodeVar>>, int> parse_func_suffix(const vector<Token>& tokens, int pos, Context& context){
    vector<unique_ptr<NodeVar>> params;
    while(auto ret = try_parse_param(tokens, pos, context)){
        params.push_back(move(ret->first));
        pos = ret->second;
        if (!is_punct(tokens, pos, ",")){
            break;
        }
        pos++;
    }
    expect_punct(tokens, pos, ")");
    return make_pair(move(params), pos+1);
}

// type-suffix = "(" func-suffix
//                | "[" num "]"
//                | ε
static
pair<variant<monostate, vector<unique_ptr<NodeVar>>, int>, int> parse_type_suffix(const vector<Token>& tokens, int pos, Context& context)
{
    if (is_punct(tokens, pos, "(")) {
        auto ret = parse_func_suffix(tokens, pos+1, context);
        return make_pair(move(ret.first), ret.second);
    }
    else if (is_punct(tokens, pos, "[")){
        expect_kind(tokens, pos+1, TokenKind::Num);
        auto array_size = tokens.at(pos+1).val;
        expect_punct(tokens, pos+2, "]");
        return make_pair(array_size, pos+3);
    }
    return make_pair(monostate{}, pos);
}

// declarator = "*"* ident type-suffix
tuple<unique_ptr<NodeVar>, vector<unique_ptr<NodeVar>>, int> parse_declarator(const vector<Token>& tokens, int pos, Type type, Context& context) {
    while(is_punct(tokens, pos, "*")){
        ++pos;
        type = Type::to_ptr(move(type));
    }
    expect_kind(tokens, pos, TokenKind::Ident);
    context.m_var_types[tokens.at(pos).ident] = move(type);
    auto var = make_unique<NodeVar>(tokens.at(pos), get_variable_offset(context, tokens.at(pos)), type);
    auto suffix = parse_type_suffix(tokens, pos+1, context);
    if (auto param = get_if<1>(&suffix.first)){
        return make_tuple(move(var), move(*param), suffix.second);
    }

    return {move(var), vector<unique_ptr<NodeVar>>{}, pos+1};
}

// initializer = declarator ("=" expr)?
pair<PtrNode,int> parse_initializer(const vector<Token>& tokens, int pos, Type type, Context& context) {
    auto [pNode, param, pos_decl] = parse_declarator(tokens, pos, type, context);
    assert(param.size() == 0);
    if (is_punct(tokens, pos_decl, "=")){
        auto [expr, pos_expr] = parse_expr(tokens, pos_decl+1, context);
        return {make_unique<NodeInitializer>(tokens.at(pos_decl), move(pNode), move(expr), type), pos_expr};
    }
    return {make_unique<NodeInitializer>(tokens.at(pos), move(pNode), nullptr, type), pos_decl};
}


// declaration = declspec (initializer ("," initializer)*)? ";"
tuple<bool, PtrNode, int> try_parse_declaration(const vector<Token>& tokens, int pos, Context& context) {
    auto [ok, type, pos1] = try_parse_declspec(tokens, pos);
    if (!ok){
        return {false, nullptr, pos1};
    }
    vector<PtrNode> pNodes;
    PtrNode pNode;
    do {
        tie(pNode, pos1) = parse_initializer(tokens, pos1, type, context);
        pNodes.emplace_back(move(pNode));
    } while(is_punct(tokens, pos1++, ","));
    expect_punct(tokens, pos1-1, ";");
    return {true, make_unique<NodeDeclaration>(tokens.at(pos), move(pNodes)), pos1};
}

// func = ident "(" assign? ("," assign)* ")"
pair<PtrTyped, int> parse_func(const vector<Token>& tokens, int pos, Context& context){
    auto& token_ident = tokens.at(pos);
    vector<PtrTyped> args;
    pos += 2;
    PtrTyped expr;
    if (is_punct(tokens, pos, ")")){
        return {make_unique<NodeFunc>(token_ident, move(args)), pos+1};
    }
    tie(expr, pos) = parse_assign(tokens, pos, context);
    args.push_back(move(expr));
    while(is_punct(tokens, pos, ",")){
        tie(expr, pos) = parse_assign(tokens, pos+1, context);
        args.push_back(move(expr));
    }
    expect_punct(tokens, pos, ")");
    return {make_unique<NodeFunc>(token_ident, move(args)), pos+1};
}

//  primary = num | ident args? | "(" expr ")"
pair<PtrTyped,int> parse_primary(const vector<Token>& tokens, int pos, Context& context){
    auto token_val = tokens.at(pos).punct;
    if (is_punct(tokens, pos, "(")){
        PtrTyped pNode;
        tie(pNode, pos) = parse_expr(tokens, pos+1, context);
        expect_punct(tokens, pos, ")");
        return {move(pNode), pos+1};
    }
    else if (is_kind(tokens, pos, TokenKind::Ident)){
        if (is_punct(tokens, pos+1, "(")){
            return parse_func(tokens, pos, context);
        }
        auto& token = tokens.at(pos);
        return {make_unique<NodeVar>(token, get_variable_offset(context, token), context.m_var_types[token.ident]), pos+1};
    }
    return {make_unique<NodeNum>(tokens.at(pos)), pos+1};
}

// unary = ("+" | "-" | "*" | "&") unary | primary
pair<PtrTyped,int> parse_unary(const vector<Token>& tokens, int pos, Context& context){
    if (is_punct(tokens, pos, "+")){
        return parse_unary(tokens, pos+1, context);
    }
    else if (is_punct(tokens, pos, "-")){
        auto [pNode, pos_end] = parse_unary(tokens, pos+1, context);
        return {make_unique<NodePunct>(tokens.at(pos), make_unique<NodeNum>(0), move(pNode)), pos_end};
    }
    else if (is_punct(tokens, pos, "*")){
        auto [pNode, pos_end] = parse_unary(tokens, pos+1, context);
        return {make_unique<NodeDeref>(tokens.at(pos), move(pNode)), pos_end};
    }
    else if (is_punct(tokens, pos, "&")){
        auto [pNode, pos_end] = parse_unary(tokens, pos+1, context);
        return {make_unique<NodeAddress>(tokens.at(pos), move(pNode)), pos_end};
    }
    return parse_primary(tokens, pos, context);
}

//  mul     = unary ("*" unary | "/" unary)*
pair<PtrTyped,int> parse_mul(const vector<Token>& tokens, int pos, Context& context){
    return parse_left_joint_binary_operator(tokens, pos, {"*", "/"}, context, parse_unary);
}

//  add    = mul ("+" mul | "-" mul)*
pair<PtrTyped,int> parse_add(const vector<Token>& tokens, int pos, Context& context){
    return parse_left_joint_binary_operator(tokens, pos, {"+", "-"}, context, parse_mul);
}

//  relational = add ("<" add | "<=" add | ">" add | ">=" add)*
pair<PtrTyped,int> parse_relational(const vector<Token>& tokens, int pos, Context& context){
    return parse_left_joint_binary_operator(tokens, pos, {"<", "<=", ">", ">="}, context, parse_add);
}

//  equality   = relational ("==" relational | "!=" relational)*
pair<PtrTyped,int> parse_equality(const vector<Token>& tokens, int pos, Context& context){
    return parse_left_joint_binary_operator(tokens, pos, {"==", "!="}, context, parse_relational);
}

// assign     = equality ("=" assign)?
pair<PtrTyped,int> parse_assign(const vector<Token>& tokens, int start_pos, Context& context){
    auto [pNode, pos] = parse_equality(tokens, start_pos, context);
    if (is_punct(tokens, pos, "=")){
        auto [pNode2, pos2] = parse_assign(tokens, pos+1, context);
        pNode = make_unique<NodeAssign>(tokens.at(pos), move(pNode), move(pNode2));
        pos = pos2;
    }
    return {move(pNode), pos};
}

// expr = assign
pair<PtrTyped,int> parse_expr(const vector<Token>& tokens, int start_pos, Context& context){
    return parse_assign(tokens, start_pos, context);
}

pair<PtrNode,int> parse_statement(const vector<Token>& tokens, int pos, Context& context);

// compound-statement = (declaration | statement)* "}"
pair<PtrNode,int> parse_compound_statement(const vector<Token>& tokens, int pos, Context& context){
    vector<PtrNode> pNodes;
    auto token_start = tokens.at(pos);
    while (!is_punct(tokens, pos, "}")){
        bool ok;
        PtrNode pNode;
        tie(ok, pNode, pos) = try_parse_declaration(tokens, pos, context);
        if(!ok){
            tie(pNode, pos) = parse_statement(tokens, pos, context);
        }
        pNodes.emplace_back(move(pNode));
    }
    return {make_unique<NodeCompoundStatement>(token_start, move(pNodes)), pos+1};
}

// expr_statement_return = "return" expr ";"
pair<PtrNode,int> parse_statement_return(const vector<Token>& tokens, int pos, Context& context){
    assert(is_keyword(tokens, pos, "return"));
    auto [pNode, pos2] = parse_expr(tokens, pos+1, context);
    expect_punct(tokens, pos2, ";");
    return {make_unique<NodeRet>(tokens.at(pos), move(pNode), context.func_name), pos2+1};
}

// expr_statement = expr? ";"
pair<PtrNode,int> parse_expr_statement(const vector<Token>& tokens, int pos, Context& context){
    if (is_punct(tokens, pos, ";")){
        return {get_null_statement(tokens.at(pos)), pos+1};
    }
    auto [pNode, pos2] = parse_expr(tokens, pos, context);
    expect_punct(tokens, pos2, ";");
    return {move(pNode), pos2+1};
}

// statement_for = "for" "(" expr_statement expr_statement expr? ")" statement
pair<PtrNode,int> parse_statement_for(const vector<Token>& tokens, int pos, Context& context){
    assert(is_keyword(tokens, pos, "for"));
    expect_punct(tokens, pos+1, "(");
    auto [expr1, pos1] = parse_expr_statement(tokens, pos+2, context);
    auto [expr2, pos2] = parse_expr_statement(tokens, pos1, context);
    PtrNode expr3 = get_null_statement(tokens.at(pos2));
    int pos3 = pos2; 
    if (!is_punct(tokens, pos2, ")")) {
        tie(expr3, pos3) = parse_expr(tokens, pos2, context);
        expect_punct(tokens, pos3, ")");
    }
    auto [statement, pos_end] = parse_statement(tokens, pos3+1, context);
    return {make_unique<NodeFor>(tokens.at(pos), move(expr1), move(expr2), move(expr3), move(statement)), pos_end};
}

// statement_while = "while" "(" expr ")" statement
pair<PtrNode,int> parse_statement_while(const vector<Token>& tokens, int pos, Context& context){
    assert(is_keyword(tokens, pos, "while"));
    expect_punct(tokens, pos+1, "(");
    auto [expr, pos_expr] = parse_expr(tokens, pos+2, context);
    expect_punct(tokens, pos_expr, ")");
    auto [statement, pos_statement] = parse_statement(tokens, pos_expr+1, context);
    return {make_unique<NodeFor>(tokens.at(pos), get_null_statement(tokens.at(pos)), move(expr), get_null_statement(tokens.at(pos)), move(statement)), pos_statement};
}

// statement_if = "if" "(" expr ")" statement ("else" statement)?
pair<PtrNode,int> parse_statement_if(const vector<Token>& tokens, int pos, Context& context){
    assert(is_keyword(tokens, pos, "if"));
    expect_punct(tokens, pos+1, "(");
    auto [expr, pos2] = parse_expr(tokens, pos+2, context);
    expect_punct(tokens, pos2, ")");
    auto [statement_if, pos3] = parse_statement(tokens, pos2+1, context);
    if (is_keyword(tokens, pos3, "else")){
        auto [statement_else, pos4] = parse_statement(tokens, pos3+1, context);
        return {make_unique<NodeIf>(tokens.at(pos), move(expr), move(statement_if), move(statement_else)), pos4};
    }
    return {make_unique<NodeIf>(tokens.at(pos), move(expr), move(statement_if), nullptr), pos3};
}

// statement = statement_if | statement_for | statement_while | "{" compound-statement |  "return" expr ";" |  expr_statement |
pair<PtrNode,int> parse_statement(const vector<Token>& tokens, int pos, Context& context){
    if (is_keyword(tokens, pos, "if")){
        return parse_statement_if(tokens, pos, context);
    }
    if (is_keyword(tokens, pos, "for")){
        return parse_statement_for(tokens, pos, context);
    }
    if (is_keyword(tokens, pos, "while")){
        return parse_statement_while(tokens, pos, context);
    }
    if (is_punct(tokens, pos, "{")){
        return parse_compound_statement(tokens, pos+1, context);
    }
    if (is_keyword(tokens, pos, "return")){
        return parse_statement_return(tokens, pos, context);
    }
    return parse_expr_statement(tokens, pos, context);
}

// func_def = declspec declarator "{" compound-statement
pair<PtrNode,int> parse_func_def(const vector<Token>& tokens, int pos){
    map<string, Type> type_map; // type map for variables
    Context context{"", type_map};
    auto [_, type, pos_decls] = try_parse_declspec(tokens, pos);
    auto [declr, param, pos_declr] = parse_declarator(tokens, pos_decls, type, context);
    context.func_name = declr->name;
    expect_punct(tokens, pos_declr, "{");
    auto [state, pos_state] = parse_compound_statement(tokens, pos_declr+1, context);
    return {make_unique<NodeFuncDef>(tokens.at(pos), declr->name, move(state), declr->get_type(), context.m_idents_index_max, move(param)), pos_state};
}

// program = func_def*
pair<PtrNode,int> parse_program(const vector<Token>& tokens, int pos){
    vector<PtrNode> nodes;
    PtrNode node;
    while(pos < tokens.size()){
        tie(node, pos)  = parse_func_def(tokens, pos);
        nodes.push_back(move(node));
    }
    return {make_unique<NodeProgram>(move(nodes)), pos};
}



