/*
 * Copyright 2026 Nobuharu Shimazu
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "parser.h"
#include "src/ast.h"
#include "src/lexer.h"

#include <stdio.h>

static Node *parse_declaration(Parser *p);
static Node *parse_import(Parser *p);
static Node *parse_fn(Parser *p, bool is_pub);
static Node *parse_type_decl(Parser *p, bool is_pub);
static Node *parse_record(Parser *p, NodeKind kind);
static Node *parse_enum(Parser *p);
static Node *parse_statement(Parser *p);
static Node *parse_var_decl(Parser *p, bool is_const);
static Node *parse_let_stmt(Parser *p, bool is_const);
static Node *parse_if(Parser *p);
static Node *parse_while(Parser *p);
static Node *parse_do_while(Parser *p);
static Node *parse_for(Parser *p);
static Node *parse_return(Parser *p);
static Node *parse_block(Parser *p);
static Node *parse_type(Parser *p);
static Node *parse_expr(Parser *p);
static Node *parse_assignment(Parser *p);
static Node *parse_binary(Parser *p, int min_prec);
static Node *parse_unary(Parser *p);
static Node *parse_postfix(Parser *p);
static Node *parse_primary(Parser *p);

static void error_at(Parser *p, Token *token, const char *msg) {
  if (p->panic_mode)
    return;

  p->panic_mode = true;
  p->had_error = true;

  fprintf(stderr, "[line %zu] Error", token->line);
  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // the lexeme is the message
  } else if (token->lexeme != NULL) {
    fprintf(stderr, " at '%s'", token->lexeme);
  }
  fprintf(stderr, ": %s\n", msg);
}

static void error_at_current(Parser *p, const char *msg) {
  error_at(p, &p->current, msg);
}

static void advance(Parser *p) {
  free_token_lexeme(p->allocator, p->previous);
  p->previous = p->current;

  for (;;) {
    p->current = scan_token(p->lexer);
    if (p->current.type != TOKEN_ERROR)
      break;
    error_at_current(p, p->current.lexeme ? p->current.lexeme : "lex error");
    free_token_lexeme(p->allocator, p->current);
  }
}

static bool check(Parser *p, TokenType type) { return p->current.type == type; }

static bool match(Parser *p, TokenType type) {
  if (!check(p, type))
    return false;
  advance(p);
  return true;
}

static void consume(Parser *p, TokenType type, const char *msg) {
  if (check(p, type)) {
    advance(p);
    return;
  }
  error_at_current(p, msg);
}

// After an error, discard tokens until the next likely statement/decl boundary
// so parsing can resume without an avalanche of errors
static void synchronize(Parser *p) {
  p->panic_mode = false;

  while (!check(p, TOKEN_EOF)) {
    if (p->previous.type == TOKEN_SEMICOLON)
      return;
    switch (p->current.type) {
    case TOKEN_FN:
    case TOKEN_TYPE:
    case TOKEN_LET:
    case TOKEN_CONST:
    case TOKEN_IMPORT:
    case TOKEN_PUB:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_DO:
    case TOKEN_FOR:
    case TOKEN_RETURN:
    case TOKEN_BREAK:
    case TOKEN_CONTINUE:
      return;
    default:
      advance(p);
    }
  }
}

static Node *make(Parser *p, NodeKind kind) {
  return new_node(p->allocator, kind, p->previous.line);
}

static char *prev_text(Parser *p) {
  return ast_copy_str(p->allocator, p->previous.lexeme);
}

static bool is_assign_op(TokenType t) {
  switch (t) {
  case TOKEN_EQUAL:
  case TOKEN_PLUS_EQUAL:
  case TOKEN_MINUS_EQUAL:
  case TOKEN_MUL_EQUAL:
  case TOKEN_DIV_EQUAL:
  case TOKEN_REM_EQUAL:
    return true;
  default:
    return false;
  }
}

// 0 means "not a binary operator"
static int get_precedence(TokenType t) {
  switch (t) {
  case TOKEN_EQUAL_EQUAL:
  case TOKEN_BANG_EQUAL:
    return 1;
  case TOKEN_LESS:
  case TOKEN_LESS_EQUAL:
  case TOKEN_GREATER:
  case TOKEN_GREATER_EQUAL:
    return 2;
  case TOKEN_PLUS:
  case TOKEN_MINUS:
    return 3;
  case TOKEN_MUL:
  case TOKEN_DIV:
  case TOKEN_REM:
    return 4;
  default:
    return 0;
  }
}

static Node *parse_declaration(Parser *p) {
  bool is_pub = match(p, TOKEN_PUB);

  if (match(p, TOKEN_FN))
    return parse_fn(p, is_pub);
  if (match(p, TOKEN_TYPE))
    return parse_type_decl(p, is_pub);

  if (is_pub) {
    error_at_current(p, "Exected 'fn' or 'type' after 'pub'.");
    return NULL;
  }

  if (match(p, TOKEN_IMPORT))
    return parse_import(p);
  if (match(p, TOKEN_LET))
    return parse_let_stmt(p, false);
  if (match(p, TOKEN_CONST))
    return parse_let_stmt(p, false);

  error_at_current(p, "Expected a declaration.");
  return NULL;
}

static Node *parse_import(Parser *p) {
  Node *node = make(p, NODE_IMPORT);
  if (!node)
    return NULL;

  consume(p, TOKEN_IDENTIFIER, "Expected module name after 'import'.");
  node->as.import.path = prev_text(p);
  consume(p, TOKEN_SEMICOLON, "Expected ';' after import.");

  return node;
}

static Node *parse_fn(Parser *p, bool is_pub) {
  Node *node = make(p, NODE_FN);
  if (!node)
    return NULL;

  consume(p, TOKEN_IDENTIFIER, "Expected function name.");
  node->as.fn.is_pub = is_pub;
  node->as.fn.name = prev_text(p);

  consume(p, TOKEN_LEFT_PAREN, "Expected '(' after function name.");
  if (!check(p, TOKEN_RIGHT_PAREN)) {
    do {
      Node *param = make(p, NODE_PARAM);
      if (!param)
        return NULL;

      consume(p, TOKEN_IDENTIFIER, "Expected parameter name.");
      param->line = p->previous.line;
      param->as.param.name = prev_text(p);
      consume(
          p, TOKEN_COLON,
          "Expected ':' after parameter name; parameters must have a type.");
      param->as.param.type = parse_type(p);

      node_list_push(p->allocator, &node->as.fn.params, param);
    } while (match(p, TOKEN_COMMA));
  }
  consume(p, TOKEN_RIGHT_PAREN, "Expected ')' after parameters.");

  // optional return type
  if (!check(p, TOKEN_LEFT_BRACE)) {
    Node *ret_type = parse_type(p);
    node->as.fn.ret_type = ret_type;
  }

  Node *body = parse_block(p);
  node->as.fn.body = body;

  return node;
}

static Node *parse_type_decl(Parser *p, bool is_pub) {
  Node *node = make(p, NODE_TYPE_DECL);
  if (!node)
    return NULL;

  consume(p, TOKEN_IDENTIFIER, "Expected type name.");
  node->as.type_decl.is_pub = is_pub;
  node->as.type_decl.name = prev_text(p);
  consume(p, TOKEN_EQUAL, "Expected '=' in type declaration.");

  Node *def;
  if (match(p, TOKEN_STRUCT)) {
    def = parse_record(p, NODE_STRUCT);
  } else if (match(p, TOKEN_UNION)) {
    def = parse_record(p, NODE_UNION);
  } else if (match(p, TOKEN_ENUM)) {
    def = parse_enum(p);
  } else {
    def = parse_type(p);
    consume(p, TOKEN_SEMICOLON, "Expected ';' after type declaration.");
  }

  node->as.type_decl.def = def;
  return node;
}

// struct/union share a field list
static Node *parse_record(Parser *p, NodeKind kind) {
  Node *node = make(p, NODE_STRUCT);
  if (!node)
    return NULL;

  node->kind = kind;

  consume(p, TOKEN_LEFT_BRACE, "Expected '{' to begin fields.");
  while (!check(p, TOKEN_RIGHT_BRACE) && !check(p, TOKEN_EOF)) {
    Node *field = make(p, NODE_FIELD);
    if (!field)
      return NULL;

    consume(p, TOKEN_IDENTIFIER, "Expected field name.");
    field->line = p->previous.line;
    field->as.field.name = prev_text(p);
    consume(p, TOKEN_COLON, "Expected ':' after field name");
    Node *ftype = parse_type(p);
    field->as.field.type = ftype;

    node_list_push(p->allocator, &node->as.record.fields, field);

    if (!match(p, TOKEN_COMMA))
      break;
  }
  consume(p, TOKEN_RIGHT_BRACE, "Expected '}' after fields.");
  return node;
}

static Node *parse_enum(Parser *p) {
  Node *node = make(p, NODE_ENUM);
  if (!node)
    return NULL;

  consume(p, TOKEN_LEFT_BRACE, "Expected '{' to begin variants.");
  while (!check(p, TOKEN_RIGHT_BRACE) && !check(p, TOKEN_EOF)) {
    Node *variant = make(p, NODE_ENUM_VAR);
    if (!variant)
      return NULL;

    consume(p, TOKEN_IDENTIFIER, "Expected variant name.");
    variant->line = p->previous.line;
    variant->as.enum_variant.name = prev_text(p);
    if (match(p, TOKEN_EQUAL))
      variant->as.enum_variant.val = parse_expr(p);

    node_list_push(p->allocator, &node->as.enom.variants, variant);

    if (!match(p, TOKEN_COMMA))
      break;
  }

  consume(p, TOKEN_RIGHT_BRACE, "Expected '}' after variants.");
  return node;
}

static Node *parse_statement(Parser *p) {
  if (match(p, TOKEN_LET))
    return parse_let_stmt(p, false);
  if (match(p, TOKEN_CONST))
    return parse_let_stmt(p, true);
  if (match(p, TOKEN_IF))
    return parse_if(p);
  if (match(p, TOKEN_WHILE))
    return parse_while(p);
  if (match(p, TOKEN_DO))
    return parse_do_while(p);
  if (match(p, TOKEN_FOR))
    return parse_for(p);
  if (match(p, TOKEN_RETURN))
    return parse_return(p);
  if (check(p, TOKEN_LEFT_BRACE))
    return parse_block(p);

  if (match(p, TOKEN_BREAK)) {
    Node *node = make(p, NODE_BREAK);
    consume(p, TOKEN_SEMICOLON, "Expected ';' after 'break'.");
    return node;
  }
  if (match(p, TOKEN_CONTINUE)) {
    Node *node = make(p, NODE_CONTINUE);
    consume(p, TOKEN_SEMICOLON, "Expected ';' after 'continue'.");
    return node;
  }

  Node *node = make(p, NODE_EXPR_STMT);
  if (!node)
    return NULL;
  Node *expr = parse_expr(p);
  node->as.expr_stmt.expr = expr;
  consume(p, TOKEN_SEMICOLON, "Expected ';' after expression.");

  return node;
}

static Node *parse_var_decl(Parser *p, bool is_const) {
  Node *node = make(p, NODE_LET);
  if (!node)
    return NULL;

  consume(p, TOKEN_IDENTIFIER, "Expected variable name.");
  node->as.let.is_const = is_const;
  node->as.let.name = prev_text(p);
  if (match(p, TOKEN_COLON))
    node->as.let.type = parse_type(p);
  if (match(p, TOKEN_EQUAL))
    node->as.let.init = parse_expr(p);

  return node;
}

static Node *parse_let_stmt(Parser *p, bool is_const) {
  Node *node = parse_var_decl(p, is_const);
  consume(p, TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
  return node;
}

static Node *parse_if(Parser *p) {
  Node *node = make(p, NODE_IF);
  if (!node)
    return NULL;

  Node *cond = parse_expr(p);
  Node *then_branch = parse_block(p);

  Node *else_branch = NULL;
  if (match(p, TOKEN_ELSE)) {
    if (match(p, TOKEN_IF))
      else_branch = parse_if(p);
    else
      else_branch = parse_block(p);
  }

  node->as.if_stmt.cond = cond;
  node->as.if_stmt.then_branch = then_branch;
  node->as.if_stmt.else_branch = else_branch;

  return node;
}

static Node *parse_while(Parser *p) {
  Node *node = make(p, NODE_WHILE);
  if (!node)
    return NULL;

  Node *cond = parse_expr(p);
  Node *body = parse_block(p);
  node->as.while_stmt.cond = cond;
  node->as.while_stmt.body = body;

  return node;
}

static Node *parse_do_while(Parser *p) {
  Node *node = make(p, NODE_DO_WHILE);
  if (!node)
    return NULL;

  Node *body = parse_block(p);
  consume(p, TOKEN_WHILE, "Expected 'while' after do-block.");
  Node *cond = parse_expr(p);
  consume(p, TOKEN_SEMICOLON, "Exected ';' after do-while condition.");
  node->as.do_while.body = body;
  node->as.do_while.cond = cond;

  return node;
}

// for { ... }                infinite
// for cond { ... }           while style
// for init; cond; post {...} C-style three-clause
static Node *parse_for(Parser *p) {
  Node *node = make(p, NODE_FOR);
  if (!node)
    return NULL;

  Node *init = NULL, *cond = NULL, *post = NULL;

  if (!check(p, TOKEN_LEFT_BRACE)) {
    if (match(p, TOKEN_LET)) {
      init = parse_var_decl(p, false);
    } else if (match(p, TOKEN_CONST)) {
      init = parse_var_decl(p, true);
    } else {
      init = parse_expr(p);
    }

    if (match(p, TOKEN_SEMICOLON)) {
      if (!check(p, TOKEN_SEMICOLON))
        cond = parse_expr(p);
      consume(p, TOKEN_SEMICOLON, "Expected ';' after loop condition");
      if (!check(p, TOKEN_LEFT_BRACE))
        post = parse_expr(p);
    } else {
      // while style
      cond = init;
      init = NULL;
    }
  }

  Node *body = parse_block(p);
  node->as.for_stmt.init = init;
  node->as.for_stmt.cond = cond;
  node->as.for_stmt.post = post;
  node->as.for_stmt.body = body;

  return node;
}

static Node *parse_return(Parser *p) {
  Node *node = make(p, NODE_RETURN);
  if (!node)
    return NULL;

  if (!check(p, TOKEN_SEMICOLON)) {
    node->as.ret.val = parse_expr(p);
  }

  consume(p, TOKEN_SEMICOLON, "Expected ';' after return.");
  return node;
}

static Node *parse_block(Parser *p) {
  Node *node = make(p, NODE_BLOCK);
  if (!node)
    return NULL;

  consume(p, TOKEN_LEFT_BRACE, "Expected '{'");
  while (!check(p, TOKEN_RIGHT_BRACE) && !check(p, TOKEN_EOF)) {
    Node *stmt = parse_statement(p);
    node_list_push(p->allocator, &node->as.block.stmts, stmt);

    if (p->panic_mode)
      synchronize(p);
  }

  consume(p, TOKEN_RIGHT_BRACE, "Exected '}' after block.");
  return node;
}

static Node *parse_type(Parser *p) {
  if (match(p, TOKEN_CARET)) {
    Node *node = make(p, NODE_TYPE_PTR);
    if (!node)
      return NULL;

    Node *pointee = parse_type(p);
    node->as.pointer.pointee = pointee;
    return node;
  }

  if (match(p, TOKEN_LEFT_BRACKET)) {
    Node *node = make(p, NODE_TYPE_ARR);
    if (!node)
      return NULL;

    Node *size = NULL;
    if (!check(p, TOKEN_RIGHT_BRACKET))
      size = parse_expr(p); // sized [N]T; empty brackets -> slice []T
    consume(p, TOKEN_RIGHT_BRACKET, "Expected ']' in array type.");
    Node *elem = parse_type(p);
    if (!elem)
      return NULL;

    node->as.array.size = size;
    node->as.array.elem = elem;

    return node;
  }

  Node *node = make(p, NODE_TYPE_NAME);
  consume(p, TOKEN_IDENTIFIER, "Expected a type name.");
  node->as.type_name.name = prev_text(p);
  return node;
}

static Node *parse_expr(Parser *p) { return parse_assignment(p); }

static Node *parse_assignment(Parser *p) {
  Node *target = parse_binary(p, 1);
  if (!target)
    return NULL;

  if (is_assign_op(p->current.type)) {
    TokenType op = p->current.type;
    advance(p);

    Node *node = make(p, NODE_ASSIGN);
    if (!node)
      return NULL;

    Node *val = parse_assignment(p); // right associative
    node->as.assign.op = op;
    node->as.assign.target = target;
    node->as.assign.val = val;
    return node;
  }

  return target;
}

static Node *parse_binary(Parser *p, int min_prec) {
  Node *left = parse_unary(p);
  if (!left)
    return NULL;

  for (;;) {
    int prec = get_precedence(p->current.type);
    if (prec == 0 || prec < min_prec)
      break;

    TokenType op = p->current.type;
    advance(p);

    Node *node = make(p, NODE_BINARY);
    if (!node)
      return NULL;

    Node *right = parse_binary(p, prec + 1);
    node->as.binary.op = op;
    node->as.binary.left = left;
    node->as.binary.right = right;
    left = node;
  }

  return left;
}

static Node *parse_unary(Parser *p) {
  if (check(p, TOKEN_BANG) || check(p, TOKEN_MINUS)) {
    TokenType op = p->current.type;
    advance(p);

    Node *node = make(p, NODE_UNARY);
    if (!node)
      return NULL;

    Node *operand = parse_unary(p);
    node->as.unary.op = op;
    node->as.unary.operand = operand;

    return node;
  }

  if (match(p, TOKEN_HEAP)) {
    Node *node = make(p, NODE_HEAP);
    if (!node)
      return NULL;

    Node *val = parse_unary(p);
    node->as.heap.val = val;
    return node;
  }

  return parse_postfix(p);
}

static Node *parse_postfix(Parser *p) {
  Node *expr = parse_primary(p);
  if (!expr)
    return NULL;

  for (;;) {
    if (match(p, TOKEN_LEFT_PAREN)) {
      Node *node = make(p, NODE_CALL);
      if (!node)
        return NULL;

      node->as.call.callee = expr;
      if (!check(p, TOKEN_RIGHT_PAREN)) {
        do {
          Node *arg = parse_expr(p);
          if (!arg)
            return NULL;
          node_list_push(p->allocator, &node->as.call.args, arg);
        } while (match(p, TOKEN_COMMA));
      }

      consume(p, TOKEN_RIGHT_PAREN, "Expected ')' after arguments");
      expr = node;
    } else if (match(p, TOKEN_DOT)) {
      Node *node = make(p, NODE_MEMBER);
      if (!node)
        return NULL;

      consume(p, TOKEN_IDENTIFIER, "Expected property name after '.'.");
      node->as.member.obj = expr;
      node->as.member.field = prev_text(p);
      expr = node;
    } else if (match(p, TOKEN_LEFT_BRACKET)) {
      Node *node = make(p, NODE_INDEX);
      if (!node)
        return NULL;

      Node *idx = parse_expr(p);
      consume(p, TOKEN_RIGHT_BRACKET, "Expected ']' after index.");
      node->as.subscript.obj = expr;
      node->as.subscript.idx = idx;
      expr = node;
    } else if (check(p, TOKEN_PLUS_PLUS) || check(p, TOKEN_MINUS_MINUS)) {
      TokenType op = p->current.type;
      advance(p);
      Node *node = make(p, NODE_POSTFIX);
      if (!node)
        return NULL;

      node->as.unary.op = op;
      node->as.unary.operand = expr;
      expr = node;
    } else {
      break;
    }
  }

  return expr;
}

static Node *parse_primary(Parser *p) {
  if (match(p, TOKEN_INTEGER)) {
    Node *node = make(p, NODE_INT_LIT);
    if (!node)
      return NULL;
    node->as.literal.text = prev_text(p);
    return node;
  }
  if (match(p, TOKEN_FLOAT)) {
    Node *node = make(p, NODE_FLOAT_LIT);
    if (!node)
      return NULL;
    node->as.literal.text = prev_text(p);
    return node;
  }
  if (match(p, TOKEN_STRING)) {
    Node *node = make(p, NODE_STRING_LIT);
    if (!node)
      return NULL;
    node->as.literal.text = prev_text(p);
    return node;
  }
  if (match(p, TOKEN_TRUE) || match(p, TOKEN_FALSE)) {
    Node *node = make(p, NODE_BOOL_LIT);
    if (!node)
      return NULL;
    node->as.boolean.val = (p->previous.type == TOKEN_TRUE);
    return node;
  }
  if (match(p, TOKEN_NULL)) {
    return make(p, NODE_NULL_LIT);
  }
  if (match(p, TOKEN_IDENTIFIER)) {
    Node *node = make(p, NODE_IDENT);
    if (!node)
      return NULL;
    node->as.ident.name = prev_text(p);
    return node;
  }
  if (match(p, TOKEN_LEFT_PAREN)) {
    Node *expr = parse_expr(p);
    consume(p, TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
    return expr;
  }

  error_at_current(p, "Expected an expression.");
  return NULL;
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void init_parser(Parser *parser, Lexer *lexer, Allocator *allocator) {
  parser->lexer = lexer;
  parser->allocator = allocator;
  parser->had_error = false;
  parser->panic_mode = false;
  // Zero the tokens so the first advance() can safely "free" previous
  parser->current.type = TOKEN_EOF;
  parser->current.lexeme = NULL;
  parser->current.line = 0;
  parser->previous = parser->current;
  advance(parser);
}

Node *parse_program(Parser *parser) {
  Node *program = new_node(parser->allocator, NODE_PROGRAM, 1);
  if (!program)
    return NULL;

  while (!check(parser, TOKEN_EOF)) {
    Node *decl = parse_declaration(parser);
    node_list_push(parser->allocator, &program->as.program.decls, decl);
    if (parser->panic_mode)
      synchronize(parser);
  }

  return program;
}

void free_parser(Parser *parser) {
  free_token_lexeme(parser->allocator, parser->current);
  free_token_lexeme(parser->allocator, parser->previous);
  parser->current.lexeme = NULL;
  parser->previous.lexeme = NULL;
}
