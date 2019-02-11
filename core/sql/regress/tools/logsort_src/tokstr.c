/*

    TOKSTR.C

    This module contains functions which analyze tokens in input
    to SQLCI.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tokstr.h"
#include "symtab.h"

#define TRUE 1
#define FALSE 0

/*  interesting token codes  */

#define TOKEN_SELECT 1
#define TOKEN_ORDER 2
#define TOKEN_BY 3
#define TOKEN_SEMICOLON 4

#define TOKEN_PREPARE 5
#define TOKEN_FROM 6
#define TOKEN_QUOTE 7

#define TOKEN_EXECUTE 8

#define TOKEN_IDENTIFIER 9

#define TOKEN_MXCI_DIRECTIVE 10   /* a SQL/MX MXCI directive */

#define TOKEN_GET 11
#define TOKEN_OBJECT_TYPE 12

#define TOKEN_OTHER 13

/*  parse states  */

#define START 0
#define IN_SELECT 1
#define FOUND_ORDER 2
#define FOUND_SELECT_WITHOUT_ORDER_BY 3

#define IN_PREPARE 4
#define IN_PREPARE_AFTER_STMT_NAME 5
#define IN_PREPARE_AFTER_FROM 6
#define IN_PREPARE_AFTER_QUOTE 7
#define IN_PREPARE_SELECT 8
#define IN_PREPARE_FOUND_ORDER 9

#define IN_EXECUTE 10
#define IN_INTERESTING_EXECUTE 11

#define IN_OTHER 12

#define START_AFTER_NULL_STMT 13

#define IN_GET 14
#define IN_INTERESTING_GET 15
#define FOUND_INTERESTING_GET 16

/*

    token_stream_create

    This function creates a token stream object.

    entry parameter:  ignore_order_by - if TRUE, then all SELECTs will
    be deemed interesting; if FALSE, then only those without ORDER BY
    will be deemed interesting.

    exit parameter:  returns an initialized token stream object.

                                                                          */

struct token_stream *token_stream_create(int ignore_order_by)
{
struct token_stream *rc;

rc = (struct token_stream *)malloc(sizeof(struct token_stream));

rc->parsestate = START;
rc->ignore_order_by = ignore_order_by;
rc->first_stmt = 0;
rc->last_stmt = 0;
rc->symbol_table = symbol_table_create();

return rc;
}



/*

    compute_parse_state

    This function computes the new parse state based on the current
    parse state and token type.

    entry parameters:  state - the parse state
    token_type - the token type
    token - a pointer to the token (for identifier tokens only - NOT
     assumed to be null-terminated)
    token_length - the length of token (for identifier tokens only)
    ignore_order_by - if TRUE, ORDER BY clauses are ignored, if FALSE
     then normal semantics are used
    s - the symbol table

    exit parameters:  returns the new parse state

                                                                           */

static int compute_parse_state(int state,
                               int token_type,
                               char *token,
                               int token_length,
                               int ignore_order_by,
                               struct symbol_table *s)

{
int rc;

rc = state;    /*  assume no change until known otherwise  */

switch (state)
   {
   case START:
      {
      if (token_type == TOKEN_SELECT) rc = IN_SELECT;
      else if (token_type == TOKEN_PREPARE) rc = IN_PREPARE;
      else if (token_type == TOKEN_EXECUTE) rc = IN_EXECUTE;
      else if (token_type == TOKEN_MXCI_DIRECTIVE) rc = START;  /* so we ignore directives */
      else if (token_type == TOKEN_GET) rc = IN_GET;
      else if (token_type != TOKEN_SEMICOLON) rc = IN_OTHER;
      else rc = START_AFTER_NULL_STMT;
      break;
      }
   case IN_SELECT:
      {
      if ((token_type == TOKEN_ORDER) && (!ignore_order_by))
         rc = FOUND_ORDER;
      else if (token_type == TOKEN_SEMICOLON)
         rc = FOUND_SELECT_WITHOUT_ORDER_BY;
      break;
      }
   case FOUND_ORDER:
      {
      if (token_type == TOKEN_BY) rc = IN_OTHER;
      else if (token_type == TOKEN_SEMICOLON)
         rc = FOUND_SELECT_WITHOUT_ORDER_BY;
      else rc = IN_SELECT;
      break;
      }
   case FOUND_INTERESTING_GET:
   case FOUND_SELECT_WITHOUT_ORDER_BY:
      {
      if (token_type == TOKEN_SELECT) rc = IN_SELECT;
      else if (token_type == TOKEN_GET) rc = IN_GET;
      else if (token_type == TOKEN_PREPARE) rc = IN_PREPARE;
      else if (token_type == TOKEN_EXECUTE) rc = IN_EXECUTE;
      else if (token_type == TOKEN_MXCI_DIRECTIVE) rc = START; /* so we ignore MXCI directives */
      else if (token_type != TOKEN_SEMICOLON) rc = IN_OTHER;
      else rc = START_AFTER_NULL_STMT;
      break;
      }
   case IN_PREPARE:
      {
      if (token_type == TOKEN_IDENTIFIER)
         {
         /*  need to make an entry in the symbol table  */
         symbol_table_add(s,token,token_length);
         rc = IN_PREPARE_AFTER_STMT_NAME;
         }
      else if (token_type == TOKEN_SEMICOLON) rc = START;
      else rc = IN_OTHER;
      break;
      }
   case IN_PREPARE_AFTER_STMT_NAME:
      {
      if (token_type == TOKEN_FROM) rc = IN_PREPARE_AFTER_FROM;
      else if (token_type == TOKEN_SEMICOLON) rc = START;
      else rc = IN_OTHER;
      break;
      }
   case IN_PREPARE_AFTER_FROM:
      {
      if (token_type == TOKEN_QUOTE) rc = IN_PREPARE_AFTER_QUOTE;
      else if (token_type == TOKEN_SELECT) rc = IN_PREPARE_SELECT;
      else if (token_type == TOKEN_SEMICOLON) rc = START;
      else rc = IN_OTHER;
      break;
      }
   case IN_PREPARE_AFTER_QUOTE:
      {
      if (token_type == TOKEN_SELECT) rc = IN_PREPARE_SELECT;
      else if (token_type == TOKEN_SEMICOLON) rc = START;
      else rc = IN_OTHER;
      break;
      }
   case IN_PREPARE_SELECT:
      {
      if ((token_type == TOKEN_ORDER) && (!ignore_order_by))
         rc = IN_PREPARE_FOUND_ORDER;
      else if (token_type == TOKEN_SEMICOLON)
         {
         /*  we have a PREPARE of a SELECT without ORDER BY;  update
             the symbol table                                         */
         symbol_table_make_interesting(s);
         rc = START;
         }
      break;
      }
   case IN_PREPARE_FOUND_ORDER:
      {
      if (token_type == TOKEN_BY) rc = IN_OTHER;
      else if (token_type == TOKEN_SEMICOLON)
         {
         /*  we have a PREPARE of a SELECT without ORDER BY;  update
             the symbol table                                         */
         symbol_table_make_interesting(s);
         rc = START;
         }
      else rc = IN_PREPARE_SELECT;
      break;
      }
   case IN_EXECUTE:
      {
      if (token_type == TOKEN_IDENTIFIER)
         {
         if (symbol_table_is_interesting(s,token,token_length))
            {
            rc = IN_INTERESTING_EXECUTE;
            }
         else rc = IN_OTHER;
         }
      else if (token_type == TOKEN_SEMICOLON) rc = START;
      else rc = IN_OTHER;
      break;
      }
   case IN_INTERESTING_EXECUTE:
      {
      if (token_type == TOKEN_SEMICOLON) rc = FOUND_SELECT_WITHOUT_ORDER_BY;
      break;
      }
   case IN_OTHER:
      {
      if (token_type == TOKEN_SEMICOLON) rc = START;
      break;
      }
   case START_AFTER_NULL_STMT:
      {
      /*  Note:  we get to this state if we see two (or more) semi-colons
          in a row.  The reason we don't simply go to START state in this
          case is that SQLCI does not produce any output for null
          statements.  Therefore, we want to behave as if the null
          statement wasn't there.  The calling function detects a
          statement end only when we traverse into START or
          FOUND_SELECT_WITHOUT_ORDER_BY state; by having this state for
          null statements, we can hide them from the calling function.   */

      if (token_type == TOKEN_SELECT) rc = IN_SELECT;
      else if (token_type == TOKEN_PREPARE) rc = IN_PREPARE;
      else if (token_type == TOKEN_EXECUTE) rc = IN_EXECUTE;
      else if (token_type != TOKEN_SEMICOLON) rc = IN_OTHER;
      break;
      }
   case IN_GET:
      {
      if (token_type == TOKEN_OBJECT_TYPE) rc = IN_INTERESTING_GET;
      else if (token_type == TOKEN_SEMICOLON) rc = START;
      /* else remain in IN_GET state */
      break;
      }
   case IN_INTERESTING_GET:
      {
      if (token_type == TOKEN_SEMICOLON) rc = FOUND_INTERESTING_GET;
      /* else remain in IN_INTERESTING_GET state */
      break;
      }
   default:
      {
      printf("Error:  unknown token type %d.\n",token_type);
      rc = START;
      break;
      }
   }

return rc;
}


/*

    determine_token

    This function figures out what characters makes up a token, and
    what type the token is.

    entry parameters:  token - text beginning with a token
    token_type_ptr - address of place to store token type

    exit parameters:  returns address of first character past token

                                                                         */

static char *determine_token(char *token,int *token_type_ptr)

{
char *next;

*token_type_ptr = TOKEN_OTHER;

if (isdigit(*token))
   {
   for (next = token+1; isdigit(*next); next++) ;
   }
else if (isalpha(*token))
   {
   int length;
   int i;
   char buffer[20];

   *token_type_ptr = TOKEN_IDENTIFIER;

   for (next = token+1; isalpha(*next) ||
                        isdigit(*next) ||
                        (*next == '_');   next++) ;
   length = next - token;
   if (length < 20)
      {
      for (i = 0; i < length; i++) buffer[i] = toupper(token[i]);
      buffer[i] = '\0';

      if (strcmp(buffer,"SELECT") == 0) *token_type_ptr = TOKEN_SELECT;
      /* Treat the TABLE token as if it were SELECT; this gives us
         support for the ANSI <explicit table> syntax. Fortunately,
         there aren't any contexts in our light parse where the TABLE
         keyword needs to be treated differently, so this works.      */
      else if (strcmp(buffer,"TABLE") == 0) *token_type_ptr = TOKEN_SELECT;
      else if (strcmp(buffer,"ORDER") == 0) *token_type_ptr = TOKEN_ORDER;
      else if (strcmp(buffer,"BY") == 0) *token_type_ptr = TOKEN_BY;
      else if (strcmp(buffer,"PREPARE") == 0) *token_type_ptr = TOKEN_PREPARE;
      else if (strcmp(buffer,"FROM") == 0) *token_type_ptr = TOKEN_FROM;
      else if (strcmp(buffer,"EXECUTE") == 0) *token_type_ptr = TOKEN_EXECUTE;
      else if (strcmp(buffer,"GET") == 0) *token_type_ptr = TOKEN_GET;
      else if (strcmp(buffer,"TABLES") == 0) *token_type_ptr = TOKEN_OBJECT_TYPE; /* for GET statement */
      else if (strcmp(buffer,"INDEXES") == 0) *token_type_ptr = TOKEN_OBJECT_TYPE; /* for GET statement */
      else if (strcmp(buffer,"LIBRARIES") == 0) *token_type_ptr = TOKEN_OBJECT_TYPE; /* for GET statement */
      else if (strcmp(buffer,"SCHEMAS") == 0) *token_type_ptr = TOKEN_OBJECT_TYPE; /* for GET statement */
      else if (strcmp(buffer,"VIEWS") == 0) *token_type_ptr = TOKEN_OBJECT_TYPE; /* for GET statement */
      else if (strcmp(buffer,"PRIVILEGES") == 0) *token_type_ptr = TOKEN_OBJECT_TYPE; /* for GET statement */
      else if (strcmp(buffer,"FUNCTIONS") == 0) *token_type_ptr = TOKEN_OBJECT_TYPE; /* for GET statement */
      else if (strcmp(buffer,"PROCEDURES") == 0) *token_type_ptr = TOKEN_OBJECT_TYPE; /* for GET statement */
      else if (strcmp(buffer,"SEQUENCES") == 0) *token_type_ptr = TOKEN_OBJECT_TYPE; /* for GET statement */
      else if (strcmp(buffer,"ROLES") == 0) *token_type_ptr = TOKEN_OBJECT_TYPE; /* for GET statement */
      else if (strcmp(buffer,"OBJECTS") == 0) *token_type_ptr = TOKEN_OBJECT_TYPE; /* for GET statement */
      }
   }
else if (*token == ';')
   {
   *token_type_ptr = TOKEN_SEMICOLON;
   next = token+1;
   }
else if ((*token == '"') || (*token == '\''))
   {
   *token_type_ptr = TOKEN_QUOTE;
   next = token+1;
   }
else if ((*token == '#') || (*token == '?'))
  {
  char temp[13];
  int j;

  for (j = 1; (j < 13) && (isalnum(token[j])); j++)
    {
    temp[j-1] = toupper(token[j]);
    }
  temp[j-1] = '\0';
  if ((strcmp(temp,"IFMX") == 0) ||
      (strcmp(temp,"IFMP") == 0) ||
      (strcmp(temp,"IFNT") == 0) ||
      (strcmp(temp,"IFNSK") == 0) ||
      (strcmp(temp,"IFNSKREL1") == 0) ||
      (strcmp(temp,"IGNORE") == 0) ||
      (strcmp(temp,"IFDEF") == 0) ||
      (strcmp(temp,"IFNDEF") == 0) ||
      (strcmp(temp,"ELSE") == 0) ||
      (strcmp(temp,"ENDIF") == 0) ||
      (strcmp(temp,"SECTION") == 0))
    {
    *token_type_ptr = TOKEN_MXCI_DIRECTIVE;
    next = token + strlen(temp) + 1; /* +1 because temp doesn't include the # or ? */
    while (*next != '\n')  /* treat the rest of the line as part of the directive token */
      {
      next++;
      }
    }
  else
    {
    next = token + 1;
    }
  }
else next = token+1;

return next;
}


/*

    issection

    This function determines if a text line is a ?section statement.

    Entry parameter:  text - points to text line

    Exit parameter:  returns TRUE if so, FALSE if not

                                                                         */

int issection(char *text)

{
int rc;
int i;
char maybe_section[9];

if (*text != '?') rc = FALSE;
else
   {
   for (i = 0; i < 9; i++) maybe_section[i] = toupper(text[i]);
   if (strncmp(maybe_section,"?SECTION",8) == 0) rc = TRUE;
   else rc = FALSE;
   }

return rc;
}




/*

    token_stream_add

    This function takes input text and appends it to the token stream.
    It also does some lightweight analysis of the token stream.

    entry parameters:  t - the token stream to add it to
    text - the input text to add

    exit parameter:  returns parse state as follows:
    1 - have traversed the end of a statement
    2 - have not traversed over end of a statement

                                                                          */

int token_stream_add(struct token_stream *t,char *text)

{
int rc;
int old_last_stmt;
char *next;

old_last_stmt = t->last_stmt;  /*  so we can see if we found a statement  */

/*  tokenize the line; advance parse state until an interesting state
    is reached; save any tokens left after the interesting state      */

if (issection(text))        /*  if it is a ?section statement  */
   {
   /*  skip over the whole statement  */
   for (next = text; (*next != '\n'); next++);
   }

/*  skip white space  */
for (next = text; (*next == ' ') || (*next == '\t'); next++) ;

while (*next != '\n')
   {
   if (strncmp(next,"--",2) == 0)
      {
      /*  the line ends with a comment or is a directive  */
      while (*next != '\n') next++;
      }
   else
      {
      /*  there is a token here  */
      int token_type;
      int token_length;
      char *tokenstart;

      tokenstart = next;
      next = determine_token(next,&token_type /* out */);
      token_length = next - tokenstart;

      t->parsestate = compute_parse_state(t->parsestate,
                                          token_type,
                                          tokenstart,
                                          token_length,
                                          t->ignore_order_by,
                                          t->symbol_table);

      /*  skip any white space after token  */
      while ((*next == ' ') || (*next == '\t')) next++;

      if ((t->parsestate == FOUND_SELECT_WITHOUT_ORDER_BY)||
          (t->parsestate == FOUND_INTERESTING_GET)||
          (t->parsestate == START))
         {
         /*  we have seen the end of a statement - record whether
             the statement was interesting                         */
         
         switch (t->parsestate)
            {
            case FOUND_SELECT_WITHOUT_ORDER_BY: 
               {
               t->stmt[t->last_stmt] = token_stream::SELECT_WITHOUT_ORDER_BY;
               break;
               }
            case FOUND_INTERESTING_GET: 
               {
               t->stmt[t->last_stmt] = token_stream::INTERESTING_GET;
               break;
               }
            default:
               {
               t->stmt[t->last_stmt] = token_stream::UNINTERESTING_STATEMENT;
               break;
               }
            }

         t->last_stmt++;
         if (t->last_stmt == STMT_QUEUE_LENGTH) t->last_stmt = 0;

         if (t->last_stmt == t->first_stmt)
            {
            printf("Error:  statement queue overflow.\n");
            }
         }
      }
   }

if (old_last_stmt != t->last_stmt) rc = 1;
else rc = 2;

return rc;
}



/*

    token_stream_interesting

    This function determines whether the statement just found is
    a SELECT without ORDER BY.

    entry parameters:  t - the token stream

    exit parameter:  returns TRUE if so, FALSE if not

                                                                          */

int token_stream_interesting(struct token_stream *t)

{
return (t->first_stmt != t->last_stmt) &&  /*  make sure at least one stmt  */
       (t->stmt[t->first_stmt] == token_stream::SELECT_WITHOUT_ORDER_BY);
}


/*

    token_stream_is_get

    This function determines whether the statement just found is
    a GET statement with sortable output.

    entry parameters:  t - the token stream

    exit parameter:  returns TRUE if so, FALSE if not

                                                                          */

int token_stream_is_get(struct token_stream *t)

{
return (t->first_stmt != t->last_stmt) &&  /*  make sure at least one stmt  */
       (t->stmt[t->first_stmt] == token_stream::INTERESTING_GET);
}


/*

    token_stream_clear

    This function resets a token stream to the empty state.

    entry parameters:  t - the token stream

    No exit parameters.

                                                                          */

void token_stream_clear(struct token_stream *t)

{
t->parsestate = START;
t->first_stmt = 0;
t->last_stmt = 0;
}



/*

    token_stream_advance

    This function advances the token stream past the current statement.

    entry parameters:  t - the token stream
    advance_factor - indicates how far the advance should go:  if 1,
     we advance (at most) one statement, if -1, we advance past all
     outstanding statements.

    No exit parameters.

                                                                          */

void token_stream_advance(struct token_stream *t,int advance_factor)

{
if (advance_factor == 1)
   {
   /*  advance one statement if one exists - if none exists, do nothing  */

   if (t->first_stmt != t->last_stmt)
      {
      t->first_stmt++;
      if (t->first_stmt == STMT_QUEUE_LENGTH) t->first_stmt = 0;
      }
   }
else if (advance_factor == -1)
   {
   /*  clear out all outstanding statements, without clearing out
       parse state (as token_stream_clear would do)                */
   t->first_stmt = t->last_stmt;
   }
}
