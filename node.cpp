#include "node.h"

Type NodeAddress::get_type() const {
    return to_ptr(var->get_type());
}

Type NodeDeref::get_type() const{
    auto type = var->get_type();
    assert_at(is_pointer_like(type), token, "non pointer type cannot be dereferenced");
    return deref(move(type));
}

Type NodePunct::get_type() const{
    auto&& l = lhs->get_type();
    auto&& r = rhs->get_type();
    auto l_is_ptr = is_pointer_like(l);
    auto r_is_ptr = is_pointer_like(r);
    auto l_is_int = get_from_box<TypeInt>(&l);
    auto r_is_int = get_from_box<TypeInt>(&r);

    if(l_is_ptr && r_is_ptr){
        assert_at(l == r, token, "diffrent types passed to operator");
        return TypeInt{};
    }
    else if (l_is_ptr && r_is_int){
        return l;
    }
    else if (l_is_int && r_is_ptr){
        return r;
    }
    else if (l_is_int && r_is_int){
        return l;

    }
    assert_at(false, token, "unsupported operator");
    abort();
}


Type NodeAssign::get_type() const{
    // check type of both side of assignement or complement of the type of left hand side
    auto tl = lhs->get_type();
    auto tr = rhs->get_type();
    assert_at(tl==tr, token, "diffrent types for left and right hand side of assignment");
    return tl;
}

Type NodeRet::get_type() const{
    return pNode->get_type();
}

Type NodeVar::get_type() const{
    return m_type;
}

Type NodeFunc::get_type() const{
    return TypeInt{};
}

