#include "node.h"

extern unique_ptr<ostream> p_ostr;

inline const vector<string> call_reg_names_8 = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
inline const vector<string> call_reg_names_1 = {"%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"};

void gen_header(string_view name){
    ostr() << "  .global "<< name << endl;
    ostr() << "  .text" << endl;
}

static void ass_pop(string_view reg){
    ostr() << "  pop " << reg << endl;
}

static void ass_push(string_view reg){
    ostr() << "  push " << reg << endl;
}

static void ass_push(int num){
    ostr() << "  push $" << num << endl;
}

static void ass_mov_1_8(string from, string to){
    ostr() << "  movsbq " << from << ", " << to << endl;
}
static void ass_mov(string from, string to){
    ostr() << "  mov " << from << ", " << to << endl;
}

static void ass_mov_1_8(int from, string to){
    ostr() << "  movsbq " << "$" << from << ", " << to << endl;
}
static void ass_mov(int from, string to){
    ostr() << "  mov " << "$" << from << ", " << to << endl;
}

static void ass_label(string s){
    ostr() << s << ":" << endl;
}

static void ass_prologue(int indent_count){
    indent_count = round_up(indent_count, 2); // Some function requires 16-byte alignment for rsp register
    ostr() << "  push %rbp" << endl;
    ostr() << "  mov %rsp, %rbp" << endl;
    ostr() << "  sub $" << indent_count*8 << ", %rsp" << endl;
}

static void ass_epilogue(const string& name){
    ostr() << ".L.return." << name << ":" << endl;
    ostr() << "  mov %rbp, %rsp" << endl;
    ostr() << "  pop %rbp" << endl;
}

string NodeVar::ass_stack_reg() const{
    auto& node = *this;
    if (node.m_is_global){
        return node.name + "(%rip)";
    }
    auto offset = node.get_offset();
    return to_string(-(*offset)) + "(%rbp)";
}

void NodeIf::generate(){
    expr->generate();
    auto count_str = to_string(count);
    ostr() << "cmp $0" << ", %rax" << endl;
    ostr() << "je " << ".L.else." << count_str << endl;
    statement_if->generate();
    ostr() << "jmp " << ".L.endif." << count_str << endl;
    ass_label(".L.else." + count_str);
    if (statement_else){
        statement_else->generate();
    }
    ass_label(".L.endif." + count_str);
}

void NodeFor::generate(){
    auto count_str = to_string(count);
    expr_init->generate();
    ass_label(".L.for." + count_str);
    if (dynamic_cast<NodeNull*>(expr_condition.get())){
        ass_mov(1, "%rax");
    }
    else {
        expr_condition->generate();
    }
    ostr() << "cmp $0" << ", %rax" << endl;
    ostr() << "je " << ".L.endfor." + count_str << endl;
    statement->generate();
    expr_increment->generate();
    ostr() << "jmp " << ".L.for." + count_str << endl;
    ass_label(".L.endfor." + count_str);
}

void NodeNum::generate(){
    ass_mov(num, "%rax");
}

void NodeCompoundStatement::generate(){
    for (auto& pNode: pNodes){
        pNode->generate();
    }
}

void emit_data(const string& name, const Type& t){
    ostr() << "  .data" << endl;
    ostr() << "  .global " << name << endl;
    ostr() << name << ":" << endl;
    ostr() << "  .zero " << visit([](auto&& t){return t->size_of();}, t)<<endl;
}

void emit_text_data(const string& name, const string& text){
    ostr() << "  .data" << endl;
    ostr() << "  .global " << name << endl;
    ostr() << name << ":" << endl;
    ostr() << "  .string \"" << text << "\"" << endl;
}

void NodeVar::generate(){
    if (is_type_of<TypeArray>(m_type)){
        generate_address();
        return;
    }
    if (size_of(m_type) == 1){
        ass_mov_1_8(ass_stack_reg(), "%rax");
    }
    else {
        ass_mov(ass_stack_reg(), "%rax");
    }
}

void NodeVar::generate_address() const{
    ostr() << "  lea " << ass_stack_reg() << ", %rax" << endl;
}

ITyped& NodeExpressVar::get_last_expr(){
    auto& nodes = m_statement->pNodes;
    assert(nodes.size());
    assert_at(nodes.size(), m_token, "expression statement should not be empty");
    auto& last = nodes.back();
    auto var = dynamic_cast<ITyped*>(last.get());
    assert_at(var != nullptr, m_token, "the last expression in expression statement should be typed");
    return *var;
}

void NodeExpressVar::generate(){
    m_statement->generate();
}

void NodeFunc::generate(){
    assert_at(m_nodes.size() < 7, token, "argument size should be less than 7");
    for(auto& node: m_nodes){
        node->generate();
        ass_push("%rax");
    }
    for (int i = ssize(m_nodes)-1; i >= 0 ; --i) {
        if (size_of(m_nodes[i]->get_type()) == 1){
            ass_pop(call_reg_names_8[i]);
        }
        else {
            ass_pop(call_reg_names_8[i]);
        }
    }
    ostr() << "  call " << name << endl;
}

void NodeAddress::generate(){
    var->generate_address();
}

void NodeDeref::generate(){
    generate_address();
    auto type = get_type();
    if (is_type_of<TypeArray>(type)){
        return;
    }
    if (size_of(get_type()) == 1) {
        ass_mov_1_8("(%rax)", "%rax");
    }
    else {
        ass_mov("(%rax)", "%rax");
    }
}

void NodeDeref::generate_address() const{
    var->generate();
}


void NodePunct::ass_adjust_address_mul(){
    auto r = rhs->get_type();
    auto l = lhs->get_type();
    if (is_pointer_like(r) && from_box<TypeInt>(l)){
        auto size = size_of_base(r);
        ostr() << "  imul $"<< size << ", %rax" << endl;
    }
    else if (from_box<TypeInt>(r) && is_pointer_like(l)){
        auto size = size_of_base(l);
        ostr() << "  imul $"<< size << ", %rdi" << endl;
    }
}

void NodePunct::ass_adjust_address_div(){
    if (is_ptr(rhs->get_type()) && is_ptr(lhs->get_type())){
        ass_mov("$8", "%rdi");
        ostr() << "  cqo" << endl;
        ostr() << "  idiv %rdi" << endl;
    }
}

void NodePunct::generate(){
    lhs->generate();
    ass_push("%rax");
    rhs->generate();
    ass_mov("%rax", "%rdi");
    ass_pop("%rax");

    auto& type = token.punct;
    if(type == "+"){
        ass_adjust_address_mul();
        ostr() << "  add %rdi, %rax" << endl;
    }
    else if(type == "-"){
        ass_adjust_address_mul();
        ostr() << "  sub %rdi, %rax" << endl;
        ass_adjust_address_div();
    }
    else if(type == "*"){
        ostr() << "  imul %rdi, %rax" << endl;
    }
    else if(type == "/"){
        ostr() << "  cqo" << endl;
        ostr() << "  idiv %rdi" << endl;
    }
    else if(type == "=="){
        ostr() << "  cmp %rdi, %rax" << endl;
        ostr() << "  sete %al" << endl;
        ostr() << "  movzb %al, %rax" << endl;
    }
    else if(type == "!="){
        ostr() << "  cmp %rdi, %rax" << endl;
        ostr() << "  setne %al" << endl;
        ostr() << "  movzb %al, %rax" << endl;
    }
    else if(type == "<="){
        ostr() << "  cmp %rdi, %rax" << endl;
        ostr() << "  setle %al" << endl;
        ostr() << "  movzb %al, %rax" << endl;
    }
    else if(type == "<"){
        ostr() << "  cmp %rdi, %rax" << endl;
        ostr() << "  setl %al" << endl;
        ostr() << "  movzb %al, %rax" << endl;
    }
    else if(type == ">="){
        ostr() << "  cmp %rax, %rdi" << endl;
        ostr() << "  setle %al" << endl;
        ostr() << "  movzb %al, %rax" << endl;
    }
    else if(type == ">"){
        ostr() << "  cmp %rax, %rdi" << endl;
        ostr() << "  setl %al" << endl;
        ostr() << "  movzb %al, %rax" << endl;
    }
    else{
        verror_at(token, "Unknown token in generate for NodePunct");
    }
}

void NodeAssign::generate(){
    rhs->generate();
    ass_push("%rax");
    lhs->generate_address();
    ass_pop("%rdi");
    if (size_of(lhs->get_type()) == 1){
        ass_mov("%dil", "(%rax)");
        ass_mov_1_8("(%rax)", "%rax");
    }
    else {
        ass_mov("%rdi", "(%rax)");
        ass_mov("(%rax)", "%rax");
    }
}

void NodeRet::generate(){
    pNode->generate();
    ostr() << "  jmp .L.return." << m_func_name << endl;
}

void NodeInitializer::generate(){
    if (expr){
        expr->generate();
        if (size_of(var->get_type()) == 1){
            ass_mov("%al", var->ass_stack_reg());
        }
        else {
            ass_mov("%rax", var->ass_stack_reg());
        }
    }
}

void NodeDeclaration::generate(){
    for (auto& pNode: pNodes){
        pNode->generate();
    }
}

void NodeFuncDef::generate(){
    gen_header(m_name);
    ostr() << m_name << ":\n";
    ass_prologue(m_local_variable_num);
    // load parameters from register
    for(int i = 0; i < m_param.size(); ++i)
    {
        if (size_of(m_param[i]->get_type())==1){
            ass_mov(call_reg_names_1[i], m_param[i]->ass_stack_reg());
        }
        else{
            ass_mov(call_reg_names_8[i], m_param[i]->ass_stack_reg());
        }
    }
    m_statement->generate();
    ass_epilogue(m_name);
    ostr() << "  ret\n";
    
}

void NodeProgram::generate(){
    // emit data for text segment
    for (const auto& [name, text] : m_string_literals){
        emit_text_data(name, *text);
    }
    // emit global var
    for (auto& pNode: pNodes){
        if (auto pNode_ = dynamic_cast<NodeVar*>(pNode.get())){
            emit_data(pNode_->name, pNode_->get_type());
        }
    }
    for (auto& pNode: pNodes){
        if (dynamic_cast<NodeFuncDef*>(pNode.get())){
            pNode->generate();
        }
    }
}

void generate_main(const PINode& node){
    node->generate();
}

