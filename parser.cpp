#include "tokenizer.h"
#include "parser.h"

static string get_global_string_id(){
    static int num = 0;
    return ".L.."s + to_string(num++);
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

// declspec = "int" | "char"
PosRet<optional<Type>> try_parse_declspec(const vector<Token>& tokens, int pos) {
    if(is_keyword(tokens, pos, "int")){
        return {TypeInt{}, pos+1};
    }
    else if (is_keyword(tokens, pos, "char")){
        return {TypeChar{}, pos+1};
    }
    return {nullopt, pos};
}

// param       = declspec declarator
optional<PosRet<PNodeVar>> try_parse_param(const vector<Token>& tokens, int pos, Context& context){
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
PosRet<vector<PNodeVar>> parse_func_suffix(const vector<Token>& tokens, int pos, Context& context){
    vector<PNodeVar> params;
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
PosRet<optional<vector<PNodeVar>>> 
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
PosRet<PNodeVar, vector<PNodeVar>>
parse_declarator(const vector<Token>& tokens, int pos, Type type, bool is_global, Context& context){
    while(is_punct(tokens, pos, "*")){
        ++pos;
        type = to_ptr(move(type));
    }
    expect_kind(tokens, pos, TokenKind::Ident);
    auto var_name = tokens.at(pos);
    pos++;
    auto [suffix, pos1] = parse_type_suffix(tokens, pos, context, type);
    auto var = make_shared<NodeVar>(var_name, type, is_global);
    context.set_variable(is_global, var);
    if (suffix){
        return make_tuple(move(var), move(*suffix), pos1);
    }
    return {move(var), vector<PNodeVar>{}, pos1};
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


//  primary =  num | string | ident | func | "sizeof" expr | "(" expr ")" |  "(" compound-statement ")"
PosRet<PITyped> 
parse_primary(const vector<Token>& tokens, int pos, Context& context){
    auto& token = tokens.at(pos);
    auto token_val = token.punct;
    if (is_punct(tokens, pos, "(") && is_punct(tokens, pos+1, "{")){
        Context sub_context(&context);
        auto [node,pos1] = parse_compound_statement(tokens, pos+2, sub_context);
        auto expr = make_unique<NodeExpressVar>(tokens.at(pos), move(node));
        expect_punct(tokens, pos1, ")");
        return {move(expr), pos1+1};
    }
    else if (is_punct(tokens, pos, "(")){
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
        auto is_global = context.variable(token.ident, true) != nullptr;
        assert_at(context.variable(token.ident) != nullptr, token, "unknown variable");
        auto tmp_var = context.variable(token.ident, is_global);
        PITyped var;
        if(tmp_var){
            auto var_ = make_shared<NodeVar>(NodeVar(*tmp_var));
            var_->m_base = tmp_var;
            context.add_locals(var_);
            var = move(var_); 
        }
        else {
            auto var_ = make_shared<NodeVar>(token, Type{}, is_global);
            context.set_variable(is_global, var_);
            var = move(var_);
        }
        return {move(var), pos+1};
    }
    else if(is_kind(tokens, pos, TokenKind::String)) {
        auto name = get_global_string_id();
        auto type = TypeArray{make_shared<Type>(TypeChar{}), static_cast<int>(token.text->size())+1};
        context.string_literal(name, token.text);
        auto var = make_shared<NodeVar>(token, type, name, true);
        context.set_variable(true, var);
        return {move(var), pos+1};
    }
    else if(is_kind(tokens, pos, TokenKind::Num)) {
        return {make_unique<NodeNum>(token), pos+1};
    }
    assert_at(false, token, "unknown token"); abort();
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
PosRet<unique_ptr<NodeCompoundStatement>> parse_compound_statement(const vector<Token>& tokens, int pos, Context& context){
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
    return {make_unique<NodeRet>(tokens.at(pos), move(pNode), context.func_name()), pos2+1};
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
        Context sub_context(&context);
        return parse_compound_statement(tokens, pos+1, sub_context);
    }
    if (is_keyword(tokens, pos, "return")){
        return parse_statement_return(tokens, pos, context);
    }
    return parse_expr_statement(tokens, pos, context);
}

// func_def = "{" compound-statement
PosRet<PINode> 
parse_func_def(const vector<Token>& tokens, int pos, const NodeVar& node, 
        std::vector<PNodeVar> &&param, Context& context){
    expect_punct(tokens, pos, "{");
    auto [state, pos_state] = parse_compound_statement(tokens, pos+1, context);
    // assign stack offset
    int offset = 0;
    for(PNodeVar local : context.locals()){
        auto this_size = size_of(local->get_type());
        if (local->m_base){
            local->offset = local->m_base->offset;
        }
        else {
            local->offset = offset;
            offset += this_size;
        }
    }
    for(PNodeVar local : context.locals()){
        local->offset = offset-local->offset;
    }
    offset = round_up(offset, 16);
    return {make_unique<NodeFuncDef>(tokens.at(pos), node.name, move(state), node.get_type(), 
            move(param), offset), pos_state};
}


// program = (declspec declarator func_def | global-variable)*
// global-veriable = declspec ( declarator ("," declarator) * ) ";"
PosRet<PINode> parse_program(const vector<Token>& tokens, int pos){
    Context context_main{};
    vector<PINode> nodes;
    PINode node;
    while(pos < tokens.size()){
        Context context(&context_main);
        context.reset_locals();
        auto [type, pos1] = try_parse_declspec(tokens, pos);
        auto [node_, param, pos2] = parse_declarator(tokens, pos1, *type, true, context);
        type = node_->get_type();
        context.set_func_name(node_->name);
        if (is_type_of<TypeFunc>(*type)){
            tie(node, pos) = parse_func_def(tokens, pos2, *node_.get(), move(param), context);
            nodes.push_back(move(node));
        }
        else {
            pos = pos2;
            nodes.push_back(node_);
            while (is_punct(tokens, pos, ",")){
                auto [node_, param, pos2] = parse_declarator(tokens, pos+1, *type, true, context);
                pos = pos2;
                nodes.push_back(node_);
            }
            expect_punct(tokens, pos, ";");
            pos++;
        }
    }
    return {make_unique<NodeProgram>(move(nodes), context_main.string_literal()), pos};
}

