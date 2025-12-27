#include <Cabbagelang/cast.h>

namespace cabbagelang {
    
void defineCastClass(llvm::LLVMContext &context, std::unordered_map<std::string, std::deque<codegen::ClassMem*>>& classMemMap, std::unordered_map<std::string, llvm::Type*>& types, std::unordered_map<std::string, std::vector<std::string>>& inhers) {
    types["cast"] = llvm::StructType::create(context, "cast");
    inhers["cast"] = {};
    classMemMap["cast"] = std::deque<codegen::ClassMem*>(0); // Initialize the class member map
}

}