#include <symbols/Value.h>

namespace symbols {

codegen::CodeGen* Value::gen = nullptr;

Value::Value(llvm::Value* V, const std::string& typeName) : value(V) {
    this->typeName = typeName;
    if (!(typeName == "int" || typeName == "short" || typeName == "long" || 
        typeName == "char" || typeName == "void" || typeName == "float" ||
        typeName == "double" || typeName == "bool" || typeName == "byte")) {
        _isObject = true;
        id = gen->getUniqueId();
        genericMem.clear();
    }
}

Value::Value(llvm::Value* V, const std::string& typeName, double constant) : value(V), isConstantValue(true), constant(constant) {
    this->typeName = typeName;
    if (!(typeName == "int" || typeName == "short" || typeName == "long" || 
        typeName == "char" || typeName == "void" || typeName == "float" ||
        typeName == "double" || typeName == "bool" || typeName == "byte")) {
        _isObject = true;
        id = gen->getUniqueId();
        genericMem.clear();
    }
}

void Value::set(bool isVariable, bool isConstant, bool isVolatile) {
    this->_isVariable = isVariable;
    this->_isConstant = isConstant;
    this->_isVolatile = isVolatile;
}

llvm::Value* Value::getValue() {
    return _isVariable?gen->builder.CreateLoad(gen->getType(typeName), value, _isVolatile):value;
}

llvm::Value* Value::getVariable() {
    if(!_isVariable)
        throw std::runtime_error("Trying to access constant as variable");
    return value;
}

std::string Value::getTypeName() {
    return typeName;
}

bool Value::isConstant() {
    return _isConstant;
}

bool Value::isVolatile() {
    return _isVolatile;
}

bool Value::isObject() {
    return _isObject;
}

bool Value::notNull() {
    return typeName != "null";
}

bool Value::isVariable() {
    return _isVariable;
}

bool Value::isNumeric() {
    return typeName == "int" || typeName == "short" || typeName == "long" || typeName == "float" || typeName == "double";
}

bool Value::isSimple() {
    return typeName == "int" || typeName == "short" || typeName == "long" || typeName == "float" || typeName == "double" || typeName == "char" || typeName == "bool" || typeName == "byte";
}

constantParam::constantParam(bool is_constant) {
    this->is_constant = is_constant;
}

constantParam::constantParam(double constant) {
    this->constant = constant;
    this->is_constant = true;
}

constantParam::constantParam(int constant) {
    this->constant = constant;
    this->is_constant = true;
}

aValue create_value(llvm::Value* V, const std::string& typeName, constantParam constant){
    if(!constant.is_constant)
        return std::make_shared<Value>(V, typeName);
    else
        return std::make_shared<Value>(V, typeName, constant.constant);
}

} // namespace symbols