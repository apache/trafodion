/*

    LINE.C

    This module contains functions which classify lines in a SQLCI
    log.
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

#include <string.h>
#include <ctype.h>
#include "line.h"


#define TRUE 1
#define FALSE 0

/*

    line_isstmt

    This function determines if a line is part of SQLCI input text
    (and therefore is possibly part of an SQL statement).

    entry parameters:  line - the line to analyze

    exit parameters:  returns TRUE if so, FALSE if not.

                                                                          */


int line_isstmt(char *line)

{
int rc;

rc = FALSE;
if (strncmp(line,">>",2) == 0) rc = TRUE;
else if (strncmp(line,"+>",2) == 0) rc = TRUE;

return rc;
}



/*

    line_strip

    This function effectively strips the SQLCI prompt from a line of
    SQLCI input text.  If the input line is a ?section directive,
    the line is effectively stripped to an empty line.

    entry parameters:  line - the line to strip

    exit parameters:  returns address of stripped line

                                                                          */


char *line_strip(char *line)

{
char *rc;

rc = line;
if (strncmp(line,">>",2) == 0)
   {
   rc = line + 2;
   if (*rc == '?')
      {
      int i;
      char buffer[8];

      strncpy(buffer,rc+1,7);
      for (i = 0; i < 7; i++) buffer[i] = toupper(buffer[i]);
      if (strncmp(buffer,"SECTION",7) == 0)
         {
         /*  it's a ?section directive - step past directive  */
         rc = rc + 7;
         while ((*rc != '\n') && (*rc != '\0')) rc++;
         }
      }
   }
else if (strncmp(line,"+>",2) == 0) rc = line + 2;

return rc;
}


/*

    line_isblank

    This function determines if a line is all blanks.

    entry parameters:  line - the line to analyze

    exit parameters:  returns TRUE if so, FALSE if not.

                                                                          */


int line_isblank(char *line)

{
char *next;
int rc;

rc = FALSE;

for (next = line; *next == ' '; next++) ; /*  step forward to 1st non-blank  */

if (*next == '\n')
   {
   next++;
   if (*next == '\0') rc = TRUE;
   }

return rc;
}



/*

    line_is0rows

    This function determines if a line is the "0 row(s) selected"
    response from SQLCI.

    entry parameters:  line - the line to analyze

    exit parameters:  returns TRUE if so, FALSE if not.

                                                                          */


int line_is0rows(char *line)

{
return (strcmp(line,"--- 0 row(s) selected.\n") == 0);
}



/*

    line_iserror

    This function determines if a line is part of SQLCI error text
    line.

    entry parameters:  line - the line to analyze

    exit parameters:  returns TRUE if so, FALSE if not.

                                                                          */


int line_iserror(char *line)

{
int rc;
char *next;

if (strncmp(line,"*** ERROR[",10) == 0) rc = TRUE;  /*  a line beginning w/ ***  */
else
   {
   /*  look for lines of the form

      ...<sql syntax>...

       or

               ^   (i.e. a caret pointing to a token)         */

   for (next = line; *next == ' '; next++) ;  /*  skip leading blanks  */
   if (*next == '^') rc = TRUE;
   else if (strncmp(next,"...",3) == 0) rc = TRUE;
   else rc = FALSE;
   }

return rc;
}



/*

    line_iswarning

    This function determines if a line is part of SQLCI warning text
    line.

    entry parameters:  line - the line to analyze

    exit parameters:  returns TRUE if so, FALSE if not.

                                                                          */


int line_iswarning(char *line)

{
int rc = (strncmp(line,"*** WARNING[",12) == 0);

return rc;
}



/*

    line_isheading

    This function determines if a line is part of SQLCI heading text.

    entry parameters:  line - the line to analyze

    exit parameters:  returns TRUE if so, FALSE if not.

                                                                          */


int line_isheading(char *line)

{
int rc;

rc = FALSE;
if (isalpha(line[0])) rc = TRUE;
else if (line[0] == '"') rc = TRUE;  /* to allow delimiter identifiers */
else if (strncmp(line,"(EXPR)",6) == 0) rc = TRUE;

return rc;
}


/*

    line_isunderline

    This function determines if a line is the underline part of a
    SQLCI report heading.

    entry parameters:  line - the line to analyze

    exit parameters:  returns TRUE if so, FALSE if not.

                                                                          */


int line_isunderline(char *line)

{
char *next;
char *block;
int rc;

next = line;
if (*next == '-')         /*  insure at least one '-'  */
   {
   rc = TRUE;             /*  assume it's underlines till known otherwise  */
   while ((*next != '\n') && rc)
      {
      block = next;

      while (*next == '-') next++;   /*  skip '-'  */
      while (*next == ' ') next++;   /*  skip blanks following '-'  */

      if ((block == next) && (*next != '\n')) rc = FALSE;
      }
   }
else rc = FALSE;

return rc;
}



/*

    line_isnnrows

    This function determines if a line is the

       --- nn row(s) selected.

    response from SQLCI.

    entry parameters:  line - the line to analyze

    exit parameters:  returns TRUE if so, FALSE if not.

                                                                          */


int line_isnnrows(char *line)

{
int rc;

rc = FALSE;
if (strncmp(line,"--- ",4) == 0)
   {
   char *next;

   for (next = line + 4; isdigit(*next); next++) ; /*  skip digits  */
   if (next != line + 4)
      {
      /*  at least one digit after "--- "  */
      if (strcmp(next," row(s) selected.\n") == 0) rc = TRUE;
      }
   }

return rc;
}



/*

    line_isignore

    This function determines if a line is the

       ?ignore

    directive.  This directive is unique to the SQL/MX SQLCI.

    entry parameters:  line - the line to analyze

    exit parameters:  returns TRUE if so, FALSE if not.

                                                                          */

int line_isignore(char *line)

{
int rc;
int i;
int length;
char possible_ignore[10];

rc = FALSE;

length = (int)strlen(line);
for (i = 0; (i < 10) && (i < length); i++)
  {
  possible_ignore[i] = tolower(line[i]);
  }

if (strncmp(possible_ignore,">>?ignore",9) == 0)
  {
  if ((isspace(possible_ignore[9])) ||
      (possible_ignore[9] == '\n'))
    rc = TRUE;
  }

return rc;
}


/*

    line_isstats

    This function determines if a line is output from the SQL/MX SQLCI
    DISPLAY STATISTICS or SET STATISTICS ON facility.

    entry parameters:  line - the line to analyze

    exit parameters:  returns TRUE if so, FALSE if not.

                                                                          */

int line_isstats(char *line)

{
int rc;

rc = FALSE;

if (strncmp(line,"Start Time ",11) == 0)
   {
   rc = TRUE;
   }
else if (strncmp(line,"End Time ",9) == 0)
   {
   rc = TRUE;
   }
else if (strncmp(line,"Elapsed Time ",13) == 0)
   {
   rc = TRUE;
   }
else if (strncmp(line,"Compile Time ",13) == 0)
   {
   rc = TRUE;
   }
else if (strncmp(line,"Execution Time ",15) == 0)
   {
   rc = TRUE;
   }

return rc;
}


/*

    line_contains

    This function determines if a line contains a specified substring.

    entry parameters:  line - the line to analyze
    substring - the substring to look for (assumed null-terminated)

    exit parameters:  returns address of first byte after first occurance
    of the substring if so, 0 if not.

                                                                          */

char *line_contains(char *line,char *substring)

{
char *rc;
int sublength;

rc = 0;
sublength = strlen(substring);

if (sublength == 0)
   {
   rc = line;  /* every string contains the empty string as a substring */
   }
else 
   {
   char *next = line;

   while ((rc == 0) && (*next))  /*  while still searching  */
      {
      /* find next incidence of first character of substring */
      while (*next && (*next != *substring))
         next++;
      /* now see if the substring occurs there */
      if (strncmp(next,substring,sublength) == 0)
         rc = next+sublength;
      else
         next++;
      }
   }

return rc;
}



/*

    line_isoptstats

    This function determines if a line is output from the SQL/MX SQLCI
    optimizer stats facility.

    entry parameters:  line - the line to analyze

    exit parameters:  returns TRUE if so, FALSE if not.

                                                                          */

int line_isoptstats(char *line)

{
int rc;

rc = FALSE;

/*                01234567890123456789012345678901234567890123456789012             */
if (strncmp(line,"*** WARNING[2052] Optimizer internal counters: pass ",52) == 0)
   {
   rc = TRUE;
   }     
else if (strcmp(line,".\n") == 0)
   {
   /*  the optimizer statistics are terminated with this funny line with
       nothing but a period on it                                          */
   rc = TRUE;
   }
else
   {
   char *next;

   next = line_contains(line,"groups,");
   if (next)
      {
      next = line_contains(next,"tasks,");
      if (next)
         {
         next = line_contains(next,"rules,");
         if (next)
            {
            /*  probably found line 
                     nnnn groups, nnnnnn tasks, nnn rules,
                (where the number of n's varies)                 */
            rc = TRUE;
            }
         }
      }
   else
      {
      next = line_contains(line,"groups merged,");
      if (next)
         {
         next = line_contains(next,"expr. cleaned up,");
         if (next)
            {
            next = line_contains(next,"tasks pruned");
            if (next)
               {
               /*  probably found line
                       nnn groups merged, nnn expr. cleaned up, nnn tasks pruned
                   (where the number of n's varies)                                */
               rc = TRUE;
               }
            }
         }
      else
         {
         next = line_contains(line,"log/phys/plans/dupl expressions in CascadesMemo");
         if (next)
            {
            /*  probably found line
                    nnn/nnn/nnn/nnn log/phys/plans/dupl expressions in CascadesMemo
                (where the number of n's varies)                                     */
            rc = TRUE;
            }
         }
      }
   }

return rc;
}

/*

    line_isgetheadingorfooting

    This function determines if a line is a header or footer line
    for GET statement output.

    entry parameters:  line - the line to analyze

    exit parameters:  returns TRUE if so, FALSE if not.

                                                                          */

int line_isgetheadingorfooting(char *line)
{
return (strncmp(line,"==========",10) == 0);
}

/*

    line_issqloperationcomplete

    This function determines if a line is "SQL operation complete."

    entry parameters:  line - the line to analyze

    exit parameters:  returns TRUE if so, FALSE if not.

                                                                          */

int line_issqloperationcomplete(char *line)
{
return (strcmp(line,"--- SQL operation complete.\n") == 0);
}
