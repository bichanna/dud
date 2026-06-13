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
  return true;
}

TEST(import_decl) {
  WITH_PARSE("import std;", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *imp = prog->as.program.decls.items[0];
  ASSERT_EQ(imp->kind, NODE_IMPORT);
  ASSERT((strcmp(imp->as.import.path, "std") == 0), "import path 'std'");
  TEARDOWN(prog, p);
  return true;
}

static Node *first_fn_stmt(Node *prog, size_t n) {
  Node *fn = prog->as.program.decls.items[0];
  return fn->as.fn.body->as.block.stmts.items[n];
}

TEST(let_with_type_and_init) {
  WITH_PARSE("fn f() { let x: i32 = 42; }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *let = first_fn_stmt(prog, 0);
  ASSERT_EQ(let->kind, NODE_LET);
  ASSERT_FALSE(let->as.let.is_const);
  ASSERT((strcmp(let->as.let.name, "x") == 0), "let name 'x'");
  ASSERT_NOT_NULL(let->as.let.type);
  ASSERT_NOT_NULL(let->as.let.init);
  ASSERT_EQ(let->as.let.init->kind, NODE_INT_LIT);
  TEARDOWN(prog, p);
  return true;
}

TEST(const_decl) {
  WITH_PARSE("fn f() { if x == 0 {} else {} }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *iff = first_fn_stmt(prog, 0);
  ASSERT_EQ(iff->kind, NODE_IF);
  ASSERT_NOT_NULL(iff->as.if_stmt.cond);
  ASSERT_EQ(iff->as.if_stmt.cond->kind, NODE_BINARY);
  ASSERT_EQ(iff->as.if_stmt.cond->as.binary.op, TOKEN_EQUAL_EQUAL);
  ASSERT_NOT_NULL(iff->as.if_stmt.then_branch);
  ASSERT_NOT_NULL(iff->as.if_stmt.else_branch);
  ASSERT_EQ(iff->as.if_stmt.else_branch->kind, NODE_BLOCK);
  TEARDOWN(prog, p);
  return true;
}

TEST(if_else) {
  WITH_PARSE("fn f() { if x == 0 { } else { } }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *iff = first_fn_stmt(prog, 0);
  ASSERT_EQ(iff->kind, NODE_IF);
  ASSERT_NOT_NULL(iff->as.if_stmt.cond);
  ASSERT_EQ(iff->as.if_stmt.cond->kind, NODE_BINARY);
  ASSERT_EQ(iff->as.if_stmt.cond->as.binary.op, TOKEN_EQUAL_EQUAL);
  ASSERT_NOT_NULL(iff->as.if_stmt.then_branch);
  ASSERT_NOT_NULL(iff->as.if_stmt.else_branch);
  ASSERT_EQ(iff->as.if_stmt.else_branch->kind, NODE_BLOCK);
  TEARDOWN(prog, p);
  return true;
}

TEST(else_if_chain) {
  WITH_PARSE("fn f() { if a {} else if b {} else {} }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *iff = first_fn_stmt(prog, 0);
  ASSERT_EQ(iff->kind, NODE_IF);
  ASSERT_NOT_NULL(iff->as.if_stmt.else_branch);
  ASSERT_EQ(iff->as.if_stmt.else_branch->kind, NODE_IF);
  TEARDOWN(prog, p);
  return true;
}

TEST(while_loop) {
  WITH_PARSE("fn f() { while i < 10 { i += 1; } }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *w = first_fn_stmt(prog, 0);
  ASSERT_EQ(w->kind, NODE_WHILE);
  ASSERT_EQ(w->as.while_stmt.cond->kind, NODE_BINARY);
  ASSERT_EQ(w->as.while_stmt.body->kind, NODE_BLOCK);
  TEARDOWN(prog, p);
  return true;
}

TEST(do_while_loop) {
  WITH_PARSE("fn f() { do { x(); } while cond; }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *d = first_fn_stmt(prog, 0);
  ASSERT_EQ(d->kind, NODE_DO_WHILE);
  ASSERT_NOT_NULL(d->as.do_while.body);
  ASSERT_NOT_NULL(d->as.do_while.cond);
  TEARDOWN(prog, p);
  return true;
}

TEST(for_three_clause) {
  WITH_PARSE("fn f() { for let i = 0; i < 10; i++ {} }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *f = first_fn_stmt(prog, 0);
  ASSERT_EQ(f->kind, NODE_FOR);
  ASSERT_NOT_NULL(f->as.for_stmt.init);
  ASSERT_EQ(f->as.for_stmt.init->kind, NODE_LET);
  ASSERT_NOT_NULL(f->as.for_stmt.cond);
  ASSERT_NOT_NULL(f->as.for_stmt.post);
  ASSERT_EQ(f->as.for_stmt.post->kind, NODE_POSTFIX);
  TEARDOWN(prog, p);
  return true;
}

TEST(for_while_style) {
  WITH_PARSE("fn f() { for i < 10 { } }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *f = first_fn_stmt(prog, 0);
  ASSERT_EQ(f->kind, NODE_FOR);
  ASSERT_NULL(f->as.for_stmt.init);
  ASSERT_NOT_NULL(f->as.for_stmt.cond);
  ASSERT_NULL(f->as.for_stmt.post);
  TEARDOWN(prog, p);
  return true;
}

TEST(for_infinite) {
  WITH_PARSE("fn f() { for { break; } }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *f = first_fn_stmt(prog, 0);
  ASSERT_EQ(f->kind, NODE_FOR);
  ASSERT_NULL(f->as.for_stmt.init);
  ASSERT_NULL(f->as.for_stmt.cond);
  ASSERT_NULL(f->as.for_stmt.post);
  Node *brk = f->as.for_stmt.body->as.block.stmts.items[0];
  ASSERT_EQ(brk->kind, NODE_BREAK);
  TEARDOWN(prog, p);
  return true;
}

/* --------------------------------------------------------------------------
 * Expression precedence
 * -------------------------------------------------------------------------- */

TEST(binary_precedence) {
  /* 1 + 2 * 3  =>  +(1, *(2, 3)) */
  WITH_PARSE("fn f() { 1 + 2 * 3; }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *stmt = first_fn_stmt(prog, 0);
  ASSERT_EQ(stmt->kind, NODE_EXPR_STMT);

  Node *add = stmt->as.expr_stmt.expr;
  ASSERT_EQ(add->kind, NODE_BINARY);
  ASSERT_EQ(add->as.binary.op, TOKEN_PLUS);
  ASSERT_EQ(add->as.binary.left->kind, NODE_INT_LIT);

  Node *mul = add->as.binary.right;
  ASSERT_EQ(mul->kind, NODE_BINARY);
  ASSERT_EQ(mul->as.binary.op, TOKEN_MUL);

  TEARDOWN(prog, p);
  return true;
}

TEST(comparison_below_term) {
  /* a + b < c  =>  <( +(a,b), c ) */
  WITH_PARSE("fn f() { a + b < c; }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *cmp = first_fn_stmt(prog, 0)->as.expr_stmt.expr;
  ASSERT_EQ(cmp->kind, NODE_BINARY);
  ASSERT_EQ(cmp->as.binary.op, TOKEN_LESS);
  ASSERT_EQ(cmp->as.binary.left->kind, NODE_BINARY);
  ASSERT_EQ(cmp->as.binary.left->as.binary.op, TOKEN_PLUS);
  TEARDOWN(prog, p);
  return true;
}

TEST(left_associativity) {
  /* a - b - c  =>  -( -(a,b), c ) */
  WITH_PARSE("fn f() { a - b - c; }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *sub = first_fn_stmt(prog, 0)->as.expr_stmt.expr;
  ASSERT_EQ(sub->kind, NODE_BINARY);
  ASSERT_EQ(sub->as.binary.left->kind, NODE_BINARY);
  ASSERT_EQ(sub->as.binary.right->kind, NODE_IDENT);
  TEARDOWN(prog, p);
  return true;
}

TEST(assignment_right_associative) {
  /* a = b = c  =>  assign(a, assign(b, c)) */
  WITH_PARSE("fn f() { a = b = c; }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *asgn = first_fn_stmt(prog, 0)->as.expr_stmt.expr;
  ASSERT_EQ(asgn->kind, NODE_ASSIGN);
  ASSERT_EQ(asgn->as.assign.op, TOKEN_EQUAL);
  ASSERT_EQ(asgn->as.assign.val->kind, NODE_ASSIGN);
  TEARDOWN(prog, p);
  return true;
}

TEST(unary_and_postfix) {
  WITH_PARSE("fn f() { -x; y++; }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *neg = first_fn_stmt(prog, 0)->as.expr_stmt.expr;
  ASSERT_EQ(neg->kind, NODE_UNARY);
  ASSERT_EQ(neg->as.unary.op, TOKEN_MINUS);

  Node *inc = first_fn_stmt(prog, 1)->as.expr_stmt.expr;
  ASSERT_EQ(inc->kind, NODE_POSTFIX);
  ASSERT_EQ(inc->as.unary.op, TOKEN_PLUS_PLUS);
  TEARDOWN(prog, p);
  return true;
}

TEST(call_member_chain) {
  /* me.boss.name  =>  member(member(me, boss), name) */
  WITH_PARSE("fn f() { me.boss.name; }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *m = first_fn_stmt(prog, 0)->as.expr_stmt.expr;
  ASSERT_EQ(m->kind, NODE_MEMBER);
  ASSERT((strcmp(m->as.member.field, "name") == 0), "outer field 'name'");
  ASSERT_EQ(m->as.member.obj->kind, NODE_MEMBER);
  TEARDOWN(prog, p);
  return true;
}

TEST(call_with_args) {
  WITH_PARSE("fn f() { User(1, \"bichanna\", null); }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *call = first_fn_stmt(prog, 0)->as.expr_stmt.expr;
  ASSERT_EQ(call->kind, NODE_CALL);
  ASSERT_EQ(call->as.call.callee->kind, NODE_IDENT);
  ASSERT_EQ(call->as.call.args.count, 3);
  ASSERT_EQ(call->as.call.args.items[0]->kind, NODE_INT_LIT);
  ASSERT_EQ(call->as.call.args.items[1]->kind, NODE_STRING_LIT);
  ASSERT_EQ(call->as.call.args.items[2]->kind, NODE_NULL_LIT);
  TEARDOWN(prog, p);
  return true;
}

TEST(heap_expression) {
  WITH_PARSE("fn f() { let s = heap User(2, \"sister\", null); }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *let = first_fn_stmt(prog, 0);
  ASSERT_EQ(let->as.let.init->kind, NODE_HEAP);
  ASSERT_EQ(let->as.let.init->as.heap.val->kind, NODE_CALL);
  TEARDOWN(prog, p);
  return true;
}

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */

TEST(pointer_type) {
  WITH_PARSE("fn f() { let p: ^User = null; }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *t = first_fn_stmt(prog, 0)->as.let.type;
  ASSERT_EQ(t->kind, NODE_TYPE_PTR);
  ASSERT_EQ(t->as.pointer.pointee->kind, NODE_TYPE_NAME);
  TEARDOWN(prog, p);
  return true;
}

TEST(array_and_slice_types) {
  WITH_PARSE("fn f() { let a: [10]i32 = x; let b: []i32 = y; }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *arr = first_fn_stmt(prog, 0)->as.let.type;
  ASSERT_EQ(arr->kind, NODE_TYPE_ARR);
  ASSERT_NOT_NULL(arr->as.array.size); /* sized array */

  Node *slice = first_fn_stmt(prog, 1)->as.let.type;
  ASSERT_EQ(slice->kind, NODE_TYPE_ARR);
  ASSERT_NULL(slice->as.array.size); /* slice */
  TEARDOWN(prog, p);
  return true;
}

/* --------------------------------------------------------------------------
 * Type declarations
 * -------------------------------------------------------------------------- */

TEST(struct_type_decl) {
  WITH_PARSE("type User = struct { id: i32, name: String, boss: ^User, }", prog,
             p);
  ASSERT_FALSE(p.had_error);
  Node *td = prog->as.program.decls.items[0];
  ASSERT_EQ(td->kind, NODE_TYPE_DECL);
  ASSERT((strcmp(td->as.type_decl.name, "User") == 0), "type name 'User'");

  Node *st = td->as.type_decl.def;
  ASSERT_EQ(st->kind, NODE_STRUCT);
  ASSERT_EQ(st->as.record.fields.count, 3); /* trailing comma tolerated */

  Node *boss = st->as.record.fields.items[2];
  ASSERT_EQ(boss->kind, NODE_FIELD);
  ASSERT((strcmp(boss->as.field.name, "boss") == 0), "third field 'boss'");
  ASSERT_EQ(boss->as.field.type->kind, NODE_TYPE_PTR);
  TEARDOWN(prog, p);
  return true;
}

TEST(enum_type_decl) {
  WITH_PARSE("type Color = enum { Red, Green = 2, Blue, }", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *en = prog->as.program.decls.items[0]->as.type_decl.def;
  ASSERT_EQ(en->kind, NODE_ENUM);
  ASSERT_EQ(en->as.enom.variants.count, 3);
  ASSERT_NULL(en->as.enom.variants.items[0]->as.enum_variant.val);
  ASSERT_NOT_NULL(en->as.enom.variants.items[1]->as.enum_variant.val);
  TEARDOWN(prog, p);
  return true;
}

TEST(type_alias) {
  WITH_PARSE("type Id = i32;", prog, p);
  ASSERT_FALSE(p.had_error);
  Node *td = prog->as.program.decls.items[0];
  ASSERT_EQ(td->kind, NODE_TYPE_DECL);
  ASSERT_EQ(td->as.type_decl.def->kind, NODE_TYPE_NAME);
  TEARDOWN(prog, p);
  return true;
}

static const char *README_PROGRAM =
    "type User = struct {\n"
    "  id: i32,\n"
    "  name: String,\n"
    "  boss: ^User,\n"
    "}\n"
    "\n"
    "fn main() {\n"
    "  let me: User = User(1, \"bichanna\", null);\n"
    "  let sister: ^User = heap User(2, \"sister\", null);\n"
    "  me.boss = sister;\n"
    "  println(\"Hi! My name is {} and my boss is {}\", me.name, "
    "me.boss.name);\n"
    "}\n";

TEST(readme_program_parses) {
  WITH_PARSE(README_PROGRAM, prog, p);
  ASSERT_FALSE(p.had_error);
  ASSERT_EQ(prog->as.program.decls.count, 2);
  ASSERT_EQ(prog->as.program.decls.items[0]->kind, NODE_TYPE_DECL);

  Node *main_fn = prog->as.program.decls.items[1];
  ASSERT_EQ(main_fn->kind, NODE_FN);
  ASSERT_EQ(main_fn->as.fn.body->as.block.stmts.count, 4);

  // me.boss = sister;  -> assignment whose target is a member access
  Node *assign_stmt = main_fn->as.fn.body->as.block.stmts.items[2];
  ASSERT_EQ(assign_stmt->kind, NODE_EXPR_STMT);
  ASSERT_EQ(assign_stmt->as.expr_stmt.expr->kind, NODE_ASSIGN);
  ASSERT_EQ(assign_stmt->as.expr_stmt.expr->as.assign.target->kind,
            NODE_MEMBER);

  TEARDOWN(prog, p);
  return true;
}

TEST(error_missing_semicolon) {
  WITH_PARSE("fn f() { let x = 1 }", prog, p);
  ASSERT_TRUE(p.had_error); // missing ';' is reported
  TEARDOWN(prog, p);
  return true;
}

TEST(error_recovers_and_continues) {
  // First fn has a bad statement; the second fn should still be parsed
  WITH_PARSE("fn bad() { @ } fn good() { }", prog, p);
  ASSERT_TRUE(p.had_error);
  // the good() declaration should survive error recovery.
  bool found_good = false;
  for (size_t i = 0; i < prog->as.program.decls.count; i++) {
    Node *d = prog->as.program.decls.items[i];
    if (d && d->kind == NODE_FN && strcmp(d->as.fn.name, "good") == 0)
      found_good = true;
  }
  ASSERT_TRUE(found_good);
  TEARDOWN(prog, p);
  return true;
}

TEST(no_leaks_on_valid_program) {
  // Silent sink: file_log with a NULL FILE* is a no-op, so the leak tracker
  // runs without flooding test output
  LogSink sink = {file_log, NULL};
  TracingContext ctx = {0};
  ctx.sink = &sink;
  Allocator tracing = {tracing_alloc, tracing_realloc, tracing_free, &ctx};

  Lexer lexer;
  init_lexer(&lexer, README_PROGRAM, &tracing);
  Parser parser;
  init_parser(&parser, &lexer, &tracing);

  Node *prog = parse_program(&parser);
  ASSERT_FALSE(parser.had_error);

  free_node(&tracing, prog);
  free_parser(&parser);

  // Everything the parser allocated must have been freed.
  ASSERT_EQ(ctx.allocated, ctx.freed);
  ASSERT_NULL(ctx.head); // no live allocation records remain
  return true;
}

int main(void) {
  TEST_SUITE("Parser - Declarations");
  RUN_TEST(fn_simple);
  RUN_TEST(fn_params_and_return_type);
  RUN_TEST(fn_untyped_params);
  RUN_TEST(import_decl);

  TEST_SUITE("Parser - Statements");
  RUN_TEST(let_with_type_and_init);
  RUN_TEST(const_decl);
  RUN_TEST(if_else);
  RUN_TEST(else_if_chain);
  RUN_TEST(while_loop);
  RUN_TEST(do_while_loop);
  RUN_TEST(for_three_clause);
  RUN_TEST(for_while_style);
  RUN_TEST(for_infinite);

  TEST_SUITE("Parser - Expression Precedence");
  RUN_TEST(binary_precedence);
  RUN_TEST(comparison_below_term);
  RUN_TEST(left_associativity);
  RUN_TEST(assignment_right_associative);
  RUN_TEST(unary_and_postfix);
  RUN_TEST(call_member_chain);
  RUN_TEST(call_with_args);
  RUN_TEST(heap_expression);

  TEST_SUITE("Parser - Types");
  RUN_TEST(pointer_type);
  RUN_TEST(array_and_slice_types);

  TEST_SUITE("Parser - Type Declarations");
  RUN_TEST(struct_type_decl);
  RUN_TEST(enum_type_decl);
  RUN_TEST(type_alias);

  TEST_SUITE("Parser - Full Program");
  RUN_TEST(readme_program_parses);

  TEST_SUITE("Parser - Error Handling");
  RUN_TEST(error_missing_semicolon);
  RUN_TEST(error_recovers_and_continues);

  TEST_SUITE("Parser - Memory");
  RUN_TEST(no_leaks_on_valid_program);

  TEST_SUMMARY();
  return TEST_EXIT_CODE();
}
