#include <parser/Parser.h>

namespace parser {
    
Parser::Parser(const std::deque<lexer::aToken> &tokens, compiler::Compiler& compiler, symbols::ClassNames& classNames)
    : tokens(tokens), mainCompiler(compiler), mainClassNames(classNames){}

Parser::~Parser() {}

aNode Parser::parse() {
    aNode package = parse_package();
    mainClassNames.check_quoted(mainCompiler.current_filename.data());
    warning_error_summary();
    return package;
}

aNode Parser::parse_package() {
    if(!match(lexer::TokenKind::TOKEN_PACKAGE)){
        compilation_error(mainCompiler.current_filename.data(), current_token()->line, current_token()->column, "Expected \"package\"");
        return nullptr;
    }
    aNode package = add_node(AST_PACKAGE);
    mainCompiler.current_package = current_token()->data;
    consume();
    package->add_child(parse_postfix_expression());
    if(!match(lexer::TokenKind::TOKEN_SEMICOLON)){
        compilation_error(mainCompiler.current_filename.data(), current_token()->line, current_token()->column, "Unexpected \"%s\"", current_token()->data.data());
        return nullptr;
    }
    consume();
    while (current_position < tokens.size()) {
        bool error_caught = false;
        size_t position = current_position;
        try{
            package->add_child(parse_class_declaration());
        } catch (utils::NotClassDefinitionException) {
            try{
                current_position = position;
                package->add_child(parse_import());
            } catch (utils::NotImportException) {
                try{
                    current_position = position;
                    package->add_child(parse_quote());
                } catch (utils::NotQuoteException) { 
                    try{
                        current_position = position;
                        package->add_child(parse_cast_function_declaration());
                    } catch (std::out_of_range) {
                        compilation_error(mainCompiler.current_filename.data(), tokens.back()->line, tokens.back()->column, "Unexpected end of file");
                        error_caught = true;
                    } catch (std::exception& e) {
                        compilation_error(mainCompiler.current_filename.data(), current_token()->line, current_token()->column, 
                            "Importation, quote, cast function or class declaration expected, but got \"%s\"", current_token()->data.data());
                        error_caught = true;
                        consume();
                    }
                } catch (std::out_of_range) {
                    compilation_error(mainCompiler.current_filename.data(), tokens.back()->line, tokens.back()->column, "Unexpected end of file");
                    error_caught = true;
                } catch (std::exception& e) {
                    compilation_error(mainCompiler.current_filename.data(), current_token()->line, current_token()->column, e.what());
                    error_caught = true;
                    consume();
                }
            } catch (std::out_of_range) {
                compilation_error(mainCompiler.current_filename.data(), tokens.back()->line, tokens.back()->column, "Unexpected end of file");
                error_caught = true;
            } catch (std::exception& e) {
                compilation_error(mainCompiler.current_filename.data(), current_token()->line, current_token()->column, e.what());
                error_caught = true;
                consume();
            }
        } catch (std::out_of_range) {
            compilation_error(mainCompiler.current_filename.data(), tokens.back()->line, tokens.back()->column, "Unexpected end of file");
            error_caught = true;
        }  catch (std::exception& e) {
            compilation_error(mainCompiler.current_filename.data(), current_token()->line, current_token()->column, e.what());
            error_caught = true;
            consume();
        }
        try{
            while(error_caught && !match(lexer::TokenKind::TOKEN_CLASS) && !match(lexer::TokenKind::TOKEN_IMPORT))
                consume();
        } catch (std::out_of_range) {
            compilation_error(mainCompiler.current_filename.data(), tokens.back()->line, tokens.back()->column, "Unexpected end of file");
        }
    }
    return package;
}

aNode Parser::parse_class_declaration() {
    int access = 0; // 0 for public, 1 for private
    if(match(lexer::TokenKind::TOKEN_PUBLIC)){
        access = 0;
        consume();
    } else if(match(lexer::TokenKind::TOKEN_PRIVATE)){
        access = 1;
        consume();
    }

    if(!match(lexer::TokenKind::TOKEN_CLASS)){
        throw utils::NotClassDefinitionException("Class definition expected");
    }
    consume();
    if(!match(lexer::TokenKind::TOKEN_IDENTIFIER))
        throw utils::SyntaxErrorException("Expected class name, but got \""+current_token()->data+"\"");
    aNode class_node = add_node(AST_CLASS_DECLARATION);
    lexer::aToken class_name = current_token();
    consume();
    mainClassNames.insert(class_name->data, mainCompiler.current_package, mainCompiler.current_filename.data(), class_name->line, class_name->column, access);
    if(match(lexer::TokenKind::TOKEN_LPAREN)){
        aNode parents = add_node(AST_CLASS_PARENTS);
        consume();
        while(!match(lexer::TokenKind::TOKEN_RPAREN)){
            if(!match(lexer::TokenKind::TOKEN_IDENTIFIER))
                throw utils::SyntaxErrorException("Expected class name, but got \""+current_token()->data+"\"");
            parents->add_child(add_node(AST_CLASS_PARENT));
            consume();
            if(!match(lexer::TokenKind::TOKEN_RPAREN) && !match(lexer::TokenKind::TOKEN_COMMA))
                throw utils::SyntaxErrorException("Expected ',' or ')', but got \""+current_token()->data+"\"");
            if(match(lexer::TokenKind::TOKEN_COMMA))
                consume();
        }consume();
        class_node->add_child(std::move(parents));
    }
    if(!match(lexer::TokenKind::TOKEN_LBRACE))
        throw utils::SyntaxErrorException("Expected {, but got \""+current_token()->data+"\"");
    consume();
    while(!match(lexer::TokenKind::TOKEN_RBRACE)){
        aNode privacy = nullptr;
        if(match(lexer::TokenKind::TOKEN_PUBLIC)){
            privacy = add_node(AST_PUBLIC);
            consume();
        } else if(match(lexer::TokenKind::TOKEN_PRIVATE)){
            privacy = add_node(AST_PRIVATE);
            consume();
        } else if(match(lexer::TokenKind::TOKEN_PROTECTED)){
            privacy = add_node(AST_PROTECTED);
            consume();
        } else if(match(lexer::TokenKind::TOKEN_SHARED)){
            privacy = add_node(AST_SHARED);
            consume();
        } else {
            privacy = add_node(AST_PUBLIC);
        }
        size_t position = current_position;
        bool error_caught = false;
        try{
            privacy->add_child(parse_function_declaration());
        } catch (utils::NotFunctionException) {
            try{
                current_position = position;
                privacy->add_child(parse_variable_declaration());
            } catch (utils::NotVariableException) {
                throw utils::SyntaxErrorException("Expected variable or function declaration, but got \""+current_token()->data+"\"");
                error_caught = true;
                consume();
            } catch (std::exception& e) {
                compilation_error(mainCompiler.current_filename.data(), current_token()->line, current_token()->column, e.what());
                error_caught = true;
                consume();
            }
        } catch (std::exception& e) {
            compilation_error(mainCompiler.current_filename.data(), current_token()->line, current_token()->column, e.what());
            error_caught = true;
            consume();
        }
        try{
            while(error_caught && !match(lexer::TokenKind::TOKEN_PUBLIC) && !match(lexer::TokenKind::TOKEN_PRIVATE) && !match(lexer::TokenKind::TOKEN_PROTECTED) && !match(lexer::TokenKind::TOKEN_SHARED) &&
                !match(lexer::TokenKind::TOKEN_STATIC) && !match(lexer::TokenKind::TOKEN_INLINE) && !match(lexer::TokenKind::TOKEN_CONST) && !match(lexer::TokenKind::TOKEN_VOLATILE) &&
                !IS_TYPE_NAME)
                consume();
        } catch (std::out_of_range) {
            throw std::out_of_range("Unexpected end of file");
        }
        if (!error_caught)
            class_node->add_child(std::move(privacy));
    }
    consume();
    return class_node;
}

aNode Parser::parse_variable_declaration() {
    aNode variable = nullptr;
    if(match(lexer::TokenKind::TOKEN_STATIC)){
        variable = add_node(AST_STATIC_VARIABLE_DECLARATION);
        consume();
    } else
        variable = add_node(AST_VARIABLE_DECLARATION);
    try{
        variable->add_child(parse_type_name());
    } catch (utils::SyntaxErrorException) {
        throw utils::NotVariableException("Not a variable declaration");
    }
    while(true){
        if(!match(lexer::TokenKind::TOKEN_IDENTIFIER) && !match(lexer::TokenKind::TOKEN_DOT))
            throw utils::SyntaxErrorException("Expected identifier, but got \""+current_token()->data+"\"");
        if(match(lexer::TokenKind::TOKEN_DOT))
            throw utils::NotVariableException("Not a variable declaration");
        aNode identifier = add_node(AST_IDENTIFIER);
        consume();
        if(match(lexer::TokenKind::TOKEN_ASSIGN)){
            consume();
            identifier->add_child(parse_assignment_expression());
        }
        variable->add_child(std::move(identifier));
        if(!match(lexer::TokenKind::TOKEN_COMMA))
            break;
        consume();
    }
    if(!match(lexer::TokenKind::TOKEN_SEMICOLON))
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    consume();
    return variable;
}

aNode Parser::parse_function_declaration() {
    aNode function = nullptr;
    bool inline_function = false;
    if(match(lexer::TokenKind::TOKEN_INLINE)){
        inline_function = true;
        consume();
    }
    if(match(lexer::TokenKind::TOKEN_STATIC)){
        function = add_node(AST_STATIC_FUNCTION_DECLARATION);
        consume();
    } else
        function = add_node(AST_FUNCTION_DECLARATION);
    if(match(lexer::TokenKind::TOKEN_INLINE)){
        inline_function = true;
        consume();
    }
    try{
        function->add_child(parse_type_name());
        if(!match(lexer::TokenKind::TOKEN_IDENTIFIER) && !match(lexer::TokenKind::TOKEN_OPERATOR))
            throw utils::SyntaxErrorException("Expected identifier or \"operator\", but got \""+current_token()->data+"\"");
    } catch (utils::SyntaxErrorException) {
        throw utils::NotFunctionException("Not a function declaration");
    }
    if(!match(lexer::TokenKind::TOKEN_OPERATOR)){   
        aNode identifier = add_node(AST_IDENTIFIER);
        function->add_child(std::move(identifier));
        consume();
    } else{
        if(function->kind == AST_STATIC_FUNCTION_DECLARATION)
            throw utils::SyntaxErrorException("Operator function must be non-static");
        consume();
        aNode operator_identifier = add_node(AST_OPERATOR);
        if(match(lexer::TokenKind::TOKEN_LBRACKET)) {
            consume();
            if(!match(lexer::TokenKind::TOKEN_RBRACKET)) {
                throw utils::SyntaxErrorException("Expected \"]\", but got \""+current_token()->data+"\"");
            }
        }
        function->add_child(std::move(operator_identifier));
        consume();
    }
    if(!match(lexer::TokenKind::TOKEN_LPAREN))
        throw utils::NotFunctionException("Not a function declaration");
    consume();
    aNode parameters = add_node(AST_FUNCTION_PARAMETERS);
    bool is_ellipsis = false;
    while(!match(lexer::TokenKind::TOKEN_RPAREN)){
        if(!match(lexer::TokenKind::TOKEN_ELLIPSIS)){
            parameters->add_child(parse_type_name());
            if(!match(lexer::TokenKind::TOKEN_IDENTIFIER))
                throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
            aNode identifier = add_node(AST_IDENTIFIER);
            consume();
            if (match(lexer::TokenKind::TOKEN_ASSIGN)){
                consume();
                identifier->add_child(parse_primary_expression(true));
            }
            parameters->add_child(std::move(identifier));
        } else {
            parameters->add_child(add_node(AST_ELLIPSIS_PARAMETER));
            is_ellipsis = true;
            consume();
        }
        if(!match(lexer::TokenKind::TOKEN_RPAREN) && !match(lexer::TokenKind::TOKEN_COMMA))
            throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
        if(match(lexer::TokenKind::TOKEN_COMMA)){
            if(is_ellipsis)
                throw utils::SyntaxErrorException("Ellipsis parameter must be the last parameter");
            consume();
        }
    }
    function->add_child(std::move(parameters));
    consume();
    function->add_child(parse_compound_statement());
    if(inline_function)
        function->add_child(add_node(AST_INLINE_TAG));
    return function;
}

aNode Parser::parse_compound_statement() {
    aNode compound = add_node(AST_COMPOUND_STATEMENT);
    if(!match(lexer::TokenKind::TOKEN_LBRACE))
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    consume();
    while(!match(lexer::TokenKind::TOKEN_RBRACE)){
        try{
            compound->add_child(parse_statement());
        } catch (std::exception& e) {
            compilation_error(mainCompiler.current_filename.data(), current_token()->line, current_token()->column, e.what());
            consume();
            while(!match(lexer::TokenKind::TOKEN_SEMICOLON) && !match(lexer::TokenKind::TOKEN_RBRACE) && !match(lexer::TokenKind::TOKEN_LBRACE))
                consume();
            if(match(lexer::TokenKind::TOKEN_SEMICOLON))
                consume();
        }
    }
    consume();
    return compound;
}

aNode Parser::parse_statement() {
    if(match(lexer::TokenKind::TOKEN_LBRACE)){
        return parse_compound_statement();
    }
    else if(match(lexer::TokenKind::TOKEN_RBRACE)){
        return add_node(AST_EMPTY_STATEMENT);
    }
    else if(match(lexer::TokenKind::TOKEN_SEMICOLON)){
        consume();
        return add_node(AST_EMPTY_STATEMENT);
    }
    else if(match(lexer::TokenKind::TOKEN_RETURN)){
        return parse_return_statement();
    }
    else if(match(lexer::TokenKind::TOKEN_IF)){
        return parse_if_statement();
    }
    else if(match(lexer::TokenKind::TOKEN_TRY)){
        return parse_try_statement();
    }
    else if(match(lexer::TokenKind::TOKEN_THROW)){
        return parse_throw_statement();
    }
    else if(match(lexer::TokenKind::TOKEN_WHILE)){
        return parse_while_statement();
    }
    else if(match(lexer::TokenKind::TOKEN_DO)){
        return parse_do_while_statement();
    }
    else if(match(lexer::TokenKind::TOKEN_FOR)){
        return parse_for_statement();
    }
    else if(match(lexer::TokenKind::TOKEN_SWITCH)){
        return parse_switch_statement();
    }
    else if(match(lexer::TokenKind::TOKEN_BREAK) && (loopDepth > 0 || switchDepth > 0)){
        consume();
        return add_node(AST_BREAK_STATEMENT);
    }
    else if(match(lexer::TokenKind::TOKEN_CONTINUE) && loopDepth > 0){
        consume();
        return add_node(AST_CONTINUE_STATEMENT);
    }
    else if(match(lexer::TokenKind::TOKEN_BREAK) && loopDepth == 0 && switchDepth == 0){
        throw utils::SyntaxErrorException("Break statement outside of loop or switch");
    }
    else if(match(lexer::TokenKind::TOKEN_CONTINUE) && loopDepth == 0){
        throw utils::SyntaxErrorException("Continue statement outside of loop");
    }
    else if(match(lexer::TokenKind::TOKEN_LABEL)){
        return parse_label_statement();
    }
    else if(match(lexer::TokenKind::TOKEN_GOTO)){
        return parse_goto_statement();
    }
    else if ((IS_TYPE_NAME || match(lexer::TokenKind::TOKEN_CONST) || match(lexer::TokenKind::TOKEN_VOLATILE)) 
         && !match(lexer::TokenKind::TOKEN_DOT, 1)){
        return parse_variable_declaration();
    }
    else{
        aNode expression = parse_expression();
        if(!match(lexer::TokenKind::TOKEN_SEMICOLON))
            throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
        consume();
        return expression;
    }
}

aNode Parser::parse_statement_body() {
    if(match(lexer::TokenKind::TOKEN_LBRACE)){
        return parse_compound_statement();
    }
    aNode compound = add_node(AST_COMPOUND_STATEMENT, lexer::create_token(lexer::TokenKind::TOKEN_LBRACE, "{", current_token()->line, current_token()->column));
    compound->add_child(parse_statement());
    return compound;
}

aNode Parser::parse_return_statement() {
    aNode return_statement = add_node(AST_RETURN_STATEMENT);
    consume();
    if(!match(lexer::TokenKind::TOKEN_SEMICOLON))
        return_statement->add_child(parse_expression());
    if(return_statement->children.size() != 0 && !match(lexer::TokenKind::TOKEN_SEMICOLON))
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    consume();
    return return_statement;
}

aNode Parser::parse_if_statement() {
    aNode if_statement = add_node(AST_IF_STATEMENT);
    consume();
    if_statement->add_child(parse_expression());
    if_statement->add_child(parse_statement_body());
    if(!match(lexer::TokenKind::TOKEN_ELSE))
        return if_statement;
    consume();
    if_statement->add_child(parse_statement_body());
    return if_statement;
}

aNode Parser::parse_try_statement() {
    aNode try_statement = add_node(AST_TRY_STATEMENT);
    consume();
    try_statement->add_child(parse_statement_body());
    if(!match(lexer::TokenKind::TOKEN_CATCH))
        throw utils::SyntaxErrorException("Catch block expected after try block");
    
    aNode catch_statement = add_node(AST_CATCH_STATEMENT);
    consume();
    bool paren=false;
    if(match(lexer::TokenKind::TOKEN_LPAREN)){
        paren=true;
        consume();
    }
    if IS_TYPE_NAME{
        catch_statement->add_child(parse_type_name());
        if(match(lexer::TokenKind::TOKEN_IDENTIFIER)){
            catch_statement->add_child(add_node(AST_IDENTIFIER));
            consume();
        }
    }
    else if(match(lexer::TokenKind::TOKEN_ELLIPSIS)){
        catch_statement->add_child(add_node(AST_ELLIPSIS_TYPE));
        consume();
    }
    else
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    if(paren){
        if(!match(lexer::TokenKind::TOKEN_RPAREN))
            throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
        consume();
    }
    catch_statement->add_child(parse_statement_body());
    try_statement->add_child(std::move(catch_statement));

    while(true){
        if(match(lexer::TokenKind::TOKEN_CATCH)){
            aNode catch_statement = add_node(AST_CATCH_STATEMENT);
            consume();
            bool paren=false;
            if(match(lexer::TokenKind::TOKEN_LPAREN)){
                paren=true;
                consume();
            }
            if IS_TYPE_NAME{
                catch_statement->add_child(parse_type_name());
                if(match(lexer::TokenKind::TOKEN_IDENTIFIER)){
                    catch_statement->add_child(add_node(AST_IDENTIFIER));
                    consume();
                }
            }
            else if(match(lexer::TokenKind::TOKEN_ELLIPSIS)){
                catch_statement->add_child(add_node(AST_ELLIPSIS_TYPE));
                consume();
            }
            else
                throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
            if(paren){
                if(!match(lexer::TokenKind::TOKEN_RPAREN))
                    throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
                consume();
            }
            catch_statement->add_child(parse_statement_body());
            try_statement->add_child(std::move(catch_statement));
        } else if (match(lexer::TokenKind::TOKEN_FINALLY)){
            aNode finally_statement = add_node(AST_FINALLY_STATEMENT);
            consume();
            finally_statement->add_child(parse_statement_body());
            try_statement->add_child(std::move(finally_statement));
            break;
        } else {
            break;
        }
    }

    return try_statement;
}

aNode Parser::parse_throw_statement() {
    aNode throw_statement = add_node(AST_THROW_STATEMENT);
    consume();
    throw_statement->add_child(parse_expression());
    if(!match(lexer::TokenKind::TOKEN_SEMICOLON))
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    consume();
    return throw_statement;
}

aNode Parser::parse_while_statement() {
    loopDepth++;
    aNode while_statement = add_node(AST_WHILE_STATEMENT);
    consume();
    while_statement->add_child(parse_expression());
    while_statement->add_child(parse_statement_body());
    loopDepth--;
    return while_statement;
}

aNode Parser::parse_do_while_statement() {
    loopDepth++;
    aNode do_while_statement = add_node(AST_DO_WHILE_STATEMENT);
    consume();
    do_while_statement->add_child(parse_statement_body());
    if(!match(lexer::TokenKind::TOKEN_WHILE))
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    consume();
    do_while_statement->add_child(parse_expression());
    if(!match(lexer::TokenKind::TOKEN_SEMICOLON))
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    consume();
    loopDepth--;
    return do_while_statement;
}

aNode Parser::parse_for_statement() {
    loopDepth++;
    aNode for_statement = add_node(AST_FOR_STATEMENT);
    consume();
    bool paren = false;
    if(match(lexer::TokenKind::TOKEN_LPAREN)){
        paren = true;
        consume();
    }
    for_statement->add_child(parse_statement());
    try{
        for_statement->add_child(parse_expression());
    }catch(utils::InvalidExpressionException){
        lexer::aToken token = lexer::create_token(lexer::TokenKind::TOKEN_TRUE, "true", current_token()->line, current_token()->column);
        aNode node = add_node(AST_PRIMARY_EXPRESSION, token);
        for_statement->add_child(std::move(node));
        delete token;
    }catch(std::exception& e){
        throw e;
    }
    if(!match(lexer::TokenKind::TOKEN_SEMICOLON))
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    consume();
    for_statement->add_child(parse_statement());
    if(match(lexer::TokenKind::TOKEN_RPAREN) && paren)
        consume();
    for_statement->add_child(parse_statement_body());
    loopDepth--;
    return for_statement;
}

aNode Parser::parse_switch_statement() {
    switchDepth++;
    aNode switch_statement = add_node(AST_SWITCH_STATEMENT);
    consume();
    switch_statement->add_child(parse_expression());
    if(!match(lexer::TokenKind::TOKEN_LBRACE))
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    consume();
    while(!match(lexer::TokenKind::TOKEN_RBRACE)){
        if(match(lexer::TokenKind::TOKEN_CASE)){
            aNode case_statement = add_node(AST_CASE_LABEL);
            consume();
            case_statement->add_child(parse_expression());
            if(!match(lexer::TokenKind::TOKEN_COLON))
                throw utils::SyntaxErrorException("Expected \":\", but got \""+current_token()->data+"\"");
            consume();
            switch_statement->add_child(std::move(case_statement));
        }else if (match(lexer::TokenKind::TOKEN_DEFAULT)){
            aNode default_statement = add_node(AST_DEFAULT_LABEL);
            consume();
            if(!match(lexer::TokenKind::TOKEN_COLON))
                throw utils::SyntaxErrorException("Expected \":\", but got \""+current_token()->data+"\"");
            consume();
            switch_statement->add_child(std::move(default_statement));
        }else switch_statement->add_child(parse_statement_body());
    }
    consume();
    switchDepth--;
    return switch_statement;
}

aNode Parser::parse_label_statement() {
    aNode label_statement = add_node(AST_LABEL_STATEMENT);
    consume();
    if(!match(lexer::TokenKind::TOKEN_IDENTIFIER))
        throw utils::SyntaxErrorException("Expected identifier, but got \""+current_token()->data+"\"");
    label_statement->add_child(add_node(AST_PRIMARY_EXPRESSION));
    consume();
    if(!match(lexer::TokenKind::TOKEN_SEMICOLON))
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    consume();
    return label_statement;
}

aNode Parser::parse_goto_statement() {
    aNode goto_statement = add_node(AST_GOTO_STATEMENT);
    consume();
    if(!match(lexer::TokenKind::TOKEN_IDENTIFIER))
        throw utils::SyntaxErrorException("Expected identifier, but got \""+current_token()->data+"\"");
    goto_statement->add_child(add_node(AST_PRIMARY_EXPRESSION));
    consume();
    if(!match(lexer::TokenKind::TOKEN_SEMICOLON))
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    consume();
    return goto_statement;
}

aNode Parser::parse_import() {
    if(!match(lexer::TokenKind::TOKEN_IMPORT)){
        throw utils::NotImportException("Importation expected");
    }
    consume();
    aNode importation = add_node(AST_IMPORT);
    while(true){
        if(!match(lexer::TokenKind::TOKEN_STRING_LITERAL))
            throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
        std::string string = current_token()->data.substr(1, current_token()->data.size()-2);
        std::string input_file = string+".cbg", output_file = string+".o";
        compiler::Compiler importationCompiler;
        aNode sub_ast = importationCompiler.compile(input_file.data(), output_file.data(), true);
        importation->add_child(std::move(sub_ast));
        consume();
        if(!match(lexer::TokenKind::TOKEN_COMMA))
            break;
        consume();
    }
    if(!match(lexer::TokenKind::TOKEN_SEMICOLON))
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    consume();
    return importation;
}

aNode Parser::parse_quote() {
    if(!match(lexer::TokenKind::TOKEN_QUOTE)){
        throw utils::NotQuoteException("Quote expected");
    }
    consume();
    while(true){
        if(!match(lexer::TokenKind::TOKEN_IDENTIFIER))
            throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
        mainClassNames.quote(current_token()->data);
        consume();
        if(!match(lexer::TokenKind::TOKEN_COMMA))
            break;
        consume();
    }
    if(!match(lexer::TokenKind::TOKEN_SEMICOLON))
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    consume();
    return add_node(AST_EMPTY_STATEMENT);
}

aNode Parser::parse_cast_function_declaration() {
    if(!match(lexer::TokenKind::TOKEN_CAST)){
        throw utils::NotCastException("Cast expected");
    }
    consume();
    if(!IS_TYPE_NAME)
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\", type name expected");
    aNode cast_function = add_node(AST_CAST_FUNCTION_DECLARATION);
    cast_function->add_child(parse_type_name());
    if(!match(lexer::TokenKind::TOKEN_COLON))
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    consume();
    if(!IS_TYPE_NAME)
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\", type name expected");
    cast_function->add_child(parse_type_name());
    cast_function->add_child(parse_compound_statement());
    return cast_function;
}

aNode Parser::parse_primary_expression(bool constant, bool typeClass) {
    if(match(lexer::TokenKind::TOKEN_IDENTIFIER) && !constant) {
        lexer::aToken identifier = current_token();
        consume();
        return add_node(AST_PRIMARY_EXPRESSION, identifier);
    }
    if(IS_TYPE_NAME && !constant && typeClass) {
        lexer::aToken type_name = current_token();
        consume();
        return add_node(AST_PRIMARY_EXPRESSION, type_name);
    }
    if(match(lexer::TokenKind::TOKEN_INT_LITERAL)) {
        lexer::aToken int_literal = current_token();
        consume();
        return add_node(AST_PRIMARY_EXPRESSION, int_literal);
    }
    if(match(lexer::TokenKind::TOKEN_FLOAT_LITERAL)) {
        lexer::aToken float_literal = current_token();
        consume();
        return add_node(AST_PRIMARY_EXPRESSION, float_literal);
    }
    if(match(lexer::TokenKind::TOKEN_STRING_LITERAL)) {
        lexer::aToken string_literal = current_token();
        consume();
        return add_node(AST_PRIMARY_EXPRESSION, string_literal);
    }
    if(match(lexer::TokenKind::TOKEN_CHAR_LITERAL)) {
        lexer::aToken char_literal = current_token();
        consume();
        return add_node(AST_PRIMARY_EXPRESSION, char_literal);
    }
    if(match(lexer::TokenKind::TOKEN_TRUE)) {
        lexer::aToken true_literal = current_token();
        consume();
        return add_node(AST_PRIMARY_EXPRESSION, true_literal);
    }
    if(match(lexer::TokenKind::TOKEN_FALSE)) {
        lexer::aToken false_literal = current_token();
        consume();
        return add_node(AST_PRIMARY_EXPRESSION, false_literal);
    }
    if(match(lexer::TokenKind::TOKEN_NULL)) {
        lexer::aToken null_literal = current_token();
        consume();
        return add_node(AST_PRIMARY_EXPRESSION, null_literal);
    }
    if(match(lexer::TokenKind::TOKEN_SELF) && !constant) {
        lexer::aToken self_literal = current_token();
        consume();
        return add_node(AST_PRIMARY_EXPRESSION, self_literal);
    }
    if(match(lexer::TokenKind::TOKEN_NEW)) {
        aNode newNode = add_node(AST_NEW);
        consume();
        if(!match(lexer::TokenKind::TOKEN_IDENTIFIER))
            throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
        newNode->add_child(add_node(AST_IDENTIFIER));
        consume();
        if(!match(lexer::TokenKind::TOKEN_LPAREN))
            return newNode;
        newNode->add_child(parse_expression());
        return newNode;
    }
    if(match(lexer::TokenKind::TOKEN_SUPER) && !constant) {
        aNode superNode = add_node(AST_SUPER);
        consume();
        if(!match(lexer::TokenKind::TOKEN_LPAREN))
            return superNode;
        consume();
        int paramCount = 0;
        while(!match(lexer::TokenKind::TOKEN_RPAREN)){
            if(!match(lexer::TokenKind::TOKEN_IDENTIFIER) && !match(lexer::TokenKind::TOKEN_SELF) || paramCount >= 2)
                throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
            superNode->add_child(add_node(AST_IDENTIFIER));
            paramCount++;
            consume();
            if(match(lexer::TokenKind::TOKEN_COMMA))
                consume();
        }
        consume();
        return superNode;
    }
    if(match(lexer::TokenKind::TOKEN_LPAREN) && !constant) {
        aNode expression = add_node(AST_PRIMARY_EXPRESSION);
        consume();
        expression->add_child(parse_expression());
        if(!match(lexer::TokenKind::TOKEN_RPAREN))
            compilation_error(mainCompiler.current_filename.data(), current_token()->line, current_token()->column, "Expected ')' after expression");
        consume();
        return expression;
    }
    throw utils::InvalidExpressionException("Unexpected \""+current_token()->data+"\"");
}

aNode Parser::parse_expression() {
    aNode expression = add_node(AST_EXPRESSION);
    if(match(lexer::TokenKind::TOKEN_LPAREN) && match(lexer::TokenKind::TOKEN_RPAREN, 1)){ //Empty expression
        consume();
        consume();
        return expression;
    }
    while (true) {
        expression->add_child(parse_assignment_expression());
        if (!match(lexer::TokenKind::TOKEN_COMMA))
            break;
        consume();
    }
    return expression;
}

aNode Parser::parse_assignment_expression() {
    aNode left = parse_conditional_expression();

    if (match(lexer::TokenKind::TOKEN_ASSIGN) || 
        match(lexer::TokenKind::TOKEN_ADD_ASSIGN) || 
        match(lexer::TokenKind::TOKEN_SUB_ASSIGN) || 
        match(lexer::TokenKind::TOKEN_MUL_ASSIGN) || 
        match(lexer::TokenKind::TOKEN_DIV_ASSIGN) || 
        match(lexer::TokenKind::TOKEN_MOD_ASSIGN) || 
        match(lexer::TokenKind::TOKEN_BIT_AND_ASSIGN) || 
        match(lexer::TokenKind::TOKEN_BIT_OR_ASSIGN) || 
        match(lexer::TokenKind::TOKEN_BIT_XOR_ASSIGN) || 
        match(lexer::TokenKind::TOKEN_BIT_NOT_ASSIGN) || 
        match(lexer::TokenKind::TOKEN_SHIFT_LEFT_ASSIGN) || 
        match(lexer::TokenKind::TOKEN_SHIFT_RIGHT_ASSIGN)) {
        aNode expression = add_node(AST_ASSIGNMENT_EXPRESSION);
        expression->add_child(std::move(left));
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        expression->add_child(parse_assignment_expression());
        return expression;
    }

    return left;
}

aNode Parser::parse_conditional_expression() {
    aNode left = parse_logical_or_expression();

    if (match(lexer::TokenKind::TOKEN_QUESTION)) {
        aNode expression = add_node(AST_CONDITIONAL_EXPRESSION);
        expression->add_child(std::move(left));
        consume();
        expression->add_child(parse_expression());
        if (!match(lexer::TokenKind::TOKEN_COLON))
            throw utils::SyntaxErrorException("Expected ':' in conditional expression");
        consume();
        expression->add_child(parse_expression());
        return expression;
    }

    return left;
}

aNode Parser::parse_logical_or_expression() {
    aNode left = parse_logical_and_expression();

    while (match(lexer::TokenKind::TOKEN_OR)) {
        aNode expression = add_node(AST_LOGICAL_OR_EXPRESSION);
        expression->add_child(std::move(left));
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        expression->add_child(parse_logical_and_expression());
        left = std::move(expression);
    }

    return left;
}

aNode Parser::parse_logical_and_expression() {
    aNode left = parse_inclusive_or_expression();

    while (match(lexer::TokenKind::TOKEN_AND)) {
        aNode expression = add_node(AST_LOGICAL_AND_EXPRESSION);
        expression->add_child(std::move(left));
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        expression->add_child(parse_inclusive_or_expression());
        left = std::move(expression);
    }

    return left;
}

aNode Parser::parse_inclusive_or_expression() {
    aNode left = parse_exclusive_or_expression();

    while (match(lexer::TokenKind::TOKEN_BIT_OR)) {
        aNode expression = add_node(AST_INCLUSIVE_OR_EXPRESSION);
        expression->add_child(std::move(left));
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        expression->add_child(parse_exclusive_or_expression());
        left = std::move(expression);
    }

    return left;
}

aNode Parser::parse_exclusive_or_expression() {
    aNode left = parse_and_expression();

    while (match(lexer::TokenKind::TOKEN_BIT_XOR)) {
        aNode expression = add_node(AST_EXCLUSIVE_OR_EXPRESSION);
        expression->add_child(std::move(left));
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        expression->add_child(parse_and_expression());
        left = std::move(expression);
    }

    return left;
}

aNode Parser::parse_and_expression() {
    aNode left = parse_equality_expression();

    while (match(lexer::TokenKind::TOKEN_BIT_AND)) {
        aNode expression = add_node(AST_AND_EXPRESSION);
        expression->add_child(std::move(left));
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        expression->add_child(parse_equality_expression());
        left = std::move(expression);
    }

    return left;
}

aNode Parser::parse_equality_expression() {
    aNode left = parse_relational_expression();

    while (match(lexer::TokenKind::TOKEN_EQ) || match(lexer::TokenKind::TOKEN_NEQ)) {
        aNode expression = add_node(AST_EQUALITY_EXPRESSION);
        expression->add_child(std::move(left));
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        expression->add_child(parse_relational_expression());
        left = std::move(expression);
    }

    return left;
}

aNode Parser::parse_relational_expression() {
    aNode left = parse_shift_expression();

    while (match(lexer::TokenKind::TOKEN_LT) || match(lexer::TokenKind::TOKEN_GT) || 
           match(lexer::TokenKind::TOKEN_LTE) || match(lexer::TokenKind::TOKEN_GTE)) {
        aNode expression = add_node(AST_RELATIONAL_EXPRESSION);
        expression->add_child(std::move(left));
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        expression->add_child(parse_shift_expression());
        left = std::move(expression);
    }

    return left;
}

aNode Parser::parse_shift_expression() {
    aNode left = parse_additive_expression();

    while (match(lexer::TokenKind::TOKEN_SHIFT_LEFT) || match(lexer::TokenKind::TOKEN_SHIFT_RIGHT)) {
        aNode expression = add_node(AST_SHIFT_EXPRESSION);
        expression->add_child(std::move(left));
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        expression->add_child(parse_additive_expression());
        left = std::move(expression);
    }

    return left;
}

aNode Parser::parse_additive_expression() {
    aNode left = parse_multiplicative_expression();

    while (match(lexer::TokenKind::TOKEN_ADD) || match(lexer::TokenKind::TOKEN_SUB)) {
        aNode expression = add_node(AST_ADDITIVE_EXPRESSION);
        expression->add_child(std::move(left));
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        expression->add_child(parse_multiplicative_expression());
        left = std::move(expression);
    }

    return left;
}

aNode Parser::parse_multiplicative_expression() {
    aNode left = parse_cast_expression();

    while (match(lexer::TokenKind::TOKEN_MUL) || match(lexer::TokenKind::TOKEN_DIV) || match(lexer::TokenKind::TOKEN_MOD)) {
        aNode expression = add_node(AST_MULTIPLICATIVE_EXPRESSION);
        expression->add_child(std::move(left));
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        expression->add_child(parse_cast_expression());
        left = std::move(expression);
    }

    return left;
}

aNode Parser::parse_cast_expression() {
    size_t position = current_position;
    if (match(lexer::TokenKind::TOKEN_LPAREN)) {
        consume();
        aNode type = nullptr;
        if IS_TYPE_NAME
            type = parse_type_name();
        else{
            current_position = position;
            return parse_unary_expression();
        }
        if (!match(lexer::TokenKind::TOKEN_RPAREN))
            throw utils::SyntaxErrorException("Expected ')' in cast expression");
        consume();
        aNode expression = add_node(AST_CAST_EXPRESSION);
        expression->add_child(std::move(type));
        expression->add_child(parse_unary_expression());
        return expression;
    }
    return parse_unary_expression();
}

aNode Parser::parse_unary_expression() {
    if (match(lexer::TokenKind::TOKEN_ADD) || match(lexer::TokenKind::TOKEN_SUB) || 
        match(lexer::TokenKind::TOKEN_NOT) || match(lexer::TokenKind::TOKEN_BIT_NOT) || 
        match(lexer::TokenKind::TOKEN_INC) || match(lexer::TokenKind::TOKEN_DEC)) {
        aNode expression = add_node(AST_UNARY_EXPRESSION);
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        expression->add_child(parse_cast_expression());
        return expression;
    }
    return parse_postfix_expression();
}

aNode Parser::parse_type_name(){
    int _current_position = current_position;
    while(match(lexer::TokenKind::TOKEN_CONST) || match(lexer::TokenKind::TOKEN_VOLATILE))
        consume();
    aNode node = add_node(AST_TYPE_NAME);
    current_position = _current_position;
    while(match(lexer::TokenKind::TOKEN_CONST) || match(lexer::TokenKind::TOKEN_VOLATILE)){
        node->add_child(add_node(AST_TYPE_QUALIFIER));
        consume();
    }
    if(!IS_TYPE_NAME){
        throw utils::SyntaxErrorException("Unexpected \""+current_token()->data+"\"");
    }
    node->add_child(add_node(AST_TYPE_SPECIFIER));
    consume();
    if(!match(lexer::TokenKind::TOKEN_LBRACKET))
        return node;
    consume();
    aNode template_bracket = add_node(AST_TYPE_TEMPLATE);
    while(true){
        aNode member = nullptr;
        if(match(lexer::TokenKind::TOKEN_OPERATOR)) {
            consume();
            member = add_node(AST_OPERATOR);
        } else if (match(lexer::TokenKind::TOKEN_IDENTIFIER)) {
            member = add_node(AST_IDENTIFIER);
            consume();
        } else {
            throw utils::SyntaxErrorException("Expected identifier or operator function");
        }
        template_bracket->add_child(std::move(member));
        if(!match(lexer::TokenKind::TOKEN_COMMA))
            break;
        consume();
    }
    if(!match(lexer::TokenKind::TOKEN_RBRACKET))
        throw utils::SyntaxErrorException("Expected ']'");
    consume();
    node->add_child(std::move(template_bracket));
    return node;
}

aNode Parser::parse_postfix_expression() {
    aNode expression = add_node(AST_POSTFIX_EXPRESSION);
    expression->add_child(parse_primary_expression(false, true));

    if (match(lexer::TokenKind::TOKEN_DOT)) {
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        expression->add_child(parse_postfix_expression());
    } else if (match(lexer::TokenKind::TOKEN_LBRACKET)) {
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        expression->add_child(parse_expression());
        if (!match(lexer::TokenKind::TOKEN_RBRACKET))
            throw utils::SyntaxErrorException("Expected ']'");
        consume();
    } else if (match(lexer::TokenKind::TOKEN_LPAREN)) {
        expression->add_child(add_node(AST_OPERATOR));
        consume();
        if (!match(lexer::TokenKind::TOKEN_RPAREN))
            expression->add_child(parse_expression());
        if (!match(lexer::TokenKind::TOKEN_RPAREN))
            throw utils::SyntaxErrorException("Expected ')'");
        consume();
    } else if (match(lexer::TokenKind::TOKEN_INC)) {
        expression->add_child(add_node(AST_OPERATOR));
        consume();
    } else if (match(lexer::TokenKind::TOKEN_DEC)) {
        expression->add_child(add_node(AST_OPERATOR));
        consume();
    }

    return expression;
}

aNode Parser::add_node(AstKind kind) {
    return std::move(create_ast_node(current_token()->data, kind, current_token()->kind, current_token()->line, current_token()->column));
}

aNode Parser::add_node(AstKind kind, lexer::aToken token) {
    return std::move(create_ast_node(token->data, kind, token->kind, token->line, token->column));
}

// Helper method to consume the current token
void Parser::consume() {
    if(current_position < tokens.size())
        current_position++;
    else
        throw std::out_of_range("Unexpected end of file");
}

// Helper method to get the current token
lexer::aToken Parser::current_token() {
    if(current_position < tokens.size())
        return tokens[current_position];
    else
        throw std::out_of_range("Unexpected end of file");
}

lexer::aToken Parser::current_token(int offset) {
    if(current_position+offset < tokens.size())
        return tokens[current_position+offset];
    else
        throw std::out_of_range("Unexpected end of file");
}

// Helper method to check if the current token matches a specific type
bool Parser::match(lexer::TokenKind kind) {
    return current_token()->kind == kind;
}

bool Parser::match(lexer::TokenKind kind, int offset) {
    if(current_position + offset < tokens.size())
        return tokens[current_position+offset]->kind == kind;
    else
        throw std::out_of_range("Unexpected end of file");
}

}