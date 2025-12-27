#include <lexer/Lexer.h>

namespace lexer {

Lexer::Lexer(compiler::Compiler& compiler)
    : mainCompiler(compiler), current_line(1), current_column(1) {
    TokenList.resize(0);
}

void Lexer::lex(const std::string& input) {
    size_t pos = 0;

    while (pos < input.size()) {
        char currentChar = input[pos];
        if (std::isspace(currentChar)) {
            if (currentChar == '\n') {
                current_line++;
                current_column = 1;
            } else {
                current_column++;
            }
            ++pos;
            continue;
        }

        if (currentChar == '/' && pos + 1 < input.size() && input[pos + 1] == '*') {
            comment(input, pos);
            continue;
        }

        if (currentChar == '/' && pos + 1 < input.size() && input[pos + 1] == '/') {
            while (pos < input.size() && input[pos] != '\n') {
                ++pos;
            }
            continue;
        }

        // Match keywords
        if (matchKeyword(input, pos)) continue;

        // Match Literals and Identifiers
        if (matchLiteral(input, pos)) continue;

        // Match operators and symbols
        if (matchOperator(input, pos)) continue;

        // Error for invalid character
        compilation_error(mainCompiler.current_filename.data(), current_line, current_column, "Invalid character");
        ++pos; // move to the next character after error
    }

    warning_error_summary();
}

void Lexer::add_token(TokenKind type, const std::string& value) {
    TokenList.push_back(create_token(type, value, current_line, current_column));
    current_column += (int)value.size();
}

void Lexer::comment(const std::string& input, size_t& pos) {
    // Handle multi-line comments "/* ... */"
    pos += 2; // Skip "/*"
    int line=0;
    while (pos + 1 < input.size()) {
        if (input[pos] == '*' && input[pos + 1] == '/') {
            pos += 2; // Skip "*/"
            current_line+=line;
            return;
        }
        if (input[pos] == '\n') {
            line++;
        }
        ++pos;
    }
    compilation_error(mainCompiler.current_filename.data(), current_line, current_column, "Unterminated comment");
}

bool Lexer::matchKeyword(const std::string& input, size_t& pos) {
    static const std::unordered_map<std::string, TokenKind> keywords = {
        {"int", TOKEN_INT}, {"short", TOKEN_SHORT}, {"long", TOKEN_LONG},
        {"float", TOKEN_FLOAT}, {"double", TOKEN_DOUBLE}, {"char", TOKEN_CHAR},
        {"void", TOKEN_VOID}, {"bool", TOKEN_BOOL}, {"byte", TOKEN_BYTE},
        {"true", TOKEN_TRUE}, {"false", TOKEN_FALSE}, {"null", TOKEN_NULL},
        {"and", TOKEN_AND}, {"or", TOKEN_OR}, {"not", TOKEN_NOT}, 
        {"const", TOKEN_CONST}, {"volatile", TOKEN_VOLATILE}, {"static", TOKEN_STATIC}, {"cast", TOKEN_CAST}, {"inline", TOKEN_INLINE},
        {"break", TOKEN_BREAK}, {"case", TOKEN_CASE}, {"switch", TOKEN_SWITCH},
        {"continue", TOKEN_CONTINUE}, {"default", TOKEN_DEFAULT}, {"label", TOKEN_LABEL},
        {"goto", TOKEN_GOTO}, {"return", TOKEN_RETURN}, {"if", TOKEN_IF}, {"else", TOKEN_ELSE},
        {"for", TOKEN_FOR}, {"while", TOKEN_WHILE}, {"do", TOKEN_DO},
        {"import", TOKEN_IMPORT}, {"package", TOKEN_PACKAGE}, {"quote", TOKEN_QUOTE},
        {"class", TOKEN_CLASS}, {"public", TOKEN_PUBLIC}, {"protected", TOKEN_PROTECTED}, {"shared", TOKEN_SHARED},
        {"private", TOKEN_PRIVATE}, {"new", TOKEN_NEW}, {"self", TOKEN_SELF}, {"super", TOKEN_SUPER}, {"operator", TOKEN_OPERATOR},
        {"try", TOKEN_TRY}, {"catch", TOKEN_CATCH}, {"throw", TOKEN_THROW}, {"finally", TOKEN_FINALLY}
    };

    for (const auto& [keyword, tokenType] : keywords) {
        if (pos>0 && isalnum(input[pos-1])) continue;
        if (pos + keyword.size() < input.size() && isalnum(input[pos + keyword.size()])) continue;
        if (input.substr(pos, keyword.size()) == keyword) {
            add_token(tokenType, keyword);
            pos += keyword.size();
            return true;
        }
    }
    return false;
}

bool Lexer::matchLiteral(const std::string& input, size_t& pos) {
    // Move numeric patterns before identifier
    static const std::regex floatLiteral(R"(\d+\.\d+([eE][+-]?\d+)?[fFlL]?|\.\d+([eE][+-]?\d+)?[fFlL]?|\d+[eE][+-]?\d+[fFlL]?)");
    static const std::regex intLiteral(R"(\d+([uU]?[lL]?|[lL]?[uU]?))");
    static const std::regex stringLiteral(R"("([^"\\\n]|\\.)*")");
    static const std::regex charLiteral(R"('([^'\\\n]|\\.)*')");
    static const std::regex identifierLiteral(R"([A-Za-z_][A-Za-z0-9_]*)");

    std::string search_str = input.substr(pos);
    std::smatch match;

    // Match floating-point literal first (since it's more specific)
    if (std::regex_search(search_str, match, floatLiteral) && match.position(0) == 0) {
        add_token(TOKEN_FLOAT_LITERAL, match.str(0));
        pos += match.length(0);
        return true;
    }

    // Then match integer literal
    if (std::regex_search(search_str, match, intLiteral) && match.position(0) == 0) {
        add_token(TOKEN_INT_LITERAL, match.str(0));
        pos += match.length(0);
        return true;
    }

    // Then other patterns...
    if (std::regex_search(search_str, match, stringLiteral) && match.position(0) == 0) {
        add_token(TOKEN_STRING_LITERAL, match.str(0));
        pos += match.length(0);
        return true;
    }

    if (std::regex_search(search_str, match, charLiteral) && match.position(0) == 0) {
        add_token(TOKEN_CHAR_LITERAL, match.str(0));
        pos += match.length(0);
        return true;
    }

    // Identifier last
    if (std::regex_search(search_str, match, identifierLiteral) && match.position(0) == 0) {
        add_token(TOKEN_IDENTIFIER, match.str(0));
        pos += match.length(0);
        return true;
    }

    return false;
}


bool Lexer::matchOperator(const std::string& input, size_t& pos) {
    static const std::vector<std::pair<std::string, TokenKind>> operators = {
        {"...", TOKEN_ELLIPSIS}, {">>=", TOKEN_SHIFT_RIGHT_ASSIGN}, {"<<=", TOKEN_SHIFT_LEFT_ASSIGN},
        {"+=", TOKEN_ADD_ASSIGN}, {"-=", TOKEN_SUB_ASSIGN}, {"*=", TOKEN_MUL_ASSIGN},
        {"/=", TOKEN_DIV_ASSIGN}, {"%=", TOKEN_MOD_ASSIGN}, {"&=", TOKEN_BIT_AND_ASSIGN},
        {"^=", TOKEN_BIT_XOR_ASSIGN}, {"|=", TOKEN_BIT_OR_ASSIGN}, {"~=", TOKEN_BIT_NOT_ASSIGN},
        {">>", TOKEN_SHIFT_RIGHT}, {"<<", TOKEN_SHIFT_LEFT}, {"++", TOKEN_INC}, {"--", TOKEN_DEC},
        {"&&", TOKEN_AND}, {"||", TOKEN_OR}, {"<=", TOKEN_LTE}, {">=", TOKEN_GTE},
        {"==", TOKEN_EQ}, {"!=", TOKEN_NEQ}, {";", TOKEN_SEMICOLON}, {"{", TOKEN_LBRACE}, {"}", TOKEN_RBRACE},
        {",", TOKEN_COMMA}, {":", TOKEN_COLON}, {"=", TOKEN_ASSIGN}, {"(", TOKEN_LPAREN}, {")", TOKEN_RPAREN},
        {"[", TOKEN_LBRACKET}, {"]", TOKEN_RBRACKET}, {".", TOKEN_DOT}, {"&", TOKEN_BIT_AND}, {"!", TOKEN_NOT},
        {"~", TOKEN_BIT_NOT}, {"-", TOKEN_SUB}, {"+", TOKEN_ADD}, {"*", TOKEN_MUL}, {"/", TOKEN_DIV},
        {"%", TOKEN_MOD}, {"<", TOKEN_LT}, {">", TOKEN_GT}, {"^", TOKEN_BIT_XOR}, {"|", TOKEN_BIT_OR},
        {"?", TOKEN_QUESTION}
    };

    for (const auto& [op, tokenType] : operators) {
        if (input.size() >= pos + op.size() && input.compare(pos, op.size(), op) == 0) {
            add_token(tokenType, op);
            pos += op.size();
            return true;
        }
    }
    return false;
}

void Lexer::free(){
    for (auto& token : TokenList)
        delete token;
}

} // namespace lexer
