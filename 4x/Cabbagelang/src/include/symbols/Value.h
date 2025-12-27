#ifndef SYMBOLS_SYMBOLVALUE_H
#define SYMBOLS_SYMBOLVALUE_H

#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>

#include <codegen/CodeGen.h>
#include <string>
#include <memory>

namespace symbols {

class Value{
    public:
        static codegen::CodeGen* gen;
        Value(llvm::Value* V, const std::string& typeName);
        Value(llvm::Value* V, const std::string& typeName, double constant);
        void set(bool isVariable, bool isConstant = false, bool isVolatile = false);
        llvm::Value* getValue();
        llvm::Value* getVariable();
        bool isVariable();
        bool isObject();
        bool notNull();
        bool isConstant();
        bool isVolatile();
        bool isNumeric();
        bool isSimple();
        std::string getTypeName();
        // Object
        int id = -1;
        bool isMember = false;
        bool currentScope = true;
        bool isConstantValue = false;
        bool isSuper = false;
        double constant = 0;
        std::vector<std::string> genericMem; // For generic types
    private:
        llvm::Value* value;
        bool _isVariable = false;
        bool _isObject = false;
        std::string typeName = "";

        bool _isConstant = false;
        bool _isVolatile = false;
};

struct constantParam {
    double constant;
    bool is_constant;
    constantParam(bool);
    constantParam(double);
    constantParam(int);
};

using aValue = std::shared_ptr<symbols::Value>;

aValue create_value(llvm::Value* value, const std::string& type, constantParam constant = false);

} // namespace symbols

#endif // SYMBOLS_SYMBOLVALUE_H