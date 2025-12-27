#include <codegen/ClassMem.h>

namespace codegen {

ClassMem::ClassMem(MemberType type, Access access, std::string name, std::string typeName) :
    type(type), access(access), name(name), typeName(typeName), from(0) {genericMem.clear();}

ClassMem::ClassMem(const ClassMem& other) {
    name = other.name;
    access = other.access;
    varIndex = other.varIndex;
    type = other.type;
    typeName = other.typeName;
    from = other.from;
    genericMem = other.genericMem;
}

}