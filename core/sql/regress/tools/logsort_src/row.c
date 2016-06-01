/*

    ROW.C

    This module implements the row object.

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

#include "row.h"

#define TRUE 1
#define FALSE 0

/*

    row_create

    This function creates a row object.

    entry parameter:  linecount - the number of lines that will make
    up a row

    exit parameter:  returns the newly created row.

                                                                          */

struct row *row_create(int linecount)

{
struct row *rc;

rc = (struct row *)malloc(sizeof(struct row));

rc->databuffer = NULL;
rc->databuffersize = 0;

rc->bufferedcharoffset = -1;

rc->expectedlines = linecount;

rc->next = NULL;

return rc;
}



/*

    row_add

    This function adds a line of text to a row.

    entry parameter:  rw - the row to add the line to
    line - the line to add

    No exit parameters.

                                                                            */

void row_add(struct row *rw, char *line)

{
if (rw->databuffer == NULL)
   {
   /*  allocate a buffer for text - take a guess that the buffer
       required will be the number of expected lines times the
       length of the current line + 10                               */

   rw->databuffersize = rw->expectedlines * (strlen(line) + 10);
   rw->databuffer = (char *)malloc(rw->databuffersize);
   if (rw->databuffer != NULL)
      {
      strcpy(rw->databuffer,line);
      }
   else
      {
      rw->databuffersize = 0;
      printf("Unable to allocate storage for data lines.\n");
      }
   }
else if ((int)(strlen(rw->databuffer) + strlen(line)) > rw->databuffersize - 1)
   {
   /*  need to allocate a larger buffer */
   int newbuffersize;
   char *newbuffer;

   /*  take a guess on a new buffer size  */
   newbuffersize = rw->databuffersize + 2 * (strlen(line) + 10);

   newbuffer = (char *)malloc(newbuffersize);
   if (newbuffer != NULL)
      {
      rw->databuffersize = newbuffersize;
      strcpy(newbuffer,rw->databuffer);
      strcpy(newbuffer+strlen(newbuffer),line);
      free(rw->databuffer);
      rw->databuffer = newbuffer;
      }
   else
      {
      printf("Unable to increase storage for data lines.\n");
      }
   }
else
   {
   strcpy(rw->databuffer+strlen(rw->databuffer),line);
   }
}



/*

    row_remove

    This function returns a pointer to a line of text belonging to
    a row.

    entry parameter:  rw - the row

    exit parameter:  returns a pointer to the next line of text in
    the row; NULL if there is no more

                                                                       */

char *row_remove(struct row *rw)

{
char *rc;
char *end;

if (rw->databuffer == NULL) rc = NULL;
else
   {
   if (rw->bufferedcharoffset >= 0)
      {
      /*  put back character overlayed with '\0' on previous call  */

      rw->databuffer[rw->bufferedcharoffset] = rw->bufferedchar;
      }
   else rw->bufferedcharoffset = 0;

   /*  find the end of the next text line  */

   rc = rw->databuffer + rw->bufferedcharoffset;
   for (end = rc; (*end != '\n') && (*end != '\0'); end++) ;

   if (*end == '\0') rc = NULL;
   else
      {
      end++;   /*  step over '\n'  */

      /*  stick a null terminator after it - save the character overlayed  */

      rw->bufferedchar = *end;
      *end = '\0';

      rw->bufferedcharoffset = end - rw->databuffer;
      }
   }

return rc;
}


/*

    row_destroy

    This function deallocates the storage for a row object.

    entry parameter:  rw - the row to destroy

    No exit parameters.

                                                                          */

void row_destroy(struct row *rw)

{
if (rw->databuffer != NULL) free(rw->databuffer);
free (rw);
}


/*

    row_less

    This function defines an ordering among rows.  It determines
    which of two rows is less.

    entry parameters:  rw1 - one row
    rw2 - another row

    exit parameters:  returns TRUE if rw1 < rw2.

                                                                            */

int row_less(struct row *rw1, struct row *rw2)

{
int rc;

/*  treat the empty row as less than any other row  */

if (rw2->databuffer == NULL) rc = FALSE;
else if (rw1->databuffer == NULL) rc = TRUE;
else if (strcmp(rw1->databuffer,rw2->databuffer) < 0) rc = TRUE;
else rc = FALSE;

return rc;
}
