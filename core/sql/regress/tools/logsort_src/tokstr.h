/*

    TOKSTR.H

    This header file contains definitions of a token stream and the
    functions which operate on it.

                                                                          */

// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

#define STMT_QUEUE_LENGTH 200

struct token_stream
   {
   /*  all of these fields are meant to be private to tokstr.c  */

   int parsestate;              /*  current parse state  */
   int ignore_order_by;         /*  if TRUE, SELECTs are deemed interesting,
                                    if FALSE, only SELECTs without ORDER BY
                                    are deemed interesting               */
   struct symbol_table *symbol_table;  /*  symbol table for this stream  */

   /*  The next few variables implement a circular queue for storing
       whether given statements are interesting.  Each time we parse
       to the end of a statement, we add an entry to stmt describing
       whether the statement was interesting.  As statements are
       consumed, they are removed from the queue.                        */

   int first_stmt;          /*  first element in the queue, if any;
                                the queue is empty if first_stmt ==
                                last_stmt                             */
   int last_stmt;           /*  next slot to add a statement in the queue  */

   enum stmtState { SELECT_WITHOUT_ORDER_BY, INTERESTING_GET, UNINTERESTING_STATEMENT };

   stmtState stmt[STMT_QUEUE_LENGTH];  /*  a circular queue for statements  */
   } ;

/*  public function definitions  */

struct token_stream *token_stream_create(int ignore_order_by_option);
int token_stream_add(struct token_stream *t,char *text);
int token_stream_interesting(struct token_stream *t);
int token_stream_is_get(struct token_stream *t);
void token_stream_clear(struct token_stream *t);
void token_stream_advance(struct token_stream *t,int advance_factor);
