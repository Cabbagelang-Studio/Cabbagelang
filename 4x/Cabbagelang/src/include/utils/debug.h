#ifndef UTILS_DEBUG_H
#define UTILS_DEBUG_H

#include <iostream>
#include <unordered_map>
#include <parser/Ast.h>
#include <lexer/Token.h>
#include <string>
#include <stdarg.h>

namespace utils {
    extern const std::unordered_map<parser::AstKind, std::string> AstKindNames;
    void printAst(const parser::aNode& node, int depth);
    void print(std::string, ...);
}

#endif // UTILS_DEBUG_H