/*

    SYMTAB.C

    This module implements a symbol table object.  The symbol table
    records whether particular symbols are "interesting" or not.

    The current implementation is quite simple-minded; symbols are
    added to the table in the order they arrive.  No attempt to make
    symbol table search efficient has been made - this is a quick
    implementation.  Yet, an efficient implementation could be done
    without changing the externals of this module.

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

#include "symtab.h"

#define TRUE 1
#define FALSE 0

/*

    symbol_table_create

    This function creates a symbol table object.

    No entry parameters.

    exit parameter:  returns an initialized symbol table object.

                                                                          */

struct symbol_table *symbol_table_create(void)

{
struct symbol_table *rc;

rc = (struct symbol_table *)malloc(sizeof(struct symbol_table));

rc->next_avail_symbol = 0;
rc->current_symbol = -1;

return rc;
}


/*

    symbol_compare

    This function compares two symbols.  It is assumed that they are
    the same length.

    entry parameters:  symbol1 - the first symbol
    symbol2 - the second symbol
    symbol_length - the symbol length (of both symbols)

    exit parameters:  returns TRUE if they are equal, FALSE otherwise.

                                                                        */

static int symbol_compare(char *symbol1, char *symbol2, int symbol_length)

{
int i;
int rc;

rc = TRUE;
for (i = 0; (i < symbol_length) && rc; i++)
   {
   if (toupper(symbol1[i]) != toupper(symbol2[i])) rc = FALSE;
   }

return rc;
}


/*

    search_table

    This function searches the symbol table for an entry containing
    the input symbol.

    entry parameters:  s - the symbol table
    symbol - a pointer to the symbol, NOT assumed to be null-terminated
    symbol_length - the length of the symbol

    exit parameters:  returns entry number if it is in the table;
    returns entry number of next free entry if not

                                                                          */

static int search_table(struct symbol_table *s,char *symbol,int symbol_length)

{
int rc;
int entry;

rc = s->next_avail_symbol;   /*  assume we won't find it  */

/*  check the current symbol first - since often this is the one
    we are looking for                                           */

if ((s->current_symbol >= 0) &&
    (s->current_symbol < s->next_avail_symbol) &&
    (s->symtab[s->current_symbol].length == symbol_length) &&
    (symbol_compare(s->symtab[s->current_symbol].data,symbol,symbol_length)))
   {
   rc = s->current_symbol;    /*  found it  */
   }
else
   {
   /*  search the entire table  */

   for (entry = 0; entry < s->next_avail_symbol; entry++)
      {
      if ((s->symtab[entry].length == symbol_length) &&
          (entry != s->current_symbol) &&    /*  already checked that one  */
          (symbol_compare(s->symtab[entry].data,symbol,symbol_length)))
         {
         rc = entry;                    /*  found it  */
         entry = s->next_avail_symbol;  /*  to force loop exit  */
         }
      }
   }

return rc;
}


/*

    symbol_table_add

    This function adds a symbol to the symbol table.  If the symbol
    already exists in the table, it is redefined.

    entry parameters:  s - the symbol table
    symbol - a pointer to the symbol, NOT assumed to be null-terminated
    symbol_length - the length of the symbol

    No exit parameters.

                                                                           */

void symbol_table_add(struct symbol_table *s,char *symbol,int symbol_length)

{
int entry;

entry = search_table(s,symbol,symbol_length);
if (entry == s->next_avail_symbol)
   {
   /*  the symbol doesn't exist yet in the symbol table - save the
       symbol                                                          */
   char *savesymbol;

   if (s->next_avail_symbol == SYMBOL_TABLE_LENGTH)
      {
      printf("Symbol table overflow - erasing current symbols.\n");
      entry = s->next_avail_symbol = 0;
      }

   savesymbol = (char *)malloc(symbol_length + 1);
   if (savesymbol == NULL)
      {
      printf("Unable to allocate storage for symbol table entry.\n");

      s->current_symbol = -1;
      }
   else
      {
      strncpy(savesymbol,symbol,symbol_length);
      savesymbol[symbol_length] = 0;

      s->symtab[entry].data = savesymbol;
      s->symtab[entry].length = symbol_length;
      s->symtab[entry].state = UNINTERESTING;

      s->current_symbol = entry;

      s->next_avail_symbol++;
      }
   }
else
   {
   /*  the symbol is already in the table - just reset its state  */

   s->symtab[entry].state = UNINTERESTING;
   s->current_symbol = entry;
   }
}


/*

    symbol_table_make_interesting

    This function marks the current symbol (i.e. the last one entered
    via symbol_table_add) as an interesting symbol.

    entry parameter:  s - the symbol table

    No exit parameters.

                                                                         */

void symbol_table_make_interesting(struct symbol_table *s)

{
if ((s->current_symbol >= 0) && (s->current_symbol < s->next_avail_symbol))
   {
   s->symtab[s->current_symbol].state = INTERESTING;
   }
}



/*

    symbol_table_is_interesting

    This function determines if the input symbol is "interesting".

    entry parameters:  s - the symbol table
    symbol - the symbol - NOT assumed to be null-terminated
    symbol_length - the length of the symbol

    exit parameter:  returns TRUE if the input symbol is interesting,
    FALSE otherwise.

                                                                          */

int symbol_table_is_interesting(struct symbol_table *s,
                                char *symbol,
                                int symbol_length)

{
int rc;
int entry;

rc = FALSE;   /*  assume symbol is not interesting until known otherwise  */

entry = search_table(s,symbol,symbol_length);
if (entry < s->next_avail_symbol)
   {
   /*  symbol exists in symbol table  */

   rc = (s->symtab[entry].state == INTERESTING);
   }

return rc;
}
