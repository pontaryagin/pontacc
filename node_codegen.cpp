#include "node.h"

inline const vector<string> call_reg_names = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};

void gen_header(){
    cout << "  .global main\n";
}

static void ass_pop(string_view reg){
    cout << "  pop " << reg << endl;
}

static void ass_push(string_view reg){
    cout << "  push " << reg << endl;
}

static void ass_push(int num){
    cout << "  push $" << num << endl;
}

static void ass_mov(string from, string to){
    cout << "  mov " << from << ", " << to << endl;
}

static void ass_mov(int from, string to){
    cout << "  mov " << "$" << from << ", " << to << endl;
}

static void ass_label(string s){
    cout << s << ":" << endl;
}

static void ass_prologue(int indent_count){
    indent_count = round_up(indent_count, 2); // Some function requires 16-byte alignment for rsp register
    cout << "  push %rbp" << endl;
    cout << "  mov %rsp, %rbp" << endl;
    cout << "  sub $" << indent_count*8 << ", %rsp" << endl;
}

static void ass_epilogue(){
    cout << ".L.return:" << endl;
    cout << "  mov %rbp, %rsp" << endl;
    cout << "  pop %rbp" << endl;
}

static string ass_stack_reg(int offset){
    return to_string(-8*(offset)) + "(%rbp)";
}

static optional<string> ass_stack_reg(const INode& node){
    auto offset = node.get_offset();
    return offset ? make_optional(ass_stack_reg(*offset)) : nullopt;
}

void NodeIf::generate(){
    expr->generate();
    auto count_str = to_string(count);
    cout << "cmp $0" << ", %rax" << endl;
    cout << "je " << ".L.else." << count_str << endl;
    statement_if->generate();
    cout << "jmp " << ".L.end." << count_str << endl;
    ass_label(".L.else." + count_str);
    if (statement_else){
        statement_else->generate();
    }
    ass_label(".L.end." + count_str);
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
    cout << "cmp $0" << ", %rax" << endl;
    cout << "je " << ".L.end." + count_str << endl;
    statement->generate();
    expr_increment->generate();
    cout << "jmp " << ".L.for." + count_str << endl;
    ass_label(".L.end." + count_str);
}

void NodeNum::generate(){
    ass_mov(num, "%rax");
}

void NodeCompoundStatement::generate(){
    for (auto& pNode: pNodes){
        pNode->generate();
    }
}

void NodeVar::generate(){
    ass_mov(*ass_stack_reg(*this), "%rax");
}

void NodeFunc::generate(){
    assert_at(m_nodes.size() < 7, token, "argument size should be less than 7");
    for(auto& node: m_nodes){
        node->generate();
        ass_push("%rax");
    }
    for (int i = ssize(m_nodes)-1; i >= 0 ; --i) {
        ass_pop(call_reg_names[i]);
    }
    cout << "  call " << name << endl;
}

void NodeAddress::generate(){
    auto reg = ass_stack_reg(*var.get());
    cout << "  lea " << *reg << ", %rax" << endl;
}

void NodeDeref::generate(){
    var->generate();
    ass_mov("(%rax)", "%rax");
}

void NodePunct::ass_adjust_address_mul(){
    auto r_ptr = rhs->get_type().ptr;
    auto l_ptr = lhs->get_type().ptr;
    if (r_ptr>0 && l_ptr == 0){
        cout << "  imul $8, %rax" << endl;
    }
    else if (r_ptr==0 && l_ptr>0){
        cout << "  imul $8, %rdi" << endl;
    }
}

void NodePunct::ass_adjust_address_div(){
    auto r_ptr = rhs->get_type().ptr;
    auto l_ptr = lhs->get_type().ptr;
    if (r_ptr > 0 && l_ptr > 0){
        ass_mov("$8", "%rdi");
        cout << "  cqo" << endl;
        cout << "  idiv %rdi" << endl;
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
        cout << "  add %rdi, %rax" << endl;
    }
    else if(type == "-"){
        ass_adjust_address_mul();
        cout << "  sub %rdi, %rax" << endl;
        ass_adjust_address_div();
    }
    else if(type == "*"){
        cout << "  imul %rdi, %rax" << endl;
    }
    else if(type == "/"){
        cout << "  cqo" << endl;
        cout << "  idiv %rdi" << endl;
    }
    else if(type == "=="){
        cout << "  cmp %rdi, %rax" << endl;
        cout << "  sete %al" << endl;
        cout << "  movzb %al, %rax" << endl;
    }
    else if(type == "!="){
        cout << "  cmp %rdi, %rax" << endl;
        cout << "  setne %al" << endl;
        cout << "  movzb %al, %rax" << endl;
    }
    else if(type == "<="){
        cout << "  cmp %rdi, %rax" << endl;
        cout << "  setle %al" << endl;
        cout << "  movzb %al, %rax" << endl;
    }
    else if(type == "<"){
        cout << "  cmp %rdi, %rax" << endl;
        cout << "  setl %al" << endl;
        cout << "  movzb %al, %rax" << endl;
    }
    else if(type == ">="){
        cout << "  cmp %rax, %rdi" << endl;
        cout << "  setle %al" << endl;
        cout << "  movzb %al, %rax" << endl;
    }
    else if(type == ">"){
        cout << "  cmp %rax, %rdi" << endl;
        cout << "  setl %al" << endl;
        cout << "  movzb %al, %rax" << endl;
    }
    else{
        verror_at(token, "Unknown token in generate for NodePunct");
    }
}

void NodeAssign::generate(){
    rhs->generate();
    if (auto ident = dynamic_cast<NodeVar*>(lhs.get())){
        cout << "  mov %rax, " << *ass_stack_reg(*ident) << endl;
    }
    else if (auto ident = dynamic_cast<NodeDeref*>(lhs.get())) {
        ass_push("%rax");
        ident->var->generate();
        ass_pop("%rdi");
        ass_mov("%rdi", "(%rax)");
    }
    else {
        verror_at(token, "Left hand side of assignment should be identifier");
    }
}

void NodeRet::generate(){
    pNode->generate();
    cout << "  jmp .L.return" << endl;
}

void NodeInitializer::generate(){
    if (expr){
        expr->generate();
        ass_mov("%rax", *ass_stack_reg(dynamic_cast<NodeVar&>(*var.get())));
    }
}

void NodeDeclaration::generate(){
    for (auto& pNode: pNodes){
        pNode->generate();
    }
}

void generate_main(const PtrNode& node){
    gen_header();
    cout << "main:\n";
    ass_prologue(Token::indents.size()+1);
    node->generate();
    ass_epilogue();
    cout << "  ret\n";
}

