#include "common.h"
#include "parser.h"
#include "tokenizer.h"

unique_ptr<ofstream> p_ostr;

static string read_all_lines(){
    string text;
    string line;
    std::stringstream buffer;
    buffer << cin.rdbuf();
    return buffer.str();
}

static string read_file(const string& file_name){
    string text;
    ifstream ifs(file_name);
    if (!ifs){
        throw invalid_argument("input file cannot be opened");
    }
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

static string read_input(const string& file_name){
    return file_name == "-" ? read_all_lines() : read_file(file_name);
}

static void show_usage(int status){
    cerr << "./pontacc [ -o output file name ] <input file name>" << endl;
    exit(status);
}

int main(int argc, char **argv){
    optional<string_view> in_file_name;
    optional<string_view> out_file_name;
    for (int i = 1; i < argc; ++i){
        auto curr = string_view(argv[i]);
        if (curr == "--help"){
            show_usage(0);
        }
        else if (curr == "-o"){
            if(i == argc -1){
                show_usage(1);
            }
            out_file_name = argv[++i];
        }
        else if (curr.starts_with("-o")){
            out_file_name = curr.substr(2);
        }
        else if (curr.size() > 1 && curr.starts_with("-")){
            throw invalid_argument("unknown option");
        }
        else {
            in_file_name = argv[i];
        }
    }
    if (!in_file_name){
        cerr << "input file name must be specified" << endl;
    }
    if (out_file_name){
        ostr(make_unique<ofstream>(out_file_name->data()));
    }

    string program = read_input(string(*in_file_name));


    auto tokens = tokenize(program.c_str());
    auto [node, pos] = parse_program(tokens, 0);
    if (pos != tokens.size()) {
        verror_at(tokens.at(pos), "Not parsed");
    }
    generate_main(node);
}
