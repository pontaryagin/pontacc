
#pragma once
#include "common.h"

enum class TokenKind {
    Num,
    Punct,
    Ident,
    Keyword,
};

inline set<string> keywords = {
    "return"
};

struct Token {
    int val;
    string punct;
    string ident;
    TokenKind kind;
    static inline map<string, int> indents;
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
        if (!indents.contains(ident)){
            indents[ident] = indents.size()+1;
        }
        val = indents.at(ident);
        return len = pos;
    }

    Token(string_view statement, int& pos)
        : statement(statement), loc(pos)
    {
        auto curr = statement.substr(pos);
        for (auto op : {"<=", ">=", "==", "!=", "+", "-", "*", "/", "(", ")", "<", ">", "=", ";"}){
            if (curr.starts_with(op)){
                pos += from_punct(op);
                return;
            }
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


void verror_at(const Token& token, string_view fmt, bool next=false){
    verror_at(token.statement, token.loc, fmt, next);
}

