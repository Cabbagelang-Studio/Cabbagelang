#include <Cabbagelang/String.h>

namespace cabbagelang {

llvm::CallInst* createMallocString(int strLength, 
                                 llvm::Module* module, llvm::IRBuilder<> &builder) {
    using namespace llvm;
    
    LLVMContext& context = module->getContext();
    
    Type* charType = Type::getInt8Ty(context);
    Type* charPtrType = charType->getPointerTo();
    
    llvm::Value* mallocCall = builder.CreateMalloc(llvm::Type::getInt32Ty(context),
        charType, 
        llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(context),
            strLength
        ),
        nullptr
    );
    
    return cast<CallInst>(builder.CreateBitCast(
        mallocCall, charPtrType
    ));
}

void defineStringMethods(llvm::Module* module, llvm::LLVMContext &context, std::unordered_map<std::string, codegen::Function*>& functions, std::unordered_map<std::string, llvm::Type*> types) {
    // Define methods for the String class
    {
        codegen::Function* function = new codegen::Function("String___init__", "void");
        functions["String___init__"] = function;
        function->addArgument("name", "String");
        function->addArgument("value", "String", llvm_nullptr);

        llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), {getPtr(context), getPtr(context)}, false);
        llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "String___init__", module);
        func->getArg(0)->setName("self");
        func->getArg(1)->setName("value");
        llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);
        llvm::IRBuilder<> builder(entry);

        builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), builder.CreateStructGEP(types["String"], func->getArg(0), 0));
        llvm::Value* notNull = builder.CreateIsNotNull(func->getArg(1));
        llvm::BasicBlock* ifNotNull = llvm::BasicBlock::Create(context, "ifNotNull", func);
        llvm::BasicBlock* ifNull = llvm::BasicBlock::Create(context, "ifNull", func);
        builder.CreateCondBr(notNull, ifNotNull, ifNull);
        builder.SetInsertPoint(ifNotNull);
        builder.CreateStore(func->getArg(1), builder.CreateStructGEP(types["String"], func->getArg(0), 1));
        builder.CreateRetVoid();
        builder.SetInsertPoint(ifNull);
        builder.CreateStore(createMallocString(1, module, builder), builder.CreateStructGEP(types["String"], func->getArg(0), 1));
        builder.CreateRetVoid();
    }
    {
        codegen::Function* function = new codegen::Function("String___del__", "void");
        functions["String___del__"] = function;
        function->addArgument("self", "String");

        llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), {getPtr(context)}, false);
        llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "String___del__", module);
        func->getArg(0)->setName("self");

        llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);
        llvm::IRBuilder<> builder(entry);

        builder.CreateFree(builder.CreateLoad(getPtr(context), builder.CreateStructGEP(types["String"], func->getArg(0), 1)));
        builder.CreateFree(func->getArg(0));

        builder.CreateRetVoid();
    }
}

void defineStringClass(llvm::Module* module, llvm::LLVMContext &context, std::unordered_map<std::string, std::deque<codegen::ClassMem*>>& classMemMap, std::unordered_map<std::string, codegen::Function*>& functions, std::unordered_map<std::string, llvm::Type*>& types, std::unordered_map<std::string, std::vector<std::string>>& inhers) {
    std::vector<llvm::Type*> elements;
    elements.push_back(llvm::Type::getInt32Ty(context));
    elements.push_back(getPtr(context)); // String data pointer
    elements.push_back(llvm::Type::getInt32Ty(context)); // String length
    types["String"] = llvm::StructType::create(context, elements, "String");
    inhers["String"] = {};
    classMemMap["String"] = std::deque<codegen::ClassMem*>(0); // Initialize the class member map
    codegen::ClassMem* strLength = new codegen::ClassMem(codegen::VARIABLE, codegen::PRIVATE, "_length", "int");
    classMemMap["String"].push_back(strLength);
    codegen::ClassMem* strLengthFun = new codegen::ClassMem(codegen::FUNCTION, codegen::PUBLIC, "length", "int");
    classMemMap["String"].push_back(strLengthFun);

    defineStringMethods(module, context, functions, types);
}

}