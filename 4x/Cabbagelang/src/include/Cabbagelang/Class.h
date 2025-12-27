#ifndef CABBAGELANG_CLASS_H
#define CABBAGELANG_CLASS_H

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>
#include <codegen/ClassMem.h>
#include <codegen/Function.h>
#include <unordered_map>
#include <deque>
#include <utils/options.h>
#include <cabbagelang/Class.h>
namespace cabbagelang {

llvm::Type* getPtr(llvm::LLVMContext &context);
void defineBuiltinClasses(llvm::Module* module, llvm::LLVMContext &context, std::unordered_map<std::string, std::deque<codegen::ClassMem*>>& classMemMap, std::unordered_map<std::string, codegen::Function*>& functions, std::unordered_map<std::string, llvm::Type*>& types, std::unordered_map<std::string, std::vector<std::string>>& inhers);

} // namespace cabbagelang

#include <Cabbagelang/cast.h>
#include <Cabbagelang/String.h>
#include <Cabbagelang/Exception.h>
#include <Cabbagelang/Generic.h>
#include <Cabbagelang/Types.h>

#endif