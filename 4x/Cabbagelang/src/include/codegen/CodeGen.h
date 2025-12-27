#ifndef CODEGEN_CODEGEN_H
#define CODEGEN_CODEGEN_H

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/GlobalVariable.h>
#include <parser/Ast.h>
#include <compiler/Compiler.h>
#include <utils/exceptions.h>
#include <utils/options.h>
#include <codegen/ClassMem.h>
#include <codegen/Function.h>
#include <string>
#include <unordered_map>
#include <deque>
#include <stack>
#include <cstdint>

#include <Cabbagelang/Class.h>

namespace codegen{
class CodeGen;
}
namespace symbols{
class Value;
}
#include <symbols/Value.h>

namespace codegen {

enum ExceptionalFnType {
    SETJMP_FN,
    LONGJMP_FN
};

using aValue = std::shared_ptr<symbols::Value>;

#define llvm_nullptr llvm::ConstantPointerNull::get(llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0))
#define retValue(value, type) aValue ret = symbols::create_value(value, type);

class CodeGen {
public:
    CodeGen(compiler::Compiler& compiler);
    ~CodeGen();
    llvm::Module* generate(const parser::aNode& ast);

private:
    // LLVM IR
    compiler::Compiler& mainCompiler;
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module;
    llvm::DataLayout DL;
    llvm::IRBuilder<> builder;
    llvm::IRBuilder<> initBuilder;

    // Exception
    llvm::GlobalVariable* __exception_hanging = nullptr;
    llvm::GlobalVariable* __exception_hanging_type = nullptr;

    // Main function
    void initMainFunction();
    void generateMainFunction();
    llvm::Function* mainFunc;

    // Maps
    std::unordered_map<std::string, Function*> functions;
    std::unordered_map<std::string, std::deque<ClassMem*>> classes;
    std::unordered_map<std::string, llvm::Type*> types;
    std::unordered_map<std::string, aValue> values;
    std::unordered_map<std::string, std::vector<std::string>> inhers;
    std::unordered_map<std::string, std::pair<llvm::BasicBlock*, llvm::BranchInst*>> labels;

    // Vectors
    std::vector<std::string> templateObjects;

    // Placeholders
    llvm::BasicBlock* LABEL_PLACEHOLDER = nullptr;
    llvm::BasicBlock* ENTRY_BLOCK_PLACEHOLDER = nullptr;
    llvm::BasicBlock* EXC_HANDLE_BLOCK_PLACEHOLDER = nullptr;

    // Current compiling
    std::string currentClass = "";
    int currentClassMemIndex = 1;
    Function* currentFunction = nullptr;
    llvm::Value* self;

    // Id
    int uniqueId = 0;
    int lableId = 0;

    // Flags
    bool defineConst = false;
    bool defineVolatile = false;

    // Break & Continue
    std::stack<llvm::BasicBlock*> breakStack;
    std::stack<llvm::BasicBlock*> continueStack;
    std::stack<llvm::GlobalVariable*> jmpBufStack;

    // Loop & Switch
    //TODO: Do it when loops are implemented

    // Emit functions
    void emit(const parser::aNode& ast);
    void emitPackage(const parser::aNode& ast);
    void emitCastFunction(const parser::aNode& ast);
    void emitClass(const parser::aNode& ast);
    void emitFunctionDeclaration(const parser::aNode& ast);
    void emitVariableDeclaration(const parser::aNode& ast);

        // Header functions
        void emitCastFunctionHeader(const parser::aNode& ast);
        void emitClassHeader(const parser::aNode& ast);
        void emitFunctionHeader(const parser::aNode& ast, Access access);
        void emitVariableHeader(const parser::aNode& ast, Access access);
    
    // Expression
    aValue emitExpression(const parser::aNode& ast);
    aValue emitPrimaryExpression(const parser::aNode& ast);
    aValue emitNew(const parser::aNode& ast);
    aValue emitSuper(const parser::aNode& ast);
    aValue emitPostfixExpression(aValue lhs, const parser::aNode& ast);
    std::pair<aValue, const parser::aNode&> emitMemberAccess(const parser::aNode& ast, aValue lhs);
    aValue emitUnaryExpression(const parser::aNode& ast);
    aValue emitCastExpression(const parser::aNode& ast);
    aValue emitMultiplicativeExpression(const parser::aNode& ast);
    aValue emitAdditiveExpression(const parser::aNode& ast);
    aValue emitShiftExpression(const parser::aNode& ast);
    aValue emitRelationalExpression(const parser::aNode& ast);
    aValue emitEqualityExpression(const parser::aNode& ast);
    aValue emitAndExpression(const parser::aNode& ast);
    aValue emitExclusiveOrExpression(const parser::aNode& ast);
    aValue emitInclusiveOrExpression(const parser::aNode& ast);
    aValue emitLogicalAndExpression(const parser::aNode& ast);
    aValue emitLogicalOrExpression(const parser::aNode& ast);
    aValue emitConditionalExpression(const parser::aNode& ast);
    aValue emitAssignmentExpression(const parser::aNode& ast);
    void emitIfStatement(const parser::aNode& ast);
    void emitWhileStatement(const parser::aNode& ast);
    void emitDoWhileStatement(const parser::aNode& ast);
    void emitForStatement(const parser::aNode& ast);
    void emitLabelStatement(const parser::aNode& ast);
    void emitGotoStatement(const parser::aNode& ast);
    void emitSwitchStatement(const parser::aNode& ast);
    void emitTryStatement(const parser::aNode& ast);
    void emitThrowStatement(const parser::aNode& ast);

    // Statements
    void emitReturn(const parser::aNode& ast);
    
    // Helper functions
    std::string getTypeStr(const parser::aNode& typeNode);
    llvm::Type* getType(const std::string& type, bool exact = false);
    llvm::Type* getType(const parser::aNode& typeNode);
    llvm::Type* getPtr();
    int getUniqueId();
    std::string getLabel();
    void Terminate(llvm::IRBuilder<>&);

    bool isExceptionType(const std::string& typeName);
    bool isSubclassOf(const std::string& child, const std::string& parent);
    llvm::Function* getExceptionalFunction(ExceptionalFnType type);
    aValue getString(const std::string& str);
    aValue getString(llvm::Value* srcPtr, int strSize);

        // Cast
        bool canCast(const std::string& fromType, const std::string& toType, std::vector<std::string> genericMembers);
        aValue cast(aValue from, const std::string& toType, std::vector<std::string> genericMembers);

        // Functional
        bool funcExists(const std::string& className, const std::string& funcName);
        std::pair<std::string, std::string> getOptFunc(const std::string& lType, const std::string& rType, const std::string& op);
        std::pair<std::string, std::string> getOptFunc(const std::string& type, const std::string& op);
        aValue callFunc(const std::string& className, const std::string& funcName, std::vector<aValue> args);

    // Friend classes & functions
    friend class symbols::Value;
    friend void cabbagelang::defineGenericMethods(llvm::Module* module, llvm::LLVMContext &context, std::unordered_map<std::string, codegen::Function*>& functions, std::unordered_map<std::string, llvm::Type*> types);

    // String messages
    llvm::Value* RERR001; // Null access
    const int RERR001_SIZE = 30;
};

typedef int NumericType;
NumericType getNumericType(const std::string& type);
std::string getNumericTypeStr(NumericType type);

} // namespace codegen

#endif // CODEGEN_CODEGEN_H