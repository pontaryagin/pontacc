#pragma once
#include <bits/stdc++.h>
#include <span>
using namespace std;

// Reports an error location and exit.
void verror_at(string_view current_input, int pos, string_view fmt, bool next=false) {
    cerr << current_input << endl;
    for (int i = 0; i < pos + (next ? 1: 0); ++i){
        cerr << " "; // print pos spaces.
    }
    cerr <<  "^ ";
    cerr << fmt << endl;
    throw;
}


string read_all_lines(){
    string text;
    string line;
    while (getline(cin, line))
    {
        if (text.size() > 0){
            text += "\n";
        }
        text += line;
    }
    return text;
}

struct INode;
