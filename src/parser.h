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

#include "allocator.h"
#include "ast.h"
#include "lexer.h"

// Errors are reported as they occur and recorded in `had_error`. After an error
// the parser enters `panic_mode` and stays quiet until synchronize() finds a
// safe boundary (a statement/decl start), so one mistake yields one message
// rather than a cascade
typedef struct Parser {
  Lexer *lexer;
  Allocator *allocator;
  Token current;
  Token previous;
  bool had_error;
  bool panic_mode;
} Parser;

void init_parser(Parser *parser, Lexer *lexer, Allocator *allocator);

// Parse a whole compilation unit and return a NODE_PROGRAM (never NULL; on
// error the tree is partial and parser->had_error is true). The caller owns the
// returned node and must free it with free_node().
Node *parse_program(Parser *parser);

void free_parser(Parser *parser);
