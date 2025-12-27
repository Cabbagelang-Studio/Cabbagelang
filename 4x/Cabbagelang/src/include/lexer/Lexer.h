#ifndef LEXER_LEXER_H
#define LEXER_LEXER_H

#include <lexer/Token.h>
#include <symbols/ClassNames.h>
#include <compiler/Compiler.h>
#include <string>
#include <cctype>
#include <regex>
#include <vector>
#include <utility>
#include <log/log.h>
#include <deque>

namespace lexer {

class Lexer {
public:
    std::deque<aToken> TokenList;
    compiler::Compiler& mainCompiler;

    Lexer(compiler::Compiler& compiler);

    void lex(const std::string& input);
    void free();

private:
    int current_line = 1;
    int current_column = 1;

    void add_token(TokenKind type, const std::string& value);
    void comment(const std::string& input, size_t& pos);
    bool matchKeyword(const std::string& input, size_t& pos);
    bool matchLiteral(const std::string& input, size_t& pos);
    bool matchOperator(const std::string& input, size_t& pos);
};

} // namespace lexer

#endif // LEXER_LEXER_H
