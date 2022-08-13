#include "tokenizer.h"
#include "parser.h"

static int get_variable_offset(Context& context, const Token& token)
{
    const Type type = context.m_var_types[token.ident];
    if (context.m_idents.contains(token.ident))
    {
        return context.m_idents[token.ident];
    }
    auto size = visit([](auto&& t){return t->size_of(); }, type);
    context.m_idents_index_max += size;
    return context.m_idents[token.ident] = context.m_idents_index_max;
}

PosRet<PITyped> parse_left_joint_binary_operator(const vector<Token>& tokens, int start_pos, const set<string>& operators, Context& context, PaserType next_perser){
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

PosRet<PITyped> parse_expr(const vector<Token>& tokens, int start_pos, Context& context);

// declspec = "int"
PosRet<optional<Type>> try_parse_declspec(const vector<Token>& tokens, int pos) {
    if(is_keyword(tokens, pos, "int")){
        return {TypeInt{}, pos+1};
    }
    return {nullopt, pos};
}

// param       = declspec declarator
optional<PosRet<unique_ptr<NodeVar>>> try_parse_param(const vector<Token>& tokens, int pos, Context& context){
    auto [type, pos1] = try_parse_declspec(tokens, pos);
    if (!type) {
        return nullopt;
    }
    auto [node, param, pos2] = parse_declarator(tokens, pos1, *type, false, context);
    assert(param.size() == 0);
    return make_pair(move(node), pos2);
}

// type-suffix = func-params? ")"
// func-params = param ("," param)*
PosRet<vector<unique_ptr<NodeVar>>> parse_func_suffix(const vector<Token>& tokens, int pos, Context& context){
    vector<unique_ptr<NodeVar>> params;
    while(auto ret = try_parse_param(tokens, pos, context)){
        auto&& [param, pos1] = *ret;
        pos = pos1;
        params.push_back(move(param));
        if (!is_punct(tokens, pos, ",")){
            break;
        }
        pos++;
    }
    expect_punct(tokens, pos, ")");
    return make_pair(move(params), pos+1);
}

// type-suffix = "(" func-suffix
//                | "[" num "]" (type-suffix)?
//                | ε
static
PosRet<optional<vector<unique_ptr<NodeVar>>>> 
parse_type_suffix(const vector<Token>& tokens, int pos, Context& context, Type& type)
{
    if (is_punct(tokens, pos, "(")) {
        auto [ret, pos2] = parse_func_suffix(tokens, pos+1, context);
        {
            // create func type
            auto t = TypeFunc{};
            t.m_ret = make_shared<Type>(type);
            for(auto& v: ret){
                t.m_params.emplace_back(make_shared<Type>(v->get_type()));
            }
            type = t;
        }
        return make_pair(move(ret), pos2);
    }
    else if (is_punct(tokens, pos, "[")){
        expect_kind(tokens, pos+1, TokenKind::Num);
        auto array_size = tokens.at(pos+1).val;
        expect_punct(tokens, pos+2, "]");
        auto [rest, pos1] = parse_type_suffix(tokens, pos+3, context, type);
        type = TypeArray{make_shared<Type>(type), array_size};
        return make_pair(nullopt, pos1);
    }
    return make_pair(nullopt, pos);
}

// declarator = "*"* ident type-suffix
PosRet<unique_ptr<NodeVar>, vector<unique_ptr<NodeVar>>>
parse_declarator(const vector<Token>& tokens, int pos, Type type, bool is_global, Context& context){
    while(is_punct(tokens, pos, "*")){
        ++pos;
        type = to_ptr(move(type));
    }
    expect_kind(tokens, pos, TokenKind::Ident);
    auto var_name = tokens.at(pos);
    pos++;
    auto [suffix, pos1] = parse_type_suffix(tokens, pos, context, type);
    (is_global ? context.m_var_types_global[var_name.ident]: context.m_var_types[var_name.ident]) = type;
    auto var = make_unique<NodeVar>(var_name, get_variable_offset(context, var_name), type, is_global);
    if (suffix){
        return make_tuple(move(var), move(*suffix), pos1);
    }
    return {move(var), vector<unique_ptr<NodeVar>>{}, pos1};
}

// initializer = declarator ("=" expr)?
PosRet<PINode> parse_initializer(const vector<Token>& tokens, int pos, Type type, Context& context) {
    auto [pNode, param, pos_decl] = parse_declarator(tokens, pos, type, false, context);
    assert(param.size() == 0);
    if (is_punct(tokens, pos_decl, "=")){
        auto [expr, pos_expr] = parse_expr(tokens, pos_decl+1, context);
        return {make_unique<NodeInitializer>(tokens.at(pos_decl), move(pNode), move(expr), type), pos_expr};
    }
    return {make_unique<NodeInitializer>(tokens.at(pos), move(pNode), nullptr, type), pos_decl};
}


// declaration = declspec (initializer ("," initializer)*)? ";"
PosRet<PINode> 
try_parse_declaration(const vector<Token>& tokens, int pos, Context& context) {
    auto [type, pos1] = try_parse_declspec(tokens, pos);
    if (!type){
        return {nullptr, pos1};
    }
    vector<PINode> pNodes;
    PINode pNode;
    do {
        tie(pNode, pos1) = parse_initializer(tokens, pos1, *type, context);
        pNodes.emplace_back(move(pNode));
    } while(is_punct(tokens, pos1++, ","));
    expect_punct(tokens, pos1-1, ";");
    return {make_unique<NodeDeclaration>(tokens.at(pos), move(pNodes)), pos1};
}

// func = ident "(" assign? ("," assign)* ")"
PosRet<PITyped> parse_func(const vector<Token>& tokens, int pos, Context& context){
    auto& token_ident = tokens.at(pos);
    vector<PITyped> args;
    pos += 2;
    PITyped expr;
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

//  primary = num | ident | func | "sizeof" expr | "(" expr ")"
PosRet<PITyped> 
parse_primary(const vector<Token>& tokens, int pos, Context& context){
    auto token_val = tokens.at(pos).punct;
    if (is_punct(tokens, pos, "(")){
        PITyped pNode;
        tie(pNode, pos) = parse_expr(tokens, pos+1, context);
        expect_punct(tokens, pos, ")");
        return {move(pNode), pos+1};
    }
    else if (is_keyword(tokens, pos, "sizeof")) {
        auto [node, pos1] = parse_unary(tokens, pos+1, context);
        auto num = make_unique<NodeNum>(size_of(node->get_type()));
        return {move(num), pos1};
    }
    else if (is_kind(tokens, pos, TokenKind::Ident)){
        if (is_punct(tokens, pos+1, "(")){
            return parse_func(tokens, pos, context);
        }
        auto& token = tokens.at(pos);
        auto is_global = context.m_var_types_global.contains(token.ident);
        return {make_unique<NodeVar>(token, get_variable_offset(context, token), 
            is_global ? context.m_var_types_global.at(token.ident): context.m_var_types.at(token.ident), is_global), pos+1};
    }
    return {make_unique<NodeNum>(tokens.at(pos)), pos+1};
}

// postfix = primary ("[" expr "]")*
PosRet<PITyped> parse_postfix(const vector<Token>& tokens, int pos_, Context& context)
{
    auto [node, pos] = parse_primary(tokens, pos_, context);
    while (is_punct(tokens, pos, "[")) {
        auto [expr, pos2] = parse_expr(tokens, pos+1, context);
        auto token_punct = tokens.at(pos);
        token_punct.from_punct("+");
        node = make_unique<NodePunct>(token_punct, move(node), move(expr));
        node = make_unique<NodeDeref>(tokens.at(pos), move(node));
        expect_punct(tokens, pos2, "]");
        pos = pos2+1;
    }
    return {move(node), pos};
}

// unary = ("+" | "-" | "*" | "&") unary | postfix
PosRet<PITyped> parse_unary(const vector<Token>& tokens, int pos, Context& context){
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
    return parse_postfix(tokens, pos, context);
}

//  mul     = unary ("*" unary | "/" unary)*
PosRet<PITyped> parse_mul(const vector<Token>& tokens, int pos, Context& context){
    return parse_left_joint_binary_operator(tokens, pos, {"*", "/"}, context, parse_unary);
}

//  add    = mul ("+" mul | "-" mul)*
PosRet<PITyped> parse_add(const vector<Token>& tokens, int pos, Context& context){
    return parse_left_joint_binary_operator(tokens, pos, {"+", "-"}, context, parse_mul);
}

//  relational = add ("<" add | "<=" add | ">" add | ">=" add)*
PosRet<PITyped> parse_relational(const vector<Token>& tokens, int pos, Context& context){
    return parse_left_joint_binary_operator(tokens, pos, {"<", "<=", ">", ">="}, context, parse_add);
}

//  equality   = relational ("==" relational | "!=" relational)*
PosRet<PITyped> parse_equality(const vector<Token>& tokens, int pos, Context& context){
    return parse_left_joint_binary_operator(tokens, pos, {"==", "!="}, context, parse_relational);
}

// assign     = equality ("=" assign)?
PosRet<PITyped> parse_assign(const vector<Token>& tokens, int start_pos, Context& context){
    auto [pNode, pos] = parse_equality(tokens, start_pos, context);
    if (is_punct(tokens, pos, "=")){
        auto [pNode2, pos2] = parse_assign(tokens, pos+1, context);
        pNode = make_unique<NodeAssign>(tokens.at(pos), move(pNode), move(pNode2));
        pos = pos2;
    }
    return {move(pNode), pos};
}

// expr = assign
PosRet<PITyped> parse_expr(const vector<Token>& tokens, int start_pos, Context& context){
    return parse_assign(tokens, start_pos, context);
}

PosRet<PINode> parse_statement(const vector<Token>& tokens, int pos, Context& context);

// compound-statement = (declaration | statement)* "}"
PosRet<PINode> parse_compound_statement(const vector<Token>& tokens, int pos, Context& context){
    vector<PINode> pNodes;
    auto token_start = tokens.at(pos);
    while (!is_punct(tokens, pos, "}")){
        PINode pNode;
        tie(pNode, pos) = try_parse_declaration(tokens, pos, context);
        if(!pNode){
            tie(pNode, pos) = parse_statement(tokens, pos, context);
        }
        pNodes.emplace_back(move(pNode));
    }
    return {make_unique<NodeCompoundStatement>(token_start, move(pNodes)), pos+1};
}

// expr_statement_return = "return" expr ";"
PosRet<PINode> parse_statement_return(const vector<Token>& tokens, int pos, Context& context){
    assert(is_keyword(tokens, pos, "return"));
    auto [pNode, pos2] = parse_expr(tokens, pos+1, context);
    expect_punct(tokens, pos2, ";");
    return {make_unique<NodeRet>(tokens.at(pos), move(pNode), context.func_name), pos2+1};
}

// expr_statement = expr? ";"
PosRet<PINode> parse_expr_statement(const vector<Token>& tokens, int pos, Context& context){
    if (is_punct(tokens, pos, ";")){
        return {get_null_statement(tokens.at(pos)), pos+1};
    }
    auto [pNode, pos2] = parse_expr(tokens, pos, context);
    expect_punct(tokens, pos2, ";");
    return {move(pNode), pos2+1};
}

// statement_for = "for" "(" expr_statement expr_statement expr? ")" statement
PosRet<PINode> parse_statement_for(const vector<Token>& tokens, int pos, Context& context){
    assert(is_keyword(tokens, pos, "for"));
    expect_punct(tokens, pos+1, "(");
    auto [expr1, pos1] = parse_expr_statement(tokens, pos+2, context);
    auto [expr2, pos2] = parse_expr_statement(tokens, pos1, context);
    PINode expr3 = get_null_statement(tokens.at(pos2));
    int pos3 = pos2; 
    if (!is_punct(tokens, pos2, ")")) {
        tie(expr3, pos3) = parse_expr(tokens, pos2, context);
        expect_punct(tokens, pos3, ")");
    }
    auto [statement, pos_end] = parse_statement(tokens, pos3+1, context);
    return {make_unique<NodeFor>(tokens.at(pos), move(expr1), move(expr2), move(expr3), move(statement)), pos_end};
}

// statement_while = "while" "(" expr ")" statement
PosRet<PINode> parse_statement_while(const vector<Token>& tokens, int pos, Context& context){
    assert(is_keyword(tokens, pos, "while"));
    expect_punct(tokens, pos+1, "(");
    auto [expr, pos_expr] = parse_expr(tokens, pos+2, context);
    expect_punct(tokens, pos_expr, ")");
    auto [statement, pos_statement] = parse_statement(tokens, pos_expr+1, context);
    return {make_unique<NodeFor>(tokens.at(pos), get_null_statement(tokens.at(pos)), move(expr), get_null_statement(tokens.at(pos)), move(statement)), pos_statement};
}

// statement_if = "if" "(" expr ")" statement ("else" statement)?
PosRet<PINode> parse_statement_if(const vector<Token>& tokens, int pos, Context& context){
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
PosRet<PINode> parse_statement(const vector<Token>& tokens, int pos, Context& context){
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

// func_def = "{" compound-statement
PosRet<PINode> 
parse_func_def(const vector<Token>& tokens, int pos, const NodeVar& node, 
        std::vector<std::unique_ptr<NodeVar>> &&param, Context& context){
    expect_punct(tokens, pos, "{");
    auto [state, pos_state] = parse_compound_statement(tokens, pos+1, context);
    return {make_unique<NodeFuncDef>(tokens.at(pos), node.name, move(state), node.get_type(), 
            context.m_idents_index_max, move(param)), pos_state};
}


// program = (declspec declarator func_def | global-variable)*
// global-veriable = declspec ( declarator ("," declarator) * ) ";"
PosRet<PINode> parse_program(const vector<Token>& tokens, int pos){
    Context context{};
    vector<PINode> nodes;
    PINode node;
    while(pos < tokens.size()){
        auto [type, pos1] = try_parse_declspec(tokens, pos);
        auto [node_, param, pos2] = parse_declarator(tokens, pos1, *type, true, context);
        type = node_->get_type();
        context.func_name = node_->name;
        if (is_type_of<TypeFunc>(*type)){
            tie(node, pos) = parse_func_def(tokens, pos2, *node_.get(), move(param), context);
            nodes.push_back(move(node));
        }
        else {
            pos = pos2;
            nodes.push_back(PINode(node_.release()));
            while (is_punct(tokens, pos, ",")){
                auto [node_, param, pos2] = parse_declarator(tokens, pos+1, *type, true, context);
                pos = pos2;
                nodes.push_back(PINode(node_.release()));
            }
            expect_punct(tokens, pos, ";");
            pos++;
        }
    }
    return {make_unique<NodeProgram>(move(nodes)), pos};
}

