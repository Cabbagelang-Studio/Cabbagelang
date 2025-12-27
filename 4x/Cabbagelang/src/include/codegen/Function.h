#ifndef CODEGEN_FUNCTION_H
#define CODEGEN_FUNCTION_H

#include <string>
#include <deque>
#include <algorithm>
#include <limits>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>

namespace codegen {

struct Argument {
    Argument(std::string name, std::string type, llvm::Value* defaultValue = nullptr);
    std::string name;
    std::string type;
    std::vector<std::string> genericMem; // for generic functions
    bool defaulted = false;
    llvm::Value* defaultValue = nullptr;
};

class Function {
    public:
        Function(std::string name, std::string returnType, bool hasEllipsis = false);
        ~Function();
        void addArgument(std::string name, std::string type);
        void addArgument(std::string name, std::string type, llvm::Value* defaultValue);
        std::string name;
        std::string returnType;
        std::deque<Argument*> arguments;
        std::vector<std::string> genericMem; // for generic functions
        bool hasEllipsis = false;

        int minArgumentCount = 0;
        int maxArgumentCount = 0;
};

}

#endif // CODEGEN_FUNCTION_H