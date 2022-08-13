#pragma once
#include "common.h"
#include "box.h"

using Type = variant<box<struct TypeInt>, box<struct TypeChar>, box<struct TypePtr>, 
    box<struct TypeArray>, box<struct TypeFunc>>;
using PtrType = shared_ptr<Type>;

struct TypeInt{
    auto operator<=>(const TypeInt&) const = default;
    int size_of() const { return 8; }
};

struct TypeChar{
    auto operator<=>(const TypeChar&) const = default;
    int size_of() const { return 1; }
};

struct TypePtr{
    PtrType base;
    strong_ordering operator<=>(const TypePtr& rhs) const;
    bool operator==(const TypePtr& rhs) const { return (*this <=> rhs) == 0; }
    int size_of() const { return 8; }
};

struct TypeArray{
    PtrType base;
    int m_size;
    strong_ordering operator<=>(const TypeArray& rhs) const;
    bool operator==(const TypeArray& rhs) const { return (*this <=> rhs) == 0; }
    int size_of() const;
};

struct TypeFunc{
    PtrType m_ret;
    vector<PtrType> m_params;
    strong_ordering operator<=>(const TypeFunc&) const;
    bool operator==(const TypeFunc& rhs) const { return (*this <=> rhs) == 0; }
    int size_of() const { return 8; }
};

template<class T, class Variant>
inline auto from_box(Variant& t) { 
    auto val = get_if<box<T>>(&t);
    return val ? &(**val) : nullptr;
}

template<class T>
inline bool is_type_of(const Type& t) { return from_box<T>(t); }
inline bool is_ptr(const Type& t) { return from_box<TypePtr>(t); }
inline Type to_ptr(Type type){
    return TypePtr{make_shared<Type>(move(type))};
}
inline Type deref(Type type){
    if (auto p = from_box<TypePtr>(type)){
        return *p->base;
    }
    else if (auto p = from_box<TypeArray>(type)){
        return *p->base;
    }
    abort();
}
inline int size_of(Type type){
    return visit([](auto&& t){ return t->size_of(); }, type);
}
inline int size_of_base(Type type){
    if (auto p = from_box<TypePtr>(type)){
        return size_of(*p->base);
    }
    else if (auto p = from_box<TypeArray>(type)){
        return size_of(*p->base);
    }
    throw invalid_argument("no base member");
}

template<class LHS, class RHS>
inline strong_ordering operator<=>(const box<LHS>& lhs, const box<RHS>& rhs){
    return (*lhs)<=>(*rhs);
}

inline strong_ordering TypePtr::operator<=>(const TypePtr& rhs) const{
    const auto& r = *rhs.base;
    return *base <=> r;
}

inline strong_ordering TypeArray::operator<=>(const TypeArray& rhs) const{
    const auto& r = *rhs.base;
    return *base <=> r;
}

inline strong_ordering TypeFunc::operator<=>(const TypeFunc& rhs) const{
    if(auto cmp = *m_ret <=> *rhs.m_ret; cmp != 0){
        return cmp;
    }
    else {
        if (auto cmp = m_params.size() <=> m_params.size(); cmp != 0) {
            return cmp;
        }
        for (size_t i = 0; i < m_params.size(); i++) {
            if (auto cmp = m_params[i] <=> rhs.m_params[i]; cmp != 0) {
                return cmp;
            }
        }
    }
    return std::strong_ordering::equal;
}

inline int TypeArray::size_of() const { 
    return m_size * visit([](auto&& t){return t->size_of(); }, *base); 
}

inline bool is_pointer_like(const Type& t){
    return from_box<TypeArray>(t) || from_box<TypePtr>(t);
};

inline bool is_number(const Type& t){
    return from_box<TypeInt>(t) || from_box<TypeChar>(t);
};
