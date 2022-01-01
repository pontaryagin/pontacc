
#pragma once
#include "common.h"

enum class TokenKind {
    Num,
    Punct,
};

struct Token {
    int val;
    string punct;
    TokenKind kind;
    // info 
    string_view statement;
    int loc; // Token location
    int len;   // Token length
    Token () = default;

    int from_num(string_view statement) {
        kind = TokenKind::Num;
        char* next;
        auto p = statement.data();
        val = strtol(p, &next, 10);
        len = next - p;
        return len;
    }

    int from_punct(string_view punct_) {
        kind = TokenKind::Punct;
        punct = punct_;
        len = punct_.size();
        return len;
    }

    Token(string_view statement, int& pos)
        : statement(statement), loc(pos)
    {
        if (isdigit(statement[pos])){
            pos += from_num(statement.substr(pos));
            return;
        }
        auto curr = statement.substr(pos);
        for (auto op : {"<=", ">=", "==", "!=", "+", "-", "*", "/", "(", ")", "<", ">", "=", ";"}){
            if (curr.starts_with(op)){
                pos += from_punct(op);
                return;
            }
        }
        verror_at(statement, loc, "Unknown operator\n");
    }
};

using Tokens = vector<Token>;

Tokens tokenize(string_view text){
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


void verror_at(const Token& token, string_view fmt){
    verror_at(token.statement, token.loc, fmt);
}

