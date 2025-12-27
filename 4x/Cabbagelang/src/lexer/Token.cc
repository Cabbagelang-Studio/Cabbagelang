#include <lexer/Token.h>
namespace lexer{

aToken create_token(TokenKind kind, const std::string& data, int line, int column){
    aToken token = new Token{kind, data, line, column};
    if(!token)
        compiler_fatal("Failed to allocate memory in create_token");
    return token;
}

} // namespace lexer