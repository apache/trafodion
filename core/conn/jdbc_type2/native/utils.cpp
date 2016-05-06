/**************************************************************************
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
**************************************************************************/

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
