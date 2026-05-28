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

#include "ast.h"

#include <stdio.h>
#include <string.h>

Node *new_node(Allocator *a, NodeKind kind, size_t line) {
  Node *node = (Node *)ALLOC(a, sizeof(Node), "Node");
  if (node == NULL)
    return NULL;
  memset(node, 0, sizeof(Node));
  node->kind = kind;
  node->line = line;
  return node;
}

void node_list_push(Allocator *a, NodeList *list, Node *node) {
  if (list->count + 1 > list->cap) {
    size_t old_cap = list->cap;
    size_t new_cap = old_cap < 8 ? 8 : old_cap * 2;
    list->items = (Node **)REALLOC(a, list->items, old_cap * sizeof(Node *),
                                   new_cap * sizeof(Node *), "NodeList");
    list->cap = new_cap;
  }

  list->items[list->count++] = node;
}

char *ast_copy_str(Allocator *a, const char *s) {
  if (s == NULL)
    return NULL;
  size_t len = strlen(s);
  char *out = (char *)ALLOC(a, len + 1, "AstString");
  if (out)
    memcpy(out, s, len + 1);
  return out;
}

static void free_str(Allocator *a, char *s) {
  if (s)
    FREE(a, s, strlen(s) + 1, "AstString");
}

static void free_list(Allocator *a, NodeList *list) {
  for (size_t i = 0; i < list->count; i++)
    free_node(a, list->items[i]);
  if (list->items)
    FREE(a, list->items, list->cap * sizeof(Node *), "NodeList");
}

void free_node(Allocator *a, Node *node) {
  if (node == NULL)
    return;

  switch (node->kind) {
  case NODE_INT_LIT:
  case NODE_FLOAT_LIT:
  case NODE_STRING_LIT:
    free_str(a, node->as.literal.text);
    break;
  case NODE_BOOL_LIT:
  case NODE_NULL_LIT:
  case NODE_BREAK:
  case NODE_CONTINUE:
    break;
  case NODE_IDENT:
    free_str(a, node->as.ident.name);
    break;
  case NODE_UNARY:
  case NODE_POSTFIX:
    free_node(a, node->as.unary.operand);
    break;
  case NODE_BINARY:
    free_node(a, node->as.binary.left);
    free_node(a, node->as.binary.right);
    break;
  case NODE_ASSIGN:
    free_node(a, node->as.assign.target);
    free_node(a, node->as.assign.val);
    break;
  case NODE_CALL:
    free_node(a, node->as.call.callee);
    free_list(a, &node->as.call.args);
    break;
  case NODE_MEMBER:
    free_node(a, node->as.member.obj);
    free_str(a, node->as.member.field);
    break;
  case NODE_INDEX:
    free_node(a, node->as.subscript.obj);
    free_node(a, node->as.subscript.idx);
    break;
  case NODE_HEAP:
    free_node(a, node->as.heap.val);
    break;
  case NODE_TYPE_NAME:
    free_str(a, node->as.type_name.name);
    break;
  case NODE_TYPE_PTR:
    free_node(a, node->as.pointer.pointee);
    break;
  case NODE_TYPE_ARR:
    free_node(a, node->as.array.size);
    free_node(a, node->as.array.elem);
    break;
  case NODE_LET:
    free_str(a, node->as.let.name);
    free_node(a, node->as.let.type);
    free_node(a, node->as.let.init);
    break;
  case NODE_EXPR_STMT:
    free_node(a, node->as.expr_stmt.expr);
    break;
  case NODE_BLOCK:
    free_list(a, &node->as.block.stmts);
    break;
  case NODE_IF:
    free_node(a, node->as.if_stmt.cond);
    free_node(a, node->as.if_stmt.then_branch);
    free_node(a, node->as.if_stmt.else_branch);
    break;
  case NODE_WHILE:
    free_node(a, node->as.while_stmt.cond);
    free_node(a, node->as.while_stmt.body);
    break;
  case NODE_DO_WHILE:
    free_node(a, node->as.do_while.body);
    free_node(a, node->as.do_while.cond);
    break;
  case NODE_FOR:
    free_node(a, node->as.for_stmt.init);
    free_node(a, node->as.for_stmt.cond);
    free_node(a, node->as.for_stmt.post);
    free_node(a, node->as.for_stmt.body);
    break;
  case NODE_RETURN:
    free_node(a, node->as.ret.val);
    break;
  case NODE_FN:
    free_str(a, node->as.fn.name);
    free_list(a, &node->as.fn.params);
    free_node(a, node->as.fn.ret_type);
    free_node(a, node->as.fn.body);
    break;
  case NODE_PARAM:
    free_str(a, node->as.param.name);
    free_node(a, node->as.param.type);
    break;
  case NODE_TYPE_DECL:
    free_str(a, node->as.type_decl.name);
    free_node(a, node->as.type_decl.def);
    break;
  case NODE_STRUCT:
  case NODE_UNION:
    free_list(a, &node->as.record.fields);
    break;
  case NODE_ENUM:
    free_list(a, &node->as.enom.variants);
    break;
  case NODE_FIELD:
    free_str(a, node->as.field.name);
    free_node(a, node->as.field.type);
    break;
  case NODE_ENUM_VAR:
    free_str(a, node->as.enum_variant.name);
    free_node(a, node->as.enum_variant.val);
    break;
  case NODE_IMPORT:
    free_str(a, node->as.import.path);
    break;
  case NODE_PROGRAM:
    free_list(a, &node->as.program.decls);
    break;
  }

  FREE(a, node, sizeof(Node), "Node");
}

const char *node_kind_to_string(NodeKind kind) {
  switch (kind) {
  case NODE_INT_LIT:
    return "IntLit";
  case NODE_FLOAT_LIT:
    return "FloatLit";
  case NODE_STRING_LIT:
    return "StringLit";
  case NODE_BOOL_LIT:
    return "BoolLit";
  case NODE_NULL_LIT:
    return "NullLit";
  case NODE_IDENT:
    return "Ident";
  case NODE_UNARY:
    return "Unary";
  case NODE_POSTFIX:
    return "Postfix";
  case NODE_BINARY:
    return "Binary";
  case NODE_ASSIGN:
    return "Assign";
  case NODE_CALL:
    return "Call";
  case NODE_MEMBER:
    return "Member";
  case NODE_INDEX:
    return "Index";
  case NODE_HEAP:
    return "Heap";
  case NODE_TYPE_NAME:
    return "TypeName";
  case NODE_TYPE_PTR:
    return "PointerType";
  case NODE_TYPE_ARR:
    return "ArrayType";
  case NODE_LET:
    return "Let";
  case NODE_EXPR_STMT:
    return "ExprStmt";
  case NODE_BLOCK:
    return "Block";
  case NODE_IF:
    return "If";
  case NODE_WHILE:
    return "While";
  case NODE_DO_WHILE:
    return "DoWhile";
  case NODE_FOR:
    return "For";
  case NODE_RETURN:
    return "Return";
  case NODE_BREAK:
    return "Break";
  case NODE_CONTINUE:
    return "Continue";
  case NODE_FN:
    return "Fn";
  case NODE_PARAM:
    return "Param";
  case NODE_TYPE_DECL:
    return "TypeDecl";
  case NODE_STRUCT:
    return "Struct";
  case NODE_UNION:
    return "Union";
  case NODE_ENUM:
    return "Enum";
  case NODE_FIELD:
    return "Field";
  case NODE_ENUM_VAR:
    return "EnumVariant";
  case NODE_IMPORT:
    return "Import";
  case NODE_PROGRAM:
    return "Program";
  }
  return "Unknown";
}

static void print_indent(int indent) {
  for (int i = 0; i < indent; i++)
    printf("  ");
}

static void print_list(const char *label, const NodeList *list, int indent) {
  if (list->count == 0)
    return;
  print_indent(indent);
  printf("%s:\n", label);
  for (size_t i = 0; i < list->count; i++)
    ast_print(list->items[i], indent + 1);
}

void ast_print(const Node *node, int indent) {
  if (node == NULL) {
    print_indent(indent);
    printf("<null>\n");
    return;
  }

  print_indent(indent);
  printf("%s", node_kind_to_string(node->kind));

  switch (node->kind) {
  case NODE_INT_LIT:
  case NODE_FLOAT_LIT:
  case NODE_STRING_LIT:
    printf(" %s\n", node->as.literal.text ? node->as.literal.text : "");
    break;
  case NODE_BOOL_LIT:
    printf(" %s\n", node->as.boolean.val ? "true" : "false");
    break;
  case NODE_NULL_LIT:
  case NODE_BREAK:
  case NODE_CONTINUE:
    printf("\n");
    break;
  case NODE_IDENT:
    printf(" %s\n", node->as.ident.name);
    break;
  case NODE_UNARY:
  case NODE_POSTFIX:
    printf(" (%s)\n", token_type_to_string(node->as.unary.op));
    ast_print(node->as.unary.operand, indent + 1);
    break;
  case NODE_BINARY:
    printf(" (%s)\n", token_type_to_string(node->as.binary.op));
    ast_print(node->as.binary.left, indent + 1);
    ast_print(node->as.binary.right, indent + 1);
    break;
  case NODE_ASSIGN:
    printf(" (%s)\n", token_type_to_string(node->as.assign.op));
    ast_print(node->as.assign.target, indent + 1);
    ast_print(node->as.assign.val, indent + 1);
    break;
  case NODE_CALL:
    printf("\n");
    ast_print(node->as.call.callee, indent + 1);
    print_list("args", &node->as.call.args, indent + 1);
    break;
  case NODE_MEMBER:
    printf(" .%s\n", node->as.member.field);
    ast_print(node->as.member.obj, indent + 1);
    break;
  case NODE_INDEX:
    printf("\n");
    ast_print(node->as.subscript.obj, indent + 1);
    ast_print(node->as.subscript.idx, indent + 1);
    break;
  case NODE_HEAP:
    printf("\n");
    ast_print(node->as.heap.val, indent + 1);
    break;
  case NODE_TYPE_NAME:
    printf(" %s\n", node->as.type_name.name);
    break;
  case NODE_TYPE_PTR:
    printf("\n");
    ast_print(node->as.pointer.pointee, indent + 1);
    break;
  case NODE_TYPE_ARR:
    printf("\n");
    if (node->as.array.size) {
      print_indent(indent + 1);
      printf("size:\n");
      ast_print(node->as.array.size, indent + 2);
    }
    ast_print(node->as.array.elem, indent + 1);
    break;
  case NODE_LET:
    printf(" %s%s\n", node->as.let.is_const ? "const " : "", node->as.let.name);
    if (node->as.let.type)
      ast_print(node->as.let.type, indent + 1);
    if (node->as.let.init)
      ast_print(node->as.let.init, indent + 1);
    break;
  case NODE_EXPR_STMT:
    printf("\n");
    ast_print(node->as.expr_stmt.expr, indent + 1);
    break;
  case NODE_BLOCK:
    printf("\n");
    print_list("stmts", &node->as.block.stmts, indent + 1);
    break;
  case NODE_IF:
    printf("\n");
    ast_print(node->as.if_stmt.cond, indent + 1);
    ast_print(node->as.if_stmt.then_branch, indent + 1);
    if (node->as.if_stmt.else_branch)
      ast_print(node->as.if_stmt.else_branch, indent + 1);
    break;
  case NODE_WHILE:
    printf("\n");
    ast_print(node->as.while_stmt.cond, indent + 1);
    ast_print(node->as.while_stmt.body, indent + 1);
    break;
  case NODE_DO_WHILE:
    printf("\n");
    ast_print(node->as.do_while.body, indent + 1);
    ast_print(node->as.do_while.cond, indent + 1);
    break;
  case NODE_FOR:
    printf("\n");
    ast_print(node->as.for_stmt.init, indent + 1);
    ast_print(node->as.for_stmt.cond, indent + 1);
    ast_print(node->as.for_stmt.post, indent + 1);
    ast_print(node->as.for_stmt.body, indent + 1);
    break;
  case NODE_RETURN:
    printf("\n");
    if (node->as.ret.val)
      ast_print(node->as.ret.val, indent + 1);
    break;
  case NODE_FN:
    printf(" %s%s\n", node->as.fn.is_pub ? "pub " : "", node->as.fn.name);
    print_list("params", &node->as.fn.params, indent + 1);
    if (node->as.fn.ret_type) {
      print_indent(indent + 1);
      printf("returns:\n");
      ast_print(node->as.fn.ret_type, indent + 2);
    }
    ast_print(node->as.fn.body, indent + 1);
    break;
  case NODE_PARAM:
    printf(" %s\n", node->as.param.name);
    if (node->as.param.type)
      ast_print(node->as.param.type, indent + 1);
    break;
  case NODE_TYPE_DECL:
    printf(" %s%s\n", node->as.type_decl.is_pub ? "pub " : "",
           node->as.type_decl.name);
    ast_print(node->as.type_decl.def, indent + 1);
    break;
  case NODE_STRUCT:
  case NODE_UNION:
    printf("\n");
    print_list("fields", &node->as.record.fields, indent + 1);
    break;
  case NODE_ENUM:
    printf("\n");
    print_list("variants", &node->as.enom.variants, indent + 1);
    break;
  case NODE_FIELD:
    printf(" %s\n", node->as.field.name);
    ast_print(node->as.field.type, indent + 1);
    break;
  case NODE_ENUM_VAR:
    printf(" %s\n", node->as.enum_variant.name);
    if (node->as.enum_variant.val)
      ast_print(node->as.enum_variant.val, indent + 1);
    break;
  case NODE_IMPORT:
    printf(" %s\n", node->as.import.path);
    break;
  case NODE_PROGRAM:
    printf("\n");
    print_list("decls", &node->as.program.decls, indent + 1);
    break;
  }
}
