#include <compiler/Compiler.h>
symbols::ClassNames mainClassNames;

namespace compiler {

bool Compiler::compile_builtins(const char* source_file) {
    if(strcmp(source_file, "IO.cbg") == 0) {
        return true;
    }
    return false;
}

parser::aNode Compiler::compile(const char* source_file, const char* output_file, bool importation) {
    if(compile_builtins(source_file)) {
        return std::move(parser::create_ast_node(source_file, parser::AstKind::AST_IMPORT, lexer::TokenKind::TOKEN_IMPORT, 0, 0));
    }

    std::filesystem::path current_path = std::filesystem::current_path();

    current_filename = source_file;
    lexer::Lexer lexer(*this);

    std::ifstream source(source_file);
    if(!source.is_open()) {
        compiler_fatal("Failed to open source file: %s", source_file);
    }

    std::filesystem::current_path(
        std::filesystem::absolute(std::filesystem::path(source_file)).parent_path()
    );

    std::string source_str="";
    utils::strio<<source.rdbuf();
    source_str = utils::strio.str();
    source.close();
    utils::strio.clear();

    lexer.lex(source_str);

    std::deque<lexer::aToken> tokens = lexer.TokenList;
    
    parser::Parser parser(tokens, *this, mainClassNames);
    parser::aNode parse_result = parser.parse();
    lexer.free();

    std::filesystem::current_path(current_path);

    if(importation){
        return parse_result;
    }
    
    if(args::cabbage["--parse-only"] == true){
        utils::printAst(std::move(parse_result), 0);
        return nullptr;
    }

    codegen::CodeGen codegen(*this);
    symbols::Value::gen = &codegen;
    llvm::Module* module = codegen.generate(parse_result);

    if(!module) {
        compiler_fatal("Failed to generate IR");
    } else if (llvm::verifyModule(*module, &llvm::errs())) {
        module->print(llvm::outs(), nullptr);
        compiler_fatal("IR verification failed");
    }

    llvm::PassBuilder PB;
    llvm::ModuleAnalysisManager MAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::LoopAnalysisManager LAM;
    llvm::CGSCCAnalysisManager CGAM;

    PB.registerModuleAnalyses(MAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    llvm::ModulePassManager MPM;
    if (auto Err = PB.parsePassPipeline(MPM, "default<O3>")) {
        compiler_fatal("Failed to build pipeline: %s", llvm::toString(std::move(Err)).c_str());
    }
    
    MPM.run(*module, MAM);
    if (llvm::verifyModule(*module, &llvm::errs())) {
        module->print(llvm::errs(), nullptr);
        compiler_fatal("Optimization broke the module");
    }
    
    module->print(llvm::outs(), nullptr);
    MAM.clear();
    FAM.clear();
    LAM.clear();
    CGAM.clear();

    return nullptr;
}

}  // namespace compiler