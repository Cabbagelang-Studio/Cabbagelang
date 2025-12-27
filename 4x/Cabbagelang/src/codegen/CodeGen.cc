#include <codegen/CodeGen.h>

namespace codegen {

bool isLittleEndian() {
    uint16_t value = 0x0001;
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&value);
    return bytePtr[0] == 0x01;
}

std::string getDataLayoutString() {
    std::string dataLayout;

    // Endianness
    if (isLittleEndian()) {
        dataLayout += "e-";
    } else {
        dataLayout += "E-";
    }

    // Pointer layout
    if (utils::bit_size == utils::BIT_32) {
        dataLayout += "p:32:32:32"; // 32-bit pointers, ABI and preferred alignment = 32 bits
    } else if (utils::bit_size == utils::BIT_64) {
        dataLayout += "p:64:64:64"; // 64-bit pointers, ABI and preferred alignment = 64 bits
    }

    // Integer layout
    dataLayout += "-i64:64:64"; // 64-bit integers, ABI and preferred alignment = 64 bits

    // Floating-point layout
    dataLayout += "-f32:32:32"; // 32-bit floats, ABI and preferred alignment = 32 bits
    dataLayout += "-f64:64:64"; // 64-bit floats, ABI and preferred alignment = 64 bits

    // Stack alignment (assume 128-bit stack alignment for both 32-bit and 64-bit)
    dataLayout += "-S128";

    return dataLayout;
}

CodeGen::CodeGen(compiler::Compiler& compiler) : mainCompiler(compiler), builder(context), initBuilder(context), DL(getDataLayoutString()) {
    module = std::make_unique<llvm::Module>("ir", context);
    module->setDataLayout(DL); // Set the data layout for the module

    // Maps initialized in the constructor
    functions = {};
    classes = {};
    types = {};
    values = {};
    inhers = {};
    labels = {};

    // Stacks initialized in the constructor
    breakStack = {};
    continueStack = {};
    jmpBufStack = {};

    // Vectors initialized in the constructor
    templateObjects = {};
    
    // Language builtin classes
    cabbagelang::defineBuiltinClasses(module.get(), context, classes, functions, types, inhers);
}

CodeGen::~CodeGen() {
    // Clean up the maps
    for(const auto& [name, func] : functions) {
        delete func;
    }
    for(const auto& [name, cls] : classes) {
        for(const auto& mem : cls)
            delete mem;
    }
    module.reset();
}

llvm::Module* CodeGen::generate(const parser::aNode& ast) {
    // Init main function
    initMainFunction();
    // Emit IR code from the AST
    emit(ast);
    // Generate the main function
    try {
        generateMainFunction();
    } catch (std::runtime_error& e) {
        // Handle compilation errors
        compilation_error(mainCompiler.current_filename.data(), 0, 0, e.what());
    }
    warning_error_summary();
    return module.get();
}

void CodeGen::initMainFunction() {
    // Define the main function type: int main()
    llvm::FunctionType* mainFuncType = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), false);

    // Add the main function to the module
    mainFunc = llvm::Function::Create(mainFuncType, llvm::Function::ExternalLinkage, "main", module.get());

    // Create the init block for the main function
    llvm::BasicBlock* initBlock = llvm::BasicBlock::Create(context, "init", mainFunc);
    llvm::BasicBlock* handleBlock = llvm::BasicBlock::Create(context, "handle", mainFunc);
    ENTRY_BLOCK_PLACEHOLDER = llvm::BasicBlock::Create(context, "entry", mainFunc);
    llvm::BasicBlock* panicBB = llvm::BasicBlock::Create(context, "panic", mainFunc);
    initBuilder.SetInsertPoint(initBlock);

    // Create global exceptional hidden variable
    __exception_hanging = new llvm::GlobalVariable(
        *module,
        getPtr(),
        false,
        llvm::GlobalValue::ExternalLinkage,
        llvm_nullptr,
        ".__exception_hanging"
    );

    __exception_hanging_type = new llvm::GlobalVariable(
        *module,
        llvm::Type::getInt64Ty(context),
        false,
        llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 0),
        ".__exception_hanging_type"
    );
    
    // String messages for error reporting
    RERR001 = initBuilder.CreateGlobalStringPtr("Trying to access null pointer");

    LABEL_PLACEHOLDER = initBlock;
    EXC_HANDLE_BLOCK_PLACEHOLDER = handleBlock;

    // Panic if there is an exception unhandled

    initBuilder.SetInsertPoint(handleBlock);
    llvm::GlobalVariable* exceptionJmpBuf = new llvm::GlobalVariable(
        *module,
        getPtr(),
        false,
        llvm::GlobalValue::ExternalLinkage,
        llvm_nullptr
    );
    jmpBufStack.push(exceptionJmpBuf);

    llvm::Value* jmp = initBuilder.CreateCall(getExceptionalFunction(SETJMP_FN), {exceptionJmpBuf});

    initBuilder.CreateCondBr(
        initBuilder.CreateICmpEQ(jmp, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0)),
        ENTRY_BLOCK_PLACEHOLDER,
        panicBB
    );

    initBuilder.SetInsertPoint(panicBB);

    // TODO: Print the exception message
    // TODO: Call abort()
    initBuilder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1));

    initBuilder.SetInsertPoint(initBlock);
}

void CodeGen::generateMainFunction() {
    // Create the entry block for the main function
    builder.SetInsertPoint(ENTRY_BLOCK_PLACEHOLDER);

    // Call the Main_main function
    llvm::Function* mainMainFunc = module->getFunction("Main_main");
    if (!mainMainFunc) {
        throw std::runtime_error("Cannot access static function \"main\" from class \"Main\"");
    }
    llvm::FunctionType* mainMainFuncType = mainMainFunc->getFunctionType();
    if (!mainMainFuncType) {
        throw std::runtime_error("Cannot access static function \"main\" from class \"Main\"");
    }
    if (mainMainFuncType->getNumParams() != 0) {
        throw std::runtime_error("Static function \"main\" from class \"Main\" should not have parameters");
    }
    llvm::Value* mainMainRet = builder.CreateCall(mainMainFuncType, mainMainFunc, {});
    llvm::Type* mainMainRetType = mainMainFuncType->getReturnType();

    // Return the value from Main_main as the return value of main
    if (mainMainRetType->isVoidTy()) {
        builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0));
    } else if (mainMainRetType == llvm::Type::getInt32Ty(context)) {
        builder.CreateRet(mainMainRet);
    } else {
        throw std::runtime_error("Static function \"main\" from class \"Main\" should return int or void");
    }
    initBuilder.CreateBr(EXC_HANDLE_BLOCK_PLACEHOLDER);
}

void CodeGen::emit(const parser::aNode& node) {
    try {
        if (!node) return;

        // Dispatch AST nodes to specific handlers
        switch (node->kind) {
            case parser::AST_PACKAGE:
                emitPackage(node);
                break;
            case parser::AST_EXPRESSION: {
                aValue value = emitExpression(node);
                break;
            }
            case parser::AST_COMPOUND_STATEMENT: { // emitCompoundStatement
                std::unordered_map<std::string, aValue> oldValues = values;
                for(const auto& [name, value] : values) {
                    value->currentScope = false;
                }
                for(const auto& child : node->children)
                    emit(child);
                for(const auto& [name, value] : values) {
                    if(oldValues.count(name) == 0) {
                        if(value->isObject()) {
                            llvm::Value* isNotNull = builder.CreateIsNotNull(value->getValue());
                            llvm::BasicBlock* ifBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                            llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                            builder.CreateCondBr(isNotNull, elseBlock, ifBlock);
                            builder.SetInsertPoint(ifBlock);
                            callFunc(value->getTypeName(), "__del__", {value});
                            builder.CreateBr(elseBlock);
                            builder.SetInsertPoint(elseBlock);
                        }
                    }
                }
                values = oldValues;
                break;
            }
            case parser::AST_VARIABLE_DECLARATION:
                emitVariableDeclaration(node);
                break;
            case parser::AST_RETURN_STATEMENT:
                emitReturn(node);
                break;
            case parser::AST_IF_STATEMENT:
                emitIfStatement(node);
                break;
            case parser::AST_WHILE_STATEMENT:
                emitWhileStatement(node);
                break;
            case parser::AST_DO_WHILE_STATEMENT:
                emitDoWhileStatement(node);
                break;
            case parser::AST_FOR_STATEMENT:
                emitForStatement(node);
                break;
            case parser::AST_BREAK_STATEMENT:
                builder.CreateBr(breakStack.top());
                break;
            case parser::AST_CONTINUE_STATEMENT:
                builder.CreateBr(continueStack.top());
                break;
            case parser::AST_LABEL_STATEMENT:
                emitLabelStatement(node);
                break;
            case parser::AST_GOTO_STATEMENT:
                emitGotoStatement(node);
                break;
            case parser::AST_SWITCH_STATEMENT:
                emitSwitchStatement(node);
                break;
            case parser::AST_TRY_STATEMENT:
                emitTryStatement(node);
                break;
            case parser::AST_THROW_STATEMENT:
                emitThrowStatement(node);
                break;
            default:
                throw std::runtime_error("Unknown AST node kind: " + std::to_string(static_cast<int>(node->kind)));
        }
    } catch (std::exception& e) {
        // Handle compilation errors
        compilation_error(mainCompiler.current_filename.data(), node->line, node->column, e.what());
    }
}

void CodeGen::emitPackage(const parser::aNode& node) {
    // Header
    for (const auto& child : node->children){
        if(child->kind == parser::AST_CAST_FUNCTION_DECLARATION)
            emitCastFunctionHeader(child);
        else if(child->kind == parser::AST_CLASS_DECLARATION)
            emitClassHeader(child);
    }

    for(auto& [name, cls] : classes) {
        ClassMem* mem = new ClassMem(VARIABLE, STATIC_PUBLIC, name, "long");
        classes["Types"].push_back(mem);
        new llvm::GlobalVariable(
            *module,
            llvm::Type::getInt64Ty(context),
            true,
            llvm::GlobalValue::ExternalLinkage,
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), llvm::hash_value(types[name])),
            "Types_"+name
        );
    }

    // Function
    for (const auto& child : node->children){
        if(child->kind == parser::AST_CAST_FUNCTION_DECLARATION)
            emitCastFunction(child);
        else if(child->kind == parser::AST_CLASS_DECLARATION)
            emitClass(child);
    }
}

void CodeGen::emitCastFunction(const parser::aNode& node) {
    std::string funcName = getTypeStr(node->children[0])+"_"+getTypeStr(node->children[1]);
    llvm::Function* func = module->getFunction("cast_" + funcName);
    currentFunction = functions[funcName];
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);
    builder.SetInsertPoint(entry);
    values.clear();
    values["__source__"] = symbols::create_value(func->getArg(0), getTypeStr(node->children[1]));
    emit(node->children[2]);

    for (llvm::BasicBlock& block : *func) {
        if(block.getTerminator()) continue;
        llvm::IRBuilder<> builder(&block, block.end());
        Terminate(builder);
    }
}

void CodeGen::emitClass(const parser::aNode& node) {
    currentClass = node->value;
    for (const auto& child : node->children) {
        if (child->kind == parser::AstKind::AST_CLASS_PARENTS)
            continue; // Skip parent class declarations
        if(child->children[0]->kind == parser::AST_FUNCTION_DECLARATION)
            emitFunctionDeclaration(child->children[0]);
        else if(child->children[0]->kind == parser::AST_STATIC_FUNCTION_DECLARATION)
            emitFunctionDeclaration(child->children[0]);
    }
}

void CodeGen::emitFunctionDeclaration(const parser::aNode& node) {
    std::string funcName = currentClass+"_"+node->children[1]->value;
    if(node->children[1]->kind == parser::AST_OPERATOR) {
        std::string opt = node->children[1]->value;
        // Unary operators (Do nothing for "+" and "-")
        if(opt == "++" || opt == "--" || opt == "~" || opt == "!") {
            if(node->children[2]->children.size() != 0)
                throw std::runtime_error("Invalid number of parameters for unary operator: "+opt);
        }
        // "+" and "-" are special cases
        else if(opt == "+" || opt == "-") {
            if(opt == "+") {
                if(node->children[2]->children.size() > 2)
                    throw std::runtime_error("Invalid number of parameters for operator: "+opt);
                // Special case as binary operator
                if(node->children[2]->children.size() == 2) {
                    funcName = funcName + getTypeStr(node->children[2]->children[0]);
                }
            }
            else if(opt == "-") {
                if(node->children[2]->children.size() != 0 && node->children[2]->children.size() != 4)
                    throw std::runtime_error("Invalid number of parameters for operator: "+opt);
                // Special case as binary operator
                if(node->children[2]->children.size() == 4) {
                    funcName = funcName + getTypeStr(node->children[2]->children[0]);
                    if(getTypeStr(node->children[2]->children[2]) != "int")
                        throw std::runtime_error("Expected LHS/RHS flag for binary operator: "+opt);
                }
            }
        }
        // Directionless binary operators
        else if(opt == "*" || opt == "&" || opt == "|" || opt == "^" || opt == "&&" || opt == "||" || opt == "==" || opt == "!=") {
            if(node->children[2]->children.size() != 2) {
                throw std::runtime_error("Invalid number of parameters for binary operator: "+opt);
            }
            funcName = funcName + getTypeStr(node->children[2]->children[0]);
        }
        // Directional binary operators
        else if(opt == ">>" || opt == "<<" || opt == "<=" || opt == ">=" || opt == "<" || opt == ">" 
                || opt == "/" || opt == "%") {
            if(node->children[2]->children.size() != 4) {
                throw std::runtime_error("Invalid number of parameters for binary operator: "+opt);
            }
            funcName = funcName + getTypeStr(node->children[2]->children[0]);
            if(getTypeStr(node->children[2]->children[2]) != "int") {
                throw std::runtime_error("Expected LHS/RHS flag for binary operator: "+opt);
            }
        }
    }
    llvm::Function* func = module->getFunction(funcName);
    currentFunction = functions[funcName];
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);
    builder.SetInsertPoint(entry);
    int argsSize = func->getFunctionType()->getNumParams();
    values.clear();
    labels.clear();
    for(int i=0; i<argsSize; i++) {
        Argument* arg = functions[funcName]->arguments[i];
        values[arg->name] = symbols::create_value(func->getArg(i), arg->type);
    }
    if(node->children[1]->value == "__init__") builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), builder.CreateStructGEP(types[currentClass], func->getArg(0), 0));
    emit(node->children[3]);
    for(const auto& [name, label] : labels)
        if(label.first == LABEL_PLACEHOLDER)
            throw std::runtime_error("Label not found: "+name);
    
    if(node->children[1]->value == "__del__") {
        llvm::BasicBlock* release = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
        llvm::BasicBlock* exit = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());

        llvm::Value* no_reference = builder.CreateICmpEQ(
            builder.CreateLoad(
                llvm::Type::getInt32Ty(context),
                builder.CreateStructGEP(types[currentClass], func->getArg(0), 0)
            ),
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0)
        );

        builder.CreateCondBr(no_reference, release, exit);

        builder.SetInsertPoint(release);
        for(auto& mem : classes[currentClass]) {
            std::string typeName = mem->typeName;
            if(mem->type == VARIABLE && mem->access < STATIC && 
                !(typeName == "int" || typeName == "short" || typeName == "long" || 
                typeName == "char" || typeName == "void" || typeName == "float" ||
                typeName == "double" || typeName == "bool" || typeName == "byte" || typeName == "")) {
                aValue value = symbols::create_value(builder.CreateLoad(getType(mem->typeName), builder.CreateStructGEP(types[currentClass], func->getArg(0), mem->varIndex)), typeName);
                llvm::Value* isNull = builder.CreateICmpEQ(value->getValue(), llvm_nullptr);
                llvm::BasicBlock* ifBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                builder.CreateCondBr(isNull, elseBlock, ifBlock);
                builder.SetInsertPoint(ifBlock);
                callFunc(value->getTypeName(), "__del__", {value});
                builder.CreateBr(elseBlock);
                builder.SetInsertPoint(elseBlock);
            }
        }
        builder.CreateFree(func->getArg(0));

        builder.CreateBr(exit);

        builder.SetInsertPoint(exit);
        builder.CreateStore(
            builder.CreateSub(
                builder.CreateLoad(
                    llvm::Type::getInt32Ty(context),
                    builder.CreateStructGEP(types[currentClass], func->getArg(0), 0)
                ),
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1)
            ),
            builder.CreateStructGEP(types[currentClass], func->getArg(0), 0)
        );
        builder.CreateRetVoid();
    }

    for (llvm::BasicBlock& block : *func) {
        if(block.getTerminator()) continue;
        llvm::IRBuilder<> builder(&block, block.end());
        Terminate(builder);
    }
    
    if(node->children.back()->kind == parser::AST_INLINE_TAG)
        func->addFnAttr(llvm::Attribute::AlwaysInline);
}

void CodeGen::emitVariableDeclaration(const parser::aNode& node) {
    for(int i=1;i<node->children.size();i++){
        defineConst = defineVolatile = false;
        std::string varName = node->children[i]->value;
        std::string typeName = getTypeStr(node->children[0]);
        if(values.count(varName) > 0 && values[varName]->currentScope)
            throw std::runtime_error("Conflict definition for variable: "+varName);
        llvm::Type* type = getType(node->children[0]);
        // Handle initialized variables
        if (node->children[i]->children.size() > 0) {
            aValue initValue = cast(emitExpression(node->children[i]->children[0]), typeName, templateObjects);
            llvm::Value* alloca_var = builder.CreateAlloca(type, nullptr, varName);
            builder.CreateStore(initValue->getValue(), alloca_var, defineVolatile);
            values[varName] = symbols::create_value(alloca_var, typeName);
            values[varName]->set(true, defineConst, defineVolatile);
            values[varName]->id = initValue->id;
            if(templateObjects.size() > 0) values[varName]->genericMem = templateObjects;
        } else {
            // Handle uninitialized variables
            llvm::Value* alloca_var = builder.CreateAlloca(type, nullptr, varName);
            builder.CreateStore(llvm::Constant::getNullValue(type), alloca_var, defineVolatile);
            values[varName] = symbols::create_value(alloca_var, typeName);
            values[varName]->set(true, defineConst, defineVolatile);
            if(templateObjects.size() > 0) values[varName]->genericMem = templateObjects;
        }
    }
}

void CodeGen::emitCastFunctionHeader(const parser::aNode& node) {
    currentClass = "cast";

    std::string funcName = getTypeStr(node->children[0])+"_"+getTypeStr(node->children[1]);
    if(functions.count(funcName) > 0)
        throw std::runtime_error("Conflict definition for cast function: "+getTypeStr(node->children[1])+" to "+getTypeStr(node->children[0]));
    
    llvm::Type* retType = getType(node->children[0]);
    llvm::Type* paramType = getType(node->children[1]);
    
    Function* function = new Function("cast_" + funcName, getTypeStr(node->children[0]));
    functions[funcName] = function;
    function->genericMem = templateObjects;

    ClassMem* mem = new ClassMem(FUNCTION, STATIC_PUBLIC, funcName, getTypeStr(node->children[0]));
    classes[currentClass].push_back(mem);

    llvm::FunctionType* funcType = llvm::FunctionType::get(retType, {paramType}, false);
    llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "cast_" + funcName, module.get());
    func->getArg(0)->setName("__source__");
}

void CodeGen::emitClassHeader(const parser::aNode& node) {
    currentClass = node->value;
    std::vector<llvm::Type*> elements;
    inhers[currentClass] = {}; // Initialize the class inheritance map
    elements.push_back(llvm::Type::getInt32Ty(context));
    for (const auto& child : node->children) {
        if (child->kind == parser::AstKind::AST_CLASS_PARENTS) {
            for (const auto& parent : child->children) {
                if(classes.count(parent->value) == 0)
                    throw std::runtime_error("Class \""+parent->value+"\" as a base of \""+currentClass+"\" not found");
                inhers[currentClass].push_back(parent->value);
            }
            continue;
        }
        if (child->children[0]->kind == parser::AST_VARIABLE_DECLARATION) {
            // Add the type of the member variable
            elements.push_back(getType(child->children[0]->children[0]));
        }
    }

    for(const auto& parent : inhers[currentClass]) {
        elements.push_back(getPtr());
    }

    // Create a struct type for the class
    llvm::Type* classType = llvm::StructType::create(context, elements, currentClass);
    classes[currentClass] = std::deque<ClassMem*>(0); // Initialize the class member map
    types[currentClass] = classType;
    currentClassMemIndex = 1+inhers[currentClass].size(); // Reset the variable index
    
    for (const auto& child : node->children) {
        if (child->kind == parser::AstKind::AST_CLASS_PARENTS) 
            continue;
        Access access = PUBLIC;
        switch(child->kind) {
            case parser::AST_PUBLIC:
                access = PUBLIC;
                break;
            case parser::AST_PRIVATE:
                access = PRIVATE;
                break;
            case parser::AST_PROTECTED:
                access = PROTECTED;
                break;
            case parser::AST_SHARED:
                access = SHARED;
                break;
        }
        if(child->children[0]->kind == parser::AST_FUNCTION_DECLARATION)
            emitFunctionHeader(child->children[0], access);
        else if(child->children[0]->kind == parser::AST_VARIABLE_DECLARATION)
            emitVariableHeader(child->children[0], access);
        else if(child->children[0]->kind == parser::AST_STATIC_FUNCTION_DECLARATION)
            emitFunctionHeader(child->children[0], Access(access + STATIC));
        else if(child->children[0]->kind == parser::AST_STATIC_VARIABLE_DECLARATION)
            emitVariableHeader(child->children[0], Access(access + STATIC));
    }
    if(!funcExists(currentClass, "__del__")) {
        llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), {llvm::PointerType::get(getType(currentClass), 0)}, false);
        llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, currentClass + "___del__", module.get());
        functions[currentClass + "___del__"] = new Function(currentClass + "___del__", "void");
        functions[currentClass + "___del__"]->addArgument("self", currentClass);
        llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);
        llvm::BasicBlock* release = llvm::BasicBlock::Create(context, "release", func);
        llvm::BasicBlock* exit = llvm::BasicBlock::Create(context, "exit", func);
        builder.SetInsertPoint(entry);
        
        // Reference checker
        llvm::Value* no_reference = builder.CreateICmpEQ(
            builder.CreateLoad(
                llvm::Type::getInt32Ty(context),
                builder.CreateStructGEP(types[currentClass], func->getArg(0), 0)
            ),
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0)
        );

        builder.CreateCondBr(no_reference, release, exit);

        builder.SetInsertPoint(release);
        for(auto& mem : classes[currentClass]) {
            std::string typeName = mem->typeName;
            if(mem->type == VARIABLE && mem->access < STATIC && 
                !(typeName == "int" || typeName == "short" || typeName == "long" || 
                typeName == "char" || typeName == "void" || typeName == "float" ||
                typeName == "double" || typeName == "bool" || typeName == "byte" || typeName == "")) {
                aValue value = symbols::create_value(builder.CreateLoad(getType(mem->typeName), builder.CreateStructGEP(types[currentClass], func->getArg(0), mem->varIndex)), typeName);
                llvm::Value* isNull = builder.CreateICmpEQ(value->getValue(), llvm_nullptr);
                llvm::BasicBlock* ifBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                builder.CreateCondBr(isNull, elseBlock, ifBlock);
                builder.SetInsertPoint(ifBlock);
                callFunc(value->getTypeName(), "__del__", {value});
                builder.CreateBr(elseBlock);
                builder.SetInsertPoint(elseBlock);
            }
        }
        builder.CreateFree(func->getArg(0));

        for(int i=1;i<=inhers[currentClass].size();i++) {
            callFunc(inhers[currentClass][i - 1], "__del__", {
                symbols::create_value(builder.CreateLoad(
                    getPtr(), builder.CreateStructGEP(types[currentClass], func->getArg(0), i)
                ), inhers[currentClass][i - 1])
            });
        }

        builder.CreateBr(exit);

        builder.SetInsertPoint(exit);
        builder.CreateStore(
            builder.CreateSub(
                builder.CreateLoad(
                    llvm::Type::getInt32Ty(context),
                    builder.CreateStructGEP(types[currentClass], func->getArg(0), 0)
                ),
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1)
            ),
            builder.CreateStructGEP(types[currentClass], func->getArg(0), 0)
        );
        builder.CreateRetVoid();
    }

    if(!funcExists(currentClass, "__init__")) {
        llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), {llvm::PointerType::get(getType(currentClass), 0)}, false);
        llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, currentClass + "___init__", module.get());
        functions[currentClass + "___init__"] = new Function(currentClass + "___init__", "void");
        functions[currentClass + "___init__"]->addArgument("self", currentClass);
        llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);
        builder.SetInsertPoint(entry);

        for(int i=1;i<=inhers[currentClass].size();i++) {
            if(functions[inhers[currentClass][i - 1] + "___init__"]->minArgumentCount == 0) {
                callFunc(inhers[currentClass][i - 1], "__init__", {
                    symbols::create_value(builder.CreateLoad(
                        getPtr(), builder.CreateStructGEP(types[currentClass], func->getArg(0), i)
                    ), inhers[currentClass][i - 1])
                });
            } else {
                throw std::runtime_error("Invalid number of parameters for constructor by base class: " + inhers[currentClass][i - 1] + " from " + currentClass);
            }
        }

        builder.CreateRetVoid();
    }

    for(auto& base : inhers[currentClass]) {
        for(auto& element : classes[base]) {
            if(element->name == "__init__" || element->name == "__del__")
                continue;
            bool found = false;
            for(auto& mem : classes[currentClass]) {
                if(mem->name == element->name)
                    found = true;
            }
            if(!found) {
                codegen::ClassMem* elementCopy = new codegen::ClassMem(*element);
                elementCopy->from = std::distance(inhers[currentClass].begin(), std::find(inhers[currentClass].begin(), inhers[currentClass].end(), base))+1;
                classes[currentClass].push_back(elementCopy);
            }
        }
    }
}

void CodeGen::emitFunctionHeader(const parser::aNode& node, Access access) {
    std::string funcName = node->children[1]->value;
    if(node->children[1]->kind == parser::AST_OPERATOR) {
        std::string opt = node->children[1]->value;
        // Unary operators (Do nothing for "+" and "-")
        if(opt == "++" || opt == "--" || opt == "~" || opt == "!") {
            if(node->children[2]->children.size() != 0)
                throw std::runtime_error("Invalid number of parameters for unary operator: "+opt);
        }
        // "+" and "-" are special cases
        else if(opt == "+" || opt == "-") {
            if(opt == "+") {
                if(node->children[2]->children.size() > 2)
                    throw std::runtime_error("Invalid number of parameters for operator: "+opt);
                // Special case as binary operator
                if(node->children[2]->children.size() == 2) {
                    funcName = funcName + getTypeStr(node->children[2]->children[0]);
                }
            }
            else if(opt == "-") {
                if(node->children[2]->children.size() != 0 && node->children[2]->children.size() != 4)
                    throw std::runtime_error("Invalid number of parameters for operator: "+opt);
                // Special case as binary operator
                if(node->children[2]->children.size() == 4) {
                    funcName = funcName + getTypeStr(node->children[2]->children[0]);
                    if(getTypeStr(node->children[2]->children[2]) != "int")
                        throw std::runtime_error("Expected LHS/RHS flag for binary operator: "+opt);
                }
            }
        }
        // Directionless binary operators
        else if(opt == "*" || opt == "&" || opt == "|" || opt == "^" || opt == "&&" || opt == "||" || opt == "==" || opt == "!=") {
            if(node->children[2]->children.size() != 2) {
                throw std::runtime_error("Invalid number of parameters for binary operator: "+opt);
            }
            funcName = funcName + getTypeStr(node->children[2]->children[0]);
        }
        // Directional binary operators
        else if(opt == ">>" || opt == "<<" || opt == "<=" || opt == ">=" || opt == "<" || opt == ">" 
                || opt == "/" || opt == "%") {
            if(node->children[2]->children.size() != 4) {
                throw std::runtime_error("Invalid number of parameters for binary operator: "+opt);
            }
            funcName = funcName + getTypeStr(node->children[2]->children[0]);
            if(getTypeStr(node->children[2]->children[2]) != "int") {
                throw std::runtime_error("Expected LHS/RHS flag for binary operator: "+opt);
            }
        }
    }

    llvm::Type* retType = getType(node->children[0]);

    if(funcName == "__init__" || funcName == "__del__") {
        if(access > 4)
            throw std::runtime_error("Constructor and destructor cannot be static.");
        if(access != 0)
            throw std::runtime_error("Constructor and destructor cannot be private, protected, or shared.");
        if(retType != llvm::Type::getVoidTy(context))
            throw std::runtime_error("Constructor and destructor cannot have a return type.");
    }

    if(funcName == "__del__") {
        if(node->children[2]->children.size() != 0)
            throw std::runtime_error("Destructor cannot have parameters.");
    }

    if(functions.count(funcName) > 0)
        throw std::runtime_error("Conflict definition for function: "+currentClass+"."+funcName);
    
    
    Function* function = new Function(currentClass+"_"+funcName, getTypeStr(node->children[0]));
    functions[currentClass+"_"+funcName] = function;
    function->genericMem = templateObjects;

    std::vector<std::string> paramTypeNames;
    std::vector<std::string> paramNames;
    struct qualifiers{
        bool isConstant = false;
        bool isVolatile = false;
    };

    if (access < STATIC) {
        paramTypeNames.push_back(currentClass);
        paramNames.push_back("self");
        function->addArgument("self", currentClass);
    }

    bool hasEllipsis = false;
    for (int i = 0; i < node->children[2]->children.size(); i += 2) {
        if(node->children[2]->children[i]->kind != parser::AstKind::AST_ELLIPSIS_PARAMETER) {
            paramTypeNames.push_back(getTypeStr(node->children[2]->children[i]));
            paramNames.push_back(node->children[2]->children[i + 1]->value);
            if(node->children[2]->children[i + 1]->children.size() > 0) {
                aValue defaultVar = emitExpression(node->children[2]->children[i + 1]->children[0]);
                function->addArgument(node->children[2]->children[i + 1]->value, getTypeStr(node->children[2]->children[i]), cast(defaultVar, getTypeStr(node->children[2]->children[i]), templateObjects)->getValue());
            }
            else
                function->addArgument(node->children[2]->children[i + 1]->value, getTypeStr(node->children[2]->children[i]));
            if(templateObjects.size() > 0) function->arguments.back()->genericMem = templateObjects;
        } else {
            hasEllipsis = true;
            break;
        }
    }

    function->hasEllipsis = hasEllipsis;
    std::vector<llvm::Type*> paramTypes;
    for (const auto& paramTypeName : paramTypeNames) {
        paramTypes.push_back(getType(paramTypeName));
    }

    llvm::FunctionType* funcType = llvm::FunctionType::get(retType, paramTypes, hasEllipsis);
    llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, currentClass + "_" + funcName, module.get());

    ClassMem* mem = new ClassMem(FUNCTION, access, funcName, getTypeStr(node->children[0]));
    classes[currentClass].push_back(mem);

    for (int i = 0; i < paramNames.size(); i++) {
        func->getArg(i)->setName(paramNames[i]);
    }
    func->setCallingConv(llvm::CallingConv::C);
}

void CodeGen::emitVariableHeader(const parser::aNode& node, Access access) {
    for (int i = 1; i < node->children.size(); i++) {
        defineConst = defineVolatile = false;
        std::string varName = node->children[i]->value;
        std::string typeName = getTypeStr(node->children[0]);
        if (access < STATIC && node->children[i]->children.size() > 0) {
            throw utils::SyntaxErrorException("Cannot declare non-static member variable with initializer in class declaration");
        }
        if (access >= STATIC && node->children[i]->children.size() == 0) {
            compilation_warning(mainCompiler.current_filename.data(), node->children[i]->line, node->children[i]->column, "Not recommended to declare static member variable without initializer in class declaration");
        }
        if (access >= 4) {
            // Handle static member variables
            aValue initValue = nullptr;
            if(node->children[i]->children.size() > 0) initValue = cast(emitExpression(node->children[i]->children[0]), typeName, templateObjects);
            llvm::GlobalVariable* globalVar = new llvm::GlobalVariable(
                *module,                       // Module to which the global variable belongs
                getType(typeName),             // Type of the global variable
                false,                         // Is the global variable constant?
                llvm::GlobalValue::ExternalLinkage, // Linkage type
                nullptr,                     // Initial value (can be nullptr)
                llvm::Twine(currentClass + "_" + varName),          // Name of the global variable
                nullptr,                       // Insert before this global variable (default is nullptr)
                llvm::GlobalValue::NotThreadLocal, // Thread-local mode
                std::nullopt,                  // Address space (default is std::nullopt)
                false                          // Is externally initialized? (default is false)
            );
            if(node->children[i]->children.size() > 0) {
                initBuilder.CreateStore(initValue->getValue(), globalVar, defineVolatile);
            }
        }
        ClassMem* mem = new ClassMem(VARIABLE, access, varName, getTypeStr(node->children[0]));
        if(templateObjects.size() > 0) mem->genericMem = templateObjects;
        mem->isConstant = defineConst;
        mem->isVolatile = defineVolatile;
        if(access < STATIC) mem->varIndex = currentClassMemIndex++;
        classes[currentClass].push_back(mem);
    }
}

aValue CodeGen::emitExpression(const parser::aNode& node) {
    if (node->kind == parser::AST_PRIMARY_EXPRESSION || node->kind == parser::AST_NEW || node->kind == parser::AST_SUPER || node->kind == parser::AST_POSTFIX_EXPRESSION && node->children.size() <= 1) {
        return emitPrimaryExpression(node);
    } else if (node->kind == parser::AST_POSTFIX_EXPRESSION) {
        return emitPostfixExpression(nullptr, node);
    } else if (node->kind == parser::AST_UNARY_EXPRESSION) {
        return emitUnaryExpression(node);
    } else if (node->kind == parser::AST_CAST_EXPRESSION) {
        return emitCastExpression(node);
    } else if (node->kind == parser::AST_MULTIPLICATIVE_EXPRESSION) {
        return emitMultiplicativeExpression(node);
    } else if (node->kind == parser::AST_ADDITIVE_EXPRESSION) {
        return emitAdditiveExpression(node);
    } else if (node->kind == parser::AST_SHIFT_EXPRESSION) {
        return emitShiftExpression(node);
    } else if (node->kind == parser::AST_RELATIONAL_EXPRESSION) {
        return emitRelationalExpression(node);
    } else if (node->kind == parser::AST_EQUALITY_EXPRESSION) {
        return emitEqualityExpression(node);
    } else if (node->kind == parser::AST_AND_EXPRESSION) {
        return emitAndExpression(node);
    } else if (node->kind == parser::AST_EXCLUSIVE_OR_EXPRESSION) {
        return emitExclusiveOrExpression(node);
    } else if (node->kind == parser::AST_INCLUSIVE_OR_EXPRESSION) {
        return emitInclusiveOrExpression(node);
    } else if (node->kind == parser::AST_LOGICAL_AND_EXPRESSION) {
        return emitLogicalAndExpression(node);
    } else if (node->kind == parser::AST_LOGICAL_OR_EXPRESSION) {
        return emitLogicalOrExpression(node);
    } else if (node->kind == parser::AST_CONDITIONAL_EXPRESSION) {
        return emitConditionalExpression(node);
    } else if (node->kind == parser::AST_ASSIGNMENT_EXPRESSION) {
        return emitAssignmentExpression(node);
    } else {
        aValue head = nullptr;
        for(int i = 0; i < node->children.size(); i++) {
            head = i == 0? emitExpression(node->children[i]) : head;
        }
        return head;
    }
}

aValue CodeGen::emitPrimaryExpression(const parser::aNode& node) {
    switch (node->token_kind) {
        case lexer::TokenKind::TOKEN_INT_LITERAL:
            return symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), std::stoi(node->value), true), "int", std::stoi(node->value));
        case lexer::TokenKind::TOKEN_FLOAT_LITERAL:
            return symbols::create_value(llvm::ConstantFP::get(llvm::Type::getFloatTy(context), std::stof(node->value)), "float", std::stof(node->value));
        case lexer::TokenKind::TOKEN_STRING_LITERAL:
            return getString(node->value.substr(1, node->value.size() - 2));
        case lexer::TokenKind::TOKEN_CHAR_LITERAL:
            return symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), node->value[1], true), "char", node->value[1]);
        case lexer::TokenKind::TOKEN_TRUE:
            return symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), 1, true), "bool", 1);
        case lexer::TokenKind::TOKEN_FALSE:
            return symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), 0, true), "bool", 0);
        case lexer::TokenKind::TOKEN_NULL:
            return symbols::create_value(llvm::ConstantPointerNull::get(llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0)), "null");
        case lexer::TokenKind::TOKEN_LPAREN:
            return emitExpression(node->children[0]);
        case lexer::TokenKind::TOKEN_SELF:
            if(values.count("self")==0)
                throw utils::InvalidExpressionException("Trying to access self outside of a non-static function" + node->value);
            return values["self"];
        case lexer::TokenKind::TOKEN_NEW:
            return emitNew(node);
        case lexer::TokenKind::TOKEN_SUPER:
            return emitSuper(node);
        case lexer::TokenKind::TOKEN_IDENTIFIER:
            if(values.count(node->value)>0)
                return values[node->value];
            else if(classes.count(node->value)>0)
                return symbols::create_value(llvm::ConstantPointerNull::get((llvm::PointerType*)getPtr()), "null");
            else
                throw utils::InvalidExpressionException("Trying to access a non-declared variable " + node->value);
    }
}

aValue CodeGen::emitNew(const parser::aNode& node) {
    std::string typeName = "";
    typeName = typeName + node->children[0]->children[0]->value;
    if (typeName == "cast" || isSubclassOf(typeName, "Generic"))
        throw utils::InvalidExpressionException("Cannot create an instance of a generic/cast type");
    if (classes.count(typeName) == 0) {
        throw utils::InvalidExpressionException("Unknown class: " + typeName);
    }
    llvm::Value* instance = builder.CreateMalloc(llvm::Type::getInt32Ty(context),
        types[typeName], 
        llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(context),
            DL.getTypeAllocSize(types[typeName])
        ),
        nullptr
    );
    aValue s_ins = symbols::create_value(instance, typeName);
    // Constructor call
    if(funcExists(typeName, "__init__")){
        std::vector<aValue> args;
        args.push_back(s_ins);
        for (int i = 0; i < node->children[0]->children[1]->children.size(); i++) {
            args.push_back(cast(emitExpression(node->children[0]->children[1]->children[i]), functions[typeName + "___init__"]->arguments[i+1]->type, functions[typeName + "___init__"]->arguments[i+1]->genericMem));
        }
        if(args.size() < functions[typeName+"___init__"]->arguments.size()){
            for (int i = args.size(); i < functions[typeName+"___init__"]->arguments.size(); i++) {
                if(!functions[typeName+"___init__"]->arguments[i]->defaultValue) continue;
                aValue arg = symbols::create_value(functions[typeName+"___init__"]->arguments[i]->defaultValue, functions[typeName+"___init__"]->arguments[i]->type);
                args.push_back(arg);
            }
        }
        if(args.size() < functions[typeName+"___init__"]->minArgumentCount || args.size() > functions[typeName+"___init__"]->maxArgumentCount)
            throw utils::InvalidExpressionException("Invalid number of arguments for constructor of " + typeName);
        callFunc(typeName, "__init__", args);
    } else {
        builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), builder.CreateStructGEP(types[typeName], instance, 0));
    }
    values[std::to_string(getUniqueId())] = s_ins;
    s_ins->set(false, true, false);
    return s_ins;
}

aValue CodeGen::emitSuper(const parser::aNode& node) {
    aValue obj = nullptr;
    std::string derived = "";
    std::string base = "";
    if(node->children.size() == 0 || values.count(node->children[0]->value)==0)
        if(values.count("self")==0)
            throw utils::InvalidExpressionException("Trying to access super from nothing");
        else
            obj = values["self"];
    else
        if(values.count(node->children[0]->value) != 0)
            obj = values[node->children[0]->value];
        else
            throw utils::InvalidExpressionException("Trying to access super from nothing");
    if(!obj->isObject())
        throw utils::InvalidExpressionException("Trying to access super from a non-object");
    derived = obj->getTypeName();
    if(inhers[derived].size() == 0)
        throw utils::InvalidExpressionException("Trying to access super from an independent object");
    if(node->children.size() == 0 || (node->children.size() == 1 && values.count(node->children[0]->value)!=0))
        base = inhers[derived][0];
    else
        if (node->children.size() == 1 && values.count(node->children[0]->value)==0)
            if(std::find(inhers[derived].begin(), inhers[derived].end(), node->children[0]->value) == inhers[derived].end())
                throw utils::InvalidExpressionException("Trying to access super from a non-existing base class");
            else
                base = node->children[0]->value;
        else if(std::find(inhers[derived].begin(), inhers[derived].end(), node->children[1]->value) == inhers[derived].end())
            throw utils::InvalidExpressionException("Trying to access super from a non-existing base class");
        else
            base = node->children[1]->value;
    int inher_id = std::distance(inhers[derived].begin(), std::find(inhers[derived].begin(), inhers[derived].end(), base))+1;
    llvm::Value* val = builder.CreateStructGEP(types[derived], obj->getValue(), inher_id);
    symbols::aValue ret = symbols::create_value(val, base); 
    ret->isSuper = true;
    return ret;
}

aValue CodeGen::emitPostfixExpression(aValue lhs, const parser::aNode& node) {
    // Handle postfix expressions with operators

    if(node->kind != parser::AST_POSTFIX_EXPRESSION || node->children.size() == 1)
        return lhs;
    if(!lhs) lhs = emitExpression(node->children[0]);

    if (node->children[1]->value == ".") {
        std::pair<aValue, const parser::aNode&> pair = emitMemberAccess(node, lhs);
        return emitPostfixExpression(pair.first, pair.second);   
    } else if (node->children[1]->value == "(") {
        // Handle function call
        llvm::Function* func = llvm::dyn_cast<llvm::Function>(lhs->getValue());
        if(!func)
            throw utils::InvalidExpressionException("Not a function: " + node->children[0]->value);
        std::vector<llvm::Value*> args;
        std::vector<symbols::aValue> _args;
        bool nonStatic = func->getFunctionType()->getNumParams() > 0 && func->getArg(0)->getName().str() == "self";
        if(nonStatic)
            args.push_back(self);
        if(node->children.size() != 2)
            for(int i = 0; i < node->children[2]->children.size(); i++) {
                _args.push_back(cast(emitExpression(node->children[2]->children[i]), functions[(std::string)func->getName()]->arguments[i+nonStatic?1:0]->type, functions[(std::string)func->getName()]->arguments[i+nonStatic?1:0]->genericMem));
                args.push_back(_args.back()->getValue());
            }
        if(args.size() < functions[(std::string)func->getName()]->arguments.size()){
            for (int i = args.size(); i < functions[(std::string)func->getName()]->arguments.size(); i++) {
                if(!functions[(std::string)func->getName()]->arguments[i]->defaultValue) continue;
                args.push_back(functions[(std::string)func->getName()]->arguments[i]->defaultValue);
            }
        }
        if(args.size() < functions[(std::string)func->getName()]->minArgumentCount || args.size() > functions[(std::string)func->getName()]->maxArgumentCount)
            throw utils::InvalidExpressionException("Invalid number of arguments for function " + node->children[0]->value);
        
        llvm::Value* call = builder.CreateCall(func->getFunctionType(), func, args);
        aValue result = symbols::create_value(call, lhs->getTypeName());
        values[std::to_string(getUniqueId())] = result;
        result->set(false, true, false);
        return result;
    } else if (node->children[1]->value == "++" || node->children[1]->value == "--") {
        // Handle increment/decrement
        std::string type = lhs->getTypeName();
        if(!lhs->isVariable())
            throw utils::InvalidExpressionException("Not mutable: " + node->children[0]->value);
        if(!lhs->isNumeric())
            throw utils::InvalidExpressionException("Not numeric: " + node->children[0]->value);
        bool isInteger = (type == "int" || type == "long" || type == "short");
        if(node->children[1]->value == "++") {
            builder.CreateStore(
                isInteger?builder.CreateAdd(lhs->getValue(), llvm::ConstantInt::get(lhs->getValue()->getType(), 1)):
                builder.CreateFAdd(lhs->getValue(), llvm::ConstantFP::get(lhs->getValue()->getType(), 1.0)),
                lhs->getVariable(),
                lhs->isVolatile()
            );
        } else {
            builder.CreateStore(
                isInteger?builder.CreateSub(lhs->getValue(), llvm::ConstantInt::get(lhs->getValue()->getType(), 1)):
                builder.CreateFSub(lhs->getValue(), llvm::ConstantFP::get(lhs->getValue()->getType(), 1.0)),
                lhs->getVariable(),
                lhs->isVolatile()
            );
        }
        return lhs;
    }
}

std::pair<aValue, const parser::aNode&> CodeGen::emitMemberAccess(const parser::aNode& node, aValue lhs) {
    if(lhs->notNull() && lhs->isObject()) {
        // Handle non-static member access
        
        aValue obj = lhs;
        std::string recog = values.count(node->children[0]->value)>0&&values[node->children[0]->value]->id == obj->id?node->children[0]->value:"<TEMP>";
        std::string memberName = node->children[2]->value;
        if (!obj->isSuper && memberName == "__init__" || !obj->isSuper && memberName == "__del__" )
            throw utils::InvalidExpressionException("Trying to access " + (memberName=="__init__"?(std::string)"constructor":(std::string)"destructor") + " of object " + obj->getTypeName());
        std::string typeName = obj->getTypeName();
        aValue ret;
        self = obj->getValue();
        if (isSubclassOf(typeName, "Generic")) {
            if(std::find(obj->genericMem.begin(), obj->genericMem.end(), memberName) == obj->genericMem.end())
                throw utils::InvalidExpressionException("Trying to access non-existing member " + memberName + " of generic object " + recog);
            std::vector<std::string> availableTypes = {};
            for (const auto& [className, classDeque] : classes) {
                availableTypes.push_back(className);
            }
            for(auto& gen : obj->genericMem) {
                auto temp = availableTypes;
                for(auto& type : temp) {
                    bool notFound = true;
                    for(auto& mem : classes[type]) {
                        if(gen == mem->name && (mem->access == PUBLIC)) {
                            notFound = false;
                            break;
                        }
                    }
                    if(notFound) availableTypes.erase(std::remove(availableTypes.begin(), availableTypes.end(), type), availableTypes.end());
                }
            }
            if(availableTypes.size() == 0) {
                throw utils::InvalidExpressionException("Not even one object suitable for this generic object " + recog);
            }

            llvm::Value* generic = builder.CreateMalloc(llvm::Type::getInt32Ty(context),
                types["Generic"], 
                llvm::ConstantInt::get(
                    llvm::Type::getInt32Ty(context),
                    DL.getTypeAllocSize(types["Generic"])
                ),
                nullptr
            );

            builder.CreateStore(
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
                builder.CreateStructGEP(types["Generic"], generic, 0)
            );
            
            llvm::BasicBlock* unreachableBB = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
            llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
            llvm::IRBuilder<> tempBuilder(unreachableBB);
            tempBuilder.CreateUnreachable();
            llvm::SwitchInst* switchInst = builder.CreateSwitch(
                builder.CreateLoad(llvm::Type::getInt64Ty(context), builder.CreateStructGEP(types[typeName], obj->getValue(), 1)),
                unreachableBB,
                availableTypes.size()
            );

            for(auto& type : availableTypes) {
                llvm::BasicBlock* caseBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                switchInst->addCase(llvm::dyn_cast<llvm::ConstantInt>(module->getGlobalVariable("Types_"+type)->getInitializer()), caseBlock);
                builder.SetInsertPoint(caseBlock);
                symbols::aValue objTMP = obj;
                self = builder.CreateStructGEP(types["Generic"], obj->getValue(), 2);
                obj = symbols::create_value(self, type);
                typeName=type;

                for(auto& mem : classes[typeName]) {
                    if(mem->name == memberName) {
                        if(mem->from != 0) {
                            self = builder.CreateStructGEP(types[typeName], obj->getValue(), mem->from);
                            typeName = inhers[typeName][mem->from-1];
                        }
                        llvm::Value* nullJudge = builder.CreateIsNotNull(self);
                        llvm::BasicBlock* accessBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                        llvm::BasicBlock* throwBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                        builder.CreateCondBr(nullJudge, accessBlock, throwBlock);
                        builder.SetInsertPoint(throwBlock);
                        for(const auto& [name, value] : values) {
                            if(value->isObject()) {
                                llvm::Value* isNotNull = builder.CreateIsNotNull(value->getValue());
                                llvm::BasicBlock* ifBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                                llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                                builder.CreateCondBr(isNotNull, ifBlock, elseBlock);
                                builder.SetInsertPoint(ifBlock);
                                callFunc(value->getTypeName(), "__del__", {value});
                                builder.CreateBr(elseBlock);
                                builder.SetInsertPoint(elseBlock);
                            }
                        }
                        llvm::Value* exception = builder.CreateMalloc(llvm::Type::getInt32Ty(context),
                            types["Exception"], // TODO: NullException 
                            llvm::ConstantInt::get(
                                llvm::Type::getInt32Ty(context),
                                DL.getTypeAllocSize(types["Exception"])
                            ),
                            nullptr
                        );
                        aValue s_exc = symbols::create_value(exception, "Exception"); // TODO: NullException
                        callFunc("Exception", "__init__", {s_exc, getString(RERR001, RERR001_SIZE)}); // TODO: NullException
                        builder.CreateStore(s_exc->getValue(), __exception_hanging);
                        builder.CreateStore(builder.getInt64(llvm::hash_value(types["Exception"])), __exception_hanging_type);
                        
                        builder.CreateCall(getExceptionalFunction(LONGJMP_FN), {jmpBufStack.top(), builder.getInt32(1)});
                        builder.CreateUnreachable();
                        builder.SetInsertPoint(accessBlock);
                        if(mem->type == VARIABLE) {
                            llvm::Value* gep = builder.CreateStructGEP(types[typeName], self, mem->varIndex);
                            builder.CreateStore(module->getGlobalVariable("Types_"+type)->getInitializer(),
                                builder.CreateStructGEP(types["Generic"], generic, 1));
                            builder.CreateStore(gep, 
                                builder.CreateStructGEP(types["Generic"], generic, 2));
                            
                        } else if(mem->type == FUNCTION) {
                            llvm::Value* retv = module->getFunction(typeName+"_"+mem->name);
                            builder.CreateStore(module->getGlobalVariable("Types_"+type)->getInitializer(),
                                builder.CreateStructGEP(types["Generic"], generic, 1));
                            builder.CreateStore(retv, 
                                builder.CreateStructGEP(types["Generic"], generic, 2));
                        }
                        ret = symbols::create_value(generic, "Generic");
                        values[std::to_string(getUniqueId())] = ret;
                        ret->set(false, true, false);
                        break;
                    }
                }
                obj = objTMP;
                self = obj->getValue();
                builder.CreateBr(mergeBB);
            }
            {
                ret->isMember = true;
                values[std::to_string(getUniqueId())] = ret;
                builder.SetInsertPoint(mergeBB);
                return {ret, std::move(node->children[2])};
            }
        }
        for(auto& mem : classes[typeName]) {
            if(mem->name == memberName) {
                if(mem->from != 0) {
                    self = builder.CreateStructGEP(types[typeName], obj->getValue(), mem->from);
                    typeName = inhers[typeName][mem->from-1];
                }
                if(mem->access >= STATIC)
                    throw utils::InvalidExpressionException("Trying to access static member " + memberName + " of object " + recog);
                if(mem->access == PRIVATE && currentClass != typeName)
                    throw utils::InvalidExpressionException("Trying to access private member \"" + node->children[2]->value + "\" of object \"" + recog + "\"");
                if(mem->access == SHARED && mainClassNames.classNames[currentClass].package != mainClassNames.classNames[typeName].package)
                    throw utils::InvalidExpressionException("Trying to access shared member \"" + node->children[2]->value + "\" of a mean object \"" + recog + "\"");
                if(mem->access == PROTECTED && std::find(inhers[currentClass].begin(), inhers[currentClass].end(), obj->getTypeName()) == inhers[currentClass].end())
                    throw utils::InvalidExpressionException("Trying to access protected member \"" + node->children[2]->value + "\" of object \"" + recog + "\" when it's not inherited from it");
                
                llvm::Value* nullJudge = builder.CreateIsNotNull(self);
                llvm::BasicBlock* accessBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                llvm::BasicBlock* throwBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                builder.CreateCondBr(nullJudge, accessBlock, throwBlock);
                builder.SetInsertPoint(throwBlock);
                for(const auto& [name, value] : values) {
                    if(value->isObject()) {
                        llvm::Value* isNotNull = builder.CreateIsNotNull(value->getValue());
                        llvm::BasicBlock* ifBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                        llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                        builder.CreateCondBr(isNotNull, ifBlock, elseBlock);
                        builder.SetInsertPoint(ifBlock);
                        callFunc(value->getTypeName(), "__del__", {value});
                        builder.CreateBr(elseBlock);
                        builder.SetInsertPoint(elseBlock);
                    }
                }
                llvm::Value* exception = builder.CreateMalloc(llvm::Type::getInt32Ty(context),
                    types["Exception"], // TODO: NullException 
                    llvm::ConstantInt::get(
                        llvm::Type::getInt32Ty(context),
                        DL.getTypeAllocSize(types["Exception"])
                    ),
                    nullptr
                );
                aValue s_exc = symbols::create_value(exception, "Exception"); // TODO: NullException
                callFunc("Exception", "__init__", {s_exc, getString(RERR001, RERR001_SIZE)}); // TODO: NullException
                builder.CreateStore(s_exc->getValue(), __exception_hanging);
                builder.CreateStore(builder.getInt64(llvm::hash_value(types["Exception"])), __exception_hanging_type);
                
                builder.CreateCall(getExceptionalFunction(LONGJMP_FN), {jmpBufStack.top(), builder.getInt32(1)});
                builder.CreateUnreachable();
                builder.SetInsertPoint(accessBlock);
                if(mem->type == VARIABLE) {
                    llvm::Value* gep = builder.CreateStructGEP(types[typeName], self, mem->varIndex);
                    ret = symbols::create_value(gep, mem->typeName);
                    ret->genericMem = mem->genericMem;
                    ret->set(true, mem->isConstant, mem->isVolatile);
                } else if(mem->type == FUNCTION) {
                    ret = symbols::create_value(module->getFunction(typeName+"_"+mem->name), functions[typeName+"_"+mem->name]->returnType);
                }
                break;
            }
        }
        if(!ret)
            throw utils::InvalidExpressionException("Unknown member \"" + node->children[2]->value +"\" of object \"" + recog + "\"");
        else {
            ret->isMember = true;
            values[std::to_string(getUniqueId())] = ret;
            return {ret, std::move(node->children[2])};
        }
    } else {
        // Static member access

        if(classes.count(node->children[0]->value) == 0)
            throw utils::InvalidExpressionException("Unknown class \"" + node->children[0]->value + "\"");

        std::string typeName = node->children[0]->value;
        std::string memberName = node->children[2]->value;
        aValue ret;
        for(auto& mem : classes[typeName]) {
            if(mem->name == memberName) {
                if(mem->from != 0) {
                    typeName = inhers[typeName][mem->from-1];
                }
                if(mem->access < STATIC)
                    throw utils::InvalidExpressionException("Trying to access non-static member " + memberName + " of class " + typeName);
                if(mem->access == STATIC_PRIVATE && currentClass != typeName)
                    throw utils::InvalidExpressionException("Trying to access private member \"" + node->children[2]->value + "\" of class \"" + typeName + "\"");
                if(mem->access == STATIC_SHARED && mainClassNames.classNames[currentClass].package != mainClassNames.classNames[typeName].package)
                    throw utils::InvalidExpressionException("Trying to access shared member \"" + node->children[2]->value + "\" of a mean class \"" + typeName + "\"");
                if(mem->access == STATIC_PROTECTED && std::find(inhers[currentClass].begin(), inhers[currentClass].end(), node->children[0]->value) == inhers[currentClass].end())
                    throw utils::InvalidExpressionException("Trying to access protected member \"" + node->children[2]->value + "\" of class \"" + typeName + "\" when it's not inherited from it");
                if(mem->type == VARIABLE){
                    llvm::Value* globalValue = module->getGlobalVariable(typeName+"_"+mem->name);
                    ret = symbols::create_value(globalValue, mem->typeName);
                    ret->genericMem = mem->genericMem;
                    ret->set(true, mem->isConstant, mem->isVolatile);
                } else if(mem->type == FUNCTION) {
                    ret = symbols::create_value(module->getFunction(typeName+"_"+mem->name), functions[typeName+"_"+mem->name]->returnType);
                }
                break;
            }
        }
        if(!ret)
            throw utils::InvalidExpressionException("Unknown member \"" + node->children[2]->value +"\" of object \"" + node->children[0]->value + "\"");
        else {
            ret->isMember = true;
            values[std::to_string(getUniqueId())] = ret;
            return {ret, std::move(node->children[2])};
        }
    }
}

aValue CodeGen::emitUnaryExpression(const parser::aNode& node) {
    if(node->children[0]->value == "+" || node->children[0]->value == "-") {
        aValue val = emitExpression(node->children[1]);
        if(getOptFunc(val->getTypeName(), node->children[0]->value) != std::make_pair(".", ".")) {
            std::pair<std::string, std::string> opt = getOptFunc(val->getTypeName(), node->children[0]->value);
            return callFunc(opt.first, opt.second, {val});
        }
        if(!val->isNumeric())
            throw utils::InvalidExpressionException("Not numeric: " + node->children[1]->value);
        if(node->children[0]->value == "+")
            return val;
        else {
            llvm::Value* neg = builder.CreateNeg(val->getValue());
            aValue ret = symbols::create_value(neg, val->getTypeName());
            return ret;
        }
    } else if(node->children[0]->value == "!") {
        aValue val = emitExpression(node->children[1]);
        if(getOptFunc(val->getTypeName(), node->children[0]->value) != std::make_pair(".", ".")) {
            std::pair<std::string, std::string> opt = getOptFunc(val->getTypeName(), node->children[0]->value);
            return callFunc(opt.first, opt.second, {val});
        }
        aValue ret = symbols::create_value(builder.CreateNot(cast(val, "bool", {})->getValue()), "bool");
        return ret;
    } else if(node->children[0]->value == "~") {
        aValue val = emitExpression(node->children[1]);
        if(getOptFunc(val->getTypeName(), node->children[0]->value) != std::make_pair(".", ".")) {
            std::pair<std::string, std::string> opt = getOptFunc(val->getTypeName(), node->children[0]->value);
            return callFunc(opt.first, opt.second, {val});
        }
        if(!val->isNumeric())
            throw utils::InvalidExpressionException("Not numeric: " + node->children[1]->value);
        llvm::Value* neg = builder.CreateNot(val->getValue());
        aValue ret = symbols::create_value(neg, val->getTypeName());
        return ret;
    } else if(node->children[0]->value == "++" || node->children[0]->value == "--") {
        aValue val = emitExpression(node->children[1]);
        if(getOptFunc(val->getTypeName(), node->children[0]->value) != std::make_pair(".", ".")) {
            std::pair<std::string, std::string> opt = getOptFunc(val->getTypeName(), node->children[0]->value);
            return callFunc(opt.first, opt.second, {val});
        }
        std::string type = val->getTypeName();
        if(!val->isVariable())
            throw utils::InvalidExpressionException("Not mutable: " + node->children[0]->value);
        if(!val->isNumeric())
            throw utils::InvalidExpressionException("Not numeric: " + node->children[0]->value);
        bool isInteger = (type == "int" || type == "long" || type == "short");
        aValue ret = nullptr;
        if(node->children[0]->value == "++") {
            ret = symbols::create_value(builder.CreateStore(
                isInteger?builder.CreateAdd(val->getValue(), llvm::ConstantInt::get(val->getValue()->getType(), 1)):
                builder.CreateFAdd(val->getValue(), llvm::ConstantFP::get(val->getValue()->getType(), 1.0)),
                val->getVariable(),
                val->isVolatile()
            ), val->getTypeName());
        } else {
            ret = symbols::create_value(builder.CreateStore(
                isInteger?builder.CreateSub(val->getValue(), llvm::ConstantInt::get(val->getValue()->getType(), 1)):
                builder.CreateFSub(val->getValue(), llvm::ConstantFP::get(val->getValue()->getType(), 1.0)),
                val->getVariable(),
                val->isVolatile()
            ), val->getTypeName());
        }
        return ret;
    }
}

aValue CodeGen::emitCastExpression(const parser::aNode& node) {
    return cast(emitExpression(node->children[1]), getTypeStr(node->children[0]), templateObjects);
}

aValue CodeGen::emitMultiplicativeExpression(const parser::aNode& node) {
    aValue left = emitExpression(node->children[0]);
    aValue right = emitExpression(node->children[2]);
    if(getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value) != std::make_pair(".", ".")) {
        std::pair<std::string, std::string> opt = getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value);
        aValue param = left, source = right;
        aValue flag = nullptr;
        if(opt.first == left->getTypeName()) {
            param = right;
            source = left;
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "int");
        } else
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1), "int");
        if(node->children[1]->value == "*")
            return callFunc(opt.first, opt.second, {source, param});
        return callFunc(opt.first, opt.second, {source, param, flag});
    }
    if(!left->isNumeric() || !right->isNumeric())
        throw utils::InvalidExpressionException("Not numeric: " + node->children[0]->value + " " + node->children[1]->value + " " + node->children[2]->value);
    bool isInteger = (left->getTypeName() == "int" || left->getTypeName() == "long" || left->getTypeName() == "short") &&
                     (right->getTypeName() == "int" || right->getTypeName() == "long" || right->getTypeName() == "short");
    std::string type = getNumericTypeStr(std::max(getNumericType(left->getTypeName()), getNumericType(right->getTypeName())));
    aValue ret = nullptr;
    if(node->children[1]->value == "*") {
        ret = symbols::create_value(
            isInteger? builder.CreateMul(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()):
                        builder.CreateFMul(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()),
            type
        );
    } else if(node->children[1]->value == "/") {
        if(node->children[2]->value == "0")
            compilation_warning(mainCompiler.current_filename.c_str(), node->children[2]->line, node->children[2]->column, "Division by zero");
        ret = symbols::create_value(
            isInteger? builder.CreateSDiv(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()):
                        builder.CreateFDiv(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()),
            type
        );
    } else {
        if(node->children[2]->value == "0")
            compilation_warning(mainCompiler.current_filename.c_str(), node->children[2]->line, node->children[2]->column, "Division by zero");
        ret = symbols::create_value(
            isInteger? builder.CreateSRem(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()):
                        builder.CreateFRem(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()),
            type
        );
    }
    return ret;
}

aValue CodeGen::emitAdditiveExpression(const parser::aNode& node) {
    aValue left = emitExpression(node->children[0]);
    aValue right = emitExpression(node->children[2]);
    if(getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value) != std::make_pair(".", ".")) {
        std::pair<std::string, std::string> opt = getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value);
        aValue param = left, source = right;
        aValue flag = nullptr;
        if(opt.first == left->getTypeName()) {
            param = right;
            source = left;
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "int");
        } else
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1), "int");
        if(node->children[1]->value == "+")
            return callFunc(opt.first, opt.second, {source, param});
        return callFunc(opt.first, opt.second, {source, param, flag});
    }
    if(!left->isNumeric() || !right->isNumeric())
        throw utils::InvalidExpressionException("Not numeric: " + node->children[0]->value + " " + node->children[1]->value + " " + node->children[2]->value);
    bool isInteger = (left->getTypeName() == "int" || left->getTypeName() == "long" || left->getTypeName() == "short") &&
                     (right->getTypeName() == "int" || right->getTypeName() == "long" || right->getTypeName() == "short");
    std::string type = getNumericTypeStr(std::max(getNumericType(left->getTypeName()), getNumericType(right->getTypeName())));
    aValue ret = nullptr;
    if(node->children[1]->value == "+") {
        ret = symbols::create_value(
            isInteger? builder.CreateAdd(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()):
                        builder.CreateFAdd(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()),
            type
        );
    } else {
        ret = symbols::create_value(
            isInteger? builder.CreateSub(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()):
                        builder.CreateFSub(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()),
            type
        );
    }
    return ret;
}

aValue CodeGen::emitShiftExpression(const parser::aNode& node) {
    aValue left = emitExpression(node->children[0]);
    aValue right = emitExpression(node->children[2]);
    if(getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value) != std::make_pair(".", ".")) {
        std::pair<std::string, std::string> opt = getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value);
        aValue param = left, source = right;
        aValue flag = nullptr;
        if(opt.first == left->getTypeName()) {
            param = right;
            source = left;
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "int");
        } else
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1), "int");
        return callFunc(opt.first, opt.second, {source, param, flag});
    }
    if(!left->isNumeric() || !right->isNumeric())
        throw utils::InvalidExpressionException("Not numeric: " + node->children[0]->value + " " + node->children[1]->value + " " + node->children[2]->value);
    bool isInteger = (left->getTypeName() == "int" || left->getTypeName() == "long" || left->getTypeName() == "short") &&
                     (right->getTypeName() == "int" || right->getTypeName() == "long" || right->getTypeName() == "short");
    if(!isInteger)
        throw utils::InvalidExpressionException("Shift operation only allowed on integers: " + node->children[0]->value + " " + node->children[1]->value + " " + node->children[2]->value);
    std::string type = getNumericTypeStr(std::max(getNumericType(left->getTypeName()), getNumericType(right->getTypeName())));
    aValue ret = nullptr;
    if(node->children[1]->value == "<<") {
        ret = symbols::create_value(
            builder.CreateShl(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()), type
        );
    } else {
        ret = symbols::create_value(
            builder.CreateLShr(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()), type
        );
    }
    return ret;
}

aValue CodeGen::emitRelationalExpression(const parser::aNode& node) {
    aValue left = emitExpression(node->children[0]);
    aValue right = emitExpression(node->children[2]);
    if(getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value) != std::make_pair(".", ".")) {
        std::pair<std::string, std::string> opt = getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value);
        aValue param = left, source = right;
        aValue flag = nullptr;
        if(opt.first == left->getTypeName()) {
            param = right;
            source = left;
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "int");
        } else
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1), "int");
        return callFunc(opt.first, opt.second, {source, param, flag});
    }
    if(!left->isNumeric() || !right->isNumeric())
        throw utils::InvalidExpressionException("Not numeric: " + node->children[0]->value + " " + node->children[1]->value + " " + node->children[2]->value);
    bool isInteger = (left->getTypeName() == "int" || left->getTypeName() == "long" || left->getTypeName() == "short") &&
                     (right->getTypeName() == "int" || right->getTypeName() == "long" || right->getTypeName() == "short");
    std::string type = getNumericTypeStr(std::max(getNumericType(left->getTypeName()), getNumericType(right->getTypeName())));
    aValue ret = nullptr;
    if(node->children[1]->value == "<") {
        ret = symbols::create_value(
            isInteger? builder.CreateICmpSLT(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()):
                        builder.CreateFCmpOLT(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()),
            type
        );
    } else if(node->children[1]->value == ">") {
        ret = symbols::create_value(
            isInteger? builder.CreateICmpSGT(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()):
                        builder.CreateFCmpOGT(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()),
            type
        );
    } else if(node->children[1]->value == "<=") {
        ret = symbols::create_value(
            isInteger? builder.CreateICmpSLE(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()):
                        builder.CreateFCmpOLE(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()),
            type
        );
    } else {
        ret = symbols::create_value(
            isInteger? builder.CreateICmpSGE(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()):
                        builder.CreateFCmpOGE(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()),
            type
        );
    }
    return ret;
} 

aValue CodeGen::emitEqualityExpression(const parser::aNode& node) {
    aValue left = emitExpression(node->children[0]);
    aValue right = emitExpression(node->children[2]);
    if(getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value) != std::make_pair(".", ".")) {
        std::pair<std::string, std::string> opt = getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value);
        aValue param = left, source = right;
        aValue flag = nullptr;
        if(opt.first == left->getTypeName()) {
            param = right;
            source = left;
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "int");
        } else
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1), "int");
        return callFunc(opt.first, opt.second, {source, param});
    }
    if(!left->isSimple() || !right->isSimple())
        throw utils::InvalidExpressionException("Not numeric: " + node->children[0]->value + " " + node->children[1]->value + " " + node->children[2]->value);
    bool isInteger = (left->getTypeName() == "int" || left->getTypeName() == "long" || left->getTypeName() == "short" || left->getTypeName() == "char" || left->getTypeName() == "byte" || left->getTypeName() == "bool") &&
                     (right->getTypeName() == "int" || right->getTypeName() == "long" || right->getTypeName() == "short" || right->getTypeName() == "char" || right->getTypeName() == "byte" || right->getTypeName() == "bool");
    std::string type = getNumericTypeStr(std::max(getNumericType(left->getTypeName()), getNumericType(right->getTypeName())));
    aValue ret = nullptr;
    if(node->children[1]->value == "==") {
        ret = symbols::create_value(
            isInteger? builder.CreateICmpEQ(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()):
                        builder.CreateFCmpOEQ(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()),
            type
        );
    } else {
        ret = symbols::create_value(
            isInteger? builder.CreateICmpNE(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()):
                        builder.CreateFCmpONE(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()),
            type
        );
    }
    return ret;
}

aValue CodeGen::emitAndExpression(const parser::aNode& node) {
    aValue left = emitExpression(node->children[0]);
    aValue right = emitExpression(node->children[2]);
    if(getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value) != std::make_pair(".", ".")) {
        std::pair<std::string, std::string> opt = getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value);
        aValue param = left, source = right;
        aValue flag = nullptr;
        if(opt.first == left->getTypeName()) {
            param = right;
            source = left;
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "int");
        } else
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1), "int");
        return callFunc(opt.first, opt.second, {source, param});
    }
    if(!left->isNumeric() || !right->isNumeric())
        throw utils::InvalidExpressionException("Not numeric: " + node->children[0]->value + " " + node->children[1]->value + " " + node->children[2]->value);
    bool isInteger = (left->getTypeName() == "int" || left->getTypeName() == "long" || left->getTypeName() == "short") &&
                     (right->getTypeName() == "int" || right->getTypeName() == "long" || right->getTypeName() == "short");
    if(!isInteger)
        throw utils::InvalidExpressionException("And operation only allowed on integers: " + node->children[0]->value + " " + node->children[1]->value + " " + node->children[2]->value);
    std::string type = getNumericTypeStr(std::max(getNumericType(left->getTypeName()), getNumericType(right->getTypeName())));
    retValue(builder.CreateAnd(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()), type);
    return ret;
}

aValue CodeGen::emitExclusiveOrExpression(const parser::aNode& node) {
    aValue left = emitExpression(node->children[0]);
    aValue right = emitExpression(node->children[2]);
    if(getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value) != std::make_pair(".", ".")) {
        std::pair<std::string, std::string> opt = getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value);
        aValue param = left, source = right;
        aValue flag = nullptr;
        if(opt.first == left->getTypeName()) {
            param = right;
            source = left;
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "int");
        } else
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1), "int");
        return callFunc(opt.first, opt.second, {source, param});
    }
    if(!left->isNumeric() || !right->isNumeric())
        throw utils::InvalidExpressionException("Not numeric: " + node->children[0]->value + " " + node->children[1]->value + " " + node->children[2]->value);
    bool isInteger = (left->getTypeName() == "int" || left->getTypeName() == "long" || left->getTypeName() == "short") &&
                     (right->getTypeName() == "int" || right->getTypeName() == "long" || right->getTypeName() == "short");
    if(!isInteger)
        throw utils::InvalidExpressionException("Xor operation only allowed on integers: " + node->children[0]->value + " " + node->children[1]->value + " " + node->children[2]->value);
    std::string type = getNumericTypeStr(std::max(getNumericType(left->getTypeName()), getNumericType(right->getTypeName())));
    retValue(builder.CreateXor(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()), type);
    return ret;
}

aValue CodeGen::emitInclusiveOrExpression(const parser::aNode& node) {
    aValue left = emitExpression(node->children[0]);
    aValue right = emitExpression(node->children[2]);
    if(getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value) != std::make_pair(".", ".")) {
        std::pair<std::string, std::string> opt = getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value);
        aValue param = left, source = right;
        aValue flag = nullptr;
        if(opt.first == left->getTypeName()) {
            param = right;
            source = left;
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "int");
        } else
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1), "int");
        return callFunc(opt.first, opt.second, {source, param});
    }
    if(!left->isNumeric() || !right->isNumeric())
        throw utils::InvalidExpressionException("Not numeric: " + node->children[0]->value + " " + node->children[1]->value + " " + node->children[2]->value);
    bool isInteger = (left->getTypeName() == "int" || left->getTypeName() == "long" || left->getTypeName() == "short") &&
                     (right->getTypeName() == "int" || right->getTypeName() == "long" || right->getTypeName() == "short");
    if(!isInteger)
        throw utils::InvalidExpressionException("Or operation only allowed on integers: " + node->children[0]->value + " " + node->children[1]->value + " " + node->children[2]->value);
    std::string type = getNumericTypeStr(std::max(getNumericType(left->getTypeName()), getNumericType(right->getTypeName())));
    retValue(builder.CreateOr(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()), type);
    return ret;
}

aValue CodeGen::emitLogicalAndExpression(const parser::aNode& node) {
    aValue left = emitExpression(node->children[0]);
    aValue right = emitExpression(node->children[2]);
    if(getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value) != std::make_pair(".", ".")) {
        std::pair<std::string, std::string> opt = getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value);
        aValue param = left, source = right;
        aValue flag = nullptr;
        if(opt.first == left->getTypeName()) {
            param = right;
            source = left;
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "int");
        } else
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1), "int");
        return callFunc(opt.first, opt.second, {source, param});
    }
    retValue(builder.CreateAnd(cast(left, "bool", {})->getValue(), cast(right, "bool", {})->getValue()), "bool");
    return ret;
}

aValue CodeGen::emitLogicalOrExpression(const parser::aNode& node) {
    aValue left = emitExpression(node->children[0]);
    aValue right = emitExpression(node->children[2]);
    if(getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value) != std::make_pair(".", ".")) {
        std::pair<std::string, std::string> opt = getOptFunc(left->getTypeName(), right->getTypeName(), node->children[1]->value);
        aValue param = left, source = right;
        aValue flag = nullptr;
        if(opt.first == left->getTypeName()) {
            param = right;
            source = left;
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), "int");
        } else
            flag = symbols::create_value(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1), "int");
        return callFunc(opt.first, opt.second, {source, param});
    }
    retValue(builder.CreateOr(cast(left, "bool", {})->getValue(), cast(right, "bool", {})->getValue()), "bool");
    return ret;
}

aValue CodeGen::emitConditionalExpression(const parser::aNode& node) {
    aValue condition = emitExpression(node->children[0]);
    aValue trueValue = emitExpression(node->children[1]);
    aValue falseValue = emitExpression(node->children[2]);
    falseValue = cast(falseValue, trueValue->getTypeName(), trueValue->genericMem);
    retValue(builder.CreateSelect(cast(condition, "bool", {})->getValue(), 
                                    cast(trueValue, trueValue->getTypeName(), trueValue->genericMem)->getValue(), 
                                    cast(falseValue, falseValue->getTypeName(), falseValue->genericMem)->getValue()), trueValue->getTypeName());
    return ret;
}

aValue CodeGen::emitAssignmentExpression(const parser::aNode& node) {
    aValue left = emitExpression(node->children[0]);
    aValue right = emitExpression(node->children[2]);
    if(!left->isVariable())
        throw utils::InvalidExpressionException("Cannot assign to non-variable: " + node->children[0]->value);
    if(left->isConstant())
        throw utils::InvalidExpressionException("Cannot assign to const: " + node->children[0]->value);
    aValue ret = nullptr;
    if(node->children[1]->value == "=") {
        ret = symbols::create_value(builder.CreateStore(cast(right, left->getTypeName(), left->genericMem)->getValue(), left->getVariable(), left->isVolatile()), left->getTypeName());
        // Quote lifetime
        if(left->isMember) {
            if(left->isObject()) {
                llvm::Value* v=builder.CreateAdd(
                    builder.CreateLoad(
                        llvm::Type::getInt32Ty(context),
                        builder.CreateStructGEP(types[left->getTypeName()], left->getValue(), 0)
                    ),
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1)
                );
                builder.CreateStore(v, left->getVariable());
            }
        } else if(isSubclassOf(left->getTypeName(), "Generic")) {
            llvm::Value* v=builder.CreateAdd(
                builder.CreateLoad(
                    llvm::Type::getInt32Ty(context),
                    builder.CreateStructGEP(types[left->getTypeName()], left->getValue(), 0)
                ),
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1)
            );
            builder.CreateStore(v, left->getVariable());
        }
    } else {
        if(!left->isNumeric() || !right->isNumeric())
            throw utils::InvalidExpressionException("Not numeric: " + node->children[0]->value + " " + node->children[1]->value + " " + node->children[2]->value);
        std::string type = left->getTypeName();
        bool isInteger = (type == "int" || type == "long" || type == "short");
        if(node->children[1]->value == "+=") {
            llvm::Value* value = isInteger? builder.CreateAdd(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()) :
                                           builder.CreateFAdd(cast(left, type, {})->getValue(), cast(right, type, {})->getValue());
            ret = symbols::create_value(builder.CreateStore(value, left->getVariable(), left->isVolatile()), type);
        } else if(node->children[1]->value == "-=") {
            llvm::Value* value = isInteger? builder.CreateSub(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()) :
                                           builder.CreateFSub(cast(left, type, {})->getValue(), cast(right, type, {})->getValue());
            ret = symbols::create_value(builder.CreateStore(value, left->getVariable(), left->isVolatile()), type);
        } else if(node->children[1]->value == "*=") {
            llvm::Value* value = isInteger? builder.CreateMul(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()) :
                                           builder.CreateFMul(cast(left, type, {})->getValue(), cast(right, type, {})->getValue());
            ret = symbols::create_value(builder.CreateStore(value, left->getVariable(), left->isVolatile()), type);
        } else if(node->children[1]->value == "/=") {
            if(node->children[2]->value == "0")
                compilation_warning(mainCompiler.current_filename.c_str(), node->children[2]->line, node->children[2]->column, "Division by zero");
            llvm::Value* value = isInteger? builder.CreateSDiv(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()) :
                                           builder.CreateFDiv(cast(left, type, {})->getValue(), cast(right, type, {})->getValue());
            ret = symbols::create_value(builder.CreateStore(value, left->getVariable(), left->isVolatile()), type);
        } else if(node->children[1]->value == "%=") {
            if(node->children[2]->value == "0")
                compilation_warning(mainCompiler.current_filename.c_str(), node->children[2]->line, node->children[2]->column, "Division by zero");
            llvm::Value* value = isInteger? builder.CreateSRem(cast(left, type, {})->getValue(), cast(right, type, {})->getValue()) :
                                           builder.CreateFRem(cast(left, type, {})->getValue(), cast(right, type, {})->getValue());
            ret = symbols::create_value(builder.CreateStore(value, left->getVariable(), left->isVolatile()), type);
        }
        if(!isInteger)
            throw utils::InvalidExpressionException("Not integer: " + node->children[0]->value + " " + node->children[1]->value + " " + node->children[2]->value);
        if(node->children[1]->value == "<<=") {
            llvm::Value* value = builder.CreateShl(cast(left, type, {})->getValue(), cast(right, type, {})->getValue());
            ret = symbols::create_value(builder.CreateStore(value, left->getVariable(), left->isVolatile()), type);
        } else if(node->children[1]->value == ">>=") {
            llvm::Value* value = builder.CreateAShr(cast(left, type, {})->getValue(), cast(right, type, {})->getValue());
            ret = symbols::create_value(builder.CreateStore(value, left->getVariable(), left->isVolatile()), type);
        } else if(node->children[1]->value == "&=") {
            llvm::Value* value = builder.CreateAnd(cast(left, type, {})->getValue(), cast(right, type, {})->getValue());
            ret = symbols::create_value(builder.CreateStore(value, left->getVariable(), left->isVolatile()), type);
        } else if(node->children[1]->value == "|=") {
            llvm::Value* value = builder.CreateOr(cast(left, type, {})->getValue(), cast(right, type, {})->getValue());
            ret = symbols::create_value(builder.CreateStore(value, left->getVariable(), left->isVolatile()), type);
        } else if(node->children[1]->value == "^=") {
            llvm::Value* value = builder.CreateXor(cast(left, type, {})->getValue(), cast(right, type, {})->getValue());
            ret = symbols::create_value(builder.CreateStore(value, left->getVariable(), left->isVolatile()), type);
        } else if(node->children[1]->value == "~=") {
            llvm::Value* value = builder.CreateNot(cast(right, type, {})->getValue());
            ret = symbols::create_value(builder.CreateStore(value, left->getVariable(), left->isVolatile()), type);
        }
    }
    return ret;
}

void CodeGen::emitIfStatement(const parser::aNode& node) {
    if(node->children.size() == 3) {
        llvm::Value* condition = emitExpression(node->children[0])->getValue();
        llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
        llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
        llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
        builder.CreateCondBr(condition, thenBlock, elseBlock);

        builder.SetInsertPoint(thenBlock);
        emit(node->children[1]);
        builder.CreateBr(mergeBlock);

        builder.SetInsertPoint(elseBlock);
        emit(node->children[2]);
        builder.CreateBr(mergeBlock);

        builder.SetInsertPoint(mergeBlock);
    } else {
        llvm::Value* condition = emitExpression(node->children[0])->getValue();
        llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
        llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
        builder.CreateCondBr(condition, thenBlock, mergeBlock);

        builder.SetInsertPoint(thenBlock);
        emit(node->children[1]);
        builder.CreateBr(mergeBlock);
        builder.SetInsertPoint(mergeBlock);
    }
}

void CodeGen::emitWhileStatement(const parser::aNode& node) {
    llvm::BasicBlock* conditionBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
    llvm::BasicBlock* loopBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
    llvm::BasicBlock* afterLoopBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());

    breakStack.push(afterLoopBlock);
    continueStack.push(conditionBlock);

    builder.CreateBr(conditionBlock);

    builder.SetInsertPoint(conditionBlock);
    llvm::Value* condition = emitExpression(node->children[0])->getValue();
    builder.CreateCondBr(condition, loopBlock, afterLoopBlock);

    builder.SetInsertPoint(loopBlock);
    emit(node->children[1]);
    builder.CreateBr(conditionBlock);

    builder.SetInsertPoint(afterLoopBlock);

    breakStack.pop();
    continueStack.pop();
}

void CodeGen::emitDoWhileStatement(const parser::aNode& node) {
    llvm::BasicBlock* conditionBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
    llvm::BasicBlock* loopBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
    llvm::BasicBlock* afterLoopBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());

    breakStack.push(afterLoopBlock);
    continueStack.push(conditionBlock);

    builder.CreateBr(loopBlock);

    builder.SetInsertPoint(loopBlock);
    emit(node->children[0]);
    builder.CreateBr(conditionBlock);
    builder.SetInsertPoint(conditionBlock);
    llvm::Value* condition = emitExpression(node->children[1])->getValue();
    builder.CreateCondBr(condition, loopBlock, afterLoopBlock);

    builder.SetInsertPoint(afterLoopBlock);

    breakStack.pop();
    continueStack.pop();
}

void CodeGen::emitForStatement(const parser::aNode& node) {
    llvm::BasicBlock* initBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
    llvm::BasicBlock* conditionBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
    llvm::BasicBlock* finalBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
    llvm::BasicBlock* loopBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
    llvm::BasicBlock* afterLoopBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());

    breakStack.push(afterLoopBlock);
    continueStack.push(finalBlock);

    builder.CreateBr(initBlock);
    builder.SetInsertPoint(initBlock);
    emit(node->children[0]);
    builder.CreateBr(conditionBlock);

    builder.SetInsertPoint(conditionBlock);
    llvm::Value* condition = emitExpression(node->children[1])->getValue();
    builder.CreateCondBr(condition, loopBlock, afterLoopBlock);

    builder.SetInsertPoint(loopBlock);
    emit(node->children[3]);
    builder.CreateBr(finalBlock);
    builder.SetInsertPoint(finalBlock);
    emit(node->children[2]);
    builder.CreateBr(conditionBlock);

    builder.SetInsertPoint(afterLoopBlock);

    breakStack.pop();
    continueStack.pop();
}

void CodeGen::emitLabelStatement(const parser::aNode& node) {
    if(labels.count(node->children[0]->value) > 0 && labels[node->children[0]->value].first != LABEL_PLACEHOLDER)
        throw std::runtime_error("Label already exists");
    llvm::BasicBlock* labelBlock = llvm::BasicBlock::Create(context, node->children[0]->value, builder.GetInsertBlock()->getParent());
    builder.CreateBr(labelBlock);
    builder.SetInsertPoint(labelBlock);
    labels[node->children[0]->value].first = labelBlock;
    labels[node->children[0]->value].second->setSuccessor(0, labelBlock);
}

void CodeGen::emitGotoStatement(const parser::aNode& node) {
    if(labels.count(node->children[0]->value) == 0)
        labels[node->children[0]->value] = {LABEL_PLACEHOLDER, nullptr};
    llvm::BranchInst* br = builder.CreateBr(labels[node->children[0]->value].first);
    labels[node->children[0]->value].second = br;
    builder.SetInsertPoint(llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent()));
}

void CodeGen::emitSwitchStatement(const parser::aNode& node) {
    aValue switch_value = emitExpression(node->children[0]);
    if(switch_value->getTypeName() != "int")
        throw std::runtime_error("Switch value must be integer.");
    llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
    breakStack.push(mergeBlock);

    llvm::BasicBlock* currentBlock = builder.GetInsertBlock();

    std::unordered_map<llvm::ConstantInt*, llvm::BasicBlock*> cases;
    llvm::BasicBlock* defaultBlock = nullptr;

    for(int i=1; i<node->children.size(); i++) {
        if(node->children[i]->kind == parser::AST_CASE_LABEL) {
            aValue case_const = emitExpression(node->children[i]->children[0]);
            if(!case_const->isConstantValue || case_const->getTypeName() != "int")
                throw std::runtime_error("Switch - case must be constant integer.");
            llvm::ConstantInt* case_value = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), case_const->constant);
            llvm::BasicBlock* caseBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
            cases[case_value] = caseBlock;
            builder.SetInsertPoint(caseBlock);
        } else if(node->children[i]->kind == parser::AST_DEFAULT_LABEL) {
            defaultBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
            builder.SetInsertPoint(defaultBlock);
        } else {
            emit(node->children[i]);
        }
    }

    if(!defaultBlock) defaultBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
    if(!defaultBlock->getTerminator()) {
        builder.SetInsertPoint(defaultBlock);
        builder.CreateBr(mergeBlock);
    }
    builder.SetInsertPoint(currentBlock);
    llvm::SwitchInst* switch_inst = builder.CreateSwitch(switch_value->getValue(), defaultBlock, cases.size());
    for(const auto& [case_value, caseBlock] : cases) {
        if(!caseBlock->getTerminator()) {
            builder.SetInsertPoint(caseBlock);
            builder.CreateBr(mergeBlock);
        }
        switch_inst->addCase(case_value, caseBlock);
    }
    
    builder.SetInsertPoint(mergeBlock);
    breakStack.pop();
}

// Exceptions

void CodeGen::emitTryStatement(const parser::aNode& node) {
    llvm::Function* func = builder.GetInsertBlock()->getParent();

    std::string tryLabel = getLabel();
    std::string normalLabel = getLabel();
    std::string catchLabel = getLabel();
    std::string resumeLabel = getLabel();
    std::string finallyLabel = getLabel();
    std::string contLabel = getLabel();
    
    llvm::BasicBlock* tryBB = llvm::BasicBlock::Create(context, tryLabel, func);
    llvm::BasicBlock* normalBB = llvm::BasicBlock::Create(context, normalLabel, func);
    llvm::BasicBlock* catchBB = llvm::BasicBlock::Create(context, catchLabel, func);
    llvm::BasicBlock* resumeBB = llvm::BasicBlock::Create(context, resumeLabel, func);
    llvm::BasicBlock* finallyBB = node->children.back()->kind == parser::AST_FINALLY_STATEMENT 
        ? llvm::BasicBlock::Create(context, finallyLabel, func) 
        : nullptr;
    llvm::BasicBlock* contBB = llvm::BasicBlock::Create(context, contLabel, func);

    builder.CreateBr(tryBB);
    builder.SetInsertPoint(tryBB);

    llvm::GlobalVariable* exceptionJmpBuf = new llvm::GlobalVariable(
        *module,
        getPtr(),
        false,
        llvm::GlobalValue::ExternalLinkage,
        llvm_nullptr
    );
    jmpBufStack.push(exceptionJmpBuf);

    llvm::Value* jmp = builder.CreateCall(getExceptionalFunction(SETJMP_FN), {exceptionJmpBuf});

    builder.CreateCondBr(
        builder.CreateICmpEQ(jmp, llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0)),
        normalBB,
        catchBB
    );

    builder.SetInsertPoint(normalBB);
    emit(node->children[0]);

    jmpBufStack.pop();
    
    if (finallyBB) {
        builder.CreateBr(finallyBB);
    } else {
        builder.CreateBr(contBB);
    }

    builder.SetInsertPoint(catchBB);
    llvm::Value* exceptionType = builder.CreateLoad(llvm::Type::getInt64Ty(context), __exception_hanging_type);
    llvm::SwitchInst* switch_inst = builder.CreateSwitch(exceptionType, resumeBB, node->children.size() - (finallyBB != nullptr)?2:1);
    for(int i=1; i<node->children.size()-(finallyBB!=nullptr)?1:0; i++) {
        llvm::BasicBlock* caseBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
        switch_inst->addCase(builder.getInt64(llvm::hash_value(types[node->children[i]->children[0]->children[0]->value])), caseBlock);
        builder.SetInsertPoint(caseBlock);
        llvm::Value* exc = builder.CreateAlloca(getType(node->children[i]->children[0]->children[0]->value), nullptr, (node->children[i]->children.size() > 2)?node->children[i]->children[1]->value:std::to_string(getUniqueId()));
        aValue s_exc = symbols::create_value(exc, node->children[i]->children[0]->children[0]->value);
        values[std::to_string(getUniqueId())] = s_exc;
        s_exc->set(false, true, false);
        builder.CreateStore(builder.CreateLoad(getPtr(), __exception_hanging), exc);
        
        emit(node->children[i]->children.back());
        if(finallyBB) builder.CreateBr(finallyBB);
        else builder.CreateBr(contBB);
    }
    builder.SetInsertPoint(resumeBB);
    
    builder.CreateCall(getExceptionalFunction(LONGJMP_FN), {jmpBufStack.top(), builder.getInt32(1)});
    builder.CreateUnreachable();

    if(finallyBB) {
        builder.SetInsertPoint(finallyBB);
        emit(node->children.back()->children[0]);
    }

    builder.SetInsertPoint(contBB);
}

void CodeGen::emitThrowStatement(const parser::aNode& node) {
    llvm::BasicBlock* newBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
    aValue excValue = emitExpression(node->children[0]);
    std::string excType = excValue->getTypeName();

    if (!isExceptionType(excType)) {
        throw std::runtime_error("Can only throw Exception or its subclasses");
    }
    
    for(const auto& [name, value] : values) {
        if(excValue->id == value->id) continue;
        if(value->isObject()) {
            llvm::Value* isNotNull = builder.CreateIsNotNull(value->getValue());
            llvm::BasicBlock* ifBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
            llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
            builder.CreateCondBr(isNotNull, ifBlock, elseBlock);
            builder.SetInsertPoint(ifBlock);
            callFunc(value->getTypeName(), "__del__", {value});
            builder.CreateBr(elseBlock);
            builder.SetInsertPoint(elseBlock);
        }
    }

    builder.CreateStore(excValue->getValue(), __exception_hanging);
    builder.CreateStore(builder.getInt64(llvm::hash_value(types[excType])), __exception_hanging_type);
    
    builder.CreateCall(getExceptionalFunction(LONGJMP_FN), {jmpBufStack.top(), builder.getInt32(1)});
    builder.CreateUnreachable();

    builder.SetInsertPoint(newBlock);
}

// End of exception handling

void CodeGen::emitReturn(const parser::aNode& node) {
    if(node->children.size() > 0) {
        aValue ret = emitExpression(node->children[0]);
        for(const auto& [name, value] : values) {
            if(ret->id == value->id) continue;
            if(value->isObject()) {
                llvm::Value* isNotNull = builder.CreateIsNotNull(value->getValue());
                llvm::BasicBlock* ifBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                builder.CreateCondBr(isNotNull, ifBlock, elseBlock);
                builder.SetInsertPoint(ifBlock);
                callFunc(value->getTypeName(), "__del__", {value});
                builder.CreateBr(elseBlock);
                builder.SetInsertPoint(elseBlock);
            }
        }
        builder.CreateRet(cast(ret, currentFunction->returnType, currentFunction->genericMem)->getValue());
    } else {
        for(const auto& [name, value] : values) {
            if(value->isObject()) {
                llvm::Value* isNotNull = builder.CreateIsNotNull(value->getValue());
                llvm::BasicBlock* ifBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                builder.CreateCondBr(isNotNull, ifBlock, elseBlock);
                builder.SetInsertPoint(ifBlock);
                callFunc(value->getTypeName(), "__del__", {value});
                builder.CreateBr(elseBlock);
                builder.SetInsertPoint(elseBlock);
            }
        }
        Terminate(builder);
    }
    builder.SetInsertPoint(llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent()));
}

std::string CodeGen::getTypeStr(const parser::aNode& typeNode) {
    // Map AST type nodes to string
    std::string type = "";

    std::vector<std::string> templateOBJ;

    for(const auto& child : typeNode->children){
        if(child->value == "const")
            defineConst = true;
        else if(child->value == "volatile")
            defineVolatile = true;
        else if(child->kind == parser::AST_TYPE_TEMPLATE) {
            for(const auto& obj : child->children)
                templateOBJ.push_back(obj->value);
        }
        else
            type += child->value;
    }
    
    if(templateOBJ.size() > 0) {
        if(!isSubclassOf(type, "Generic"))
            throw std::runtime_error("Generic type must be subclass of Generic");
        templateObjects = templateOBJ;
    }

    return type;
}

llvm::Type* CodeGen::getType(const std::string& type, bool exact) {
    if (type == "int") return llvm::Type::getInt32Ty(context);
    else if (type == "short") return llvm::Type::getInt16Ty(context);
    else if (type == "long") return llvm::Type::getInt64Ty(context);
    else if (type == "char") return llvm::Type::getInt8Ty(context);
    else if (type == "void") return llvm::Type::getVoidTy(context);
    else if (type == "float") return llvm::Type::getFloatTy(context);
    else if (type == "double") return llvm::Type::getDoubleTy(context);
    else if (type == "bool") return llvm::Type::getInt1Ty(context);
    else if (type == "byte") return llvm::Type::getInt8Ty(context);
    else return exact?types[type]:getPtr();
}

llvm::Type* CodeGen::getType(const parser::aNode& typeNode) {
    return getType(getTypeStr(typeNode));
}

llvm::Type* CodeGen::getPtr(){
    if(utils::bit_size == utils::BIT_32)
        return llvm::PointerType::get(llvm::Type::getInt32Ty(context), 0);
    else
        return llvm::PointerType::get(llvm::Type::getInt64Ty(context), 0);
}

int CodeGen::getUniqueId() {
    return uniqueId++;
}

std::string CodeGen::getLabel() {
    return "label_" + std::to_string(lableId++);
}

void CodeGen::Terminate(llvm::IRBuilder<>& builder) {
    if(currentFunction->returnType == "void")
        builder.CreateRetVoid();
    else {
        std::string type = currentFunction->returnType;
        if(type == "int") builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0, true));
        else if(type == "short") builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt16Ty(context), 0, true));
        else if(type == "long") builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 0, true));
        else if(type == "char") builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), 0, true));
        else if(type == "float") builder.CreateRet(llvm::ConstantFP::get(llvm::Type::getFloatTy(context), 0.0));
        else if(type == "double") builder.CreateRet(llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), 0.0));
        else if(type == "bool") builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), 0, true));
        else if(type == "byte") builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), 0, true));
        else builder.CreateRet(llvm_nullptr);
    }
}

bool CodeGen::isExceptionType(const std::string& typeName) {
    if (types.count(typeName) == 0) return false;
    return typeName == "Exception" || isSubclassOf(typeName, "Exception");
}

bool CodeGen::isSubclassOf(const std::string& child, const std::string& parent) {
    if(child == parent)
        return true;
    if(std::find(inhers[child].begin(), inhers[child].end(), parent) != inhers[child].end())
        return true;
    for(auto& _parent : inhers[child])
        if(isSubclassOf(_parent, parent))
            return true;
    return false;
}

llvm::Function* CodeGen::getExceptionalFunction(ExceptionalFnType type) {
    switch (type) {
    // C++ Exception Personality Function
    case SETJMP_FN: {
        llvm::FunctionType *setjmpTy = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(context),
            {getPtr()},
            false
        );
        llvm::Function *setjmpFn = llvm::cast<llvm::Function>(
            module->getOrInsertFunction("setjmp", setjmpTy).getCallee()
        );
        setjmpFn->addFnAttr(llvm::Attribute::ReturnsTwice);
        return setjmpFn;
    }

    case LONGJMP_FN: {
        llvm::FunctionType *longjmpTy = llvm::FunctionType::get(
            llvm::Type::getVoidTy(context),
            {getPtr(),
            llvm::Type::getInt32Ty(context)},
            false
        );
        llvm::Function *longjmpFn = llvm::cast<llvm::Function>(
            module->getOrInsertFunction("longjmp", longjmpTy).getCallee()
        );
        longjmpFn->addFnAttr(llvm::Attribute::NoReturn);
        return longjmpFn;
    }

    default:
        throw std::runtime_error("Unknown function type");
    }
}

aValue CodeGen::getString(const std::string& str) {
        size_t strSize = str.size() + 1;
        
        llvm::Value* mallocSize = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), strSize);
        llvm::Value* charPtr = builder.CreateMalloc(llvm::Type::getInt32Ty(context),
            llvm::Type::getInt8Ty(context),
            mallocSize,
            nullptr
        );
        
        llvm::Value* srcPtr = builder.CreateGlobalStringPtr(str);
        builder.CreateMemCpy(
            charPtr,
            llvm::MaybeAlign(1),
            srcPtr,
            llvm::MaybeAlign(1),
            strSize
        );
        
        // new String()

        llvm::Value* instance = builder.CreateMalloc(llvm::Type::getInt32Ty(context),
            types["String"], 
            llvm::ConstantInt::get(
                llvm::Type::getInt32Ty(context),
                DL.getTypeAllocSize(types["String"])
            ),
            nullptr
        );

        builder.CreateStore(srcPtr, 
            builder.CreateStructGEP(types["String"], instance, 1));
        
        builder.CreateStore(
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), strSize - 1),
            builder.CreateStructGEP(types["String"], instance, 2)
        );

        aValue ret = nullptr;
        ret = symbols::create_value(instance, "String");
        
        return ret;
}

aValue CodeGen::getString(llvm::Value* srcPtr, int strSize) {
        llvm::Value* instance = builder.CreateMalloc(llvm::Type::getInt32Ty(context),
            types["String"], 
            llvm::ConstantInt::get(
                llvm::Type::getInt32Ty(context),
                DL.getTypeAllocSize(types["String"])
            ),
            nullptr
        );

        builder.CreateStore(srcPtr, 
            builder.CreateStructGEP(types["String"], instance, 0));
        
        builder.CreateStore(
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), strSize - 1),
            builder.CreateStructGEP(types["String"], instance, 1)
        );

        aValue ret = nullptr;
        ret = symbols::create_value(instance, "String");
        
        return ret;
}

bool CodeGen::canCast(const std::string& from, const std::string& to, std::vector<std::string> genericMembers) {
    if(isSubclassOf(to, "Generic")) {
        bool notFound = false;
        for(auto mem : genericMembers) {
            if(mem == "!" || mem == "&&" || mem == "||") {}
            else if(from == "int" || from == "short" || from == "long" || from == "float" || from == "double") {
                if(mem == "++" || mem == "--" || mem == "!" || mem == "~" ||
                    mem == "+" || mem == "-" || mem == "*" || mem == "&" ||
                    mem == "|" || mem == "^" || mem == "==" || mem == "!=" ||
                    mem == "<" || mem == ">" || mem == "<=" || mem == ">=" ||
                    mem == "<<" || mem == ">>" || mem == "%" || mem == "/" ||
                    mem == "&&" || mem == "||" || mem == "!"){}
                else notFound = true;
            } else if(!(from == "int" || from == "short" || from == "long" || 
                        from == "char" || from == "void" || from == "float" ||
                        from == "double" || from == "bool" || from == "byte")) {
                if(getOptFunc(from, mem) != std::make_pair(".", ".")) {}
                else {
                    notFound = true;
                    for(auto element : classes[from]) {
                        if(element->name == mem) {
                            notFound = false;
                            if(element->access != PUBLIC)
                                notFound = true;
                            break;
                        }
                    }
                }
            } else notFound = true;
        }
        if(notFound)
            return false;
        return true;
    } else if(isSubclassOf(from, "Generic") && (to == "int" || to == "short" || to == "long" || 
            to == "char" || to == "void" || to == "float" ||
            to == "double" || to == "bool" || to == "byte")) {
        return true;
    }
    if(from == "null" && classes.count(to) != 0) return true;
    if(from == "void" || to == "void" || from == "bool") return false;
    if(classes.count(from) == 0 && classes.count(to) == 0) return true;
    if(from == to) return true;
    if(funcExists("cast", from + "_" + to)) return true;
    if(from == "char") {
        if (funcExists("cast", "byte_"+to)) return true;
    } else if(from == "byte") {
        if (funcExists("cast", "char_"+to)) return true;
    } else if(to == "char") {
        if (funcExists("cast", from+"_byte")) return true;
    } else if(to == "byte") {
        if (funcExists("cast", from+"_char")) return true;
    }
    return false;
}

aValue CodeGen::cast(aValue from, const std::string& toType, std::vector<std::string> genericMembers) {
    std::string fromType = from->getTypeName();
    if(!canCast(fromType, toType, genericMembers))
        throw utils::InvalidExpressionException("Cannot cast from " + fromType + " to " + toType);
    if(fromType == toType) return from;
    
    if(isSubclassOf(toType, "Generic")) {
        
        llvm::Value* instance = builder.CreateMalloc(llvm::Type::getInt32Ty(context),
            types["Generic"], 
            llvm::ConstantInt::get(
                llvm::Type::getInt32Ty(context),
                DL.getTypeAllocSize(types["Generic"])
            ),
            nullptr
        );

        builder.CreateStore(
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
            builder.CreateStructGEP(types["Generic"], instance, 0)
        );
        
        builder.CreateStore(module->getGlobalVariable("Types_"+fromType)->getInitializer(),
            builder.CreateStructGEP(types["Generic"], instance, 1));
        builder.CreateStore(from->getValue(), 
            builder.CreateStructGEP(types["Generic"], instance, 2));

        aValue ret = symbols::create_value(instance, "Generic");
        values[std::to_string(getUniqueId())] = ret;
        ret->set(false, true, false);
        ret->genericMem = genericMembers;

        return ret;
    } else if(isSubclassOf(fromType, "Generic")) {

        llvm::Value* typeJudge = builder.CreateICmpEQ(
            builder.CreateLoad(llvm::Type::getInt64Ty(context), builder.CreateStructGEP(types["Generic"], from->getValue(), 1)),
            module->getGlobalVariable("Types_"+toType)->getInitializer()
        );

        llvm::BasicBlock* castBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
        llvm::BasicBlock* throwBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
        builder.CreateCondBr(typeJudge, castBlock, throwBlock);
        builder.SetInsertPoint(throwBlock);
        for(const auto& [name, value] : values) {
            if(value->isObject()) {
                llvm::Value* isNotNull = builder.CreateIsNotNull(value->getValue());
                llvm::BasicBlock* ifBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context, getLabel(), builder.GetInsertBlock()->getParent());
                builder.CreateCondBr(isNotNull, ifBlock, elseBlock);
                builder.SetInsertPoint(ifBlock);
                callFunc(value->getTypeName(), "__del__", {value});
                builder.CreateBr(elseBlock);
                builder.SetInsertPoint(elseBlock);
            }
        }
        llvm::Value* exception = builder.CreateMalloc(llvm::Type::getInt32Ty(context),
            types["Exception"], // TODO: NullException 
            llvm::ConstantInt::get(
                llvm::Type::getInt32Ty(context),
                DL.getTypeAllocSize(types["Exception"])
            ),
            nullptr
        );
        aValue s_exc = symbols::create_value(exception, "Exception"); // TODO: NullException
        callFunc("Exception", "__init__", {s_exc, getString(RERR001, RERR001_SIZE)}); // TODO: NullException
        builder.CreateStore(s_exc->getValue(), __exception_hanging);
        builder.CreateStore(builder.getInt64(llvm::hash_value(types["Exception"])), __exception_hanging_type);
        
        builder.CreateCall(getExceptionalFunction(LONGJMP_FN), {jmpBufStack.top(), builder.getInt32(1)});
        builder.CreateUnreachable();

        builder.SetInsertPoint(castBlock);
        llvm::Value* instance = builder.CreateLoad(
            getType(toType, true),
            builder.CreateStructGEP(types["Generic"], from->getValue(), 2)
        );
        aValue ret = symbols::create_value(instance, toType);
        ret->set(false, true, false);
        return ret;
    }

    // Numbers
    if(classes.count(fromType) == 0 && classes.count(toType) == 0) {
        llvm::Type* fromTypeLLVM = getType(fromType);
        llvm::Type* toTypeLLVM = getType(toType);

        if (toTypeLLVM->isIntegerTy(1)) {
            if (fromTypeLLVM->isIntegerTy()) {
                retValue(builder.CreateICmpNE(from->getValue(), llvm::ConstantInt::get(fromTypeLLVM, 0, true)), toType);
                return ret;
            }
            else if (fromTypeLLVM->isFloatingPointTy()) {
                retValue(builder.CreateFCmpUNE(from->getValue(), llvm::ConstantFP::get(fromTypeLLVM, 0.0)), toType);
                return ret;
            }
            else if (fromTypeLLVM->isPointerTy()) {
                retValue(builder.CreateICmpNE(
                    builder.CreateBitCast(from->getValue(), llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0)),
                    llvm::ConstantPointerNull::get(llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0))), toType);
                return ret;
            }
            else {
                retValue(builder.CreateICmpNE(from->getValue(), llvm::ConstantInt::get(fromTypeLLVM, 0, true)), toType);
                return ret;
            }
        }

        if (fromTypeLLVM->isIntegerTy() && toTypeLLVM->isIntegerTy()) {
            unsigned fromBits = fromTypeLLVM->getIntegerBitWidth();
            unsigned toBits = toTypeLLVM->getIntegerBitWidth();
            
            // Narrowing conversion (i32 -> i16)
            if (fromBits > toBits) {
                retValue(builder.CreateTrunc(from->getValue(), toTypeLLVM), toType);
                return ret;
            }
            // Widening conversion (i16 -> i32)
            else {
                retValue(builder.CreateSExt(from->getValue(), toTypeLLVM), toType);
                return ret;
            }
        }

        // Floating point to floating point conversion
        if (fromTypeLLVM->isFloatingPointTy() && toTypeLLVM->isFloatingPointTy()) {
            // float -> double (extend)
            if (fromTypeLLVM->isFloatTy() && toTypeLLVM->isDoubleTy()) {
                retValue(builder.CreateFPExt(from->getValue(), toTypeLLVM), toType);
                return ret;
            }
            // double -> float (truncate)
            else if (fromTypeLLVM->isDoubleTy() && toTypeLLVM->isFloatTy()) {
                retValue(builder.CreateFPTrunc(from->getValue(), toTypeLLVM), toType);
                return ret;
            }
        }

        // Integer to floating point conversion
        if (fromTypeLLVM->isIntegerTy() && toTypeLLVM->isFloatingPointTy()) {
            // Signed integer to float
            retValue(builder.CreateSIToFP(from->getValue(), toTypeLLVM), toType);
            return ret;
        }
        // Floating point to integer conversion
        if (fromTypeLLVM->isFloatingPointTy() && toTypeLLVM->isIntegerTy()) {
            // Float to signed integer
            retValue(builder.CreateFPToSI(from->getValue(), toTypeLLVM), toType);
            return ret;
        }
    }

    // Classes
    if(funcExists("cast", fromType + "_" + toType))
        return callFunc("cast", fromType + "_" + toType, {from});
    if(fromType == "char" && funcExists("cast", "byte_" + toType))
        return callFunc("cast", "byte_" + toType, {from});
    if(fromType == "byte" && funcExists("cast", fromType + "_char"))
       return callFunc("cast", fromType + "_char", {from});
    if(toType == "char" && funcExists("cast", fromType + "_byte"))
       return callFunc("cast", fromType + "_byte", {from});
    if(fromType == "byte" && funcExists("cast", fromType + "_char"))
       return callFunc("cast", fromType + "_char", {from});
    if(fromType == "null") {
        retValue(llvm_nullptr, toType);
        return ret;
    }
}

bool CodeGen::funcExists(const std::string& className, const std::string& funcName) {
    return functions.count(className+"_"+funcName)>0;
}

std::pair<std::string, std::string> CodeGen::getOptFunc(const std::string& lType, const std::string& rType, const std::string& op) {
    if(funcExists(lType, op+rType)) {
        return std::make_pair(lType, op+rType);
    } else if(funcExists(rType, op+lType)) {
        return std::make_pair(rType, op+lType);
    } else {
        return std::make_pair(".", ".");
    }
}

std::pair<std::string, std::string> CodeGen::getOptFunc(const std::string& type, const std::string& op) {
    if(funcExists(type, op)) {
        return std::make_pair(type, op);
    } else {
        return std::make_pair(".", ".");
    }
}

aValue CodeGen::callFunc(const std::string& className, const std::string& funcName, std::vector<aValue> args) {
    if(!funcExists(className, funcName))
        throw utils::InvalidExpressionException("Function " + className + "." + funcName + " does not exist");
    Function* func = functions[className+"_"+funcName];
    for(int i = 0; i < args.size(); i++) {
        if(isSubclassOf(func->arguments[i]->type, "Generic")) {
            bool notFound = false;
            for(auto mem : func->arguments[i]->genericMem) {
                if(mem == "!" || mem == "&&" || mem == "||") {}
                else if(args[i]->isNumeric()) {
                    if(mem == "++" || mem == "--" || mem == "!" || mem == "~" ||
                        mem == "+" || mem == "-" || mem == "*" || mem == "&" ||
                        mem == "|" || mem == "^" || mem == "==" || mem == "!=" ||
                        mem == "<" || mem == ">" || mem == "<=" || mem == ">=" ||
                        mem == "<<" || mem == ">>" || mem == "%" || mem == "/" ||
                        mem == "&&" || mem == "||" || mem == "!"){}
                    else notFound = true;
                } else if(args[i]->isObject()) {
                    if(getOptFunc(args[i]->getTypeName(), mem) != std::make_pair(".", ".")) {}
                    else {
                        notFound = true;
                        for(auto element : classes[args[i]->getTypeName()]) {
                            if(element->name == mem) {
                                notFound = false;
                                if(element->access != PUBLIC)
                                    notFound = true;
                                break;
                            }
                        }
                    }
                } else notFound = true;
            }
            if(notFound)
                throw utils::InvalidExpressionException("Variable is not available for such Generic");
        }
    }
    std::vector<llvm::Value*> llvmArgs;
    for (int i = 0; i < args.size(); i++) {
        llvmArgs.push_back(args[i]->getValue());
    }
    retValue(builder.CreateCall(module->getFunction(className+"_"+funcName), llvmArgs), func->returnType);
    return ret;
}

NumericType getNumericType(const std::string& type) {
    if(type == "bool")
        return -1;
    else if(type == "char")
        return 0;
    else if(type == "byte")
        return 0;
    else if(type == "short")
        return 1;
    else if(type == "int")
        return 2;
    else if(type == "long")
        return 3;
    else if(type == "float")
        return 4;
    else if(type == "double")
        return 5;
}

std::string getNumericTypeStr(NumericType type) {
    switch(type) {
        case -1:
            return "bool";
            break;
        case 0:
            return "char";
            break;
        case 1:
            return "short";
            break;
        case 2:
            return "int";
            break;
        case 3:
            return "long";
            break;
        case 4:
            return "float";
            break;
        case 5:
            return "double";
            break;
    }
}

}