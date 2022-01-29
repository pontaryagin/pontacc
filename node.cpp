#include "node.h"

Type NodeAddress::get_type() {
    return Type::to_ptr(var->get_type());
}

Type NodeDeref::get_type(){
    auto type = var->get_type();
    assert_at(get_if<TypePtr>(&type), token, "non pointer type cannot be dereferenced");
    return Type::deref(move(type));
}

Type NodePunct::get_type(){
    auto&& l = lhs->get_type();
    auto&& r = rhs->get_type();
    auto l_is_ptr = get_if<TypePtr>(&l);
    auto r_is_ptr = get_if<TypePtr>(&r);
    auto l_is_int = get_if<TypeInt>(&l);
    auto r_is_int = get_if<TypeInt>(&r);

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


Type NodeAssign::get_type(){
    // check type of both side of assignement or complement of the type of left hand side
    auto tl = lhs->get_type();
    auto tr = rhs->get_type();
    assert_at(tl==tr, token, "diffrent types for left and right hand side of assignment");
    return tl;
}

Type NodeRet::get_type(){
    return pNode->get_type();
}

Type NodeVar::get_type(){
    return var_types[name];
}

Type NodeFunc::get_type(){
    return TypeInt{};
}

