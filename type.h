#include "common.h"

struct Type;

using PtrType = shared_ptr<Type>;

struct TypeInt{
    auto operator<=>(const TypeInt&) const = default;
};

struct TypePtr{
    PtrType base;
    strong_ordering operator<=>(const TypePtr& rhs) const;
    bool operator==(const TypePtr& rhs) const { return (*this <=> rhs) == 0; }
};

struct TypeFunc{
    PtrType ret;
    vector<PtrType> params;
    strong_ordering operator<=>(const TypeFunc&) const;
    bool operator==(const TypeFunc& rhs) const { return (*this <=> rhs) == 0; }
};

struct Type: variant<TypeInt, TypePtr, TypeFunc>{
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

inline strong_ordering TypeFunc::operator<=>(const TypeFunc& rhs) const{
    if(auto cmp = *ret <=> *rhs.ret; cmp != 0){
        return cmp;
    }
    else {
        if (auto cmp = params.size() <=> params.size(); cmp != 0) {
            return cmp;
        }
        for (size_t i = 0; i < params.size(); i++) {
            if (auto cmp = params[i] <=> rhs.params[i]; cmp != 0) {
                return cmp;
            }
        }
    }
    return std::strong_ordering::equal;
}

