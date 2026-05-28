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

#include <stdbool.h>
#include <stddef.h>

#include "allocator.h"
#include "lexer.h"

typedef struct Node Node;

// A growable array of owned child nodes (used for arg/param/field/stmt lists)
typedef struct NodeList {
  Node **items;
  size_t count;
  size_t cap;
} NodeList;

typedef enum NodeKind {
  NODE_INT_LIT,
  NODE_FLOAT_LIT,
  NODE_STRING_LIT,
  NODE_BOOL_LIT,
  NODE_NULL_LIT,
  NODE_IDENT,
  NODE_UNARY,
  NODE_POSTFIX,
  NODE_BINARY,
  NODE_ASSIGN,
  NODE_CALL,
  NODE_MEMBER,
  NODE_INDEX,
  NODE_HEAP,
  NODE_TYPE_NAME,
  NODE_TYPE_PTR,
  NODE_TYPE_ARR,
  NODE_LET,
  NODE_EXPR_STMT,
  NODE_BLOCK,
  NODE_IF,
  NODE_WHILE,
  NODE_DO_WHILE,
  NODE_FOR,
  NODE_RETURN,
  NODE_BREAK,
  NODE_CONTINUE,
  NODE_FN,
  NODE_PARAM,
  NODE_TYPE_DECL,
  NODE_STRUCT,
  NODE_UNION,
  NODE_ENUM,
  NODE_FIELD,
  NODE_ENUM_VAR,
  NODE_IMPORT,
  NODE_PROGRAM,
} NodeKind;

struct Node {
  NodeKind kind;
  size_t line;
  union {
    struct {
      char *text;
    } literal;
    struct {
      bool val;
    } boolean;
    struct {
      char *name;
    } ident;
    struct {
      TokenType op;
      Node *operand;
    } unary;
    struct {
      TokenType op;
      Node *left;
      Node *right;
    } binary;
    struct {
      TokenType op;
      Node *target;
      Node *val;
    } assign;
    struct {
      Node *callee;
      NodeList args;
    } call;
    struct {
      Node *obj;
      char *field;
    } member;
    struct {
      Node *obj;
      Node *idx;
    } subscript;
    struct {
      Node *val;
    } heap;
    struct {
      char *name;
    } type_name;
    struct {
      Node *pointee;
    } pointer;
    struct {
      Node *size; // NULL => slice []T, otherwise sized array [N]T
      Node *elem;
    } array;
    struct {
      bool is_const;
      char *name;
      Node *type; // maybe NULL
      Node *init; // maybe NULL
    } let;
    struct {
      Node *expr;
    } expr_stmt;
    struct {
      NodeList stmts;
    } block;
    struct {
      Node *cond;
      Node *then_branch;
      Node *else_branch; // maybe NULL
    } if_stmt;
    struct {
      Node *cond;
      Node *body;
    } while_stmt;
    struct {
      Node *body;
      Node *cond;
    } do_while;
    struct {
      Node *init;
      Node *cond;
      Node *post;
      Node *body;
    } for_stmt;
    struct {
      Node *val;
    } ret;
    struct {
      bool is_pub;
      char *name;
      NodeList params;
      Node *ret_type; // may be NULL
      Node *body;
    } fn;
    struct {
      char *name;
      Node *type;
    } param;
    struct {
      bool is_pub;
      char *name;
      Node *def; // struct/union/enum, or a type (alias)
    } type_decl;
    struct {
      NodeList fields; // of NODE_FIELD, shared by struct and union
    } record;
    struct {
      NodeList variants; // of NODE_ENUM_VARIANT
    } enom;
    struct {
      char *name;
      Node *type;
    } field;
    struct {
      char *name;
      Node *val; // may be NULL
    } enum_variant;
    struct {
      char *path;
    } import;
    struct {
      NodeList decls;
    } program;
  } as;
};

Node *new_node(Allocator *a, NodeKind kind, size_t line);
void free_node(Allocator *a, Node *node);
void node_list_push(Allocator *a, NodeList *list, Node *node);

// Duplicate a NUL-terminated string into allocator-owned memory (NULL-safe)
char *ast_copy_str(Allocator *a, const char *s);

// Pretty-print a node tree to stdout for debugging
void ast_print(const Node *node, int indent);

const char *node_kind_to_string(NodeKind kind);
