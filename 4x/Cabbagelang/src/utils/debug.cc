#include <utils/debug.h>

namespace utils {

const std::unordered_map<parser::AstKind, std::string> AstKindNames = {
    {parser::AstKind::AST_PACKAGE, "AST_PACKAGE"},
    {parser::AstKind::AST_IMPORT, "AST_IMPORT"},
    {parser::AstKind::AST_RETURN_STATEMENT, "AST_RETURN_STATEMENT"},
    {parser::AstKind::AST_IF_STATEMENT, "AST_IF_STATEMENT"},
    {parser::AstKind::AST_TRY_STATEMENT, "AST_TRY_STATEMENT"},
    {parser::AstKind::AST_CATCH_STATEMENT, "AST_CATCH_STATEMENT"},
    {parser::AstKind::AST_FINALLY_STATEMENT, "AST_FINALLY_STATEMENT"},
    {parser::AstKind::AST_THROW_STATEMENT, "AST_THROW_STATEMENT"},
    {parser::AstKind::AST_WHILE_STATEMENT, "AST_WHILE_STATEMENT"},
    {parser::AstKind::AST_DO_WHILE_STATEMENT, "AST_DO_WHILE_STATEMENT"},
    {parser::AstKind::AST_FOR_STATEMENT, "AST_FOR_STATEMENT"},
    {parser::AstKind::AST_BREAK_STATEMENT, "AST_BREAK_STATEMENT"},
    {parser::AstKind::AST_CONTINUE_STATEMENT, "AST_CONTINUE_STATEMENT"},
    {parser::AstKind::AST_LABEL_STATEMENT, "AST_LABEL_STATEMENT"},
    {parser::AstKind::AST_GOTO_STATEMENT, "AST_GOTO_STATEMENT"},
    {parser::AstKind::AST_SWITCH_STATEMENT, "AST_SWITCH_STATEMENT"},
    {parser::AstKind::AST_CASE_LABEL, "AST_CASE_LABEL"},
    {parser::AstKind::AST_DEFAULT_LABEL, "AST_DEFAULT_LABEL"},
    {parser::AstKind::AST_CLASS_DECLARATION, "AST_CLASS_DECLARATION"},
    {parser::AstKind::AST_CLASS_PARENTS, "AST_CLASS_PARENTS"},
    {parser::AstKind::AST_CLASS_PARENT, "AST_CLASS_PARENT"},
    {parser::AstKind::AST_IDENTIFIER, "AST_IDENTIFIER"},
    {parser::AstKind::AST_PUBLIC, "AST_PUBLIC"},
    {parser::AstKind::AST_PRIVATE, "AST_PRIVATE"},
    {parser::AstKind::AST_PROTECTED, "AST_PROTECTED"},
    {parser::AstKind::AST_SHARED, "AST_SHARED"},
    {parser::AstKind::AST_VARIABLE_DECLARATION, "AST_VARIABLE_DECLARATION"},
    {parser::AstKind::AST_STATIC_VARIABLE_DECLARATION, "AST_STATIC_VARIABLE_DECLARATION"},
    {parser::AstKind::AST_FUNCTION_DECLARATION, "AST_FUNCTION_DECLARATION"},
    {parser::AstKind::AST_INLINE_TAG, "AST_INLINE_TAG"},
    {parser::AstKind::AST_STATIC_FUNCTION_DECLARATION, "AST_STATIC_FUNCTION_DECLARATION"},
    {parser::AstKind::AST_CAST_FUNCTION_DECLARATION, "AST_CAST_FUNCTION_DECLARATION"},
    {parser::AstKind::AST_FUNCTION_PARAMETERS, "AST_FUNCTION_PARAMETERS"},
    {parser::AstKind::AST_ELLIPSIS_PARAMETER, "AST_ELLIPSIS_PARAMETER"},
    {parser::AstKind::AST_COMPOUND_STATEMENT, "AST_COMPOUND_STATEMENT"},
    {parser::AstKind::AST_OPERATOR, "AST_OPERATOR"},
    {parser::AstKind::AST_PRIMARY_EXPRESSION, "AST_PRIMARY_EXPRESSION"},
    {parser::AstKind::AST_NEW, "AST_NEW"},
    {parser::AstKind::AST_SUPER, "AST_SUPER"},
    {parser::AstKind::AST_EXPRESSION, "AST_EXPRESSION"},
    {parser::AstKind::AST_ASSIGNMENT_EXPRESSION, "AST_ASSIGNMENT_EXPRESSION"},
    {parser::AstKind::AST_CONDITIONAL_EXPRESSION, "AST_CONDITIONAL_EXPRESSION"},
    {parser::AstKind::AST_LOGICAL_OR_EXPRESSION, "AST_LOGICAL_OR_EXPRESSION"},
    {parser::AstKind::AST_LOGICAL_AND_EXPRESSION, "AST_LOGICAL_AND_EXPRESSION"},
    {parser::AstKind::AST_INCLUSIVE_OR_EXPRESSION, "AST_INCLUSIVE_OR_EXPRESSION"},
    {parser::AstKind::AST_EXCLUSIVE_OR_EXPRESSION, "AST_EXCLUSIVE_OR_EXPRESSION"},
    {parser::AstKind::AST_AND_EXPRESSION, "AST_AND_EXPRESSION"},
    {parser::AstKind::AST_EQUALITY_EXPRESSION, "AST_EQUALITY_EXPRESSION"},
    {parser::AstKind::AST_RELATIONAL_EXPRESSION, "AST_RELATIONAL_EXPRESSION"},
    {parser::AstKind::AST_SHIFT_EXPRESSION, "AST_SHIFT_EXPRESSION"},
    {parser::AstKind::AST_ADDITIVE_EXPRESSION, "AST_ADDITIVE_EXPRESSION"},
    {parser::AstKind::AST_MULTIPLICATIVE_EXPRESSION, "AST_MULTIPLICATIVE_EXPRESSION"},
    {parser::AstKind::AST_CAST_EXPRESSION, "AST_CAST_EXPRESSION"},
    {parser::AstKind::AST_UNARY_EXPRESSION, "AST_UNARY_EXPRESSION"},
    {parser::AstKind::AST_POSTFIX_EXPRESSION, "AST_POSTFIX_EXPRESSION"},
    {parser::AstKind::AST_TYPE_NAME, "AST_TYPE_NAME"},
    {parser::AstKind::AST_TYPE_QUALIFIER, "AST_TYPE_QUALIFIER"},
    {parser::AstKind::AST_TYPE_SPECIFIER, "AST_TYPE_SPECIFIER"},
    {parser::AstKind::AST_TYPE_TEMPLATE, "AST_TYPE_TEMPLATE"},
    {parser::AstKind::AST_ELLIPSIS_TYPE, "AST_ELLIPSIS_TYPE"},
    {parser::AstKind::AST_EMPTY_STATEMENT, "AST_EMPTY_STATEMENT"}
};
    
void printAst(const parser::aNode& node, int depth) {
    for(int i = 0; i < depth; i++) {
        std::cout<<"  ";
    }
    std::string kindName = AstKindNames.at(node->kind);
    std::cout<<kindName<<": ";
    std::cout<<node->value;
    std::cout<<"("<<node->line<<":"<<node->column<<")"<<std::endl;
    for(auto& child : node->children) {
        printAst(std::move(child), depth+1);
    }
}

void print(std::string str, ...) {
    va_list args;
    va_start(args, str);
    vprintf(str.c_str(), args);
    printf("\n");
    va_end(args);
}

} // namespace utils