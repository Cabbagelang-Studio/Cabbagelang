#include <parser/Ast.h>

namespace parser{

AstNode::AstNode(std::string value, AstKind kind, lexer::TokenKind token_kind, int line, int column): value(value), kind(kind), token_kind(token_kind), line(line), column(column){}
AstNode::~AstNode(){}

void AstNode::add_child(aNode node){
    children.push_back(std::move(node));
}

aNode create_ast_node(std::string value, AstKind kind, lexer::TokenKind token_kind, int line, int column){
    return std::make_unique<AstNode>(value, kind, token_kind, line, column);
}

}