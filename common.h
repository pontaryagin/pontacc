#pragma once
#include <bits/stdc++.h>
#include <span>
using namespace std;

// Reports an error location and exit.
inline void verror_at(string_view current_input, int pos, string_view fmt, bool next=false) {
    cerr << current_input << endl;
    for (int i = 0; i < pos + (next ? 1: 0); ++i){
        cerr << " "; // print pos spaces.
    }
    cerr <<  "^ ";
    cerr << fmt << endl;
    throw;
}


inline string read_all_lines(){
    string text;
    string line;
    std::stringstream buffer;
    buffer << cin.rdbuf();
    return buffer.str();
}

inline string read_file(const string& file_name){
    string text;
    ifstream ifs(file_name);
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

// Round up n
inline constexpr int round_up(int n, int align){
    return (n + align - 1) / align * align;
}

// cast by perfect forwarding between variant
template <class ToType, class FromType>
inline ToType variant_cast(FromType&& v)
{
    return std::visit([](auto&& arg) -> ToType{ return forward<decltype(arg)>(arg); },
                    std::forward<FromType>(v));
}

template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
