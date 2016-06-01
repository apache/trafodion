/*

    SYMTAB.C

    This header file defines a symbol table object.  The symbol table
    records whether particular symbols are "interesting" or not.

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
//

/*  the object structure - all fields intended to be private  */


#define SYMBOL_TABLE_LENGTH 100

struct symbol_table
   {
   int current_symbol;      /*  last symbol added to symtab; -1 if none  */

   int next_avail_symbol;   /*  next free entry in symtab  */

   /*  entries in symtab are allocated sequentially starting at entry 0  */

   struct symbol_table_entry
      {
      int length;           /*  symbol length  */
      char *data;           /*  symbol  */
#define INTERESTING 1
#define UNINTERESTING 0
      int state;            /*  whether symbol is interesting or not */
      }  symtab[SYMBOL_TABLE_LENGTH];
   } ;



/*  function definitions - all intended to be public  */


struct symbol_table *symbol_table_create(void);

void symbol_table_add(struct symbol_table *s,char *symbol,int symbol_length);

void symbol_table_make_interesting(struct symbol_table *s);

int symbol_table_is_interesting(struct symbol_table *s,
                                char *symbol,
                                int symbol_length);
