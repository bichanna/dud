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

#include "lexer.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static bool is_at_end(Lexer *lexer) { return *lexer->current == '\0'; }

static char advance(Lexer *lexer) {
  lexer->current++;
  return lexer->current[-1];
}

static char peek(Lexer *lexer) { return *lexer->current; }

static char peek_next(Lexer *lexer) {
  if (is_at_end(lexer))
    return '\0';
  return lexer->current[1];
}

static bool match(Lexer *lexer, char expected) {
  if (is_at_end(lexer))
    return false;
  if (*lexer->current != expected)
    return false;
  lexer->current++;
  return true;
}

static Token make_token(Lexer *lexer, TokenType type) {
  Token token;
  token.type = type;
  token.line = lexer->line;
  token.lexeme = NULL;

  if (type == TOKEN_IDENTIFIER || type == TOKEN_STRING || type == TOKEN_ERROR ||
      type == TOKEN_INTEGER || type == TOKEN_FLOAT) {
    size_t len = (size_t)(lexer->current - lexer->start);
    token.lexeme = (char *)ALLOC(lexer->allocator, len + 1, "TokenLexeme");
    if (token.lexeme) {
      memcpy(token.lexeme, lexer->start, len);
      token.lexeme[len] = '\0';
    } else {
      // TODO: Failed to allocate ;()
    }
  }

  return token;
}

static Token make_error_token(Lexer *lexer, const char *message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.line = lexer->line;

  size_t len = strlen(message);
  token.lexeme = (char *)ALLOC(lexer->allocator, len + 1, "TokenError");
  if (token.lexeme) {
    strcpy(token.lexeme, message);
  }

  return token;
}

static void skip_whitespace(Lexer *lexer) {
  for (;;) {
    char c = peek(lexer);
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance(lexer);
      break;
    case '\n':
      lexer->line++;
      advance(lexer);
      break;
    case '/':
      if (peek_next(lexer) == '/') {
        while (peek(lexer) != '\n' && !is_at_end(lexer))
          advance(lexer);
      } else {
        return;
      }
      break;
    default:
      return;
    }
  }
}

static Token make_string_token(Lexer *lexer) {
  while (peek(lexer) != '"' && !is_at_end(lexer)) {
    if (peek(lexer) == '\n')
      lexer->line++;
    advance(lexer);
  }

  if (is_at_end(lexer))
    return make_error_token(lexer, "Unterminated string.");

  lexer->start++;
  Token token = make_token(lexer, TOKEN_STRING);
  lexer->start--;

  advance(lexer);
  return token;
}

static Token make_number_token(Lexer *lexer) {
  bool is_float = false;

  while (isdigit(peek(lexer)))
    advance(lexer);

  if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
    is_float = true;
    advance(lexer);
    while (isdigit(peek(lexer)))
      advance(lexer);
  } else if (peek(lexer) == '.' && !isdigit(peek_next(lexer))) {
    return make_error_token(lexer, "Invalid number literal");
  }

  return make_token(lexer, is_float ? TOKEN_FLOAT : TOKEN_INTEGER);
}

static TokenType check_keyword(Lexer *lexer, size_t start, size_t len,
                               const char *rest, TokenType type) {
  size_t current_len = (size_t)(lexer->current - lexer->start);
  if (current_len == start + len &&
      memcmp(lexer->start + start, rest, len) == 0)
    return type;
  else
    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(Lexer *lexer) {
  switch (lexer->start[0]) {
  case 'i':
    if (lexer->current - lexer->start > 1) {
      switch (lexer->start[1]) {
      case 'm':
        return check_keyword(lexer, 2, 4, "port", TOKEN_IMPORT);
      case 'f':
        return check_keyword(lexer, 2, 0, "", TOKEN_IF);
      }
    }
    break;
  case 'l':
    return check_keyword(lexer, 1, 2, "et", TOKEN_LET);
  case 'c':
    if (lexer->current - lexer->start > 1) {
      switch (lexer->start[1]) {
      case 'o':
        if (lexer->current - lexer->start > 2 && lexer->start[2] == 'n') {
          if (lexer->current - lexer->start == 5) {
            return check_keyword(lexer, 3, 2, "st", TOKEN_CONST);
          }
          if (lexer->current - lexer->start == 8) {
            return check_keyword(lexer, 3, 5, "tinue", TOKEN_CONTINUE);
          }
        }
        break;
      }
    }
    break;
  case 'f':
    if (lexer->current - lexer->start > 1) {
      switch (lexer->start[1]) {
      case 'n':
        return check_keyword(lexer, 2, 0, "", TOKEN_FN);
      case 'a':
        return check_keyword(lexer, 2, 3, "lse", TOKEN_FALSE);
      case 'o':
        return check_keyword(lexer, 2, 1, "r", TOKEN_FOR);
      }
    }
    break;
  case 'e':
    if (lexer->current - lexer->start > 1) {
      switch (lexer->start[1]) {
      case 'l':
        return check_keyword(lexer, 2, 2, "se", TOKEN_ELSE);
      case 'n':
        return check_keyword(lexer, 2, 2, "um", TOKEN_ENUM);
      }
    }
    break;
  case 't':
    if (lexer->current - lexer->start > 1) {
      switch (lexer->start[1]) {
      case 'r':
        return check_keyword(lexer, 2, 2, "ue", TOKEN_TRUE);
      case 'y':
        return check_keyword(lexer, 2, 2, "pe", TOKEN_TYPE);
      }
    }
    break;
  case 'w':
    return check_keyword(lexer, 1, 4, "hile", TOKEN_WHILE);
  case 'd':
    return check_keyword(lexer, 1, 1, "o", TOKEN_DO);
  case 'r':
    return check_keyword(lexer, 1, 5, "eturn", TOKEN_RETURN);
  case 'b':
    return check_keyword(lexer, 1, 4, "reak", TOKEN_BREAK);
  case 'h':
    return check_keyword(lexer, 1, 3, "eap", TOKEN_HEAP);
  case 's':
    return check_keyword(lexer, 1, 5, "truct", TOKEN_STRUCT);
  case 'u':
    return check_keyword(lexer, 1, 4, "nion", TOKEN_UNION);
  case 'n':
    return check_keyword(lexer, 1, 3, "ull", TOKEN_NULL);
  case 'p':
    return check_keyword(lexer, 1, 2, "ub", TOKEN_PUB);
  }

  return TOKEN_IDENTIFIER;
}

static Token make_identifier_or_keyword(Lexer *lexer) {
  while (isalnum(peek(lexer)) || peek(lexer) == '_')
    advance(lexer);
  return make_token(lexer, identifier_type(lexer));
}

void init_lexer(Lexer *lexer, const char *src, Allocator *allocator) {
  lexer->start = src;
  lexer->current = src;
  lexer->line = 1;
  lexer->allocator = allocator;
}

void free_token_lexeme(Allocator *allocator, Token token) {
  if (token.lexeme != NULL) {
    size_t len = strlen(token.lexeme) + 1;
    FREE(allocator, token.lexeme, len, "TokenLexeme");
  }
}

Token scan_token(Lexer *lexer) {
  skip_whitespace(lexer);

  lexer->start = lexer->current;

  if (is_at_end(lexer))
    return make_token(lexer, TOKEN_EOF);

  char c = advance(lexer);
  if (isalpha(c) || c == '_')
    return make_identifier_or_keyword(lexer);
  if (isdigit(c))
    return make_number_token(lexer);

  switch (c) {
  case '(':
    return make_token(lexer, TOKEN_LEFT_PAREN);
  case ')':
    return make_token(lexer, TOKEN_RIGHT_PAREN);
  case '{':
    return make_token(lexer, TOKEN_LEFT_BRACE);
  case '}':
    return make_token(lexer, TOKEN_RIGHT_BRACE);
  case '[':
    return make_token(lexer, TOKEN_LEFT_BRACKET);
  case ']':
    return make_token(lexer, TOKEN_RIGHT_BRACKET);
  case ';':
    return make_token(lexer, TOKEN_SEMICOLON);
  case ',':
    return make_token(lexer, TOKEN_COMMA);
  case '.':
    return make_token(lexer, TOKEN_DOT);
  case ':':
    return make_token(lexer, TOKEN_COLON);
  case '^':
    return make_token(lexer, TOKEN_CARET);

  case '!':
    return make_token(lexer, match(lexer, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '=':
    return make_token(lexer,
                      match(lexer, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '>':
    return make_token(lexer,
                      match(lexer, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
  case '<':
    return make_token(lexer, match(lexer, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '"':
    return make_string_token(lexer);
  }

  char buf[30];
  snprintf(buf, sizeof(buf), "%s%c%s", "Unexpected character '", c, "'.");
  return make_error_token(lexer, buf);
}

const char *token_type_to_string(TokenType type) {
  switch (type) {
  case (TOKEN_LEFT_PAREN):
    return "TOKEN_LEFT_PAREN";
  case (TOKEN_RIGHT_PAREN):
    return "TOKEN_RIGHT_PAREN";
  case (TOKEN_LEFT_BRACE):
    return "TOKEN_LEFT_BRACE";
  case (TOKEN_RIGHT_BRACE):
    return "TOKEN_RIGHT_BRACE";
  case (TOKEN_LEFT_BRACKET):
    return "TOKEN_LEFT_BRACKET";
  case (TOKEN_RIGHT_BRACKET):
    return "TOKEN_RIGHT_BRACKET";
  case (TOKEN_COMMA):
    return "TOKEN_COMMA";
  case (TOKEN_DOT):
    return "TOKEN_DOT";
  case (TOKEN_SEMICOLON):
    return "TOKEN_SEMICOLON";
  case (TOKEN_COLON):
    return "TOKEN_COLON";
  case (TOKEN_CARET):
    return "TOKEN_CARET";
  case (TOKEN_BANG):
    return "TOKEN_BANG";
  case (TOKEN_BANG_EQUAL):
    return "TOKEN_BANG_EQUAL";
  case (TOKEN_EQUAL):
    return "TOKEN_EQUAL";
  case (TOKEN_EQUAL_EQUAL):
    return "TOKEN_EQUAL_EQUAL";
  case (TOKEN_GREATER):
    return "TOKEN_GREATER";
  case (TOKEN_GREATER_EQUAL):
    return "TOKEN_GREATER_EQUAL";
  case (TOKEN_LESS):
    return "TOKEN_LESS";
  case (TOKEN_LESS_EQUAL):
    return "TOKEN_LESS_EQUAL";
  case (TOKEN_IDENTIFIER):
    return "TOKEN_IDENTIFIER";
  case (TOKEN_STRING):
    return "TOKEN_STRING";
  case (TOKEN_INTEGER):
    return "TOKEN_INTEGER";
  case (TOKEN_FLOAT):
    return "TOKEN_FLOAT";
  case (TOKEN_IMPORT):
    return "TOKEN_IMPORT";
  case (TOKEN_LET):
    return "TOKEN_LET";
  case (TOKEN_CONST):
    return "TOKEN_CONST";
  case (TOKEN_FN):
    return "TOKEN_FN";
  case (TOKEN_IF):
    return "TOKEN_IF";
  case (TOKEN_ELSE):
    return "TOKEN_ELSE";
  case (TOKEN_TRUE):
    return "TOKEN_TRUE";
  case (TOKEN_FALSE):
    return "TOKEN_FALSE";
  case (TOKEN_FOR):
    return "TOKEN_FOR";
  case (TOKEN_WHILE):
    return "TOKEN_WHILE";
  case (TOKEN_DO):
    return "TOKEN_DO";
  case (TOKEN_RETURN):
    return "TOKEN_RETURN";
  case (TOKEN_CONTINUE):
    return "TOKEN_CONTINUE";
  case (TOKEN_BREAK):
    return "TOKEN_BREAK";
  case (TOKEN_HEAP):
    return "TOKEN_HEAP";
  case (TOKEN_STRUCT):
    return "TOKEN_STRUCT";
  case (TOKEN_UNION):
    return "TOKEN_UNION";
  case (TOKEN_ENUM):
    return "TOKEN_ENUM";
  case (TOKEN_TYPE):
    return "TOKEN_TYPE";
  case (TOKEN_NULL):
    return "TOKEN_NULL";
  case (TOKEN_PUB):
    return "TOKEN_PUB";
  case (TOKEN_ERROR):
    return "TOKEN_ERROR";
  case (TOKEN_EOF):
    return "TOKEN_EOF";
  default:
    return "TOKEN_UNKNOWN";
  }
}
