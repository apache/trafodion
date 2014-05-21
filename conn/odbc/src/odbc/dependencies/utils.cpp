// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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
#include <platform_ndcs.h>

// String pointed to by parameter "str" is converted to upper case
// characters.
char * _strupr(char * str) 
{

	char *s;
    for (s=str ; *s != '\0'; s++)  *s = (char) toupper(*s);
  
	return str;
}


// Convert an integer to its string representation.
char * _itoa(int n, char *buff, int base) {

   char t[100], *c=t, *f=buff;
   int d;
   char sign;
   int bit;
   unsigned int un;

   if (base == 10) {
		if (n < 0) {
			*(f++) = '-';
			un = -n;
		} else
			un = n;

	   while ( un > 0) {
			d = un % base;
			*(c++) = (char)(d + '0');
			un = un / base;
	   }
   } else {
	  if (base == 2) bit = 1;
      else if (base == 8) bit = 3;
      else if (base == 16) bit = 4;
      else 
		  return (char *) "";

	  while (n != 0) {
		 d = (n  & (base-1));
		 *(c++) = (char)(d < 10 ? d + '0' : d - 10 + 'A');
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

// Convert a long to its string representation.
char * _ltoa(long n, char *buff, int base) 
{

   char t[100], *c=t, *f=buff;
   long d;
   char sign;
   int bit;
   unsigned long un;

   if (base == 10) {
     if (n < 0) {
        *(f++) = '-';
        un = -n;
	 }
	 else
		 un = n;

     while (un > 0) {
        d = un % base;
        *(c++) = (char)(d + '0');
        un = un / base;
	 }
   }
   
   else {

     if (base == 2) bit = 1;
     else if (base == 8) bit = 3;
     else if (base == 16) bit = 4;
     else 
	 { base = 16; bit = 4; }  // printf("Base value unknown!\n");

     while (n != 0) {
        d = (n  & (base-1));
        *(c++) = (char)(d < 10 ? d + '0' : d - 10 + 'A');
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

// Convert a string representation of a number to a 64-bit integer
// and return the result.
__int64 _atoi64( const char *s )
{
	__int64 n = 0;
	char* t = (char*)s;
	char c;

	while(*t != 0)
	{
		c = *t++;
		if (c < '0' || c > '9') continue;
		n = n * 10 +c - '0';
	}
	if (*s == '-') n = -n;
	return n;
}

char* trim(char *string)
{
	char sep[] = " ";
	char *token;
	char *assembledStr;
	char* saveptr;

	assembledStr = (char*)malloc( strlen(string) + 1);
	if (assembledStr == NULL ) return string;
	assembledStr[0]=0;

	token = strtok_r( string, sep, &saveptr );
	while( token != NULL )   {
	  strcat(assembledStr, token); 
	  token = strtok_r( NULL, sep, &saveptr  );
	  if(token != NULL)
		strcat(assembledStr, sep);
	  }
	strcpy( string, assembledStr);
	free( assembledStr);
	return string;
}

void listAllocatedMemory(char* description)
{
#ifdef TRACE_MEMORY_LEAK
	memList.list_node(description);
#endif
}

void markNOperator(char* file,const char* function,long line)
{
#ifdef TRACE_MEMORY_LEAK
	strncpy(gFile,file,50);
	strncpy(gFunction,function,50);
	gLine=line;
#endif
}
