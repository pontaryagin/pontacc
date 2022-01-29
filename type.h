#include "common.h"

struct Type;
// std::strong_ordering operator<=>(const Type&, const Type&);
inline strong_ordering operator<=>(const Type&, const Type&);

using PtrType = shared_ptr<Type>;

struct TypeInt{
    auto operator<=>(const TypeInt&) const = default;
};

struct TypePtr{
    PtrType base;
    strong_ordering operator<=>(const TypePtr& rhs) const;
    bool operator==(const TypePtr& rhs) const = default;
};

struct TypeFunc{
    PtrType ret;
    vector<PtrType> params;
    strong_ordering operator<=>(const TypeFunc&) const;
    bool operator==(const TypeFunc& rhs) const = default;
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
    // friend std::strong_ordering operator<=>(const Type&, const Type&) = default;
    // friend strong_ordering operator<=>(const Type&, const Type&) = default;
    friend inline strong_ordering operator<=>(const Type& lhs, const Type& rhs);
    // bool operator==(const Type&) const = default;
};

inline strong_ordering operator<=>(const Type& lhs, const Type& rhs)
{
    if(auto cmp = lhs.index() <=> rhs.index(); cmp != 0) return cmp;
    return visit([&](auto&& l, auto&& r){
        if constexpr (is_same_v<decltype(lhs), decltype(rhs)>){
            return l<=>r;
        }
        else{
            return lhs.index() <=> rhs.index();
        }
    }, lhs.as_variant(), rhs.as_variant());
}

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

