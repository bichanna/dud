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

#include "src/ast.h"
#include "src/lexer.h"
#include "src/parser.h"
#include "test.h"

#include <string.h>

static Node *parse_src(const char *src, Parser *out_parser) {
  static Lexer lexer;
  init_lexer(&lexer, src, &raw_allocator);
  init_parser(out_parser, &lexer, &raw_allocator);
  return parse_program(out_parser);
}

#define WITH_PARSE(src, prog_var, parser_var)                                  \
  Parser parser_var;                                                           \
  Node *prog_var = parse_src((src), &parser_var)

#define TEARDOWN(prog_var, parser_var)                                         \
  do {                                                                         \
    free_node(&raw_allocator, prog_var);                                       \
    free_parser(&parser_var);                                                  \
  } while (0)

TEST(fn_simple) {
  WITH_PARSE("fn main() {}", prog, p);
  ASSERT_NOT_NULL(prog);
  ASSERT_FALSE(p.had_error);
  ASSERT_EQ(prog->kind, NODE_PROGRAM);

  Node *fn = prog->as.program.decls.items[0];
  ASSERT_EQ(fn->kind, NODE_FN);
  ASSERT((strcmp(fn->as.fn.name, "main") == 0), "fn name should be 'main'");
  ASSERT_EQ(fn->as.fn.params.count, 0);
  ASSERT_NULL(fn->as.fn.ret_type);
  ASSERT_NOT_NULL(fn->as.fn.body);
  ASSERT_EQ(fn->as.fn.body->kind, NODE_BLOCK);

  TEARDOWN(prog, p);
  return true;
}

TEST(fn_params_and_return_type) {
  WITH_PARSE("pub fn add(a: i32, b: i32) i32 { return a + b; }", prog, p);
  ASSERT_FALSE(p.had_error);

  Node *fn = prog->as.program.decls.items[0];
  ASSERT_EQ(fn->kind, NODE_FN);
  ASSERT_TRUE(fn->as.fn.is_pub);
  ASSERT_EQ(fn->as.fn.params.count, 2);

  Node *a = fn->as.fn.params.items[0];
  ASSERT_EQ(a->kind, NODE_PARAM);
  ASSERT((strcmp(a->as.param.name, "a") == 0), "first param name 'a'");
  ASSERT_NOT_NULL(a->as.param.type);
  ASSERT_EQ(a->as.param.type->kind, NODE_TYPE_NAME);

  ASSERT_NOT_NULL(fn->as.fn.ret_type);
  ASSERT_EQ(fn->as.fn.ret_type->kind, NODE_TYPE_NAME);

  TEARDOWN(prog, p);
  return true;
}

TEST(fn_untyped_params) {
  WITH_PARSE("fn add(a, b) { }", prog, p);
  ASSERT_TRUE(p.had_error);
  TEARDOWN(prog, p);
  return false;
}

int main(void) {
  TEST_SUITE("Parser - Declarations");
  RUN_TEST(fn_simple);
  RUN_TEST(fn_params_and_return_type);
  RUN_TEST(fn_untyped_params);

  TEST_SUMMARY();
  return TEST_EXIT_CODE();
}
