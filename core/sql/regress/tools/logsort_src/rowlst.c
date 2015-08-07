/*

    ROWLST.C

    This module implements the row list object.

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

#include <stdlib.h>

#include "row.h"
#include "rowlst.h"



/*

    row_list_create

    This function creates a row list.

    No entry parameters.

    exit parameter:  returns the created row list.

                                                                         */

struct row_list *row_list_create(void)

{
struct row_list *rc;

rc = (struct row_list *)malloc(sizeof(struct row_list));

rc->first = NULL;
rc->last = NULL;

return rc;
}



/*

    row_list_remove_min

    This function removes the row with the minimum value from the
    row list, and returns it.

    entry parameter:  r - the row list

    exit parameter:  returns the row if one exists, NULL otherwise

                                                                         */

struct row *row_list_remove_min(struct row_list *r)

{
struct row *rc;
struct row *rc_prev;
struct row *prev;
struct row *current;

rc = NULL;
if (r->first != NULL)
   {
   /*  the row list is non-empty; figure out the minimum one  */

   rc_prev = NULL;
   rc = r->first;
   prev = r->first;

   for (current = rc->next; current != NULL; current = current->next)
      {
      if (row_less(current,rc))
         {
         rc = current;
         rc_prev = prev;
         }
      prev = current;
      }

   /*  at this point, rc is the minimum row; remove it from the row list  */


   if (rc_prev != NULL)
      {
      rc_prev->next = rc->next;
      }
   else
      {
      r->first = rc->next;
      if (r->first == NULL) r->last = NULL;
      }
   }

return rc;
}


/*

   row_list_add

   This function adds a row to a row list.

   entry parameters:  r - the row list
   rw - the row

   No exit parameters.

                                                                          */

void row_list_add(struct row_list *r,struct row *rw)

{
if (r->first == NULL)
   {
   r->first = r->last = rw;
   }
else
   {
   struct row *rw_last;

   rw_last = r->last;
   r->last = rw_last->next = rw;
   }
}
