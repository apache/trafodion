/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "CommonNSKFunctions.h"

static void ltrim(char *buffer);
static void rtrim(char *buffer);
static void trim(char *buffer);
static void removenl(char *buffer);
static int  strnicmp(const char *s1, const char *s2, int n);
static char *strupr(char *buffer);
static char *itoa(int n, char *buff, int base);

#define False 0
#define True  1

//LCOV_EXCL_START
void remEOL(char *buffer)
{
  for(;buffer[0];buffer++){
    if(buffer[0]=='\n')
    {
      buffer[0]=0;
      return;
    }
        if(buffer[0]=='\r')
    {
    	buffer[0]=0;
	return;
    }
  }
}

/*------------------------------------------------------------------------------ */
/* titlePos: get a section title position & length in a string. */
/*------------------------------------------------------------------------------ */
static char *titlePos( char *buf, int *len )
{
  char *p = buf, *q;

  while( *p && isspace(*p) ) p++;
  if( *p != '[' )
    return 0;

  q = p+1;
  while( *q && *q != ']' ) q++;
  if( *q != ']' )
    return 0;
  if( len )
    *len = (int)(q - p - 1);
  return p+1;
}

/*------------------------------------------------------------------------------
// isTitleLine: check if a string is a section title line
//------------------------------------------------------------------------------*/
static int isTitleLine( char *bufPtr )
{
  return titlePos( bufPtr, 0 ) != 0;
}

/*------------------------------------------------------------------------------
// containTitle: check if a string contain a section a title
//------------------------------------------------------------------------------*/
static int containTitle( char *buf, const char *section )
{
  char *p;
  int len;

  p = titlePos( buf, &len );
  if( p )
  {
    if( strlen( section ) == len && strnicmp( section, p, len ) == 0 )
      return True;
  }
  return False;
}

/*------------------------------------------------------------------------------
// gotoSection: move file position to start line of a section
//------------------------------------------------------------------------------*/
static int gotoSection( FILE *is, const char *section )
{
  char line[256];
  while( fgets( line, 256, is) )
    if( containTitle( line, section ) )
      return True;
  return False;
}

/*------------------------------------------------------------------------------ */
/* textPos: get content's position of a entry */
/*------------------------------------------------------------------------------ */
static char *textPos( char *buf, const char *entry )
{
  char *p,*p1,*p2;
  int len;

  p = buf;
  while( *p && isspace(*p) ) p++;
  if ( !*p )
	return 0;

  if( *p == ';' ) /* it is comment line */
	return 0;

  if ((p1 = strchr( buf, '=' )) == NULL)
	return 0;
  if( !*p1 )
    return 0;
  p2 = p1;
  while(isspace(*++p2));

  while(isspace(*--p1));
  p1++;

  len = (int)(p1 - p);
  if( strlen(entry) == len && strnicmp( p, entry, len ) == 0 )
    return p2;

  return 0;
}

/* ------------------------------------------------------------------------------ */
/* stripQuotationChar: strip a pair of quotation chars in a string */
/* ------------------------------------------------------------------------------ */
static bool stripQuotationChar( char *buf )
{
  char *p;
  char *q;
  int len;

  if (buf == 0)
	  return false;

  p = buf;
  while( *p && isspace(*p) ) p++;

  if( !(*p == '\"' || *p == '\'') )
    return false;

  q = p+strlen(p);
  while( *q != *p && q > p ) q--;
  if( q == p )
    return false;
  len = (int)(q - p - 1);
  memmove( buf, p+1, len );
  buf[len] = 0;
  return true;
}

/* ------------------------------------------------------------------------------ */
/* stripComment: strip a comment text */
/* ------------------------------------------------------------------------------ */
static void stripComment( char *buf )
{
  char *p;

  p = buf+strlen(buf) - 1;
  while( isspace(*p) && p > buf ) *p-- = 0;
  if( p == buf )
    return;
  p = buf;
  while( *p && *p != ';' ) p++;
  if ( !*p )
	  return;
  *p = 0;
}

/* ------------------------------------------------------------------------------ */
/* readEntry: read content of entry */
/* ------------------------------------------------------------------------------ */
static int readEntry( FILE *is, const char *entry, char *buf, int bufSize,
		      int strip )
{
  char lineBuf[256];
  char *p, *cur;
  int  len;
  bool quote = false;

  cur  = buf;
  *cur = '\0';
  len  = -1;
  while( fgets( lineBuf, 256, is ) )
  {
    remEOL(lineBuf);
    if( isTitleLine( lineBuf ) )       /* section is ended */
      break;

    p = textPos( lineBuf, entry );     /* not equal this entry */
    if( p == 0 )
      continue;

    if( strip )
      quote = stripQuotationChar( p );

	if (quote == false)
		stripComment( p );

	rtrim(p);

    len = (int)strlen(p);
    if( bufSize-1 < len )
      len = bufSize-1;

    strncpy( cur, p, len );
    cur[len] = 0;
    break;
  }

  return len;
}


//================ util ===================================

static void removenl(char *buffer)
{
  for(;buffer[0];buffer++){
    if(buffer[0]=='\n')
    {
      buffer[0]=0;
      return;
    }
    if(buffer[0]=='\r')
    {
    	buffer[0]=0;
	return;
    }
  }
}

static void ltrim(char* buffer)
{
  char* tmp;

  for(tmp = buffer; tmp[0] && isspace(tmp[0]); tmp++);
  strcpy(buffer,tmp);
}
/*
void rtrim(char* buffer){
  char* tmp;

  for(tmp = buffer-1; buffer[0]; buffer++)
    if(!isspace(buffer[0]))
      tmp=buffer;
  tmp[1]=0;    
}
*/
static void rtrim(char *buf)
{
  char *p;

  p = buf+strlen(buf) - 1;
  while( isspace(*p) && p > buf ) *p-- = 0;
}

static void trim(char*buffer)
{
  ltrim(buffer);
  rtrim(buffer);
}

static int strnicmp(const char *s1, const char *s2, int n)
{
  if (n == 0)
    return 0;
  do
  {
    if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2++))
      return (int)tolower((unsigned char)*s1) - (int)tolower((unsigned char)*--s2);
    if (*s1++ == 0)
      break;
  } while (--n != 0);
  return 0;
}

static char *strupr(char *buffer)
{
	char *ptr;

	ptr = buffer;
	while(*ptr)
	{
		ptr[0] = (char) toupper(*ptr);
		ptr++;
	}
	return buffer;
}

static char * itoa(int n, char *buff, int base) {

   char t[100], *c=t, *f=buff;
   int d;
   char sign;
   int bit;

   if (base == 10) {
     if (n < 0) {
        *(f++) = '-';
        n = -n;
     }

   while ( n > 0) {
      d = n % base;
      *(c++) = d + '0';
      n = n / base;
   }
   
   }
   
   else {
	  if (base == 2) bit = 1;
      else if (base == 8) bit = 3;
      else if (base == 16) bit = 4;
      else {
		  buff[0]=0;
		  return buff;
	  }

	  while (n != 0) {
		 d = (n  & (base-1));
		 *(c++) = d < 10 ? d + '0' : d - 10 + 'A';
		 n = (unsigned int) n >> bit;
	  }

   }

   c--;

   while (c >= t) *(f++) = *(c--);
     
   if (buff == f)
	   *(f++) = '0';
   *f = '\0';
   return buff;
}

//==========================================================================
// This function is called back from MXOSERVER CORE
//==========================================================================

#ifdef NSK_ODBC_SRVR

bool DO_WouldLikeToExecuteForReprepare2(long stmtHandle
				, long *returnCode
				, long *sqlWarningOrErrorLength
				, char *&sqlWarningOrError);

bool rePrepare2WouldLikeToExecute(
				  long stmtHandle
				, int *returnCode
				, int *sqlWarningOrErrorLength
				, char *&sqlWarningOrError
)
{
	return DO_WouldLikeToExecuteForReprepare2(stmtHandle, returnCode, sqlWarningOrErrorLength, sqlWarningOrError);
}

#else

bool rePrepare2WouldLikeToExecute(
				  long stmtHandle
				, int *returnCode
				, int *sqlWarningOrErrorLength
				, char *&sqlWarningOrError
)
{
	
	return true;
}

#endif


#ifdef NSK_ODBC_SRVR

short qrysrvc_ExecuteFinished(
				  const char *stmtLabel
				, const long stmtHandle
				, const bool bCheckSqlQueryType
				, const short error_code
				, const bool bFetch
				, const bool bException
				, const bool bErase);

short qrysrvcExecuteFinished(
				  const char *stmtLabel
				, const long stmtHandle
				, const bool bCheckSqlQueryType
				, const short error_code
				, const bool bFetch
				, const bool bException = false
				, const bool bErase = true
)
{
	return qrysrvc_ExecuteFinished(
				  stmtLabel
				, stmtHandle
				, bCheckSqlQueryType
				, error_code
				, bFetch
				, bException
				, bErase);
}

#else
short qrysrvc_ExecuteFinished(
				  const char *stmtLabel
				, const long stmtHandle
				, const bool bCheckSqlQueryType
				, const short error_code
				, const bool bFetch
				, const bool bException
				, const bool bErase);

short qrysrvcExecuteFinished(
				  const char *stmtLabel
				, const long stmtHandle
				, const bool bCheckSqlQueryType
				, const short error_code
				, const bool bFetch
				, const bool bException = false	
				, const bool bErase = true
)
{

	return qrysrvc_ExecuteFinished(
				  stmtLabel
				, stmtHandle
				, bCheckSqlQueryType
				, error_code
				, bFetch
				, bException
				, bErase);
}

#endif
