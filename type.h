#include "common.h"

struct Type;

using PtrType = shared_ptr<Type>;

struct TypeInt{
    auto operator<=>(const TypeInt&) const = default;
    int size_of() const { return 1; }
};

struct TypePtr{
    PtrType base;
    strong_ordering operator<=>(const TypePtr& rhs) const;
    bool operator==(const TypePtr& rhs) const { return (*this <=> rhs) == 0; }
    int size_of() const { return 1; }
};

struct TypeArray{
    PtrType base;
    int m_size;
    strong_ordering operator<=>(const TypeArray& rhs) const;
    bool operator==(const TypeArray& rhs) const { return (*this <=> rhs) == 0; }
    int size_of() const { return m_size; }
};

struct TypeFunc{
    PtrType m_ret;
    vector<PtrType> m_params;
    strong_ordering operator<=>(const TypeFunc&) const;
    bool operator==(const TypeFunc& rhs) const { return (*this <=> rhs) == 0; }
    int size_of() const { return 1; }
};

struct Type: variant<TypeInt, TypePtr, TypeArray, TypeFunc>{
    // NOTE: this class should not have extra member variable, since variant's destructor is non-virtual
    using variant::variant;
    const variant& as_variant() const { return static_cast<const variant&>(*this); }

    template<class T>
    bool is_type_of() const { return get_if<T>(this); }
    bool is_ptr() const { return get_if<TypePtr>(this); }
    static Type to_ptr(Type type){
        return TypePtr{move(make_unique<Type>(move(type)))};
    }
    static Type deref(Type type){
        auto p = get<TypePtr>(type);
        return move(*p.base);
    }
    friend strong_ordering operator<=>(const Type&, const Type&) = default;
};

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

