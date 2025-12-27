#include <Cabbagelang/Types.h>

namespace cabbagelang {

void defineTypesClass(llvm::Module* module, llvm::LLVMContext &context, std::unordered_map<std::string, std::deque<codegen::ClassMem*>>& classMemMap, std::unordered_map<std::string, codegen::Function*>& functions, std::unordered_map<std::string, llvm::Type*>& types, std::unordered_map<std::string, std::vector<std::string>>& inhers) {
    types["Types"] = llvm::StructType::create(context, {}, "Types");
    inhers["Types"] = {};
    classMemMap["Types"] = std::deque<codegen::ClassMem*>(0);

    codegen::ClassMem* intType = new codegen::ClassMem(codegen::VARIABLE, codegen::STATIC_PUBLIC, "int", "long");
    intType->isConstant = true;
    new llvm::GlobalVariable(
        *module,
        llvm::Type::getInt64Ty(context),
        true,
        llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::hash_value(llvm::Type::getInt32Ty(context))),
        "Types_int"
    );
    classMemMap["Types"].push_back(intType);

    codegen::ClassMem* shortType = new codegen::ClassMem(codegen::VARIABLE, codegen::STATIC_PUBLIC, "short", "long");
    shortType->isConstant = true;
    new llvm::GlobalVariable(
        *module,
        llvm::Type::getInt64Ty(context),
        true,
        llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::hash_value(llvm::Type::getInt16Ty(context))),
        "Types_short"
    );
    classMemMap["Types"].push_back(shortType);

    codegen::ClassMem* longType = new codegen::ClassMem(codegen::VARIABLE, codegen::STATIC_PUBLIC, "long", "long");
    longType->isConstant = true;
    new llvm::GlobalVariable(
        *module,
        llvm::Type::getInt64Ty(context),
        true,
        llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::hash_value(llvm::Type::getInt64Ty(context))),
        "Types_long"
    );
    classMemMap["Types"].push_back(longType);

    codegen::ClassMem* floatType = new codegen::ClassMem(codegen::VARIABLE, codegen::STATIC_PUBLIC, "float", "long");
    floatType->isConstant = true;
    new llvm::GlobalVariable(
        *module,
        llvm::Type::getInt64Ty(context),
        true,
        llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::hash_value(llvm::Type::getFloatTy(context))),
        "Types_float"
    );
    classMemMap["Types"].push_back(floatType);

    codegen::ClassMem* doubleType = new codegen::ClassMem(codegen::VARIABLE, codegen::STATIC_PUBLIC, "double", "long");
    doubleType->isConstant = true;
    new llvm::GlobalVariable(
        *module,
        llvm::Type::getInt64Ty(context),
        true,
        llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::hash_value(llvm::Type::getDoubleTy(context))),
        "Types_double"
    );
    classMemMap["Types"].push_back(doubleType);

    codegen::ClassMem* charType = new codegen::ClassMem(codegen::VARIABLE, codegen::STATIC_PUBLIC, "char", "long");
    charType->isConstant = true;
    new llvm::GlobalVariable(
        *module,
        llvm::Type::getInt64Ty(context),
        true,
        llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::hash_value(llvm::Type::getInt8Ty(context))),
        "Types_char"
    );
    classMemMap["Types"].push_back(charType);

    codegen::ClassMem* boolType = new codegen::ClassMem(codegen::VARIABLE, codegen::STATIC_PUBLIC, "bool", "long");
    boolType->isConstant = true;
    new llvm::GlobalVariable(
        *module,
        llvm::Type::getInt64Ty(context),
        true,
        llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::hash_value(llvm::Type::getInt1Ty(context))),
        "Types_bool"
    );
    classMemMap["Types"].push_back(boolType);

    codegen::ClassMem* byteType = new codegen::ClassMem(codegen::VARIABLE, codegen::STATIC_PUBLIC, "byte", "long");
    byteType->isConstant = true;
    new llvm::GlobalVariable(
        *module,
        llvm::Type::getInt64Ty(context),
        true,
        llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::hash_value(llvm::Type::getInt8Ty(context))),
        "Types_byte"
    );
    classMemMap["Types"].push_back(byteType);

    codegen::ClassMem* voidType = new codegen::ClassMem(codegen::VARIABLE, codegen::STATIC_PUBLIC, "void", "long");
    voidType->isConstant = true;
    new llvm::GlobalVariable(
        *module,
        llvm::Type::getInt64Ty(context),
        true,
        llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::hash_value(llvm::Type::getVoidTy(context))),
        "Types_void"
    );
    classMemMap["Types"].push_back(voidType);
}

}