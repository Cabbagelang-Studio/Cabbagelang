#include <Cabbagelang/Generic.h>

namespace cabbagelang {

void defineGenericMethods(llvm::Module* module, llvm::LLVMContext &context, std::unordered_map<std::string, codegen::Function*>& functions, std::unordered_map<std::string, llvm::Type*> types) {
    // Define methods for the Generic class
    {
        codegen::Function* function = new codegen::Function("Generic___del__", "void");
        functions["Generic___del__"] = function;
        function->addArgument("self", "Generic");

        llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), {getPtr(context)}, false);
        llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "Generic___del__", module);
        func->getArg(0)->setName("self");

        llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);
        llvm::IRBuilder<> builder(entry);

        builder.CreateFree(func->getArg(0));

        builder.CreateRetVoid();
    }
    {
        codegen::Function* function = new codegen::Function("Generic_actual", "long");
        functions["Generic_actual"] = function;
        function->addArgument("element", "Generic");

        llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getInt64Ty(context), {getPtr(context)}, false);
        llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "Generic_actual", module);
        func->getArg(0)->setName("element");

        llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);
        llvm::IRBuilder<> builder(entry);
        builder.CreateRet(builder.CreateLoad(
            llvm::Type::getInt64Ty(context),
            builder.CreateStructGEP(types["Generic"], func->getArg(0), 1)
        ));
    }
}

void defineGenericClass(llvm::Module* module, llvm::LLVMContext &context, std::unordered_map<std::string, std::deque<codegen::ClassMem*>>& classMemMap, std::unordered_map<std::string, codegen::Function*>& functions, std::unordered_map<std::string, llvm::Type*>& types, std::unordered_map<std::string, std::vector<std::string>>& inhers) {
    std::vector<llvm::Type*> elements;
    elements.push_back(llvm::Type::getInt32Ty(context));
    elements.push_back(llvm::Type::getInt64Ty(context)); // Data type - to be determined at runtime
    elements.push_back(getPtr(context)); // Pointer to the actual data

    // Generic class has no member, inheritance or function.
    // It is just a placeholder for the generic type.
    // The actual type is determined at runtime.
    
    types["Generic"] = llvm::StructType::create(context, elements, "Generic");
    inhers["Generic"] = {};
    classMemMap["Generic"] = std::deque<codegen::ClassMem*>(0); // Initialize the class member map

    codegen::ClassMem* actualFun = new codegen::ClassMem(codegen::FUNCTION, codegen::STATIC_PUBLIC, "actual", "long");
    classMemMap["Generic"].push_back(actualFun);

    defineGenericMethods(module, context, functions, types);
}

}