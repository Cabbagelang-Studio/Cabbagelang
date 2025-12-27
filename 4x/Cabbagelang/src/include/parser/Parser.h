#ifndef PARSER_PARSER_H  
#define PARSER_PARSER_H

#include <compiler/Compiler.h>
#include <lexer/Token.h>
#include <parser/Ast.h>
#include <log/log.h>
#include <utils/exceptions.h>

#define IS_TYPE_NAME (match(lexer::TokenKind::TOKEN_INT) || match(lexer::TokenKind::TOKEN_CHAR) || match(lexer::TokenKind::TOKEN_SHORT) || \
        match(lexer::TokenKind::TOKEN_LONG) || match(lexer::TokenKind::TOKEN_FLOAT) || match(lexer::TokenKind::TOKEN_DOUBLE) ||\
        match(lexer::TokenKind::TOKEN_BOOL) || match(lexer::TokenKind::TOKEN_BYTE) || match(lexer::TokenKind::TOKEN_VOID) ||\
        (match(lexer::TokenKind::TOKEN_IDENTIFIER) && mainClassNames.lookup(current_token()->data)))

namespace parser{

class Parser{
    public:
        Parser(const std::deque<lexer::aToken>& tokens, compiler::Compiler& compiler, symbols::ClassNames& classNames);
        ~Parser();
        aNode parse();

    private:
        compiler::Compiler& mainCompiler;
        symbols::ClassNames& mainClassNames;

        std::deque<lexer::aToken> tokens;
        size_t current_position = 0;

        int loopDepth = 0, switchDepth = 0;

        aNode parse_package();

        aNode parse_import();
        aNode parse_quote();
        aNode parse_cast_function_declaration();
        aNode parse_class_declaration();

        aNode parse_variable_declaration();
        aNode parse_function_declaration();

        aNode parse_compound_statement();
        aNode parse_statement();
        aNode parse_statement_body();

        aNode parse_return_statement();
        aNode parse_if_statement();
        aNode parse_try_statement();
        aNode parse_throw_statement();
        aNode parse_while_statement();
        aNode parse_do_while_statement();
        aNode parse_for_statement();
        aNode parse_label_statement();
        aNode parse_goto_statement();
        aNode parse_switch_statement();

        aNode parse_primary_expression(bool constant = false, bool typeClass = false);
        aNode parse_expression();
        aNode parse_assignment_expression();
        aNode parse_conditional_expression();
        aNode parse_logical_or_expression();
        aNode parse_logical_and_expression();
        aNode parse_inclusive_or_expression();
        aNode parse_exclusive_or_expression();
        aNode parse_and_expression();
        aNode parse_equality_expression();
        aNode parse_relational_expression();
        aNode parse_shift_expression();
        aNode parse_additive_expression();
        aNode parse_multiplicative_expression();
        aNode parse_cast_expression();
        aNode parse_unary_expression();
        aNode parse_postfix_expression();

        aNode parse_type_name();

        aNode add_node(AstKind kind);
        aNode add_node(AstKind kind, lexer::aToken token);
        void consume();  // Consume the current token and move to the next one
        lexer::aToken current_token();  // Get the current token
        lexer::aToken current_token(int offset);  // Get the token at a given offset
        bool match(lexer::TokenKind kind);  // Check if the current token matches a given type
        bool match(lexer::TokenKind kind, int offset);  // Check if the token at a given offset matches a given type
};

}

#endif //PARSER_PARSER_H