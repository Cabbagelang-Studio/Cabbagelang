#include <Cabbagelang/Exception.h>

namespace cabbagelang {

void defineExceptionMethods(llvm::Module* module, llvm::LLVMContext &context, std::unordered_map<std::string, codegen::Function*>& functions, std::unordered_map<std::string, llvm::Type*> types) {
    // Define methods for the Exception class
    {
        codegen::Function* function = new codegen::Function("Exception___init__", "Exception");
        functions["Exception___init__"] = function;
        function->addArgument("self", "Exception");
        function->addArgument("message", "String");

        llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), {getPtr(context), getPtr(context)}, false);
        llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "Exception___init__", module);
        func->getArg(0)->setName("self");
        func->getArg(1)->setName("message");

        llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);
        llvm::IRBuilder<> builder(entry);

        builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), builder.CreateStructGEP(types["Exception"], func->getArg(0), 0));
        builder.CreateStore(func->getArg(1), builder.CreateStructGEP(types["Exception"], func->getArg(0), 1));
        builder.CreateRetVoid();
    }
    {
        codegen::Function* function = new codegen::Function("Exception_what", "String");
        functions["Exception_what"] = function;
        function->addArgument("self", "Exception");

        llvm::FunctionType* funcType = llvm::FunctionType::get(getPtr(context), {getPtr(context)}, false);
        llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "Exception_what", module);
        func->getArg(0)->setName("self");

        llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);
        llvm::IRBuilder<> builder(entry);

        builder.CreateRet(builder.CreateStructGEP(types["Exception"], func->getArg(0), 1));
    }
    {
        codegen::Function* function = new codegen::Function("Exception___del__", "void");
        functions["Exception___del__"] = function;
        function->addArgument("self", "Exception");

        llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), {getPtr(context)}, false);
        llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "Exception___del__", module);
        func->getArg(0)->setName("self");

        llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);
        llvm::IRBuilder<> builder(entry);

        llvm::BasicBlock* ifBlock = llvm::BasicBlock::Create(context, "if", func);
        llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context, "else", func);

        builder.CreateCondBr(
            builder.CreateIsNotNull(builder.CreateLoad(
                getPtr(context),
                builder.CreateStructGEP(types["Exception"], func->getArg(0), 1)
            )),
            ifBlock,
            elseBlock
        );

        builder.SetInsertPoint(ifBlock);
        builder.CreateCall(
            module->getFunction("String___del__"),
            {builder.CreateLoad(
                getPtr(context),
                builder.CreateStructGEP(types["Exception"], func->getArg(0), 1)
            )}
        );

        builder.CreateBr(elseBlock);

        builder.SetInsertPoint(elseBlock);
        builder.CreateFree(func->getArg(0));
        builder.CreateRetVoid();
    }
}

void defineExceptionClass(llvm::Module* module, llvm::LLVMContext &context, std::unordered_map<std::string, std::deque<codegen::ClassMem*>>& classMemMap, std::unordered_map<std::string, codegen::Function*>& functions, std::unordered_map<std::string, llvm::Type*>& types, std::unordered_map<std::string, std::vector<std::string>>& inhers) {
    std::vector<llvm::Type*> elements;
    elements.push_back(llvm::Type::getInt32Ty(context));
    elements.push_back(getPtr(context)); // String data pointer
    
    types["Exception"] = llvm::StructType::create(context, elements, "Exception");
    inhers["Exception"] = {};
    classMemMap["Exception"] = std::deque<codegen::ClassMem*>(0); // Initialize the class member map

    codegen::ClassMem* excWhat = new codegen::ClassMem(codegen::FUNCTION, codegen::PUBLIC, "what", "String");
    classMemMap["Exception"].push_back(excWhat);

    defineExceptionMethods(module, context, functions, types);
}

}