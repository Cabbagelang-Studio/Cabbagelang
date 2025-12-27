#ifndef CABBAGELANG_STRING_H
#define CABBAGELANG_STRING_H

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>
#include <codegen/ClassMem.h>
#include <codegen/Function.h>
#include <unordered_map>
#include <deque>
#include <utils/options.h>
#include <cabbagelang/Class.h>
namespace cabbagelang {

#define llvm_nullptr llvm::ConstantPointerNull::get(llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0))

void defineStringClass(llvm::Module* module, llvm::LLVMContext &context, std::unordered_map<std::string, std::deque<codegen::ClassMem*>>& classMemMap, std::unordered_map<std::string, codegen::Function*>& functions, std::unordered_map<std::string, llvm::Type*>& types, std::unordered_map<std::string, std::vector<std::string>>& inhers);

} // namespace cabbagelang

#endif