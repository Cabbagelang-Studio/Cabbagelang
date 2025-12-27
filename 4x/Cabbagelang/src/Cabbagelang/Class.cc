#include <Cabbagelang/Class.h>

namespace cabbagelang {

llvm::Type* getPtr(llvm::LLVMContext &context){
    if(utils::bit_size == utils::BIT_32)
        return llvm::PointerType::get(llvm::Type::getInt32Ty(context), 0);
    else
        return llvm::PointerType::get(llvm::Type::getInt64Ty(context), 0);
}

void defineBuiltinClasses(llvm::Module* module, llvm::LLVMContext &context, std::unordered_map<std::string, std::deque<codegen::ClassMem*>>& classMemMap, std::unordered_map<std::string, codegen::Function*>& functions, std::unordered_map<std::string, llvm::Type*>& types, std::unordered_map<std::string, std::vector<std::string>>& inhers){
    cabbagelang::defineCastClass(context, classMemMap, types, inhers);
    cabbagelang::defineStringClass(module, context, classMemMap, functions, types, inhers);
    cabbagelang::defineExceptionClass(module, context, classMemMap, functions, types, inhers);
    cabbagelang::defineGenericClass(module, context, classMemMap, functions, types, inhers);
    cabbagelang::defineTypesClass(module, context, classMemMap, functions, types, inhers);
}

}