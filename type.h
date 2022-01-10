#include "common.h"

enum class TypeKind {
    Int
};

struct Type {
    TypeKind kind;
    int ptr = 0;
    auto operator<=>(const Type&) const = default;
};

