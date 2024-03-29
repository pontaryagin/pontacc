#pragma once
#include <bits/stdc++.h>
#include <span>
using namespace std;

// Reports an error location and exit.
inline void verror_at(string_view current_input, int pos, string_view fmt, bool next=false) {
    auto eol = current_input.find("\n");
    if (eol != std::string_view::npos){
        current_input = current_input.substr(0, eol);
    }
    cerr << current_input << endl;
    for (int i = 0; i < pos + (next ? 1: 0); ++i){
        cerr << " "; // print pos spaces.
    }
    cerr <<  "^ ";
    cerr << fmt << endl;
    throw;
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

inline ostream& ostr(unique_ptr<ostream> os = nullptr){
    static unique_ptr<ostream> os_;
    if (os)
        os_ = move(os);
    return *os_;
}

template<class T>
using optref = optional<reference_wrapper<T>>;
