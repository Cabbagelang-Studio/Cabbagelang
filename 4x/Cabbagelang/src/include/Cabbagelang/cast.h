#ifndef CABBAGELANG_CAST_H
#define CABBAGELANG_CAST_H

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>
#include <codegen/ClassMem.h>
#include <unordered_map>
#include <deque>
namespace cabbagelang {

void defineCastClass(llvm::LLVMContext &context, std::unordered_map<std::string, std::deque<codegen::ClassMem*>>& classMemMap, std::unordered_map<std::string, llvm::Type*>& types, std::unordered_map<std::string, std::vector<std::string>>& inhers);

} // namespace cabbagelang

#endif