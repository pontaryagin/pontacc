
#pragma once
#include "common.h"

enum class TokenKind {
    Unknown,
    Num,
    Punct,
    Ident,
    Keyword,
    String,
};

inline set<string> keywords = {
    "return",
    "if",
    "else",
    "for",
    "while",
    "continue",
    "break",
    "int",
    "char",
    "sizeof"
};

struct Token {
    int val;
    shared_ptr<const string> text;
    string punct;
    string ident;
    TokenKind kind;
    // info 
    string_view statement;
    int loc; // Token location
    int len;   // Token length
    Token () = default;

    int from_num(string_view statement) {
        char* next;
        auto p = statement.data();
        val = strtol(p, &next, 10);
        len = next - p;
        if (len == 0){
            return len;
        }
        kind = TokenKind::Num;
        return len;
    }

    int from_punct(string_view punct_) {
        kind = TokenKind::Punct;
        punct = punct_;
        len = punct_.size();
        return len;
    }

    static bool is_identifier_char(char c){
        return ('0'<= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
    }

    int from_ident(string_view statement){
        int pos = 0;
        while(pos < statement.size() && is_identifier_char(statement[pos])){
            pos += 1;
        }
        if(pos == 0){
            return 0;
        }
        ident = statement.substr(0, pos);
        kind = keywords.contains(ident) ? TokenKind::Keyword : TokenKind::Ident;
        return len = pos;
    }

    static string as_string(char c){
        // convert to octal number
        return "\\" + to_string(c / 0100) + to_string((c%0100) / 010) + to_string((c%010));
    }

    static string read_escaped_char(char c){
        switch (c){
            case 'a': 
                return as_string('\a');
            case 'b':
                return as_string('\b');
            case 't':
                return as_string('\t');
            case 'n':
                return as_string('\n');
            case 'v':
                return as_string('\v');
            case 'f':
                return as_string('\f');
            case 'r':
                return as_string('\r');
            case 'e': return "\\033";
            default: return string{c};
        }
    }

    int from_string(string_view statement){
        kind = TokenKind::String;
        if (statement[0] != '"'){
            return 0;
        }
        int pos = 1;
        string res;
        for (;pos < statement.size();++pos){
            auto curr_char = statement[pos];
            auto prev_char = statement[pos-1];
            if (curr_char == '\n'){
                verror_at(statement, loc, "\" did not closed.\n");
            }
            else if (curr_char == '"'){
                text = make_shared<string>(move(res));
                return len = pos+1;
            }
            else if (curr_char == '\\'){
                continue;
            }
            else if(prev_char == '\\'){
                res += read_escaped_char(curr_char);
            }
            else {
                res.push_back(curr_char);
            }
        }
        verror_at(statement, loc, "\" did not closed.\n"); abort();
    }

    Token(string_view statement, int& pos)
        : statement(statement), loc(pos)
    {
        auto curr = statement.substr(pos);
        if (curr.size() == 0){
            verror_at(statement, loc, "Fail to tokenize at the end of statement\n");
        }
        for (auto op : {"<=", ">=", "==", "!=", "+", "-", "*", "/", "(", ")", "<", ">", "=", ";", "{", "}", "&",",", "[", "]"}){
            if (curr.starts_with(op)){
                pos += from_punct(op);
                return;
            }
        }
        if (auto len = from_string(curr)){
            pos += len;
            return;
        }
        if (auto len = from_num(curr)){
            pos += len;
            return;
        }
        if (auto len = from_ident(curr)){
            pos += len;
            return;
        }
        verror_at(statement, loc, "Unknown operator\n");
    }
};

using Tokens = vector<Token>;

inline Tokens tokenize(string_view text){
    Tokens tokens;
    int p = 0;
    while(p < text.size()){
        if(isspace(text[p])){
            p++;
            continue;
        }
        tokens.emplace_back(text, p);
    }
    return tokens;
}


inline void verror_at(optional<Token> token, string_view fmt, bool next=false){
    if(token){
        verror_at(token->statement, token->loc, fmt, next);
    }
    else{
        verror_at("", 0, fmt, false);
    }
}

inline void assert_at(bool check, const Token& token, string_view fmt, bool next=false){
    if (!check){
        verror_at(token, fmt, next);
    }
}

