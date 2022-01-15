#include "node.h"

OptionalType NodeAddress::get_type() {
    auto type = var->get_type();
    return Type{type->kind, type->ptr+1};
}

OptionalType NodeDeref::get_type(){
    auto type = var->get_type();
    if (type->ptr<=0){
        verror_at(token, "non pointer type cannot be dereferenced");
    }
    return Type{type->kind, type->ptr-1};
}

OptionalType NodePunct::get_type(){
    auto tl = *lhs->get_type();
    auto tr = *rhs->get_type();
    if ((tl.ptr > 0 && tr.ptr == 0) || (tl.ptr ==0 && tr.ptr > 0)){
        return tl.ptr > 0 ? tl: tr;
    }
    else if (tl.ptr > 0 && tr.ptr > 0){
        if (tl!= tr){
            verror_at(token, "diffrent types passed to operator");
        }
        return Type{TypeKind::Int, 0};
    }
    return tl;
}


OptionalType NodeAssign::get_type(){
    // check type of both side of assignement or complement of the type of left hand side
    auto tl = lhs->get_type();
    auto tr = rhs->get_type();
    assert_at(tr.has_value() ,token, "type of right hand side of assignment not defined");
    if (!tl){
        assert_at(dynamic_cast<NodeVar*>(lhs.get()), token, "variable should come at lhs of assignment");
    }
    else {
        assert_at(*tl==*tr, token, "diffrent types for left and right hand side of assignment");
    }
    return *tl;
}

OptionalType NodeRet::get_type(){
    return pNode->get_type();
}

optional<Type> NodeVar::get_type(){
    return var_types[name];
}
