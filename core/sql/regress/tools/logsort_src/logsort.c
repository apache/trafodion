/*

    LOGSORT.C

    This program transforms a SQLCI actual result file containing
    unordered SELECT output into a new file with the output in a
    canonical order.

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

#include "line.h"
#include "symtab.h"   /*  only because tokstr.h needs it  */
#include "tokstr.h"
#include "row.h"
#include "rowlst.h"

#define TRUE 1
#define FALSE 0


/*

    checkoptions

    This function determines what command line options have been
    specified.

    entry parameters:  options - a character string containing options
    debug_option_ptr - pointer to debug option flag
    ignore_option_ptr - pointer to ignore order by option flag
    strip_stats_option_ptr - pointer to strip statistics option flag

    exit parameter:  returns TRUE if valid options were specified,
    FALSE otherwise.

    Note:  The caller is expected to have initialized the option flags
    to their default values before calling this function.

                                                                        */
int checkoptions(char *options,int *debug_option_ptr,
   int *ignore_option_ptr, int *strip_stats_option_ptr)

{
int d_count = 0;
int i_count = 0;
int s_count = 0;
int other_count = 0;

if (*options == '-')
   {
   options++;
   while (*options)
      {
      if (*options == 'd')
         d_count++;
      else if (*options == 'i')
         i_count++;
      else if (*options == 's')
         s_count++;
      else
         other_count++;
      options++;
      }

   if (d_count > 0)
      {
      *debug_option_ptr = TRUE;
      d_count--;
      }

   if (i_count > 0)
      {
      *ignore_option_ptr = TRUE;
      i_count--;
      }

   if (s_count > 0)
      {
      *strip_stats_option_ptr = TRUE;
      s_count--;
      }
   }

return (*options == '\0') && (d_count + i_count + s_count + other_count == 0);
}


/*

    interpretrc

    This function maps return codes from token_stream_add to parse
    states used by main().

    entry parameter:  trc - return code from token_stream_add

    exit parameter:  a parse state as known by main()

                                                                        */
int interpretrc(int trc)

{
int rc;

rc = 0;
if (trc == 1) rc = 2;       /*  trc = 1 ==> at end of statement  */
else if (trc == 2) rc = 1;  /*  trc = 2 ==> not at end of statement  */
else printf("Unexpected return code %d from token_stream_add.\n",trc);

return rc;
}



/*  state for myFputs function


                                                                          */

#define NOT_STRIPPING 0     /*  not stripping any stats output right now  */
#define SAW_LEADING_BLANK_LINE  1  /*  saw what might be the blank line
                                       before stats output                */
#define STRIPPING  2        /*  saw known stats output line and stripped it */
#define SAW_TRAILING_BLANK_LINE 3  /*  saw blank line after known stats  */
#define STRIPPING_OPT 4     /*  saw known optimizer stats output line  */

/*  the reason for separate STRIPPING and STRIPPING_OPT states is that the
    DISPLAY STATISTICS stats output has a trailing blank line while the
    optimizer stats do not                                                */

static int myFputsState = NOT_STRIPPING;

/*

    myFputs

    This function removes trailing blanks from a line, then writes that
    to a file.

    Entry parameters:  str - the line to be written
    strm - the file to write it to
    strip_stats_option - TRUE if caller wishes to strip output from
     DISPLAY STATISTICS or SET STATISTICS ON from the log; FALSE otherwise
    might_be_stats - TRUE if the line might be output from DISPLAY
     STATISTICS or SET STATISTICS ON; FALSE otherwise

    No exit parameters.

    Maintains state in the static variable myFputsState.

                                                                           */

void myFputs(char * str,
             FILE * strm,
             int strip_stats_option,
             int might_be_stats)
{
int i;
char * newstr;

i = strlen(str) - 1;

/* skip trailing endl */
if (str[i] == '\n')
  i--;

/* remove trailing blanks */
while ( (i > 0) && (str[i] == ' ') )
  i--;

newstr = (char *) malloc(i+3);
if (i+1 > 0)
  strncpy (newstr, str, i+1);

newstr[i+1] = '\n';
newstr[i+2] = 0;

if (strip_stats_option && might_be_stats)
   {
   switch (myFputsState)
      {
      case NOT_STRIPPING:
         {
         if (line_isblank(str))
            {
            /*  might be blank line before stats -- suppress its
                output until we know for sure                      */
            myFputsState = SAW_LEADING_BLANK_LINE;
            }
         break;
         }
      case SAW_LEADING_BLANK_LINE:
         {
         if (line_isstats(str))
            myFputsState = STRIPPING;
         else if (line_isoptstats(str))
            myFputsState = STRIPPING_OPT;
         else
            {
            /*  the blank line we saw isn't the introduction to stats
                output, so output it now                               */
            fputs("\n",strm);
            myFputsState = NOT_STRIPPING;
            }
         break;
         }
      case STRIPPING:
         {
         if (line_isblank(str))
            myFputsState = SAW_TRAILING_BLANK_LINE;
         else if (!line_isstats(str))
            myFputsState = NOT_STRIPPING;  /* don't expect this... */
         break;
         }
      case SAW_TRAILING_BLANK_LINE:
         {
         myFputsState = NOT_STRIPPING;
         break;
         }
      case STRIPPING_OPT:
         {
         if (line_isblank(str))
            myFputsState = SAW_LEADING_BLANK_LINE;
         else if (!line_isoptstats(str))
            myFputsState = NOT_STRIPPING;
         break;
         }
      default:
         {
         printf("Unexpected myFputsState encountered: %d.\n",myFputsState);
         break;
         }
      }
   }
else
   {
   if (myFputsState == SAW_LEADING_BLANK_LINE)
      {
      /*  the blank line we saw isn't the introduction to stats
          output, so output it now                               */
      fputs("\n",strm);
      }
   myFputsState = NOT_STRIPPING;
   }

if (myFputsState == NOT_STRIPPING)
   {
   fputs(newstr, strm);
   }

free(newstr);
}


/*

    mightbestats

    This function defines which contexts we expect statistics output may
    occur in.

    In Trafodion SQLCI, there are two kinds of statistics output. These can
    be classed as DISPLAY STATISTICS type vs. optimizer internal statistics.
    The first type appear after any DISPLAY STATISTICS statement. They also
    appear after the output of any DML statement if a SET STATISTICS ON
    statement is currently active. The second type appear after a DML
    statement but before its output if a CONTROL QUERY DEFFAULT
    OPTIMIZER_PRINT_INTERNAL_COUNTERS 'ON' statement is currently active.

    It follows (should one analyze the state machine in the main() function)
    that we expect statistics either immediately after a statement and before
    its output, or after its output. This happens to be precisely parse states
    0, 3 and 8.

    entry parameter: state - the parse state as determined by main()

    exit parameter: returns TRUE if this is a context where we may see
    statistics output, FALSE otherwise.

                                                                                     */
int mightbestats(int state)

{
return (state == 0) || (state == 3) || (state == 8);
}




/*

    main

    entry parameters:  argc - number of arguments (should be 3 or 4)
    argv[0] - the name of this program
    argv[1] - the name of the input file
    argv[2] - the name of the output file
    argv[3] (if present) - -d (debug) option, which causes states to be
    shown as we traverse the file, or -i option, which causes all
    select output to be sorted (i.e. ignore ORDER BY).

    exit parameters:  none

                                                                           */

main(int argc,char *argv[])

{
FILE *in;
FILE *out;
int ignore_order_by_option = FALSE; /*  default options  */
int debug_option = FALSE;
int strip_stats_option = FALSE;

if ((argc < 3) || (argc > 4) ||
    ((argc == 4) &&
     (!checkoptions(argv[3],
                    &debug_option,
                    &ignore_order_by_option,
                    &strip_stats_option))))
   {
   printf("\nUsage:   %s  <in-file> <out-file> [-[d][i][s]] \n",argv[0]);
   printf("\nwhere <in-file> is a SQLCI actual results file to be ");
   printf("transformed.  The\ntransformed results will be written to ");
   printf("<out-file>.\n\n");
   printf("The -i option tells %s to sort output from *all*\nselects, ",
          argv[0]);
   printf("not just those without order by clauses.  This should be used ");
   printf("with\ncare, since the correctness of the results could be ");
   printf("compromised.\n\n");
   printf("The -d option is a debug option:  If specified, internal parse ");
   printf("states are\ndisplayed.\n\n");
   printf("The -s option tells logsort to strip out statistics generated ");
   printf("by DISPLAY\nSTATISTICS and by the Trafodion optimizer.\n\n");
   }
else if ((in = fopen(argv[1],"r")) == NULL)
   {
   printf("\n*** Could not open input file %s.\n",argv[1]);
   }
else if ((out = fopen(argv[2],"w")) == NULL)
   {
   printf("\n*** Could not open output file %s.\n",argv[2]);
   fclose(in);
   }
else
   {
   /*  we have good arguments, and have opened both in and out files  */

   struct token_stream *t;
   struct row_list *r;
   struct row *rw;
   int state;
   int saved_state;
   int line_within_row;
   int might_be_end;
#define linebufsz 5000
   char linebuf[linebufsz+1];
   int headingcount;

   headingcount = 0;
   rw = NULL;

   t = token_stream_create(ignore_order_by_option);
   r = row_list_create();

   state = 0;
   while ((fgets(linebuf,linebufsz,in)) != NULL)
      {
      if (debug_option)
         {
         /*  debug option -d was specified; print out state as we go  */
         printf("State %d\n",state);
         printf("%s",linebuf);
         }
      if (((strncmp(linebuf,"  ATTRIBUTES EXTENT (51200, 102400), MAXEXTENTS 768",51)) == 0) || ((strncmp(linebuf,"  ATTRIBUTES BLOCKSIZE 32768",28)) == 0 ) || ((strncmp(linebuf,"--  ATTRIBUTES BLOCKSIZE 32768",30)) == 0 ) )
         {
         continue;
	 }

      if ((strncmp(linebuf,"  HASH2 PARTITION",17) == 0) || (strncmp(linebuf,"  HASH PARTITION",16) == 0)  || (strncmp(linebuf,"--  HASH2 PARTITION",19) == 0))
         {
         while ((fgets(linebuf,linebufsz,in)) != NULL)
            {
            if (debug_option)
               {
               /* debug option -d was specified; print out state as we go  */
               printf("State %d\n",state);
               printf("%s",linebuf);
               }
            if ((strncmp(linebuf,"  STORE BY",10) == 0 )  || (strncmp(linebuf,"  ;",3) == 0) || (strncmp(linebuf,"--  ;",5) == 0 ))
               {
               break;
               }
            continue;
            }
         }

	if ((strncmp(linebuf,"DISK-",5) == 0) || (strncmp(linebuf,"Partition[",10) == 0))
         {
         while ((fgets(linebuf,linebufsz,in)) != NULL)
            {
            if (debug_option)
               {
               /* debug option -d was specified; print out state as we go  */
               printf("State %d\n",state);
               printf("%s",linebuf);
               }
            if (strcmp(linebuf,"\n") == 0)
               {
               break;
               }
            continue;
            }
         }

      switch (state)
         {
         case 0:         /*  not in a statement  */
            {
            if (line_isignore(linebuf))
               {
               saved_state = state;
               state = 7;  /*  saw a Trafodion sqlci ?ignore directive  */
               }
            else if (line_isstmt(linebuf))
               {
               state = interpretrc(token_stream_add(t,line_strip(linebuf)));
               }
            myFputs(linebuf,out,strip_stats_option,mightbestats(state));
            break;
            }
         case 1:         /*  in a statement, but not at the end of it yet  */
            {
            if (line_isignore(linebuf))
               {
               saved_state = state;
               state = 7;  /*  saw a Trafodion sqlci ?ignore directive  */
               }
            else if (line_isstmt(linebuf))
               {
               state = interpretrc(token_stream_add(t,line_strip(linebuf)));
               }
            else
               {
               /*  We are seeing response text back from SQLCI before
                   having seen a semi-colon; it's probably a syntax
                   error (or possibly the 'exit' command), so assume
                   that the next SQLCI input line will be the beginning
                   of a statement.                                       */
               token_stream_clear(t);
               state = 0;
               }
            myFputs(linebuf,out,strip_stats_option,mightbestats(state));
            break;
            }
         case 2:         /*  previous line saw end-of-statement  */
            {
            if (token_stream_interesting(t))
               {
               /*  we are just after an unordered SELECT  */

               headingcount = 0;
               if (line_isblank(linebuf)) state = 3;
               else if (line_is0rows(linebuf)) state = 0;
               else if (line_iserror(linebuf)) state = 0;
               else
                  {
                  printf("Unexpected line after a SELECT statement:\n");
                  printf("%s",linebuf);
                  state = 0;
                  }
               token_stream_advance(t,1);  /*  advance past SELECT  */
               }
            else if (token_stream_is_get(t))
               {
               /*  we are just after a GET with sortable output  */
               headingcount = 0;
               if (line_isblank(linebuf)) state = 10;
               else if (line_is0rows(linebuf)) state = 0;
               else if (line_iserror(linebuf)) state = 0;
               else
                  {
                  printf("Unexpected line after a GET statement:\n");
                  printf("%s",linebuf);
                  state = 0;
                  }
               token_stream_advance(t,1);  /*  advance past GET  */
               }
            else
               {
               /*  We are just after some other statement.  If the
                   current line is another input line, then all
                   previous statements did not produce SQLCI output,
                   so we advance past all of them.  If the current line
                   is NOT an input line, this line is an output line
                   from ONE of the preceeding statements.  We don't
                   know which one, so the best we can do is assume that
                   there is only one statement producing output and
                   skip past it.                                        */

               if (line_isstmt(linebuf))
                  {
                  /*  do the advance *before* tokenizing current line  */
                  token_stream_advance(t,-1); /* skip all outstanding stmts */
                  state = interpretrc(token_stream_add(t,line_strip(linebuf)));
                  }
               else
                  {
                  state = 0;
                  token_stream_advance(t,1);  /*  skip just one line  */
                  }
               }
            myFputs(linebuf,out,strip_stats_option,mightbestats(state));
            break;
            }
         case 3:   /*  we expect to see a heading line or a blank line  */
            {
            if (line_isheading(linebuf)) state = 4;
            else if (line_isblank(linebuf))
               {
               if (headingcount == 0)
                  {
                  printf("Heading missing after SELECT.\n");
                  state = 0;
                  }
               else
                  {
                  state = 5;
                  }
               }
            /* 0 rows selected after a blank line can happen with Trafodion sqlci */
            else if (line_is0rows(linebuf))
               state = 0;  /* for Trafodion */
            /* error message after a blank line can happen with Trafodion sqlci */
            else if (line_iserror(linebuf))
               state = 0;  /* for Trafodion */
            /* warning messages occur after a blank line also in Trafodion sqlci */
            else if (line_iswarning(linebuf))
               state = 9;  /* there will be a blank line after the warning */
            else if (line_isoptstats(linebuf))
               state = 8;  /* must be Trafodion Optimizer stats output */
            else
               {
               printf("Unexpected line after a SELECT statement:\n");
               printf("%s",linebuf);
               state = 0;
               }
            myFputs(linebuf,out,strip_stats_option,mightbestats(state));
            break;
            }
         case 4:    /*  we last saw a heading line  */
            {
            if (line_isunderline(linebuf))
               {
               headingcount++;
               state = 3;
               }
            else
               {
               printf("Unexpected line after heading line:\n");
               printf("%s",linebuf);
               state = 0;
               }
            myFputs(linebuf,out,strip_stats_option,mightbestats(state));
            break;
            }
         case 5:    /*  we last saw a blank line  */
            {
            /*  the blank line might have been part of the heading -
                e.g. part of a long character string heading - or it
                might be the blank line before the first row           */

            if (line_isunderline(linebuf))
               {
               /*  the current line is an underline - so assume the
                   previous line was part of the heading            */
               headingcount++;
               state = 3;
               }
            else
               {
               /*  the current line is not an underline - so assume
                   the previous line was the blank line before the
                   first row, and that this line is the first line
                   of the first row                                  */
               line_within_row = 0;
               might_be_end = FALSE;
               state = 6;
               goto case6;  /*  reprocess this line in state 6  */
               }
            myFputs(linebuf,out,strip_stats_option,mightbestats(state));
            break;
            }
         case 6:   /*  we have just seen a line in a row  */
            {
         case6:    /*  can branch in from previous case  */
            if ((line_within_row == 1) && might_be_end &&
                line_isnnrows(linebuf))
               {
               /*  we've seen all the rows - get rid of any partial
                   outstanding row, then output rows in sorted order  */

               struct row *rw1;
               char *returned_line;

               while ((rw1 = row_list_remove_min(r)) != NULL)
                  {
                  while ((returned_line = row_remove(rw1)) != NULL)
                     {
                     myFputs(returned_line,out,strip_stats_option,mightbestats(state));
                     }
                  row_destroy(rw1);
                  }

               /*  don't forget to write out buffered blank line also  */

               returned_line = row_remove(rw);
               myFputs(returned_line,out,strip_stats_option,mightbestats(state));
               row_destroy(rw);
               rw = NULL;
               might_be_end = FALSE;

               myFputs(linebuf,out,strip_stats_option,mightbestats(state));

               /*  BUG - the test in the next statement should be:
                   if (there are outstanding statements) ...
                   But, need to write a new token_stream fn. to do this  */

               if (token_stream_interesting(t)) state = 2;
               else state = 0;
               }
            else
               {
               if (line_within_row == headingcount)
                  {
                  row_list_add(r,rw);
                  rw = NULL;
                  line_within_row = 0;
                  }
               if (line_within_row == 0)
                  {
                  if (line_isblank(linebuf)) might_be_end = TRUE;
                  else might_be_end = FALSE;
                  rw = row_create(headingcount);
                  }
               row_add(rw,linebuf);
               line_within_row++;
               }
            break;
            }
         case 7:   /*  within a Trafodion sqlci ?ignore block  */
            {
            if (line_isignore(linebuf))
               {
               /*  saw end of ?ignore block, so pop the saved state  */
               state = saved_state;
               }
            myFputs(linebuf,out,strip_stats_option,mightbestats(state));
            break;
            }
         case 8:   /*  within Trafodion Optimizer stats output  */
            {
            if (line_isblank(linebuf))
               state = 3;
            else if (!line_isoptstats(linebuf))
               {
               printf("Unexpected line after Trafodion Optimizer stats:");
               printf("%s",linebuf);
               state = 0;
               }
            myFputs(linebuf,out,strip_stats_option,mightbestats(state));
            break;
            }
         case 9:   /* after a warning */
            {
             /* we expect a blank line or another warning */
            if (line_isblank(linebuf))
               state = 3;
            else if (!line_iswarning(linebuf))
               state = 0;
            myFputs(linebuf,out,strip_stats_option,mightbestats(state));
            break;
            }
         case 10:   /* after blank line after GET statement */
            {
            if (line_isheading(linebuf)) state = 11;
            /* 0 rows selected after a blank line can happen with Trafodion sqlci */
            else if (line_issqloperationcomplete(linebuf))
               state = 0;  /* for Trafodion */
            /* error message after a blank line can happen with Trafodion sqlci */
            else if (line_iserror(linebuf))
               state = 0;  /* for Trafodion */
            /* warning messages occur after a blank line also in Trafodion sqlci */
            else if (line_iswarning(linebuf))
               state = 9;  /* there will be a blank line after the warning */
            else
               {
               printf("Unexpected line after a GET statement:\n");
               printf("%s",linebuf);
               state = 0;
               }
            myFputs(linebuf,out,strip_stats_option,mightbestats(state));
            break;
            }
         case 11:  /* after GET statement header text, e.g. "Tables in Schema TRAFODION.SCH" */
            {
            if (line_isgetheadingorfooting(linebuf))
               {
               headingcount++;
               state = 12;
               }
            /* 0 rows selected after a blank line can happen with Trafodion sqlci */
            else if (line_is0rows(linebuf))
               state = 0;  /* for Trafodion */
            /* error message after a blank line can happen with Trafodion sqlci */
            else if (line_iserror(linebuf))
               state = 0;  /* for Trafodion */
            /* warning messages occur after a blank line also in Trafodion sqlci */
            else if (line_iswarning(linebuf))
               state = 9;  /* there will be a blank line after the warning */
            else
               {
               printf("Unexpected line after a GET statement:\n");
               printf("%s",linebuf);
               state = 0;
               }
            myFputs(linebuf,out,strip_stats_option,mightbestats(state));
            break;
            }
         case 12: /* after "===============" header line for GET statement */
            {
            if (line_isblank(linebuf))
               {
               state = 13;
               line_within_row = 0;
               }
            else
               {
               printf("Unexpected line after a GET header:\n");
               printf("%s",linebuf);
               state = 0;
               }
            myFputs(linebuf,out,strip_stats_option,mightbestats(state));
            break;
            }
         case 13:  /*  we are at a row of GET statement output */
            {
            if (line_isblank(linebuf))
               {
               /*  a blank line means no more GET output  */
               
               if (rw)  /* add any outstanding row to the list */
                  {
                  row_list_add(r,rw);
                  rw = NULL;
                  }

               struct row *rw1;
               char *returned_line;

               while ((rw1 = row_list_remove_min(r)) != NULL)
                  {
                  while ((returned_line = row_remove(rw1)) != NULL)
                     {
                     myFputs(returned_line,out,strip_stats_option,mightbestats(state));
                     }
                  row_destroy(rw1);
                  }

               myFputs(linebuf,out,strip_stats_option,mightbestats(state));

               /*  BUG - the test in the next statement should be:
                   if (there are outstanding statements) ...
                   But, need to write a new token_stream fn. to do this  */

               if (token_stream_interesting(t)) state = 2;
               else if (token_stream_is_get(t)) state = 2;
               else state = 0;
               }
            else
               {
               if (line_within_row == headingcount)
                  {
                  row_list_add(r,rw);
                  rw = NULL;
                  line_within_row = 0;
                  }
               if (line_within_row == 0)
                  {
                  rw = row_create(headingcount);
                  }
               row_add(rw,linebuf);
               line_within_row++;
               }
            break;
            }
         default:
            {
            printf("Error:  got bad state %d.\n",state);
            break;
            }
         }
      }
   fclose(in);
   fclose(out);
   }

return 0;
}
