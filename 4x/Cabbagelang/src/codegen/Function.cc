#include <codegen/Function.h>

namespace codegen {

Argument::Argument(std::string name, std::string type, llvm::Value* defaultValue) :
    name(name), type(type), defaultValue(defaultValue) {
    genericMem.clear();
    if(defaultValue != nullptr)
        defaulted = true;
}

Function::Function(std::string name, std::string returnType, bool hasEllipsis) :
    name(name), returnType(returnType), hasEllipsis(hasEllipsis){
        if(hasEllipsis) maxArgumentCount = std::numeric_limits<int>::max();
    }

Function::~Function() {
    for (auto& arg : arguments) {
        delete arg;
    }
}

void Function::addArgument(std::string name, std::string type) {
    for(auto& arg : arguments) {
        if (arg->name == name) throw std::runtime_error("Conflict declaration for the same argument");
    }
    arguments.push_back(new Argument(name, type));
    minArgumentCount++;
    maxArgumentCount++;
}

void Function::addArgument(std::string name, std::string type, llvm::Value* defaultValue) {
    for(auto& arg : arguments) {
        if (arg->name == name) throw std::runtime_error("Conflict declaration for the same argument");
    }
    arguments.push_back(new Argument(name, type, defaultValue));
    maxArgumentCount++;
}

}