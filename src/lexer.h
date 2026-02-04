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

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "allocator.h"

typedef enum TokenType {
  // Single-character tokens
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,
  TOKEN_LEFT_BRACKET,
  TOKEN_RIGHT_BRACKET,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_SEMICOLON,
  TOKEN_COLON,
  TOKEN_CARET,

  // One or two character tokens
  TOKEN_BANG,
  TOKEN_BANG_EQUAL,
  TOKEN_EQUAL,
  TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER,
  TOKEN_GREATER_EQUAL,
  TOKEN_LESS,
  TOKEN_LESS_EQUAL,
  TOKEN_PLUS,
  TOKEN_PLUS_PLUS,
  TOKEN_PLUS_EQUAL,
  TOKEN_MINUS,
  TOKEN_MINUS_MINUS,
  TOKEN_MINUS_EQUAL,
  TOKEN_MUL,
  TOKEN_MUL_EQUAL,
  TOKEN_DIV,
  TOKEN_DIV_EQUAL,
  TOKEN_REM,
  TOKEN_REM_EQUAL,

  // Literals
  TOKEN_IDENTIFIER,
  TOKEN_STRING,
  TOKEN_INTEGER,
  TOKEN_FLOAT,

  // Keywords
  TOKEN_IMPORT,
  TOKEN_LET,
  TOKEN_CONST,
  TOKEN_FN,
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_FOR,
  TOKEN_WHILE,
  TOKEN_DO,
  TOKEN_RETURN,
  TOKEN_CONTINUE,
  TOKEN_BREAK,
  TOKEN_HEAP,
  TOKEN_STRUCT,
  TOKEN_UNION,
  TOKEN_ENUM,
  TOKEN_TYPE,
  TOKEN_NULL,
  TOKEN_PUB,

  TOKEN_ERROR,
  TOKEN_EOF
} TokenType;

typedef struct Token {
  TokenType type;
  size_t line;
  char *lexeme;
} Token;

typedef struct Lexer {
  const char *start;
  const char *current;
  size_t line;
  Allocator *allocator;
} Lexer;

void init_lexer(Lexer *lexer, const char *src, Allocator *allocator);

Token scan_token(Lexer *lexer);
void free_token_lexeme(Allocator *allocator, Token token);
const char *token_type_to_string(TokenType type);
