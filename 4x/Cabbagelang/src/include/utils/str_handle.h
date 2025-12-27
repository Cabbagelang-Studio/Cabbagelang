#ifndef UTILS_STR_HANDLE_H
#define UTILS_STR_HANDLE_H

#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>

#include <sstream>
#include <string>
#include <string.h>

namespace utils {

extern std::stringstream strio;

std::string typeToString(llvm::Type *type);
std::string splitFirst(std::string str, std::string delim);

}

#endif // UTILS_STR_HANDLE_H