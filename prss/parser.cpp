#include "parser.hpp"


Node *parseSingleInput(PyLexer &lexer) {
    switch (lexer.curr->getType()) {
        case Python3Parser::NEWLINE: {
            lexer.consume(Python3Parser::NEWLINE);
            break;
        }
        case Python3Parser::STRING:
        case Python3Parser::NUMBER:
        case Python3Parser::RETURN:
        case Python3Parser::RAISE:
        case Python3Parser::FROM:
        case Python3Parser::IMPORT:
        case Python3Parser::GLOBAL:
        case Python3Parser::NONLOCAL:
        case Python3Parser::ASSERT:
        case Python3Parser::LAMBDA:
        case Python3Parser::NOT:
        case Python3Parser::NONE:
        case Python3Parser::TRUE:
        case Python3Parser::FALSE:
        case Python3Parser::YIELD:
        case Python3Parser::DEL:
        case Python3Parser::PASS:
        case Python3Parser::CONTINUE:
        case Python3Parser::BREAK:
        case Python3Parser::AWAIT:
        case Python3Parser::NAME:
        case Python3Parser::ELLIPSIS:
        case Python3Parser::STAR:
        case Python3Parser::OPEN_PAREN:
        case Python3Parser::OPEN_BRACK:
        case Python3Parser::ADD:
        case Python3Parser::MINUS:
        case Python3Parser::NOT_OP:
        case Python3Parser::OPEN_BRACE: {
            return parseSimpleStmt(lexer);
        }
        case Python3Parser::DEF:
        case Python3Parser::IF:
        case Python3Parser::WHILE:
        case Python3Parser::FOR:
        case Python3Parser::TRY:
        case Python3Parser::WITH:
        case Python3Parser::CLASS:
        case Python3Parser::ASYNC:
        case Python3Parser::AT: {
            const auto comp_stmt = parseCompoundStmt(lexer);
            lexer.consume(Python3Parser::NEWLINE);
            return comp_stmt;
        }
    }

    return nullptr;
}

Node *parseFileInput(PyLexer &lexer) {
    auto file_input = new FileInput({});
    while (isStmt(lexer.curr)) {
        switch (lexer.curr->getType()) {
            case Python3Parser::NEWLINE: {
                lexer.consume(Python3Parser::NEWLINE);
                break;
            }
            case Python3Parser::STRING:
            case Python3Parser::NUMBER:
            case Python3Parser::DEF:
            case Python3Parser::RETURN:
            case Python3Parser::RAISE:
            case Python3Parser::FROM:
            case Python3Parser::IMPORT:
            case Python3Parser::GLOBAL:
            case Python3Parser::NONLOCAL:
            case Python3Parser::ASSERT:
            case Python3Parser::IF:
            case Python3Parser::WHILE:
            case Python3Parser::FOR:
            case Python3Parser::TRY:
            case Python3Parser::WITH:
            case Python3Parser::LAMBDA:
            case Python3Parser::NOT:
            case Python3Parser::NONE:
            case Python3Parser::TRUE:
            case Python3Parser::FALSE:
            case Python3Parser::CLASS:
            case Python3Parser::YIELD:
            case Python3Parser::DEL:
            case Python3Parser::PASS:
            case Python3Parser::CONTINUE:
            case Python3Parser::BREAK:
            case Python3Parser::ASYNC:
            case Python3Parser::AWAIT:
            case Python3Parser::NAME:
            case Python3Parser::ELLIPSIS:
            case Python3Parser::STAR:
            case Python3Parser::OPEN_PAREN:
            case Python3Parser::OPEN_BRACK:
            case Python3Parser::ADD:
            case Python3Parser::MINUS:
            case Python3Parser::NOT_OP:
            case Python3Parser::OPEN_BRACE:
            case Python3Parser::AT: {
                file_input->statements.push_back(parseStmt(lexer));
                break;
            }
        }
    }

    return file_input;
}


Parameters *parseParameters(PyLexer &lexer) {
    Parameters *parameters;
    lexer.consume(Python3Parser::OPEN_PAREN);
    // if function takes some parameters
    if (lexer.curr->getType() != Python3Parser::CLOSE_PAREN) {
        parameters = parseTypedArgsList(lexer);
    }
    lexer.consume(Python3Parser::CLOSE_PAREN);
    return parameters;
}

Parameter *parseParameter(PyLexer &lexer, const bool check_type = true) {
    const auto current_token = lexer.curr;
    lexer.consume(Python3Parser::NAME);

    auto param = new Parameter(new Name(current_token->getText()), nullptr, nullptr);
    param->pos_info = getTokPos(current_token);
    if (check_type && lexer.curr->getType() == Python3Parser::COLON) {
        lexer.consume(Python3Parser::COLON);
        param->type = parseTest(lexer);
    }

    return param;
}

// factor: ('+'|'-'|'~') factor | power;
Node *parseFactor(PyLexer &lexer) {
    // parsing unary expression
    auto current_token = lexer.curr;
    switch (current_token->getType()) {
        case Python3Parser::ADD:
        case Python3Parser::MINUS:
        case Python3Parser::NOT_OP:
            lexer.consume(current_token->getType());
            return new UnaryOp(current_token->getType(), parseFactor(lexer));
        default:
            return parsePower(lexer);
    }
}

// (tfpdef ( ASSIGN test )? ( COMMA tfpdef (ASSIGN test)? )* ( COMMA ( STAR (tfpdef)? (COMMA tfpdef (ASSIGN test)? )* ( COMMA (STAR (tfpdef)? (COMMA tfpdef (ASSIGN test)?)* (COMMA (POWER tfpdef (COMMA)? )?)?
//      | POWER tfpdef (COMMA)?)?)?
//      | STAR (tfpdef)? (COMMA tfpdef (ASSIGN test)?)* (COMMA (POWER tfpdef (COMMA)?)?)?
//      | POWER tfpdef (COMMA)?
// )
Parameters *parseTypedArgsList(PyLexer &lexer) {
    Parameters *parameters = new Parameters({}, {});
    switch (lexer.curr->getType()) {
        case Python3Parser::NAME: {
            auto parameter = parseParameter(lexer, true);

            if (lexer.curr->getType() == Python3Parser::ASSIGN) {
                lexer.consume(Python3Parser::ASSIGN);
                parameter->default_val = parseTest(lexer);
            }

            parameters->params.push_back(parameter);
            // (',' tfpdef ('=' test)?)*
            // able to parse parameters represented as follows:
            // (param, param1: type, param2: type = default)
            while (lexer.curr->getType() == Python3Parser::COMMA &&
                   lexer.next->getType() == Python3Parser::NAME) {
                lexer.consume(Python3Parser::COMMA);
                auto param = parseParameter(lexer, true);
                if (lexer.curr->getType() == Python3Parser::ASSIGN) {
                    lexer.consume(Python3Parser::ASSIGN);
                    parameter->default_val = parseTest(lexer);
                }
                parameters->params.push_back(param);
            }

            if (lexer.curr->getType() == Python3Parser::COMMA) {
                lexer.consume(Python3Parser::COMMA);
                switch (lexer.curr->getType()) {
                    case Python3Parser::STAR: {
                        lexer.consume(Python3Parser::STAR);
                        if (lexer.curr->getType() == Python3Parser::NAME) {
                            parameters->extra.vararg = parseParameter(lexer, true);
                        }

                        while (lexer.curr->getType() == Python3Parser::COMMA &&
                               lexer.next->getType() == Python3Parser::NAME) {

                            lexer.consume(Python3Parser::COMMA);
                            parameters->extra.kw_only_args.push_back(parseParameter(lexer, true));
                            if (lexer.curr->getType() == Python3Parser::ASSIGN) {
                                parameters->extra.kw_defaults.push_back(parseTest(lexer));
                            } else {
                                // might seem really silly, but i've no idea (yet) how to do this anyway
                                parameters->extra.kw_defaults.push_back(nullptr);
                            }
                        }

                        if (lexer.curr->getType() == Python3Parser::COMMA) {
                            lexer.consume(Python3Parser::COMMA);

                            if (lexer.curr->getType() == Python3Parser::POWER) {
                                lexer.consume(Python3Parser::POWER);
                                parameters->extra.kwarg = parseParameter(lexer, true);
                            }
                        }
                        break;
                    }
                    case Python3Parser::POWER: {
                        lexer.consume(Python3Parser::POWER);
                        parameters->extra.kwarg = parseParameter(lexer, true);
                        if (lexer.curr->getType() == Python3Parser::COMMA) {
                            lexer.consume(Python3Parser::COMMA);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
            break;
        }
        case Python3Parser::STAR: {
            lexer.consume(Python3Parser::STAR);

            if (lexer.curr->getType() == Python3Parser::NAME) {
                parameters->extra.vararg = parseParameter(lexer, true);
            }

            while (lexer.curr->getType() == Python3Parser::COMMA &&
                   lexer.next->getType() == Python3Parser::NAME) {

                lexer.consume(Python3Parser::COMMA);
                parameters->extra.kw_only_args.push_back(parseParameter(lexer, true));
                if (lexer.curr->getType() == Python3Parser::ASSIGN) {
                    parameters->extra.kw_defaults.push_back(parseTest(lexer));
                } else {
                    // might seem really silly, but i've no idea (yet) how to do this anyway
                    parameters->extra.kw_defaults.push_back(nullptr);
                }
            }

            if (lexer.curr->getType() == Python3Parser::COMMA) {
                lexer.consume(Python3Parser::COMMA);

                if (lexer.curr->getType() == Python3Parser::POWER) {
                    lexer.consume(Python3Parser::POWER);
                    parameters->extra.kwarg = parseParameter(lexer, true);
                    if (lexer.curr->getType() == Python3Parser::COMMA) {
                        lexer.consume(Python3Parser::COMMA);
                    }
                }
            }
            break;
        }
        case Python3Parser::POWER: {
            lexer.consume(Python3Parser::POWER);
            parameters->extra.kwarg = parseParameter(lexer, true);
            if (lexer.curr->getType() == Python3Parser::COMMA) {
                lexer.consume(Python3Parser::COMMA);
            }
            break;
        }
        default:
        ERR_MSG_EXIT("Expected NAME, STAR or POWER");
    }


    return parameters;
}

//(vfpdef
//  ('=' test)? (',' vfpdef ('=' test)?)*
//  (',' ( '*' (vfpdef)? (',' vfpdef ('=' test)?)*
//  (',' ('**' vfpdef (',')?)?)? | '**' vfpdef (',')?)?)?
// | '*' (vfpdef)? (',' vfpdef ('=' test)?)* (',' ('**' vfpdef (',')?)?)?
// | '**' vfpdef (',')?
// );
Node *parseVarArgsList(PyLexer &lexer) {
    auto parameters = new Parameters({}, {});
    switch (lexer.curr->getType()) {
        case Python3Parser::NAME: {
            const auto arg = parseParameter(lexer, false);

            if (lexer.curr->getType() == Python3Parser::ASSIGN) {
                lexer.consume(Python3Parser::ASSIGN);
                arg->default_val = parseTest(lexer);
            }

            parameters->params.push_back(arg);

            while (lexer.curr->getType() == Python3Parser::COMMA &&
                   lexer.next->getType() == Python3Parser::NAME) {
                lexer.consume(Python3Parser::COMMA);
                const auto arg = parseParameter(lexer, false);

                if (lexer.curr->getType() == Python3Parser::ASSIGN) {
                    lexer.consume(Python3Parser::ASSIGN);
                    arg->default_val = parseTest(lexer);
                }

                parameters->params.push_back(arg);
            }

            const auto text = lexer.curr->getText();
            if (lexer.curr->getType() == Python3Parser::COMMA) {
                lexer.consume(Python3Parser::COMMA);

                if (lexer.curr->getType() == Python3Parser::STAR) {
                    if (lexer.curr->getType() == Python3Parser::NAME) {
                        parameters->extra.vararg = parseParameter(lexer, false);
                    }

                    while (lexer.curr->getType() == Python3Parser::COMMA &&
                           lexer.next->getType() == Python3Parser::NAME) {

                        lexer.consume(Python3Parser::COMMA);
                        const auto arg = parseParameter(lexer, false);
                        if (lexer.curr->getType() == Python3Parser::ASSIGN) {
                            lexer.consume(Python3Parser::ASSIGN);
                            parameters->extra.kw_defaults.push_back(parseTest(lexer));
                        }
                        parameters->extra.kw_only_args.push_back(arg);
                    }
                }

                if (lexer.curr->getType() == Python3Parser::COMMA) {
                    lexer.consume(Python3Parser::COMMA);

                    if (lexer.curr->getType() == Python3Parser::POWER) {
                        lexer.consume(Python3Parser::POWER);
                        parameters->extra.kwarg = parseParameter(lexer, false);

                        if (lexer.curr->getType() == Python3Parser::COMMA) {
                            lexer.consume(Python3Parser::COMMA);
                        }
                    }
                }

                lexer.consume(Python3Parser::POWER);
                parameters->extra.kwarg = parseParameter(lexer, false);

                if (lexer.curr->getType() == Python3Parser::COMMA) {
                    lexer.consume(Python3Parser::COMMA);
                }
            }
            break;
        }
        case Python3Parser::STAR: {
            lexer.consume(Python3Parser::STAR);

            if (lexer.curr->getType() == Python3Parser::NAME) {
                parameters->extra.vararg = parseParameter(lexer, false);
            }

            while (lexer.curr->getType() == Python3Parser::COMMA &&
                   lexer.next->getType() == Python3Parser::NAME) {

                lexer.consume(Python3Parser::COMMA);
                auto arg = parseParameter(lexer, false);

                if (lexer.curr->getType() == Python3Parser::ASSIGN) {
                    lexer.consume(Python3Parser::ASSIGN);
                    parameters->extra.kw_defaults.push_back(parseTest(lexer));
                }
                parameters->extra.kw_only_args.push_back(arg);
            }

            if (lexer.curr->getType() == Python3Parser::COMMA) {
                lexer.consume(Python3Parser::COMMA);

                if (lexer.curr->getType() == Python3Parser::POWER) {
                    lexer.consume(Python3Parser::POWER);
                    parameters->extra.kwarg = parseParameter(lexer, false);

                    if (lexer.curr->getType() == Python3Parser::COMMA) {
                        lexer.consume(Python3Parser::COMMA);
                    }
                }
            }
            break;
        }
        case Python3Parser::POWER: {
            lexer.consume(Python3Parser::POWER);
            parameters->extra.kwarg = parseParameter(lexer, false);
            if (lexer.curr->getType() == Python3Parser::COMMA) {
                lexer.consume(Python3Parser::COMMA);
            }
            break;
        }
        default:
        ERR_MSG_EXIT("Expected NAME, STAR or POWER");
    }


    return parameters;
}


FuncDef *parseFuncDef(PyLexer &lexer, std::vector<Node *> &&decorator_list) {
    lexer.consume(Python3Parser::DEF);

    const auto current_token = lexer.curr;
    lexer.consume(Python3Parser::NAME);
    auto func_def = new FuncDef(current_token->getText(), nullptr, nullptr, nullptr, {});

    func_def->parameters = parseParameters(lexer);

    if (lexer.curr->getType() == Python3Parser::ARROW) {
        lexer.consume(Python3Parser::ARROW);
        func_def->return_type = parseTest(lexer);
    }

    lexer.consume(Python3Parser::COLON);

    func_def->body = parseSuite(lexer);
    func_def->decorator_list = std::move(decorator_list);

    return func_def;
}

AsyncFuncDef *parseAsyncFuncDef(PyLexer &lexer, std::vector<Node *> &&decorator_list) {
    lexer.consume(Python3Parser::ASYNC);
    auto async_func_def = new AsyncFuncDef(parseFuncDef(lexer, std::forward<std::vector<Node *>>(decorator_list)),
                                           true);
    return async_func_def;
}

Delete *parseDelStmt(PyLexer &lexer) {
    lexer.consume(Python3Parser::DEL);
    auto node = new Delete(nullptr);
    node->targets = parseExprList(lexer);
    return node;
}


Node *parseSimpleStmt(PyLexer &lexer) {
    auto simple_stmt = new SimpleStmt({});
    const auto node = parseSmallStmt(lexer);

    simple_stmt->small_stmts.push_back(node);
    while (lexer.curr->getType() == Python3Parser::SEMI_COLON) {
        lexer.consume(Python3Parser::SEMI_COLON);

        // handle a terminal semicolon
        if (lexer.curr->getType() == Python3Parser::EOF ||
            lexer.curr->getType() == Python3Parser::NEWLINE) {
            break;
        }
        const auto node = parseSmallStmt(lexer);
        simple_stmt->small_stmts.push_back(node);
    }

    if (lexer.curr->getType() == Python3Parser::NEWLINE) {
        lexer.consume(Python3Parser::NEWLINE);
    }

    return simple_stmt;
}

Node *parseSmallStmt(PyLexer &lexer) {
    switch (lexer.curr->getType()) {
        case Python3Parser::ASSERT:
            return parseAssertStmt(lexer);
        case Python3Parser::NONLOCAL:
            return parseNonlocalStmt(lexer);
        case Python3Parser::GLOBAL:
            return parseGlobalStmt(lexer);
        case Python3Parser::FROM:
        case Python3Parser::IMPORT:
            return parseImportStmt(lexer);
        case Python3Parser::PASS:
            return parsePassStmt(lexer);
        case Python3Parser::DEL:
            return parseDelStmt(lexer);
        case Python3Parser::BREAK:
        case Python3Parser::CONTINUE:
        case Python3Parser::RETURN:
        case Python3Parser::RAISE:
        case Python3Parser::YIELD:
            return parseFlowStmt(lexer);
        case Python3Parser::STAR:
        case Python3Parser::STRING:
        case Python3Parser::NUMBER:
        case Python3Parser::LAMBDA:
        case Python3Parser::NOT:
        case Python3Parser::NONE:
        case Python3Parser::TRUE:
        case Python3Parser::FALSE:
        case Python3Parser::AWAIT:
        case Python3Parser::NAME:
        case Python3Parser::ELLIPSIS:
        case Python3Parser::OPEN_PAREN:
        case Python3Parser::OPEN_BRACK:
        case Python3Parser::ADD:
        case Python3Parser::MINUS:
        case Python3Parser::NOT_OP:
        case Python3Parser::OPEN_BRACE:
            return parseExprStmt(lexer);
        default:
        ERR_MSG_EXIT("expected EXPR, DEL, PASS, FLOW, IMPORT, GLOBAL, NONLOCAL or ASSERT");
    }
}

Assert *parseAssertStmt(PyLexer &lexer) {
    lexer.consume(Python3Parser::ASSERT);

    auto assert = new Assert(nullptr, nullptr);

    assert->test = parseTest(lexer);

    if (lexer.curr->getType() == Python3Parser::COMMA) {
        lexer.consume(Python3Parser::COMMA);
        assert->message = parseTest(lexer);
    }

    return assert;
}

Node *parseFlowStmt(PyLexer &lexer) {
    switch (lexer.curr->getType()) {
        case Python3Parser::BREAK:
            return parseBreakStmt(lexer);
        case Python3Parser::CONTINUE:
            return parseContinueStmt(lexer);
        case Python3Parser::RETURN:
            return parseReturnStmt(lexer);
        case Python3Parser::RAISE:
            return parseRaiseStmt(lexer);
        case Python3Parser::YIELD:
            return parseYieldStmt(lexer);
        default:
        ERR_MSG_EXIT("expected BREAK, CONTINUE, RETURN, RAISE or YIELD");
    }
}


Pass *parsePassStmt(PyLexer &lexer) {
    lexer.consume(Python3Parser::PASS);
    return new Pass();
}

ExprList *parseExprList(PyLexer &lexer) {
    auto expr_list = new ExprList({});

    if (lexer.curr->getType() == Python3Parser::STAR) {
        expr_list->expr_list.push_back(parseStarExpr(lexer));
    } else {
        expr_list->expr_list.push_back(parseExpr(lexer));
    }

    while (lexer.curr->getType() == Python3Parser::COMMA) {
        lexer.consume(Python3Parser::COMMA);
        if (lexer.curr->getType() == Python3Parser::STAR) {
            expr_list->expr_list.push_back(parseStarExpr(lexer));
        } else {
            expr_list->expr_list.push_back(parseExpr(lexer));
        }
    }

    return expr_list;
}


ImportFrom *parseImportFrom(PyLexer &lexer) {
    lexer.consume(Python3Parser::FROM);
    auto import_from = new ImportFrom(nullptr, nullptr, 0);
    int32_t level;

    // TODO(threadedstream): handle case with ellipsis
    while (lexer.curr->getType() == Python3Parser::DOT) {
        lexer.consume(Python3Parser::DOT);
        level++;
    }

    if (level == 0 && !(lexer.curr->getType() == Python3Parser::NAME)) {
        // TODO(threadedstream): adhoc method to report an error and terminate application
        // call to destroyAst() would be great
        ERR_TOK(Python3Parser::NAME);
        exit(1);
    }

    import_from->level = level;
    if (lexer.curr->getType() == Python3Parser::NAME) {
        const auto name = parseDottedName(lexer);
        import_from->module = name;
    }

    lexer.consume(Python3Parser::IMPORT);

    // may look really dumb
    switch (lexer.curr->getType()) {
        case Python3Parser::STAR: {
            const auto name = new Name("*");
            import_from->aliases->aliases.push_back(new Alias(name, nullptr));
            break;
        }
        case Python3Parser::OPEN_PAREN: {
            lexer.consume(Python3Parser::OPEN_PAREN);
            import_from->aliases = parseImportAsNames(lexer);
            lexer.consume(Python3Parser::CLOSE_PAREN);
            break;
        }
        case Python3Parser::NAME:
            import_from->aliases = parseImportAsNames(lexer);
            break;
        default:
        ERR_MSG_EXIT("expected STAR, OPEN_PAREN or NAME\n");
    }

    return import_from;
}

// "as_attr" parameter serves as an indicator for parseDottedName to
// treat dotted name as a structure of attributes put one into another, rather than
// a simple name
Node *parseDottedName(PyLexer &lexer, bool as_attr) {
    if (!as_attr) {
        std::string name;
        const auto current_token = lexer.curr;
        lexer.consume(Python3Parser::NAME);

        name += current_token->getText();

        while (lexer.curr->getType() == Python3Parser::DOT) {
            lexer.consume(Python3Parser::DOT);
            const auto current_token = lexer.curr;
            lexer.consume(Python3Parser::NAME);
            name += current_token->getText();
        }

        auto dotted_name = new Name(name);

        return dotted_name;
    } else {
        const auto attr_value_token = lexer.curr;
        lexer.consume(Python3Parser::NAME);
        if (lexer.curr->getType() == Python3Parser::DOT) {
            auto attribute = new Attribute(new Name(attr_value_token->getText()), nullptr);
            lexer.consume(Python3Parser::DOT);
            const auto attr_attr_token = lexer.curr;
            lexer.consume(Python3Parser::NAME);
            attribute->attr = new Name(attr_attr_token->getText());

            while (lexer.curr->getType() == Python3Parser::DOT) {
                lexer.consume(Python3Parser::DOT);
                const auto attr_attr_token = lexer.curr;
                lexer.consume(Python3Parser::NAME);
                attribute = new Attribute(attribute, new Name(attr_attr_token->getText()));
            }

            return attribute;
        } else {
            return new Name(attr_value_token->getText());
        }
    }
}

Alias *parseDottedAsName(PyLexer &lexer) {
    const auto dotted_name = parseDottedName(lexer);
    auto alias = new Alias(dotted_name, nullptr);
    if (lexer.curr->getType() == Python3Parser::AS) {
        lexer.consume(Python3Parser::AS);
        alias->as = new Name(lexer.curr->getText());
    }

    return alias;
}

Aliases *parseDottedAsNames(PyLexer &lexer) {
    const auto dotted_as_name = parseDottedAsName(lexer);
    auto aliases = new Aliases({});
    aliases->aliases.push_back(dotted_as_name);

    while (lexer.curr->getType() == Python3Parser::COMMA) {
        lexer.consume(Python3Parser::COMMA);
        const auto dotted_as_name = parseDottedAsName(lexer);
        aliases->aliases.push_back(dotted_as_name);
    }

    return aliases;
}

IfStmt *parseIfStmt(PyLexer &lexer, int32_t depth) {
    if (depth == 0) {
        lexer.consume(Python3Parser::IF);
    }
    auto if_stmt = new IfStmt(nullptr, nullptr, nullptr);

    if_stmt->test = parseTest(lexer);
    lexer.consume(Python3Parser::COLON);

    if_stmt->body = parseSuite(lexer);

    while (lexer.curr->getType() == Python3Parser::ELIF) {
        lexer.consume(Python3Parser::ELIF);
        if_stmt->or_else = parseIfStmt(lexer, depth + 1);
    }

    if (lexer.curr->getType() == Python3Parser::ELSE) {
        lexer.consume(Python3Parser::ELSE);
        lexer.consume(Python3Parser::COLON);

        if_stmt->or_else = parseSuite(lexer);
    }

    return if_stmt;
}

WhileStmt *parseWhileStmt(PyLexer &lexer) {
    lexer.consume(Python3Parser::WHILE);
    auto while_stmt = new WhileStmt(nullptr, nullptr, nullptr);
    while_stmt->test = parseTest(lexer);
    lexer.consume(Python3Parser::COLON);
    while_stmt->body = parseSuite(lexer);
    if (lexer.curr->getType() == Python3Parser::ELSE) {
        lexer.consume(Python3Parser::ELSE);
        lexer.consume(Python3Parser::COLON);
        while_stmt->or_else = parseSuite(lexer);
    }

    return while_stmt;
}

Alias *parseImportAsName(PyLexer &lexer) {
    auto alias = new Alias(nullptr, nullptr);

    auto name = new Name(lexer.curr->getText());

    lexer.consume(Python3Parser::NAME);
    if (lexer.curr->getType() == Python3Parser::AS) {
        lexer.consume(Python3Parser::AS);
        auto as = new Name(lexer.curr->getText());
        lexer.consume(Python3Parser::NAME);
        alias->as = as;
    }

    alias->name = name;

    return alias;
}

Aliases *parseImportAsNames(PyLexer &lexer) {
    auto aliases = new Aliases({});
    const auto alias = parseImportAsName(lexer);

    aliases->aliases.push_back(alias);

    while (lexer.curr->getType() == Python3Parser::COMMA) {
        lexer.consume(Python3Parser::COMMA);
        const auto alias = parseImportAsName(lexer);
        aliases->aliases.push_back(alias);
    }

    return aliases;
}

Node *parseStmt(PyLexer &lexer) {
    switch (lexer.curr->getType()) {
        case Python3Parser::IF:
        case Python3Parser::WHILE:
        case Python3Parser::FOR:
        case Python3Parser::TRY:
        case Python3Parser::WITH:
        case Python3Parser::DEF:
        case Python3Parser::CLASS:
        case Python3Parser::AT:
        case Python3Parser::ASYNC:
            return parseCompoundStmt(lexer);
        default:
            return parseSimpleStmt(lexer);
    }
}

Node *parseCompIf(PyLexer &lexer) {
    lexer.consume(Python3Parser::IF);

    const auto test_no_cond = parseTestNoCond(lexer);


    return test_no_cond;
}


Node *parseCompIter(PyLexer &lexer) {
    switch (lexer.curr->getType()) {
        case Python3Parser::IF:
            return parseCompIf(lexer);
        case Python3Parser::ASYNC:
        case Python3Parser::FOR:
            return parseCompFor(lexer);
        default:
        ERR_MSG_EXIT("Expected IF, ASYNC or FOR");
    }
}

Node *parseCompoundStmt(PyLexer &lexer) {
    switch (lexer.curr->getType()) {
        case Python3Parser::IF:
            return parseIfStmt(lexer, 0);
        case Python3Parser::WHILE:
            return parseWhileStmt(lexer);
        case Python3Parser::FOR:
            return parseForStmt(lexer);
        case Python3Parser::TRY:
            return parseTryStmt(lexer);
        case Python3Parser::WITH:
            return parseWithStmt(lexer);
        case Python3Parser::DEF:
            return parseFuncDef(lexer, {});
        case Python3Parser::CLASS:
            return parseClassDef(lexer, {});
        case Python3Parser::AT:
            return parseDecorated(lexer);
        case Python3Parser::ASYNC:
            return parseAsyncStmt(lexer);
        default: {
            ERR_MSG_EXIT("expected IF, WHILE, FOR, TRY, DEF, CLASS, AT or ASYNC");
        }
    }
}

// classdef: 'class' NAME ('(' (arglist)? ')')? ':' suite;
ClassDef *parseClassDef(PyLexer &lexer, std::vector<Node *> &&decorator_list) {
    const auto line_start = lexer.curr->getLine();
    const auto col_start = lexer.curr->getStartIndex();
    lexer.consume(Python3Parser::CLASS);

    const auto class_name = lexer.curr->getText();
    Arguments *arglist;
    lexer.consume(Python3Parser::NAME);
    if (lexer.curr->getType() == Python3Parser::OPEN_PAREN) {
        lexer.consume(Python3Parser::OPEN_PAREN);
        arglist = parseArglist(lexer);
        lexer.consume(Python3Parser::CLOSE_PAREN);
    }

    lexer.consume(Python3Parser::COLON);
    const auto body = parseSuite(lexer);
    auto class_def = new ClassDef(class_name, arglist, body, std::move(decorator_list));

    // temporary
    (void) line_start;
    (void) col_start;

    return class_def;
}

Node *parseSuite(PyLexer &lexer) {
    if (lexer.curr->getType() == Python3Parser::NEWLINE) {
        lexer.consume(Python3Parser::NEWLINE);
        lexer.consume(Python3Parser::INDENT);
        if (!isStmt(lexer.curr)) {
            ERR_MSG_EXIT("should be a statement");
        }
        auto stmt = new Stmt({});
        while (isStmt(lexer.curr)) {
            stmt->nodes.push_back(parseStmt(lexer));
        }
        lexer.consume(Python3Parser::DEDENT);
        return stmt;
    } else {
        return parseSimpleStmt(lexer);
    }
}

Node *parseImportStmt(PyLexer &lexer) {
    if (lexer.curr->getType() == Python3Parser::IMPORT) {
        return parseImportName(lexer);
    } else if (lexer.curr->getType() == Python3Parser::FROM) {
        return parseImportFrom(lexer);
    } else {
        ERR_MSG_EXIT("expected IMPORT or FROM\n");
    }
}

Import *parseImportName(PyLexer &lexer) {
    lexer.consume(Python3Parser::IMPORT);
    const auto aliases = parseDottedAsNames(lexer);
    auto import = new Import(aliases);

    return import;
}

Break *parseBreakStmt(PyLexer &lexer) {
    lexer.consume(Python3Parser::BREAK);
    auto node = new Break();
    return node;
}

Continue *parseContinueStmt(PyLexer &lexer) {
    lexer.consume(Python3Parser::CONTINUE);
    auto node = new Continue();
    return node;
}

Node *parseTestNoCond(PyLexer &lexer) {
    switch (lexer.curr->getType()) {
        case Python3Parser::STRING:
        case Python3Parser::NUMBER:
        case Python3Parser::NOT:
        case Python3Parser::NONE:
        case Python3Parser::TRUE:
        case Python3Parser::FALSE:
        case Python3Parser::AWAIT:
        case Python3Parser::NAME:
        case Python3Parser::ELLIPSIS:
        case Python3Parser::OPEN_PAREN:
        case Python3Parser::OPEN_BRACK:
        case Python3Parser::ADD:
        case Python3Parser::MINUS:
        case Python3Parser::NOT_OP:
        case Python3Parser::OPEN_BRACE: {
            return parseOrTest(lexer);
        }
        case Python3Parser::LAMBDA:
            return parseLambDefNoCond(lexer);
        default:
        ERR_MSG_EXIT("Expected OR_TEST or LAMBDA");
    }
}

Node *parseLambDefNoCond(PyLexer &lexer) {
    lexer.consume(Python3Parser::LAMBDA);
    auto lambda = new Lambda(nullptr, nullptr);
    const auto current_type = lexer.curr->getType();
    if (current_type == Python3Parser::NAME ||
        current_type == Python3Parser::STAR ||
        current_type == Python3Parser::POWER) {
        lambda->args = parseVarArgsList(lexer);
    }
    lexer.consume(Python3Parser::COLON);
    lambda->body = parseTestNoCond(lexer);

    return lambda;
}

TestList *parseTestlist(PyLexer &lexer) {
    auto test_list = new TestList({});

    auto node = parseTest(lexer);
    test_list->nodes.push_back(node);

    while (lexer.curr->getType() == Python3Parser::COMMA) {
        lexer.consume(Python3Parser::COMMA);

        const auto node = parseTest(lexer);
        test_list->nodes.push_back(node);
    }

    return test_list;
}


// with_stmt: 'with' with_item (',' with_item)*  ':' suite;
WithStmt *parseWithStmt(PyLexer &lexer) {
    const auto current_token = lexer.curr;
    lexer.consume(Python3Parser::WITH);

    auto with_stmt = new WithStmt({}, nullptr, nullptr);
    with_stmt->pos_info = getTokPos(current_token);

    const auto with_item = parseWithItem(lexer);
    with_stmt->items.push_back(with_item);

    while (lexer.curr->getType() == Python3Parser::COMMA) {
        lexer.consume(Python3Parser::COMMA);

        const auto with_item = parseWithItem(lexer);
        with_stmt->items.push_back(with_item);
    }

    lexer.consume(Python3Parser::COLON);
    with_stmt->body = parseSuite(lexer);

    return with_stmt;
}

// with_item: test ('as' expr)?;
WithItem *parseWithItem(PyLexer &lexer) {
    auto with_item = new WithItem(nullptr, nullptr);
    with_item->context_expr = parseTest(lexer);

    if (lexer.curr->getType() == Python3Parser::AS) {
        lexer.consume(Python3Parser::AS);
        with_item->optional_vars = parseExpr(lexer);
    }

    return with_item;
}

Return *parseReturnStmt(PyLexer &lexer) {
    lexer.consume(Python3Parser::RETURN);
    auto node = new Return(nullptr);

    node->test_list = parseTestlist(lexer);

    return node;
}

Raise *parseRaiseStmt(PyLexer &lexer) {
    lexer.consume(Python3Parser::RAISE);

    auto raise = new Raise(nullptr, nullptr);

    raise->exception = parseTest(lexer);

    if (lexer.curr->getType() == Python3Parser::FROM) {
        lexer.consume(Python3Parser::FROM);

        raise->from = parseTest(lexer);
    }

    return raise;
}

Node *parseYieldStmt(PyLexer &lexer) {
    return parseYieldExpr(lexer);
}

Node *parseYieldExpr(PyLexer &lexer) {
    lexer.consume(Python3Parser::YIELD);

    if (lexer.curr->getType() == Python3Parser::FROM) {
        lexer.consume(Python3Parser::FROM);

        auto yield_from = new YieldFrom(nullptr);
        yield_from->target = parseTest(lexer);
        return yield_from;
    }


    auto yield_stmt = new Yield(nullptr);

    yield_stmt->target = parseTestlist(lexer);

    return yield_stmt;
}

TestList *parseTestList(PyLexer &lexer) {
    auto test_list = new TestList({});

    const auto node = parseTest(lexer);
    test_list->nodes.push_back(node);

    while (lexer.curr->getType() == Python3Parser::COMMA) {
        lexer.consume(Python3Parser::COMMA);
        test_list->nodes.push_back(parseTest(lexer));
    }

    return test_list;
}

Arguments *parseArglist(PyLexer &lexer) {
    const auto argument = parseArgument(lexer);

    auto arguments = new Arguments({}, {});

    if (const auto kwd = dynamic_cast<Keyword *>(argument)) {
        arguments->keywords.push_back(kwd);
    } else {
        arguments->args.push_back(argument);
    }

    while (lexer.curr->getType() == Python3Parser::COMMA) {
        lexer.consume(Python3Parser::COMMA);
        switch (lexer.curr->getType()) {
            case Python3Parser::STRING:
            case Python3Parser::NUMBER:
            case Python3Parser::LAMBDA:
            case Python3Parser::NOT:
            case Python3Parser::NONE:
            case Python3Parser::TRUE:
            case Python3Parser::FALSE:
            case Python3Parser::AWAIT:
            case Python3Parser::NAME:
            case Python3Parser::ELLIPSIS:
            case Python3Parser::OPEN_PAREN:
            case Python3Parser::OPEN_BRACK:
            case Python3Parser::ADD:
            case Python3Parser::MINUS:
            case Python3Parser::NOT_OP:
            case Python3Parser::OPEN_BRACE:
            case Python3Parser::POWER:
            case Python3Parser::STAR: {
                const auto argument = parseArgument(lexer);
                // i'm going to leave as it is until i come up with
                // something much better
                if (const auto kwd = dynamic_cast<Keyword *>(argument)) {
                    arguments->keywords.push_back(kwd);
                } else {
                    arguments->args.push_back(argument);
                }
                break;
            }
            default:
                goto ret;
        }
    }

    ret:
    return arguments;

}

Node *parseArgument(PyLexer &lexer) {
    Node *fallback_arg;
    switch (lexer.curr->getType()) {
        case Python3Parser::STRING:
        case Python3Parser::NUMBER:
        case Python3Parser::LAMBDA:
        case Python3Parser::NOT:
        case Python3Parser::NONE:
        case Python3Parser::TRUE:
        case Python3Parser::FALSE:
        case Python3Parser::AWAIT:
        case Python3Parser::NAME:
        case Python3Parser::ELLIPSIS:
        case Python3Parser::OPEN_PAREN:
        case Python3Parser::OPEN_BRACK:
        case Python3Parser::ADD:
        case Python3Parser::MINUS:
        case Python3Parser::NOT_OP:
        case Python3Parser::OPEN_BRACE: {
            const auto arg_name = lexer.curr->getText();
            fallback_arg = parseTest(lexer);
            if (lexer.curr->getType() == Python3Parser::ASYNC ||
                lexer.curr->getType() == Python3Parser::FOR) {
                const auto generator_exp = new GeneratorExp(fallback_arg, {});
                while (lexer.curr->getType() == Python3Parser::FOR) {
                    const auto comprehension = parseCompFor(lexer);
                    generator_exp->generators.push_back(comprehension);
                }
                return generator_exp;
            }
            if (lexer.curr->getType() == Python3Parser::ASSIGN) {
                lexer.consume(Python3Parser::ASSIGN);
                const auto value = parseTest(lexer);
                const auto keyword = new Keyword(arg_name, value);
                return keyword;
            }
            break;
        }
        case Python3Parser::POWER: {
            lexer.consume(Python3Parser::POWER);
            const auto value = parseTest(lexer);
            const auto keyword = new Keyword("", value);
            return keyword;
        }
        case Python3Parser::STAR: {
            lexer.consume(Python3Parser::STAR);
            const auto value = parseTest(lexer);
            const auto keyword = new StarredExpr(value);
            return keyword;
        }
        default:
        ERR_MSG_EXIT("Expected STAR, POWER or TEST");
    }

    return fallback_arg;
}

TryStmt *parseTryStmt(PyLexer &lexer) {
    lexer.consume(Python3Parser::TRY);
    lexer.consume(Python3Parser::COLON);
    auto try_stmt = new TryStmt(nullptr, {}, nullptr, nullptr);
    try_stmt->body = parseSuite(lexer);
    switch (lexer.curr->getType()) {
        case Python3Parser::EXCEPT: {
            while (lexer.curr->getType() == Python3Parser::EXCEPT) {
                auto except_handler = parseExceptClause(lexer);
                lexer.consume(Python3Parser::COLON);
                except_handler->body = parseSuite(lexer);
                try_stmt->handlers.push_back(except_handler);
            }

            if (lexer.curr->getType() == Python3Parser::ELSE) {
                lexer.consume(Python3Parser::ELSE);
                lexer.consume(Python3Parser::COLON);
                try_stmt->or_else = parseSuite(lexer);
            }

            if (lexer.curr->getType() == Python3Parser::FINALLY) {
                lexer.consume(Python3Parser::FINALLY);
                lexer.consume(Python3Parser::COLON);
                try_stmt->final_body = parseSuite(lexer);
            }
            break;
        }
        case Python3Parser::FINALLY: {
            lexer.consume(Python3Parser::FINALLY);
            lexer.consume(Python3Parser::COLON);
            try_stmt->final_body = parseSuite(lexer);
            break;
        }
        default:
            break;
    }

    return try_stmt;
}

ExceptHandler *parseExceptClause(PyLexer &lexer) {
    lexer.consume(Python3Parser::EXCEPT);
    auto except_handler = new ExceptHandler(nullptr, "", nullptr);
    const auto current_type = lexer.curr->getType();
    if (current_type == Python3Parser::STRING ||
        current_type == Python3Parser::NUMBER ||
        current_type == Python3Parser::LAMBDA ||
        current_type == Python3Parser::NOT ||
        current_type == Python3Parser::NONE ||
        current_type == Python3Parser::TRUE ||
        current_type == Python3Parser::FALSE ||
        current_type == Python3Parser::AWAIT ||
        current_type == Python3Parser::NAME ||
        current_type == Python3Parser::ELLIPSIS ||
        current_type == Python3Parser::OPEN_PAREN ||
        current_type == Python3Parser::OPEN_BRACK ||
        current_type == Python3Parser::ADD ||
        current_type == Python3Parser::MINUS ||
        current_type == Python3Parser::NOT_OP ||
        current_type == Python3Parser::OPEN_BRACE) {

        except_handler->type = parseTest(lexer);

        if (lexer.curr->getType() == Python3Parser::AS) {
            lexer.consume(Python3Parser::AS);
            except_handler->name = lexer.curr->getText();
            lexer.consume(Python3Parser::NAME);
        }
    }

    return except_handler;
}


Node *parseDecorator(PyLexer &lexer) {
    lexer.consume(Python3Parser::AT);
    auto call = new Call(nullptr, nullptr);
    call->func = parseDottedName(lexer, false);
    if (lexer.curr->getType() == Python3Parser::OPEN_PAREN) {
        lexer.consume(Python3Parser::OPEN_PAREN);
        call->arguments = parseArglist(lexer);
        lexer.consume(Python3Parser::CLOSE_PAREN);
    }

    lexer.consume(Python3Parser::NEWLINE);

    return call;
}

std::vector<Node *> parseDecorators(PyLexer &lexer) {
    std::vector<Node *> decorators;
    while (lexer.curr->getType() == Python3Parser::AT) {
        decorators.push_back(parseDecorator(lexer));
    }

    return decorators;
}

Node *parseDecorated(PyLexer &lexer) {
    auto decorator_list = parseDecorators(lexer);

    switch (lexer.curr->getType()) {
        case Python3Parser::CLASS:
            return parseClassDef(lexer, std::forward<std::vector<Node *>>(decorator_list));
        case Python3Parser::ASYNC:
            return parseAsyncFuncDef(lexer, decorator_list);
        case Python3Parser::DEF:
            return parseFuncDef(lexer, std::forward<std::vector<Node *>>(decorator_list));
        default:
        ERR_MSG_EXIT("Expected CLASS, ASYNC or DEF");
    }
}

Node *parseAsyncStmt(PyLexer &lexer) {
    lexer.consume(Python3Parser::ASYNC);

    switch (lexer.curr->getType()) {
        case Python3Parser::DEF:
            return parseAsyncFuncDef(lexer, {});
        case Python3Parser::WITH:
            return parseAsyncWithStmt(lexer);
        case Python3Parser::FOR:
            return parseAsyncForStmt(lexer);
        default:
        ERR_MSG_EXIT("Expected DEF, WITH or FOR");
    }
}

AsyncFuncDef *parseAsyncFuncDef(PyLexer &lexer, std::vector<Node *> &decorator_list) {
    lexer.consume(Python3Parser::ASYNC);
    const auto async_func_def = new AsyncFuncDef(
            parseFuncDef(lexer, std::forward<std::vector<Node *>>(decorator_list)), true);
    return async_func_def;
}

Node *parseAsyncForStmt(PyLexer &lexer) {
    lexer.consume(Python3Parser::ASYNC);
    auto async_for_stmt = new AsyncForStmt(parseForStmt(lexer), true);
    return async_for_stmt;
}

AsyncWithStmt *parseAsyncWithStmt(PyLexer &lexer) {
    lexer.consume(Python3Parser::ASYNC);
    const auto async_with_stmt = new AsyncWithStmt(parseWithStmt(lexer), true);
    return async_with_stmt;
}

Node *parseOrTest(PyLexer &lexer) {
    auto node = parseAndTest(lexer);

    while (lexer.curr->getType() == Python3Parser::OR) {
        lexer.consume(Python3Parser::OR);

        const auto rhs = parseAndTest(lexer);
        node = new BoolOp(node, rhs, Python3Parser::OR);
    }

    return node;
}

ForStmt *parseForStmt(PyLexer &lexer) {
    const auto pos_info = getTokPos(lexer.curr);
    lexer.consume(Python3Parser::FOR);

    auto for_stmt = new ForStmt(nullptr, nullptr, nullptr, nullptr);
    for_stmt->pos_info = pos_info;
    for_stmt->target = parseExprList(lexer);
    lexer.consume(Python3Parser::IN);
    for_stmt->iter = parseTestlist(lexer);
    lexer.consume(Python3Parser::COLON);
    for_stmt->body = parseSuite(lexer);

    if (lexer.curr->getType() == Python3Parser::ELSE) {
        lexer.consume(Python3Parser::ELSE);
        lexer.consume(Python3Parser::COLON);
        for_stmt->or_else = parseSuite(lexer);
    }

    return for_stmt;
}

Node *parseAndTest(PyLexer &lexer) {
    auto node = parseNotTest(lexer);

    while (lexer.curr->getType() == Python3Parser::AND) {
        lexer.consume(Python3Parser::AND);

        const auto rhs = parseNotTest(lexer);
        node = new BoolOp(node, rhs, Python3Parser::AND);
    }

    return node;
}

Node *parseNotTest(PyLexer &lexer) {
    Node *node;
    switch (lexer.curr->getType()) {
        case Python3Parser::NOT: {
            lexer.consume(Python3Parser::NOT);
            const auto expr = parseNotTest(lexer);
            node = new UnaryOp(Python3Parser::NOT, expr);
            break;
        }
        case Python3Parser::STRING:
        case Python3Parser::NUMBER:
        case Python3Parser::NONE:
        case Python3Parser::TRUE:
        case Python3Parser::FALSE:
        case Python3Parser::AWAIT:
        case Python3Parser::NAME:
        case Python3Parser::ELLIPSIS:
        case Python3Parser::OPEN_PAREN:
        case Python3Parser::OPEN_BRACK:
        case Python3Parser::ADD:
        case Python3Parser::MINUS:
        case Python3Parser::NOT_OP:
        case Python3Parser::OPEN_BRACE: {
            node = parseComparison(lexer);
            break;
        }
        default: {
            ERR_MSG_EXIT("expected NOT or EXPR");
        }
    }

    return node;
}

Node *parseLambDef(PyLexer &lexer) {
    lexer.consume(Python3Parser::LAMBDA);
    auto lambda = new Lambda(nullptr, nullptr);
    const auto current_type = lexer.curr->getType();
    if (current_type == Python3Parser::NAME ||
        current_type == Python3Parser::STAR ||
        current_type == Python3Parser::POWER) {
        lambda->args = parseVarArgsList(lexer);
    }
    lexer.consume(Python3Parser::COLON);
    lambda->body = parseTest(lexer);

    return lambda;
}

Node *parseTest(PyLexer &lexer) {
    auto node = parseOrTest(lexer);

    switch (lexer.curr->getType()) {
        case Python3Parser::IF: {
            lexer.consume(Python3Parser::IF);
            const auto if_test = parseOrTest(lexer);
            lexer.consume(Python3Parser::ELSE);
            const auto else_body = parseTest(lexer);
            node = new IfStmt(if_test, node, else_body);
            break;
        }
        case Python3Parser::LAMBDA:
            node = parseLambDef(lexer);
            break;
    }

    return node;
}

Node *parseExpr(PyLexer &lexer) {
    auto xor_expr = parseXorExpr(lexer);

    while (lexer.curr->getType() == Python3Parser::OR_OP) {
        lexer.consume(Python3Parser::OR_OP);

        const auto rhs = parseXorExpr(lexer);

        xor_expr = new BinOp(xor_expr, rhs, Python3Parser::OR_OP);
    }

    return xor_expr;
}

// testlist_star_expr: (test|star_expr) (',' (test|star_expr))* (',')?;
TestList *parseTestlistStarExpr(PyLexer &lexer) {
    auto test_list = new TestList({});
    Node *expr;
    if (lexer.curr->getType() == Python3Parser::STAR) {
        expr = parseStarExpr(lexer);
    } else {
        expr = parseTest(lexer);
    }

    test_list->nodes.push_back(expr);

    while (lexer.curr->getType() == Python3Parser::COMMA &&
           (lexer.next->getType() == Python3Parser::STAR || isTest(lexer.next))) {
        lexer.consume(Python3Parser::COMMA);
        switch (lexer.curr->getType()) {
            case Python3Parser::STAR: {
                test_list->nodes.push_back(parseStarExpr(lexer));
                break;
            }
            case Python3Parser::STRING:
            case Python3Parser::NUMBER:
            case Python3Parser::LAMBDA:
            case Python3Parser::NOT:
            case Python3Parser::NONE:
            case Python3Parser::TRUE:
            case Python3Parser::FALSE:
            case Python3Parser::AWAIT:
            case Python3Parser::NAME:
            case Python3Parser::ELLIPSIS:
            case Python3Parser::OPEN_PAREN:
            case Python3Parser::OPEN_BRACK:
            case Python3Parser::ADD:
            case Python3Parser::MINUS:
            case Python3Parser::NOT_OP:
            case Python3Parser::OPEN_BRACE: {
                test_list->nodes.push_back(parseTest(lexer));
                break;
            }
            default: {
                ERR_MSG_EXIT("expected STAR or TEST");
            }
        }
    }

    if (lexer.curr->getType() == Python3Parser::COMMA) {
        lexer.consume(Python3Parser::COMMA);
    }

    return test_list;
}

Global *parseGlobalStmt(PyLexer &lexer) {
    lexer.consume(Python3Parser::GLOBAL);

    auto global = new Global({});

    const auto current_token = lexer.curr;
    lexer.consume(Python3Parser::NAME);
    const auto name = new Name(current_token->getText());

    global->names.push_back(name);
    while (lexer.curr->getType() == Python3Parser::COMMA) {
        lexer.consume(Python3Parser::COMMA);
        const auto name = new Name(lexer.curr->getText());
        global->names.push_back(name);
    }

    return global;
}

Nonlocal *parseNonlocalStmt(PyLexer &lexer) {
    lexer.consume(Python3Parser::NONLOCAL);

    auto nonlocal = new Nonlocal({});

    const auto current_token = lexer.curr;
    lexer.consume(Python3Parser::NAME);
    const auto name = new Name(current_token->getText());

    nonlocal->names.push_back(name);
    while (lexer.curr->getType() == Python3Parser::COMMA) {
        lexer.consume(Python3Parser::COMMA);
        const auto name = new Name(lexer.curr->getText());
        nonlocal->names.push_back(name);
    }

    return nonlocal;
}

// expr_stmt: testlist_star_expr (annassign | augassign (yield_expr|testlist) |
//                     ('=' (yield_expr|testlist_star_expr))*);
Node *parseExprStmt(PyLexer &lexer) {
    const auto test_list_star_expr = parseTestlistStarExpr(lexer);

    switch (lexer.curr->getType()) {
        case Python3Parser::COLON: {
            const auto ann_assign = parseAnnAssign(lexer);
            ann_assign->target = test_list_star_expr;
            return ann_assign;
        }
        case Python3Parser::ASSIGN: {
            auto assign = new Assign(nullptr, nullptr);
            assign->targets = test_list_star_expr;
            while (lexer.curr->getType() == Python3Parser::ASSIGN) {
                lexer.consume(Python3Parser::ASSIGN);
                if (lexer.curr->getType() == Python3Parser::YIELD) {
                    assign->value = parseYieldExpr(lexer);
                } else {
                    assign->value = parseTestlistStarExpr(lexer);
                }
            }
            return assign;
        }
        case Python3Parser::ADD_ASSIGN:
        case Python3Parser::SUB_ASSIGN:
        case Python3Parser::MULT_ASSIGN:
        case Python3Parser::AT_ASSIGN:
        case Python3Parser::DIV_ASSIGN:
        case Python3Parser::MOD_ASSIGN:
        case Python3Parser::AND_ASSIGN:
        case Python3Parser::OR_ASSIGN:
        case Python3Parser::XOR_ASSIGN:
        case Python3Parser::LEFT_SHIFT_ASSIGN:
        case Python3Parser::RIGHT_SHIFT_ASSIGN:
        case Python3Parser::POWER_ASSIGN:
        case Python3Parser::IDIV_ASSIGN: {
            auto aug_assign = new AugAssign(nullptr, lexer.curr->getType(), nullptr);
            aug_assign->target = test_list_star_expr;
            lexer.consume(lexer.curr->getType());
            Node *value;
            if (lexer.curr->getType() == Python3Parser::YIELD) {
                value = parseYieldExpr(lexer);
            } else {
                value = parseTestList(lexer);
            }
            aug_assign->value = value;
            return aug_assign;
        }
        default:
            break;
    }

    return test_list_star_expr;
}

AnnAssign *parseAnnAssign(PyLexer &lexer) {
    lexer.consume(Python3Parser::COLON);

    auto ann_assign = new AnnAssign(nullptr, nullptr, nullptr);

    const auto annotation = parseTest(lexer);

    ann_assign->annotation = annotation;
    if (lexer.curr->getType() == Python3Parser::ASSIGN) {
        lexer.consume(Python3Parser::ASSIGN);

        const auto value = parseTest(lexer);
        ann_assign->value = value;
    }

    return ann_assign;
}

TestList *parseTestListStarExpr(PyLexer &lexer) {
    TestList *test_list = new TestList({});

    Node *node;
    if (lexer.curr->getType() == Python3Parser::STAR) {
        node = parseStarExpr(lexer);
    } else {
        node = parseTest(lexer);
    }

    test_list->nodes.push_back(node);
    while (lexer.curr->getType() == Python3Parser::COMMA) {
        lexer.consume(Python3Parser::COMMA);

        Node *n;
        if (lexer.curr->getType() == Python3Parser::STAR) {
            n = parseStarExpr(lexer);
        } else {
            n = parseTest(lexer);
        }
        test_list->nodes.push_back(n);
    }

    return test_list;
}

StarredExpr *parseStarExpr(PyLexer &lexer) {
    lexer.consume(Python3Parser::STAR);

    auto node = new StarredExpr(nullptr);
    node->expr = parseExpr(lexer);

    return node;
}

Node *parseXorExpr(PyLexer &lexer) {
    auto node = parseAndExpr(lexer);

    while (lexer.curr->getType() == Python3Parser::XOR) {
        lexer.consume(Python3Parser::XOR);

        const auto rhs = parseAndExpr(lexer);

        node = new BinOp(node, rhs, Python3Parser::XOR);
    }

    return node;
}


Node *parseAndExpr(PyLexer &lexer) {
    auto node = parseShiftExpr(lexer);

    while (lexer.curr->getType() == Python3Parser::AND_OP) {
        lexer.consume(Python3Parser::AND_OP);

        const auto rhs = parseShiftExpr(lexer);

        node = new BinOp(node, rhs, Python3Parser::AND_OP);
    }

    return node;
}

Node *parseShiftExpr(PyLexer &lexer) {
    auto node = parseArithExpr(lexer);

    while (lexer.curr->getType() == Python3Parser::LEFT_SHIFT ||
           lexer.curr->getType() == Python3Parser::RIGHT_SHIFT) {
        const auto current_token = lexer.curr;
        lexer.consume(current_token->getType());

        const auto rhs = parseArithExpr(lexer);
        node = new BinOp(node, rhs, current_token->getType());
    }

    return node;
}

Node *parseComparison(PyLexer &lexer) {
    auto node = parseExpr(lexer);

    bool eat_twice{false};
    int32_t op;
    while (isCompOp(lexer, eat_twice, op)) {
        lexer.consume(lexer.curr->getType());
        if (eat_twice)
            lexer.consume(lexer.curr->getType());
        const auto rhs = parseExpr(lexer);
        node = new Comparison(node, rhs, op);
    }

    return node;
}

// power: atom_expr ('**' factor)?;
Node *parsePower(PyLexer &lexer) {
    auto node = parseAtomExpr(lexer);

    while (lexer.curr->getType() == Python3Parser::POWER) {
        lexer.consume(Python3Parser::POWER);
        auto rhs = parseFactor(lexer);

        node = new BinOp(node, rhs, Python3Parser::POWER);
    }

    return node;
}

Node *parseAtomExpr(PyLexer &lexer) {
    if (lexer.curr->getType() == Python3Parser::AWAIT) {
        lexer.consume(Python3Parser::AWAIT);
    }

    auto atom = parseAtom(lexer);
    while (lexer.curr->getType() == Python3Parser::DOT ||
           lexer.curr->getType() == Python3Parser::OPEN_PAREN ||
           lexer.curr->getType() == Python3Parser::OPEN_BRACK) {

        switch (lexer.curr->getType()) {
            case Python3Parser::OPEN_PAREN: {
                lexer.consume(Python3Parser::OPEN_PAREN);
                auto call = new Call(atom, nullptr);
                if (lexer.curr->getType() != Python3Parser::CLOSE_PAREN)
                    call->arguments = parseArglist(lexer);
                lexer.consume(Python3Parser::CLOSE_PAREN);
                atom = call;
                break;
            }
            case Python3Parser::OPEN_BRACK: {
                lexer.consume(Python3Parser::OPEN_BRACK);
                auto subscript = parseSubscriptList(lexer);
                subscript->value = atom;
                lexer.consume(Python3Parser::CLOSE_BRACK);
                atom = subscript;
                break;
            }
            case Python3Parser::DOT: {
                lexer.consume(Python3Parser::DOT);
                auto attribute = new Attribute(atom, nullptr);
                const auto current_token = lexer.curr;
                lexer.consume(Python3Parser::NAME);
                attribute->attr = new Name(current_token->getText());
                atom = attribute;
                break;
            }
            default:
            ERR_MSG_EXIT("Expected OPEN_PAREN, OPEN_BRACK or DOT");
        }
    }

    return atom;
}

static Node *parseTestlistCompCommaSeparated(Node *value, PyLexer &lexer) {
    auto list = new List({value});
    while (lexer.curr->getType() == Python3Parser::COMMA &&
           (isTest(lexer.next) || lexer.next->getType() == Python3Parser::STAR)) {
        lexer.consume(Python3Parser::COMMA);

        switch (lexer.curr->getType()) {
            case Python3Parser::STRING:
            case Python3Parser::NUMBER:
            case Python3Parser::LAMBDA:
            case Python3Parser::NOT:
            case Python3Parser::NONE:
            case Python3Parser::TRUE:
            case Python3Parser::FALSE:
            case Python3Parser::AWAIT:
            case Python3Parser::NAME:
            case Python3Parser::ELLIPSIS:
            case Python3Parser::OPEN_PAREN:
            case Python3Parser::OPEN_BRACK:
            case Python3Parser::ADD:
            case Python3Parser::MINUS:
            case Python3Parser::NOT_OP:
            case Python3Parser::OPEN_BRACE: {
                list->elements.push_back(parseTest(lexer));
                break;
            }
            case Python3Parser::STAR: {
                list->elements.push_back(parseStarExpr(lexer));
                break;
            }
            default: {
                if (lexer.curr->getType() == Python3Parser::CLOSE_BRACE) {
                    break;
                } else {
                    ERR_MSG_EXIT("expected TEST or STAR");
                }
            }
        }
    }
    if (lexer.curr->getType() == Python3Parser::COMMA) {
        lexer.consume(Python3Parser::COMMA);
    }

    return list;
}

Node *parseTestlistComp(PyLexer &lexer) {
    Node *node;
    switch (lexer.curr->getType()) {
        case Python3Parser::STRING:
        case Python3Parser::NUMBER:
        case Python3Parser::LAMBDA:
        case Python3Parser::NOT:
        case Python3Parser::NONE:
        case Python3Parser::TRUE:
        case Python3Parser::FALSE:
        case Python3Parser::AWAIT:
        case Python3Parser::NAME:
        case Python3Parser::ELLIPSIS:
        case Python3Parser::OPEN_PAREN:
        case Python3Parser::OPEN_BRACK:
        case Python3Parser::ADD:
        case Python3Parser::MINUS:
        case Python3Parser::NOT_OP:
        case Python3Parser::OPEN_BRACE: {
            node = parseTest(lexer);
            break;
        }
        case Python3Parser::STAR: {
            node = parseStarExpr(lexer);
            break;
        }
        default: {
            ERR_MSG_EXIT("expected TEST or STAR");
        }
    }
    switch (lexer.curr->getType()) {
        case Python3Parser::FOR:
        case Python3Parser::ASYNC: {
            node = new ListComp(node, {});
            while (lexer.curr->getType() == Python3Parser::ASYNC ||
                   lexer.curr->getType() == Python3Parser::FOR) {

                auto comprehension = parseCompFor(lexer);
                dynamic_cast<ListComp *>(node)->generators.push_back(comprehension);
            }
            break;
        }
        case Python3Parser::CLOSE_PAREN:
        case Python3Parser::COMMA:
        case Python3Parser::CLOSE_BRACK: {
            node = new List({node});
            while (lexer.curr->getType() == Python3Parser::COMMA &&
                   (isTest(lexer.next) || lexer.next->getType() == Python3Parser::STAR)) {
                lexer.consume(Python3Parser::COMMA);
                switch (lexer.curr->getType()) {
                    case Python3Parser::STRING:
                    case Python3Parser::NUMBER:
                    case Python3Parser::LAMBDA:
                    case Python3Parser::NOT:
                    case Python3Parser::NONE:
                    case Python3Parser::TRUE:
                    case Python3Parser::FALSE:
                    case Python3Parser::AWAIT:
                    case Python3Parser::NAME:
                    case Python3Parser::ELLIPSIS:
                    case Python3Parser::OPEN_PAREN:
                    case Python3Parser::OPEN_BRACK:
                    case Python3Parser::ADD:
                    case Python3Parser::MINUS:
                    case Python3Parser::NOT_OP:
                    case Python3Parser::OPEN_BRACE: {
                        dynamic_cast<List *>(node)->elements.push_back(parseTest(lexer));
                        break;
                    }
                    case Python3Parser::STAR: {
                        dynamic_cast<List *>(node)->elements.push_back(parseStarExpr(lexer));
                        break;
                    }
                    default: {
                        ERR_MSG_EXIT("Expected TEST or STAR");
                    }
                }
            }

            if (lexer.curr->getType() == Python3Parser::COMMA) {
                lexer.consume(Python3Parser::COMMA);
            }
        }
    }

    return node;
}


// testlist_comp: (test|star_expr) ( comp_for | (',' (test|star_expr))* (',')? );
//Node *parseTestlistComp(PyLexer &lexer) {
//    std::cout << lexer.curr->getText() << '\n';
//    switch (lexer.curr->getType()) {
//        case Python3Parser::STRING:
//        case Python3Parser::NUMBER:
//        case Python3Parser::LAMBDA:
//        case Python3Parser::NOT:
//        case Python3Parser::NONE:
//        case Python3Parser::TRUE:
//        case Python3Parser::FALSE:
//        case Python3Parser::AWAIT:
//        case Python3Parser::NAME:
//        case Python3Parser::ELLIPSIS:
//        case Python3Parser::OPEN_PAREN:
//        case Python3Parser::OPEN_BRACK:
//        case Python3Parser::ADD:
//        case Python3Parser::MINUS:
//        case Python3Parser::NOT_OP:
//        case Python3Parser::OPEN_BRACE: {
//            const auto value = parseTest(lexer);
//
//            switch (lexer.curr->getType()) {
//                case Python3Parser::ASYNC:
//                case Python3Parser::FOR: {
//                    auto list_comp = new ListComp(value, {});
//
//                    while (lexer.curr->getType() == Python3Parser::ASYNC ||
//                           lexer.curr->getType() == Python3Parser::FOR) {
//
//                        const auto comprehension = parseCompFor(lexer);
//                        list_comp->generators.push_back(comprehension);
//                    }
//
//                    return list_comp;
//                }
//                case Python3Parser::COMMA: {
//                    return parseTestlistCompCommaSeparated(value, lexer);
//                }
//            }
//            return value;
//        }
//        case Python3Parser::STAR: {
//            const auto value = parseStarExpr(lexer);
//            return parseTestlistCompCommaSeparated(value, lexer);
//        }
//        default:
//        ERR_MSG_EXIT("Expected TEST or STAR");
//    }
//}

Subscript *parseSubscriptList(PyLexer &lexer) {
    auto subscript = parseSubscript(lexer);

    if (lexer.curr->getType() == Python3Parser::COMMA) {
        auto ext_slice = new ExtSlice({subscript->slice});
        while (lexer.curr->getType() == Python3Parser::COMMA && isTest(lexer.next)) {
            lexer.consume(Python3Parser::COMMA);

            const auto value = parseTest(lexer);
            if (lexer.curr->getType() == Python3Lexer::COLON) {
                lexer.consume(Python3Parser::COLON);
                auto slice = new Slice(value, nullptr, nullptr);
                if (isTest(lexer.curr)) {
                    slice->upper = parseTest(lexer);
                }
                if (lexer.curr->getType() == Python3Parser::COLON) {
                    lexer.consume(Python3Parser::COLON);

                    if (isTest(lexer.curr)) {
                        slice->step = parseTest(lexer);
                    }
                }
                ext_slice->dims.push_back(slice);
            } else {
                ext_slice->dims.push_back(new Index(value));
            }
        }
        if (lexer.curr->getType() == Python3Parser::COMMA) {
            lexer.consume(Python3Parser::COMMA);
        }

        subscript->slice = ext_slice;
    }

    return subscript;
}

Subscript *parseSubscript(PyLexer &lexer) {
    Subscript *subscript;
    if (isTest(lexer.curr)) {
        subscript = new Subscript(nullptr, nullptr);
        const auto value = parseTest(lexer);
        if (lexer.curr->getType() == Python3Lexer::COLON) {
            lexer.consume(Python3Parser::COLON);
            auto slice = new Slice(value, nullptr, nullptr);
            if (isTest(lexer.curr)) {
                slice->upper = parseTest(lexer);
            }
            if (lexer.curr->getType() == Python3Parser::COLON) {
                lexer.consume(Python3Parser::COLON);

                if (isTest(lexer.curr)) {
                    slice->step = parseTest(lexer);
                }
            }
            subscript->slice = slice;
        } else {
            subscript->slice = new Index(value);
        }
        return subscript;
    } else if (lexer.curr->getType() == Python3Parser::COLON) {
        lexer.consume(Python3Parser::COLON);
        subscript = new Subscript(nullptr, nullptr);
        auto slice = new Slice(nullptr, nullptr, nullptr);
        if (isTest(lexer.curr)) {
            slice->upper = parseTest(lexer);
        }
        if (lexer.curr->getType() == Python3Parser::COLON) {
            lexer.consume(Python3Parser::COLON);

            if (isTest(lexer.curr)) {
                slice->step = parseTest(lexer);
            }
        }
        subscript->slice = slice;
    } else {
        ERR_MSG_EXIT("expected TEST or COLON");
    }
}

Node *parseAtom(PyLexer &lexer) {
    Node *node;
    const auto current_token = lexer.curr;
    switch (current_token->getType()) {
        case Python3Parser::OPEN_PAREN: {
            lexer.consume(Python3Parser::OPEN_PAREN);

            switch (lexer.curr->getType()) {
                case Python3Parser::YIELD: {
                    node = parseYieldExpr(lexer);
                    break;
                }
                case Python3Parser::STRING:
                case Python3Parser::NUMBER:
                case Python3Parser::LAMBDA:
                case Python3Parser::NOT:
                case Python3Parser::NONE:
                case Python3Parser::TRUE:
                case Python3Parser::FALSE:
                case Python3Parser::AWAIT:
                case Python3Parser::NAME:
                case Python3Parser::ELLIPSIS:
                case Python3Parser::OPEN_PAREN:
                case Python3Parser::OPEN_BRACK:
                case Python3Parser::ADD:
                case Python3Parser::MINUS:
                case Python3Parser::NOT_OP:
                case Python3Parser::OPEN_BRACE:
                case Python3Parser::STAR: {
                    node = parseTestlistComp(lexer);
                    break;
                }
                default:
                    break;
            }
            lexer.consume(Python3Parser::CLOSE_PAREN);
            break;
        }
        case Python3Parser::OPEN_BRACK: {
            lexer.consume(Python3Parser::OPEN_BRACK);

            if (isTestlistComp(lexer.curr))
                node = parseTestlistComp(lexer);
            else
                node = new List({});
            lexer.consume(Python3Parser::CLOSE_BRACK);
            break;
        }
        case Python3Parser::OPEN_BRACE: {
            lexer.consume(Python3Parser::OPEN_BRACE);

            switch (lexer.curr->getType()) {
                case Python3Parser::STRING:
                case Python3Parser::NUMBER:
                case Python3Parser::LAMBDA:
                case Python3Parser::NOT:
                case Python3Parser::NONE:
                case Python3Parser::TRUE:
                case Python3Parser::FALSE:
                case Python3Parser::AWAIT:
                case Python3Parser::NAME:
                case Python3Parser::ELLIPSIS:
                case Python3Parser::OPEN_PAREN:
                case Python3Parser::OPEN_BRACK:
                case Python3Parser::ADD:
                case Python3Parser::MINUS:
                case Python3Parser::NOT_OP:
                case Python3Parser::OPEN_BRACE:
                case Python3Parser::POWER:
                    node = parseDictorsetmaker(lexer);
                    break;
                default:
                    node = new Dict({}, {});
                    break;
            }

            lexer.consume(Python3Parser::CLOSE_BRACE);
            break;
        }
        case Python3Parser::NUMBER: {
            lexer.consume(Python3Parser::NUMBER);
            node = new Const(current_token->getText(), Python3Parser::NUMBER);
            break;
        }
        case Python3Parser::STRING: {
            std::string str;
            while (lexer.curr->getType() == Python3Parser::STRING) {
                str += lexer.curr->getText();
                lexer.consume(Python3Parser::STRING);
            }
            node = new Const(str, Python3Parser::STRING);
            break;
        }
        case Python3Parser::NAME: {
            lexer.consume(Python3Parser::NAME);
            node = new Name(current_token->getText());
            break;
        }
        case Python3Parser::ELLIPSIS: {
            // not sure about that one
            node = new Const(lexer.curr->getText(), Python3Parser::NONE);
            lexer.consume(Python3Parser::ELLIPSIS);
            break;
        }
        case Python3Parser::NONE: {
            node = new Const(lexer.curr->getText(), Python3Parser::NONE);
            lexer.consume(Python3Parser::NONE);
            break;
        }
        case Python3Parser::FALSE: {
            node = new Const(lexer.curr->getText(), Python3Parser::NONE);
            lexer.consume(Python3Parser::FALSE);
            break;
        }
        case Python3Parser::TRUE: {
            node = new Const(lexer.curr->getText(), Python3Parser::NONE);
            lexer.consume(Python3Parser::TRUE);
            break;
        }
        default: {
            printf("%s", current_token->getText().c_str());
            ERR_MSG_EXIT("Encountered an unknown node");
        }
    }

    return node;
}

Node *parseCompFor(PyLexer &lexer) {
    auto comp_for = new Comprehension(nullptr, nullptr, {}, false);
    if (lexer.curr->getType() == Python3Parser::ASYNC) {
        lexer.consume(Python3Parser::ASYNC);
        comp_for->is_async = true;
    }

    lexer.consume(Python3Parser::FOR);
    comp_for->target = parseExprList(lexer);

    lexer.consume(Python3Parser::IN);
    comp_for->iter = parseOrTest(lexer);

    while (lexer.curr->getType() == Python3Parser::IF) {
        lexer.consume(Python3Parser::IF);
        comp_for->ifs.push_back(parseTestNoCond(lexer));
    }

    return comp_for;
}

Node *parseDict(PyLexer &lexer) {
    const auto key = parseTest(lexer);
    lexer.consume(Python3Parser::COLON);
    const auto value = parseTest(lexer);

    switch (lexer.curr->getType()) {
        case Python3Parser::ASYNC:
        case Python3Parser::FOR: {
            auto dict = new DictComp(key, value, {});
            const auto comprehension = parseCompFor(lexer);
            dict->generators.push_back(comprehension);
            return dict;
        }
        default:
            return nullptr;
    }
}

static Node *parseDictorsetmakerTestColon(PyLexer &lexer) {
    const auto key = parseTest(lexer);
    lexer.consume(Python3Parser::COLON);
    const auto value = parseTest(lexer);

    switch (lexer.curr->getType()) {
        case Python3Parser::ASYNC:
        case Python3Parser::FOR: {
            auto dict_comp = new DictComp(key, value, {});
            while (lexer.curr->getType() == Python3Parser::ASYNC ||
                   lexer.curr->getType() == Python3Parser::FOR) {

                const auto comprehension = parseCompFor(lexer);
                dict_comp->generators.push_back(comprehension);
            }

            return dict_comp;
        }
        case Python3Parser::COMMA: {
            auto dict = new Dict({key}, {value});
            while (lexer.curr->getType() == Python3Parser::COMMA) {
                lexer.consume(Python3Parser::COMMA);

                switch (lexer.curr->getType()) {
                    case Python3Parser::STRING:
                    case Python3Parser::NUMBER:
                    case Python3Parser::LAMBDA:
                    case Python3Parser::NOT:
                    case Python3Parser::NONE:
                    case Python3Parser::TRUE:
                    case Python3Parser::FALSE:
                    case Python3Parser::AWAIT:
                    case Python3Parser::NAME:
                    case Python3Parser::ELLIPSIS:
                    case Python3Parser::OPEN_PAREN:
                    case Python3Parser::OPEN_BRACK:
                    case Python3Parser::ADD:
                    case Python3Parser::MINUS:
                    case Python3Parser::NOT_OP:
                    case Python3Parser::OPEN_BRACE: {
                        dict->keys.push_back(parseTest(lexer));
                        lexer.consume(Python3Parser::COLON);
                        dict->values.push_back(parseTest(lexer));
                        break;
                    }
                    case Python3Parser::POWER: {
                        lexer.consume(Python3Parser::POWER);
                        dict->values.push_back(parseTest(lexer));
                        break;
                    }
                    default: {
                        if (lexer.curr->getType() == Python3Parser::CLOSE_BRACE) {
                            break;
                        } else {
                            ERR_MSG_EXIT("expected TEST or POWER");
                        }
                    }
                }
            }
            return dict;
        }
        default: {
            if (lexer.curr->getType() == Python3Parser::CLOSE_BRACE) {
                auto dict = new Dict({key}, {value});
                return dict;
            } else {
                ERR_MSG_EXIT("Expected COMP_FOR or COMMA at line %d", lexer.curr->getLine());
            }
        }
    }
}

static Node *parseDictorsetmakerTest(PyLexer &lexer) {
    const auto value = parseTest(lexer);

    switch (lexer.curr->getType()) {
        case Python3Parser::ASYNC:
        case Python3Parser::FOR: {
            auto set_comp = new SetComp(value, {});
            while (lexer.curr->getType() == Python3Parser::ASYNC ||
                   lexer.curr->getType() == Python3Parser::FOR) {

                const auto comprehension = parseCompFor(lexer);
                set_comp->generators.push_back(comprehension);
            }

            return set_comp;
        }
        case Python3Parser::COMMA: {
            auto set = new Set({});
            while (lexer.curr->getType() == Python3Parser::COMMA) {
                lexer.consume(Python3Parser::COMMA);

                switch (lexer.curr->getType()) {
                    case Python3Parser::STRING:
                    case Python3Parser::NUMBER:
                    case Python3Parser::LAMBDA:
                    case Python3Parser::NOT:
                    case Python3Parser::NONE:
                    case Python3Parser::TRUE:
                    case Python3Parser::FALSE:
                    case Python3Parser::AWAIT:
                    case Python3Parser::NAME:
                    case Python3Parser::ELLIPSIS:
                    case Python3Parser::OPEN_PAREN:
                    case Python3Parser::OPEN_BRACK:
                    case Python3Parser::ADD:
                    case Python3Parser::MINUS:
                    case Python3Parser::NOT_OP:
                    case Python3Parser::OPEN_BRACE: {
                        set->elements.push_back(parseTest(lexer));
                        break;
                    }
                    case Python3Parser::STAR: {
                        set->elements.push_back(parseStarExpr(lexer));
                        break;
                    }
                    default: {
                        if (lexer.curr->getType() == Python3Parser::CLOSE_BRACE) {
                            break;
                       // how horrific it is
                auto set = new Set({value});
                return set;
                 } else {
                            ERR_MSG_EXIT("expected TEST or STAR");
                        }
                    }
                }
            }
            return set;
        }
        default: {
            if (lexer.curr->getType() == Python3Parser::CLOSE_BRACE) {
                // how horrific it is
                auto set = new Set({value});
                return set;
            } else {
                ERR_MSG_EXIT("Expected COMP_FOR or COMMA");
            }
        }
    }
}

//(
//  (
//      (test ':' test | '**' expr)
//      (comp_for | (',' (test ':' test | '**' expr))* (',')?)
//  ) |
//  ((test | star_expr) (comp_for | (',' (test | star_expr))* (',')?))
// );
Node *parseDictorsetmaker(PyLexer &lexer) {

    switch (lexer.curr->getType()) {
        case Python3Parser::STRING:
        case Python3Parser::NUMBER:
        case Python3Parser::LAMBDA:
        case Python3Parser::NOT:
        case Python3Parser::NONE:
        case Python3Parser::TRUE:
        case Python3Parser::FALSE:
        case Python3Parser::AWAIT:
        case Python3Parser::NAME:
        case Python3Parser::ELLIPSIS:
        case Python3Parser::OPEN_PAREN:
        case Python3Parser::OPEN_BRACK:
        case Python3Parser::ADD:
        case Python3Parser::MINUS:
        case Python3Parser::NOT_OP:
        case Python3Parser::OPEN_BRACE: {
            if (lexer.next->getType() == Python3Parser::COLON) {
                // parsing a dictionary
                return parseDictorsetmakerTestColon(lexer);
            } else {
                // parsing a set
                return parseDictorsetmakerTest(lexer);
            }
        }
        case Python3Parser::POWER: {
            lexer.consume(Python3Parser::POWER);
            const auto value = parseExpr(lexer);
            auto dict = new Dict({}, {value});
            while (lexer.curr->getType() == Python3Parser::COMMA) {
                lexer.consume(Python3Parser::COMMA);

                switch (lexer.curr->getType()) {
                    case Python3Parser::STRING:
                    case Python3Parser::NUMBER:
                    case Python3Parser::LAMBDA:
                    case Python3Parser::NOT:
                    case Python3Parser::NONE:
                    case Python3Parser::TRUE:
                    case Python3Parser::FALSE:
                    case Python3Parser::AWAIT:
                    case Python3Parser::NAME:
                    case Python3Parser::ELLIPSIS:
                    case Python3Parser::OPEN_PAREN:
                    case Python3Parser::OPEN_BRACK:
                    case Python3Parser::ADD:
                    case Python3Parser::MINUS:
                    case Python3Parser::NOT_OP:
                    case Python3Parser::OPEN_BRACE: {
                        dict->keys.push_back(parseTest(lexer));
                        lexer.consume(Python3Parser::COLON);
                        dict->values.push_back(parseTest(lexer));
                        break;
                    }
                    case Python3Parser::POWER: {
                        lexer.consume(Python3Parser::POWER);
                        dict->values.push_back(parseTest(lexer));
                        break;
                    }
                    default: {
                        if (lexer.curr->getType() == Python3Parser::CLOSE_BRACE) {
                            break;
                        } else {
                            ERR_MSG_EXIT("expected TEST or POWER");
                        }
                    }
                }
            }

            return dict;
        }
        case Python3Parser::STAR: {
            const auto value = parseStarExpr(lexer);

            auto set = new Set({value});
            while (lexer.curr->getType() == Python3Parser::COMMA) {
                lexer.consume(Python3Parser::COMMA);

                switch (lexer.curr->getType()) {
                    case Python3Parser::STRING:
                    case Python3Parser::NUMBER:
                    case Python3Parser::LAMBDA:
                    case Python3Parser::NOT:
                    case Python3Parser::NONE:
                    case Python3Parser::TRUE:
                    case Python3Parser::FALSE:
                    case Python3Parser::AWAIT:
                    case Python3Parser::NAME:
                    case Python3Parser::ELLIPSIS:
                    case Python3Parser::OPEN_PAREN:
                    case Python3Parser::OPEN_BRACK:
                    case Python3Parser::ADD:
                    case Python3Parser::MINUS:
                    case Python3Parser::NOT_OP:
                    case Python3Parser::OPEN_BRACE: {
                        set->elements.push_back(parseTest(lexer));
                        break;
                    }
                    case Python3Parser::STAR: {
                        set->elements.push_back(parseStarExpr(lexer));
                        break;
                    }
                    default: {
                        if (lexer.curr->getType() == Python3Parser::CLOSE_BRACE) {
                            break;
                        } else {
                            ERR_MSG_EXIT("expected TEST or POWER");
                        }
                    }
                }
            }
        }
        default: {
            ERR_MSG_EXIT("Expected TEST, STAR or POWER");
            break;
        }
    }
}

Node *parseTrailer(PyLexer &lexer) {

    switch (lexer.curr->getType()) {
        case Python3Parser::OPEN_PAREN: {
            lexer.consume(Python3Parser::OPEN_PAREN);
            switch (lexer.curr->getType()) {
                case Python3Parser::STRING:
                case Python3Parser::NUMBER:
                case Python3Parser::LAMBDA:
                case Python3Parser::NOT:
                case Python3Parser::NONE:
                case Python3Parser::TRUE:
                case Python3Parser::FALSE:
                case Python3Parser::AWAIT:
                case Python3Parser::NAME:
                case Python3Parser::ELLIPSIS:
                case Python3Parser::OPEN_PAREN:
                case Python3Parser::OPEN_BRACK:
                case Python3Parser::ADD:
                case Python3Parser::MINUS:
                case Python3Parser::NOT_OP:
                case Python3Parser::OPEN_BRACE:
                case Python3Parser::POWER:
                case Python3Parser::STAR: {
                    return parseArglist(lexer);
                }
            }
            break;
        }
        case Python3Parser::OPEN_BRACK: {
            lexer.consume(Python3Parser::OPEN_BRACK);
            auto subscript_list = parseSubscriptList(lexer);
            lexer.consume(Python3Parser::CLOSE_BRACK);
            return subscript_list;
        }
        case Python3Parser::DOT: {
            break;
        }
        default: {

            break;
        }
    }

    return nullptr;
}

Node *parseTerm(PyLexer &lexer) {
    auto node = parseFactor(lexer);

    while (isTermOp(lexer.curr)) {

        const auto token = lexer.curr;
        switch (token->getType()) {
            case Python3Parser::STAR:
                lexer.consume(Python3Parser::STAR);
                break;
            case Python3Parser::DIV:
                lexer.consume(Python3Parser::DIV);
                break;
            case Python3Parser::IDIV:
                lexer.consume(Python3Parser::IDIV);
                break;
            case Python3Parser::MOD:
                lexer.consume(Python3Parser::MOD);
                break;
        }

        const auto rhs = parseFactor(lexer);
        node = new BinOp(node, rhs, token->getType());
    }

    return node;
}

Node *parseArithExpr(PyLexer &lexer) {
    auto node = parseTerm(lexer);

    while (isArithOp(lexer.curr)) {

        const auto token = lexer.curr;
        switch (token->getType()) {
            case Python3Parser::ADD:
                lexer.consume(Python3Parser::ADD);
                break;
            case Python3Parser::MINUS:
                lexer.consume(Python3Parser::MINUS);
                break;
        }

        node = new BinOp(node, parseTerm(lexer), token->getType());
    }

    return node;
}

Node *buildAst(PyLexer &lexer) {
    lexer.updateCurr(1);
    return parseFileInput(lexer);
}

void destroyNode(Node *root) {
    if (!root) return;
    if (root->getChildren().empty()) {
        delete root;
    } else {
        // destroy all children
        for (const auto &child: root->getChildren()) {
            destroyNode(child);
        }

        // and only then destroy the parent
        delete root;
    }
}



