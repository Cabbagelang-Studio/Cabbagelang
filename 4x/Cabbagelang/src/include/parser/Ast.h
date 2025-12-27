#ifndef PARSER_AST_H
#define PARSER_AST_H

#include <lexer/Token.h>
#include <deque>
#include <string>
#include <memory>

namespace parser {

enum AstKind {
    AST_PACKAGE,
    AST_IMPORT,
    AST_RETURN_STATEMENT,
    AST_IF_STATEMENT,
    AST_TRY_STATEMENT,
    AST_CATCH_STATEMENT,
    AST_FINALLY_STATEMENT,
    AST_THROW_STATEMENT,
    AST_WHILE_STATEMENT,
    AST_DO_WHILE_STATEMENT,
    AST_FOR_STATEMENT,
    AST_BREAK_STATEMENT,
    AST_CONTINUE_STATEMENT,
    AST_LABEL_STATEMENT,
    AST_GOTO_STATEMENT,
    AST_SWITCH_STATEMENT,
    AST_CASE_LABEL,
    AST_DEFAULT_LABEL,
    AST_CLASS_DECLARATION,
    AST_CLASS_PARENTS,
    AST_CLASS_PARENT,
    AST_IDENTIFIER,
    AST_PUBLIC, // default
    AST_PRIVATE,
    AST_PROTECTED,
    AST_SHARED,
    AST_VARIABLE_DECLARATION,
    AST_STATIC_VARIABLE_DECLARATION,
    AST_FUNCTION_DECLARATION,
    AST_INLINE_TAG,
    AST_STATIC_FUNCTION_DECLARATION,
    AST_CAST_FUNCTION_DECLARATION,
    AST_FUNCTION_PARAMETERS,
    AST_ELLIPSIS_PARAMETER,
    AST_COMPOUND_STATEMENT,
    AST_OPERATOR,
    AST_PRIMARY_EXPRESSION,
    AST_NEW,
    AST_SUPER,
    AST_EXPRESSION,
    AST_ASSIGNMENT_EXPRESSION,
    AST_CONDITIONAL_EXPRESSION,
    AST_LOGICAL_OR_EXPRESSION,
    AST_LOGICAL_AND_EXPRESSION,
    AST_INCLUSIVE_OR_EXPRESSION,
    AST_EXCLUSIVE_OR_EXPRESSION,
    AST_AND_EXPRESSION,
    AST_EQUALITY_EXPRESSION,
    AST_RELATIONAL_EXPRESSION,
    AST_SHIFT_EXPRESSION,
    AST_ADDITIVE_EXPRESSION,
    AST_MULTIPLICATIVE_EXPRESSION,
    AST_CAST_EXPRESSION,
    AST_UNARY_EXPRESSION,
    AST_POSTFIX_EXPRESSION,
    AST_TYPE_NAME,
    AST_TYPE_QUALIFIER,
    AST_TYPE_SPECIFIER,
    AST_TYPE_TEMPLATE,
    AST_ELLIPSIS_TYPE,
    AST_EMPTY_STATEMENT
};

class AstNode {
    public:
        AstNode(std::string value, AstKind kind, lexer::TokenKind token_kind, int line, int column);
        virtual ~AstNode();
        void add_child(std::unique_ptr<AstNode> child);
        AstKind kind;
        int line, column;
        std::string value;
        lexer::TokenKind token_kind;
        std::deque<std::unique_ptr<AstNode>> children;
};

using aNode = std::unique_ptr<AstNode>;

aNode create_ast_node(std::string value, AstKind kind, lexer::TokenKind token_kind, int line, int column);

}

#endif // PARSER_AST_H