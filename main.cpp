#include "common.h"
#include "parser.h"
#include "tokenizer.h"

int main(int argc, char **argv){
    string program = argc == 2 ? argv[1]: read_all_lines();
    auto tokens = tokenize(program.c_str());
    auto [node, pos] = parse_program(tokens, 0);
    if (pos != tokens.size()) {
        verror_at(tokens.at(pos), "Not parsed");
    }
    generate_main(node);
}
