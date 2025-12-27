#ifndef CABBAGELANG_TYPES_H
#define CABBAGELANG_TYPES_H

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

void defineTypesClass(llvm::Module* module, llvm::LLVMContext &context, std::unordered_map<std::string, std::deque<codegen::ClassMem*>>& classMemMap, std::unordered_map<std::string, codegen::Function*>& functions, std::unordered_map<std::string, llvm::Type*>& types, std::unordered_map<std::string, std::vector<std::string>>& inhers);

} // namespace cabbagelang

#endif