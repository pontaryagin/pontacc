#include "common.h"
#include "parser.h"
#include "tokenizer.h"

int main(int argc, char **argv){
    if(argc!=2)
        throw invalid_argument("no input file specified");
    string program = argv[1] == "-"s ? read_all_lines(): read_file(argv[1]);

    auto tokens = tokenize(program.c_str());
    auto [node, pos] = parse_program(tokens, 0);
    if (pos != tokens.size()) {
        verror_at(tokens.at(pos), "Not parsed");
    }
    generate_main(node);
}
