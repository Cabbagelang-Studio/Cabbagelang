#ifndef CODEGEN_CLASSMEM_H
#define CODEGEN_CLASSMEM_H

#include <string>
#include <vector>

namespace codegen {

enum MemberType {
    VARIABLE, FUNCTION
};

enum Access {
    PUBLIC, PRIVATE, PROTECTED, SHARED,
    STATIC_PUBLIC, STATIC_PRIVATE, STATIC_PROTECTED, STATIC_SHARED
};

const Access STATIC = (Access)4;

class ClassMem {
    public:
        ClassMem(MemberType type, Access access, std::string name, std::string typeName);
        ClassMem(const ClassMem& other);
        MemberType type;
        Access access;
        std::string name;
        std::string typeName;
        std::vector<std::string> genericMem; // for generic classes
        int from=0; // for inherited members
        int varIndex=0;

        bool isConstant = false;
        bool isVolatile = false;
};

} // namespace codegen

#endif