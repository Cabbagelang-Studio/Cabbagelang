#ifndef COMPILER_COMPILER_H
#define COMPILER_COMPILER_H

#include <symbols/ClassNames.h>
#include <lexer/Token.h>
#include <parser/Ast.h>
#include <utils/debug.h>
#include <args/Args.h>
#include <fstream>
#include <string>
#include <filesystem>
#include <stdio.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>

extern symbols::ClassNames mainClassNames;

namespace compiler {

class Compiler {
    public:
        parser::aNode compile(const char *source_file, const char *output_file, bool importation);
        std::string current_filename="";
        std::string current_package="";
    private:
        bool compile_builtins(const char *source_file);
};

} // namespace compiler

#include <lexer/Lexer.h>
#include <parser/Parser.h>
#include <codegen/CodeGen.h>

#endif // COMPILER_COMPILER_H