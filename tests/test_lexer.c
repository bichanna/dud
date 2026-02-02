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

#include "src/lexer.h"
#include "test.h"

#include <string.h>

static bool scan_single(const char *src, TokenType expected_type,
                        const char *expected_lexeme) {
  Lexer lexer;
  init_lexer(&lexer, src, &raw_allocator);

  Token tok = scan_token(&lexer);

  ASSERT_EQ(tok.type, expected_type);
  if (expected_lexeme != NULL) {
    ASSERT_NOT_NULL(tok.lexeme);
    ASSERT((strcmp(tok.lexeme, expected_lexeme) == 0),
           "lexeme should match expected value");
  }
  free_token_lexeme(&raw_allocator, tok);

  Token eof = scan_token(&lexer);
  ASSERT_EQ(eof.type, TOKEN_EOF);

  return true;
}

/* --------------------------------------------------------------------------
 * Empty / whitespace
 * -------------------------------------------------------------------------- */

TEST(empty_input) { return scan_single("", TOKEN_EOF, NULL); }
TEST(only_whitespace) { return scan_single("   \t\r\n  ", TOKEN_EOF, NULL); }

/* --------------------------------------------------------------------------
 * Single-character tokens
 * -------------------------------------------------------------------------- */

TEST(single_char_left_paren) {
  return scan_single("(", TOKEN_LEFT_PAREN, NULL);
}
TEST(single_char_right_paren) {
  return scan_single(")", TOKEN_RIGHT_PAREN, NULL);
}
TEST(single_char_left_brace) {
  return scan_single("{", TOKEN_LEFT_BRACE, NULL);
}
TEST(single_char_right_brace) {
  return scan_single("}", TOKEN_RIGHT_BRACE, NULL);
}
TEST(single_char_left_bracket) {
  return scan_single("[", TOKEN_LEFT_BRACKET, NULL);
}
TEST(single_char_right_bracket) {
  return scan_single("]", TOKEN_RIGHT_BRACKET, NULL);
}
TEST(single_char_semicolon) { return scan_single(";", TOKEN_SEMICOLON, NULL); }
TEST(single_char_comma) { return scan_single(",", TOKEN_COMMA, NULL); }
TEST(single_char_dot) { return scan_single(".", TOKEN_DOT, NULL); }
TEST(single_char_colon) { return scan_single(":", TOKEN_COLON, NULL); }
TEST(single_char_caret) { return scan_single("^", TOKEN_CARET, NULL); }
TEST(single_char_bang) { return scan_single("!", TOKEN_BANG, NULL); }
TEST(single_char_equal) { return scan_single("=", TOKEN_EQUAL, NULL); }
TEST(single_char_greater) { return scan_single(">", TOKEN_GREATER, NULL); }
TEST(single_char_less) { return scan_single("<", TOKEN_LESS, NULL); }

/* --------------------------------------------------------------------------
 * Two-character tokens
 * -------------------------------------------------------------------------- */

TEST(two_char_bang_equal) { return scan_single("!=", TOKEN_BANG_EQUAL, NULL); }
TEST(two_char_equal_equal) {
  return scan_single("==", TOKEN_EQUAL_EQUAL, NULL);
}
TEST(two_char_greater_equal) {
  return scan_single(">=", TOKEN_GREATER_EQUAL, NULL);
}
TEST(two_char_less_equal) { return scan_single("<=", TOKEN_LESS_EQUAL, NULL); }

/* --------------------------------------------------------------------------
 * Identifiers
 * -------------------------------------------------------------------------- */

TEST(identifier_simple) { return scan_single("foo", TOKEN_IDENTIFIER, "foo"); }

TEST(identifier_with_underscore_prefix) {
  return scan_single("_bar", TOKEN_IDENTIFIER, "_bar");
}

TEST(identifier_with_digits) {
  return scan_single("x123", TOKEN_IDENTIFIER, "x123");
}

TEST(identifier_mixed) {
  return scan_single("my_var_2", TOKEN_IDENTIFIER, "my_var_2");
}

TEST(identifier_prefix_of_keyword_let) {
  return scan_single("letter", TOKEN_IDENTIFIER, "letter");
}

TEST(identifier_prefix_of_keyword_if) {
  return scan_single("iffy", TOKEN_IDENTIFIER, "iffy");
}

TEST(identifier_prefix_of_keyword_for) {
  return scan_single("fork", TOKEN_IDENTIFIER, "fork");
}

TEST(identifier_prefix_of_keyword_const) {
  return scan_single("constant", TOKEN_IDENTIFIER, "constant");
}

TEST(identifier_prefix_of_keyword_continue) {
  return scan_single("continued", TOKEN_IDENTIFIER, "continued");
}

TEST(identifier_prefix_of_keyword_while) {
  return scan_single("whileloop", TOKEN_IDENTIFIER, "whileloop");
}

/* --------------------------------------------------------------------------
 * Keywords
 * -------------------------------------------------------------------------- */

TEST(keyword_import) { return scan_single("import", TOKEN_IMPORT, NULL); }
TEST(keyword_let) { return scan_single("let", TOKEN_LET, NULL); }
TEST(keyword_const) { return scan_single("const", TOKEN_CONST, NULL); }
TEST(keyword_fn) { return scan_single("fn", TOKEN_FN, NULL); }
TEST(keyword_if) { return scan_single("if", TOKEN_IF, NULL); }
TEST(keyword_else) { return scan_single("else", TOKEN_ELSE, NULL); }
TEST(keyword_true) { return scan_single("true", TOKEN_TRUE, NULL); }
TEST(keyword_false) { return scan_single("false", TOKEN_FALSE, NULL); }
TEST(keyword_for) { return scan_single("for", TOKEN_FOR, NULL); }
TEST(keyword_while) { return scan_single("while", TOKEN_WHILE, NULL); }
TEST(keyword_do) { return scan_single("do", TOKEN_DO, NULL); }
TEST(keyword_continue) { return scan_single("continue", TOKEN_CONTINUE, NULL); }
TEST(keyword_break) { return scan_single("break", TOKEN_BREAK, NULL); }
TEST(keyword_heap) { return scan_single("heap", TOKEN_HEAP, NULL); }
TEST(keyword_struct) { return scan_single("struct", TOKEN_STRUCT, NULL); }
TEST(keyword_union) { return scan_single("union", TOKEN_UNION, NULL); }
TEST(keyword_enum) { return scan_single("enum", TOKEN_ENUM, NULL); }
TEST(keyword_type) { return scan_single("type", TOKEN_TYPE, NULL); }
TEST(keyword_null) { return scan_single("null", TOKEN_NULL, NULL); }
TEST(keyword_pub) { return scan_single("pub", TOKEN_PUB, NULL); }
TEST(keyword_return) { return scan_single("return", TOKEN_RETURN, NULL); }

/* --------------------------------------------------------------------------
 * Numbers — integers
 * -------------------------------------------------------------------------- */

TEST(integer_single_digit) { return scan_single("0", TOKEN_INTEGER, "0"); }

TEST(integer_multi_digit) { return scan_single("42", TOKEN_INTEGER, "42"); }

TEST(integer_large) { return scan_single("1000000", TOKEN_INTEGER, "1000000"); }

/* --------------------------------------------------------------------------
 * Numbers — floats
 * -------------------------------------------------------------------------- */

TEST(float_simple) { return scan_single("3.14", TOKEN_FLOAT, "3.14"); }

TEST(float_leading_zero) { return scan_single("0.5", TOKEN_FLOAT, "0.5"); }

TEST(float_long_fractional) {
  return scan_single("1.23456", TOKEN_FLOAT, "1.23456");
}

/* --------------------------------------------------------------------------
 * Numbers — error cases
 * -------------------------------------------------------------------------- */

TEST(number_trailing_dot_is_error) {
  Lexer lexer;
  init_lexer(&lexer, "1.", &raw_allocator);

  Token tok = scan_token(&lexer);
  ASSERT_EQ(tok.type, TOKEN_ERROR);
  ASSERT_NOT_NULL(tok.lexeme);
  free_token_lexeme(&raw_allocator, tok);

  return true;
}

/* --------------------------------------------------------------------------
 * Strings
 * -------------------------------------------------------------------------- */

TEST(string_simple) { return scan_single("\"hello\"", TOKEN_STRING, "hello"); }

TEST(string_empty) { return scan_single("\"\"", TOKEN_STRING, ""); }

TEST(string_with_spaces) {
  return scan_single("\"hello world\"", TOKEN_STRING, "hello world");
}

TEST(string_unterminated) {
  Lexer lexer;
  init_lexer(&lexer, "\"no end", &raw_allocator);

  Token tok = scan_token(&lexer);
  ASSERT_EQ(tok.type, TOKEN_ERROR);
  ASSERT_NOT_NULL(tok.lexeme);
  ASSERT((strcmp(tok.lexeme, "Unterminated string.") == 0),
         "error message should be 'Unterminated string.'");
  free_token_lexeme(&raw_allocator, tok);

  return true;
}

/* --------------------------------------------------------------------------
 * Line tracking
 * -------------------------------------------------------------------------- */

TEST(line_starts_at_one) {
  Lexer lexer;
  init_lexer(&lexer, "x", &raw_allocator);

  Token tok = scan_token(&lexer);
  ASSERT_EQ(tok.line, 1);
  free_token_lexeme(&raw_allocator, tok);

  return true;
}

TEST(line_increments_on_newline) {
  Lexer lexer;
  init_lexer(&lexer, "a\nb", &raw_allocator);

  Token t1 = scan_token(&lexer);
  ASSERT_EQ(t1.line, 1);
  free_token_lexeme(&raw_allocator, t1);

  Token t2 = scan_token(&lexer);
  ASSERT_EQ(t2.line, 2);
  free_token_lexeme(&raw_allocator, t2);

  return true;
}

TEST(line_multiple_newlines) {
  Lexer lexer;
  init_lexer(&lexer, "a\n\n\nb", &raw_allocator);

  Token t1 = scan_token(&lexer);
  ASSERT_EQ(t1.line, 1);
  free_token_lexeme(&raw_allocator, t1);

  Token t2 = scan_token(&lexer);
  ASSERT_EQ(t2.line, 4);
  free_token_lexeme(&raw_allocator, t2);

  return true;
}

TEST(line_multiline_string) {
  /* A string spanning two lines; the token after it should be on line 3. */
  Lexer lexer;
  init_lexer(&lexer, "\"line1\nline2\"\nx", &raw_allocator);

  Token str = scan_token(&lexer);
  ASSERT_EQ(str.type, TOKEN_STRING);
  free_token_lexeme(&raw_allocator, str);

  Token id = scan_token(&lexer);
  ASSERT_EQ(id.type, TOKEN_IDENTIFIER);
  ASSERT_EQ(id.line, 3);
  free_token_lexeme(&raw_allocator, id);

  return true;
}

/* --------------------------------------------------------------------------
 * Comments
 * -------------------------------------------------------------------------- */

TEST(comment_skipped) {
  /* Everything after // until newline is ignored */
  return scan_single("// this is a comment\nx", TOKEN_IDENTIFIER, "x");
}

TEST(comment_at_end_of_input) {
  /* A trailing comment with no newline; only EOF should follow */
  Lexer lexer;
  init_lexer(&lexer, "// comment", &raw_allocator);

  Token tok = scan_token(&lexer);
  ASSERT_EQ(tok.type, TOKEN_EOF);

  return true;
}

TEST(comment_between_tokens) {
  Lexer lexer;
  init_lexer(&lexer, "a // comment\nb", &raw_allocator);

  Token t1 = scan_token(&lexer);
  ASSERT_EQ(t1.type, TOKEN_IDENTIFIER);
  free_token_lexeme(&raw_allocator, t1);

  Token t2 = scan_token(&lexer);
  ASSERT_EQ(t2.type, TOKEN_IDENTIFIER);
  ASSERT_NOT_NULL(t2.lexeme);
  ASSERT((strcmp(t2.lexeme, "b") == 0), "second identifier should be 'b'");
  free_token_lexeme(&raw_allocator, t2);

  return true;
}

/* --------------------------------------------------------------------------
 * Unexpected / error characters
 * -------------------------------------------------------------------------- */

TEST(unexpected_character) {
  Lexer lexer;
  init_lexer(&lexer, "@", &raw_allocator);

  Token tok = scan_token(&lexer);
  ASSERT_EQ(tok.type, TOKEN_ERROR);
  ASSERT_NOT_NULL(tok.lexeme);
  free_token_lexeme(&raw_allocator, tok);

  return true;
}

TEST(unexpected_character_tilde) {
  Lexer lexer;
  init_lexer(&lexer, "~", &raw_allocator);

  Token tok = scan_token(&lexer);
  ASSERT_EQ(tok.type, TOKEN_ERROR);
  ASSERT_NOT_NULL(tok.lexeme);
  free_token_lexeme(&raw_allocator, tok);

  return true;
}

/* --------------------------------------------------------------------------
 * token_type_to_string
 * -------------------------------------------------------------------------- */

TEST(token_type_to_string_samples) {
  ASSERT((strcmp(token_type_to_string(TOKEN_EOF), "TOKEN_EOF") == 0),
         "TOKEN_EOF string");
  ASSERT(
      (strcmp(token_type_to_string(TOKEN_LEFT_PAREN), "TOKEN_LEFT_PAREN") == 0),
      "TOKEN_LEFT_PAREN string");
  ASSERT(
      (strcmp(token_type_to_string(TOKEN_IDENTIFIER), "TOKEN_IDENTIFIER") == 0),
      "TOKEN_IDENTIFIER string");
  ASSERT((strcmp(token_type_to_string(TOKEN_RETURN), "TOKEN_RETURN") == 0),
         "TOKEN_RETURN string");
  ASSERT((strcmp(token_type_to_string(TOKEN_STRING), "TOKEN_STRING") == 0),
         "TOKEN_STRING string");

  return true;
}

/* --------------------------------------------------------------------------
 * Integration — multi-token sequences
 * -------------------------------------------------------------------------- */

TEST(integration_let_binding) {
  Lexer lexer;
  init_lexer(&lexer, "let x = 10;", &raw_allocator);

  Token t;

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_LET);
  free_token_lexeme(&raw_allocator, t);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_IDENTIFIER);
  ASSERT((strcmp(t.lexeme, "x") == 0), "identifier should be 'x'");
  free_token_lexeme(&raw_allocator, t);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_EQUAL);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_INTEGER);
  ASSERT((strcmp(t.lexeme, "10") == 0), "integer lexeme should be '10'");
  free_token_lexeme(&raw_allocator, t);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_SEMICOLON);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_EOF);

  return true;
}

TEST(integration_fn_signature) {
  Lexer lexer;
  init_lexer(&lexer, "fn add(a, b) { }", &raw_allocator);

  Token t;

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_FN);
  free_token_lexeme(&raw_allocator, t);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_IDENTIFIER);
  ASSERT((strcmp(t.lexeme, "add") == 0), "function name should be 'add'");
  free_token_lexeme(&raw_allocator, t);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_LEFT_PAREN);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_IDENTIFIER);
  ASSERT((strcmp(t.lexeme, "a") == 0), "first param should be 'a'");
  free_token_lexeme(&raw_allocator, t);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_COMMA);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_IDENTIFIER);
  ASSERT((strcmp(t.lexeme, "b") == 0), "second param should be 'b'");
  free_token_lexeme(&raw_allocator, t);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_RIGHT_PAREN);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_LEFT_BRACE);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_RIGHT_BRACE);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_EOF);

  return true;
}

TEST(integration_if_else) {
  Lexer lexer;
  init_lexer(&lexer, "if x == 0 { } else { }", &raw_allocator);

  Token t;

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_IF);
  free_token_lexeme(&raw_allocator, t);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_IDENTIFIER);
  free_token_lexeme(&raw_allocator, t);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_EQUAL_EQUAL);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_INTEGER);
  free_token_lexeme(&raw_allocator, t);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_LEFT_BRACE);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_RIGHT_BRACE);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_ELSE);
  free_token_lexeme(&raw_allocator, t);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_LEFT_BRACE);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_RIGHT_BRACE);

  t = scan_token(&lexer);
  ASSERT_EQ(t.type, TOKEN_EOF);

  return true;
}

/* --------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------- */

int main(void) {
  TEST_SUITE("Lexer — Empty / Whitespace");
  RUN_TEST(empty_input);
  RUN_TEST(only_whitespace);

  TEST_SUITE("Lexer — Single-Character Tokens");
  RUN_TEST(single_char_left_paren);
  RUN_TEST(single_char_right_paren);
  RUN_TEST(single_char_left_brace);
  RUN_TEST(single_char_right_brace);
  RUN_TEST(single_char_left_bracket);
  RUN_TEST(single_char_right_bracket);
  RUN_TEST(single_char_semicolon);
  RUN_TEST(single_char_comma);
  RUN_TEST(single_char_dot);
  RUN_TEST(single_char_colon);
  RUN_TEST(single_char_caret);
  RUN_TEST(single_char_bang);
  RUN_TEST(single_char_equal);
  RUN_TEST(single_char_greater);
  RUN_TEST(single_char_less);

  TEST_SUITE("Lexer — Two-Character Tokens");
  RUN_TEST(two_char_bang_equal);
  RUN_TEST(two_char_equal_equal);
  RUN_TEST(two_char_greater_equal);
  RUN_TEST(two_char_less_equal);

  TEST_SUITE("Lexer — Identifiers");
  RUN_TEST(identifier_simple);
  RUN_TEST(identifier_with_underscore_prefix);
  RUN_TEST(identifier_with_digits);
  RUN_TEST(identifier_mixed);
  RUN_TEST(identifier_prefix_of_keyword_let);
  RUN_TEST(identifier_prefix_of_keyword_if);
  RUN_TEST(identifier_prefix_of_keyword_for);
  RUN_TEST(identifier_prefix_of_keyword_const);
  RUN_TEST(identifier_prefix_of_keyword_continue);
  RUN_TEST(identifier_prefix_of_keyword_while);

  TEST_SUITE("Lexer — Keywords");
  RUN_TEST(keyword_import);
  RUN_TEST(keyword_let);
  RUN_TEST(keyword_const);
  RUN_TEST(keyword_fn);
  RUN_TEST(keyword_if);
  RUN_TEST(keyword_else);
  RUN_TEST(keyword_true);
  RUN_TEST(keyword_false);
  RUN_TEST(keyword_for);
  RUN_TEST(keyword_while);
  RUN_TEST(keyword_do);
  RUN_TEST(keyword_continue);
  RUN_TEST(keyword_break);
  RUN_TEST(keyword_heap);
  RUN_TEST(keyword_struct);
  RUN_TEST(keyword_union);
  RUN_TEST(keyword_enum);
  RUN_TEST(keyword_type);
  RUN_TEST(keyword_null);
  RUN_TEST(keyword_pub);
  RUN_TEST(keyword_return);

  TEST_SUITE("Lexer — Integers");
  RUN_TEST(integer_single_digit);
  RUN_TEST(integer_multi_digit);
  RUN_TEST(integer_large);

  TEST_SUITE("Lexer — Floats");
  RUN_TEST(float_simple);
  RUN_TEST(float_leading_zero);
  RUN_TEST(float_long_fractional);

  TEST_SUITE("Lexer — Number Errors");
  RUN_TEST(number_trailing_dot_is_error);

  TEST_SUITE("Lexer — Strings");
  RUN_TEST(string_simple);
  RUN_TEST(string_empty);
  RUN_TEST(string_with_spaces);
  RUN_TEST(string_unterminated);

  TEST_SUITE("Lexer — Line Tracking");
  RUN_TEST(line_starts_at_one);
  RUN_TEST(line_increments_on_newline);
  RUN_TEST(line_multiple_newlines);
  RUN_TEST(line_multiline_string);

  TEST_SUITE("Lexer — Comments");
  RUN_TEST(comment_skipped);
  RUN_TEST(comment_at_end_of_input);
  RUN_TEST(comment_between_tokens);

  TEST_SUITE("Lexer — Unexpected Characters");
  RUN_TEST(unexpected_character);
  RUN_TEST(unexpected_character_tilde);

  TEST_SUITE("Lexer — token_type_to_string");
  RUN_TEST(token_type_to_string_samples);

  TEST_SUITE("Lexer — Integration");
  RUN_TEST(integration_let_binding);
  RUN_TEST(integration_fn_signature);
  RUN_TEST(integration_if_else);

  TEST_SUMMARY();
  return TEST_EXIT_CODE();
}
