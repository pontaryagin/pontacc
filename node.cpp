#include "node.h"

Type NodeAddress::get_type() {
    auto type = var->get_type();
    return Type{type.kind, type.ptr+1};
}

Type NodeDeref::get_type(){
    auto type = var->get_type();
    assert_at(type.ptr>0, token, "non pointer type cannot be dereferenced");
    return Type{type.kind, type.ptr-1};
}

Type NodePunct::get_type(){
    auto tl = lhs->get_type();
    auto tr = rhs->get_type();
    if ((tl.ptr > 0 && tr.ptr == 0) || (tl.ptr ==0 && tr.ptr > 0)){
        return tl.ptr > 0 ? tl: tr;
    }
    else if (tl.ptr > 0 && tr.ptr > 0){
        assert_at(tl == tr, token, "diffrent types passed to operator");
        return Type{TypeKind::Int, 0};
    }
    return tl;
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
    return Type{TypeKind::Int, 0};
}

