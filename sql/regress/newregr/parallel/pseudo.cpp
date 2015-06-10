/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1999-2015 Hewlett-Packard Development Company, L.P.
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
**********************************************************************/
#include "Pseudo.h"

/****************************************************************/
/*                                                              */
/*   PseudoBase::rand15()                                       */
/*                                                              */
/*   Generates a unique random number in [1..limit_]            */
/*   up to a maximum limit of 15000, by generating all          */
/*   elements in the Galios field Z(17383) under multiplication */
/*   All elements above limit are discarded, note that 17383    */
/*   is a prime number.                                         */
/*                                                              */
/*                                                              */
/*                                                              */
/****************************************************************/
long  PseudoBase::rand15()
{
  do {
       seed_ = (3167 * seed_) % 17383;
     }  while (seed_ > limit_);

  return(seed_);
}


//<pb>
//=============================================================================
//  Return next pseudo random integer.
//
// Input:
//  none
//
// Output:
//  none
//
// Return:
//  A pseudo random integer
//
// Exceptions:
//  none
//
//=============================================================================
long
PseudoInt::getNextPseudo()
{
  long randomNum;
 
  randomNum = rand15() - 1;

  return randomNum % modulus_;

} // PseudoInt::getNextPseudo()

//<pb>
//=============================================================================
//  Return next pseudo random integer in key order.
//
// Input:
//  none
//
// Output:
//  none
//
// Return:
//  A pseudo random integer in key order
//
// Exceptions:
//  none
//
//=============================================================================
long
PseudoIntKey::getNextPseudo()
{
  long curValue = nextValue_;
  ++nextValue_;
  if (nextValue_ >= range_)
  {
    nextValue_ = 0;
  }

  return curValue;

} // PseudoIntKey::getNextPseudo()

//<pb>
//=============================================================================
//  Return next pseudo random, null terminated character array.
//
// Input:
//  pCharArray -- a null terminated character array initialized with all 'A'
//                  values
//
// Output:
//  pCharArray -- a pseudo random, null terminated character array
//
// Return:
//  none
//
// Exceptions:
//  none
//
//=============================================================================
void
PseudoChar::getNextPseudo(char* pCharArray)
{
  long randomNum;
 
  randomNum = rand15() - 1;

  pCharArray[0] = (char)('A' + (randomNum % col0Modulus_));
  pCharArray[1] = (char)('A' + (randomNum % col1Modulus_));

} // PseudoChar::getNextPseudo()

//<pb>
//=============================================================================
//  Return next pseudo random, null terminated character array in key order.
//
// Input:
//  pCharArray -- a null terminated character array initialized with all 'A'
//                  values
//
// Output:
//  pCharArray -- a pseudo random, null terminated character array
//
// Return:
//  none
//
// Exceptions:
//  none
//
//=============================================================================
void
PseudoCharKey::getNextPseudo(char* pCharArray)
{
  long curCol1Val = col1NextVal_;
  ++col1NextVal_;
  if (col1NextVal_ >= col1Range_)
    col1NextVal_ = 0;

  long curCol0Val = col0NextVal_;
  if (col1NextVal_ == 0)
    ++col0NextVal_;
  if (col0NextVal_ >= col0Range_)
    col0NextVal_ = 0;
  
  pCharArray[0] = (char)('A' + curCol0Val);
  pCharArray[1] = (char)('A' + curCol1Val);
 
} // PseudoCharKey::getNextPseudo()

//<pb>
//=============================================================================
//  Return next pseudo random date as a 64 bit Julian timestamp.
//
// Input:
//  pBaseDateTime -- a starting Julian timestamp from which pseudo random 
//                     dates are based
//
// Output:
//  pBaseDateTime -- a pseudo random date as a 64 bit Julian timestamp
//
// Return:
//  none
//
// Exceptions:
//  none
//
//=============================================================================
void
PseudoDate::getNextPseudo(__int64& pBaseDateTime)
{
  long randomNum;
 
  randomNum = rand15() - 1;

  pBaseDateTime = pBaseDateTime + ((randomNum % modulus_) * 86400000000);

} // PseudoDate::getNextPseudo()

//<pb>
//=============================================================================
//  Return next pseudo random date (as a 64 bit Julian timestamp) in key order.
//
// Input:
//  pBaseDateTime -- a starting Julian timestamp from which pseudo random 
//                     dates are based
//
// Output:
//  pBaseDateTime -- a pseudo random date as a 64 bit Julian timestamp
//
// Return:
//  none
//
// Exceptions:
//  none
//
//
//=============================================================================
void
PseudoDateKey::getNextPseudo(__int64& pBaseDateTime)
{
  long curValue = nextValue_;
  ++nextValue_;
  if (nextValue_ >= range_)
  {
    nextValue_ = 0;
  }

  pBaseDateTime = pBaseDateTime + (curValue * 86400000000);

} // PseudoDateKey::getNextPseudo()

//<pb>
/****************************************************************/
/*                                                              */
/*   PseudoFixedPoint::reverse()                                */
/*       Reverses the parameter string in place.                */
/*   The function accepts one parameter:                        */
/*                                                              */
/*   s: string to be reversed.                                  */
/*                                                              */
/*   called by: ltoa                                            */
/*                                                              */
/****************************************************************/
void
PseudoFixedPoint::reverse(char* s)
{
  char c;
  long i, j;

  for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
  {
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  }
}

//<pb>
/****************************************************************/
/*                                                              */
/*   PseudoFixedPoint::ltoa()                                   */
/*                                                              */
/*       converts a long integer to a character string.         */
/*   The function accepts two parameters:                       */
/*                                                              */
/*   n: number to be converted.                                 */
/*   s: string to hold converted number                         */
/*                                                              */
/*   functions called: reverse                                  */
/*                                                              */
/****************************************************************/
void
PseudoFixedPoint::ltoa(long n,char* s)
{
  long i, sign;

  if ( (sign = n) < 0)   /* record sign */
    n = -n;             /* make n positive */

  i = 0;
  do {      /* generate digits in reverse order */
      s[i++] = n % 10 + '0';       /* pick off LSD */
    }  while ( (n /= 10) > 0 );    /* truncate LSD from number */

  if (sign < 0)   /* if number was negative, make it negative again */
    s[i++] = '-';
  s[i] = '\0';    /* add null byte */
  reverse(s);     /* string needs to be reversed, since digits were */
                  /* generated in reverse order */

}

//<pb>
/**********************************************************************/
/*                                                                    */
/*   PseudoFixedPoint::zeropad()                                      */
/*                                                                    */
/*       Pads string arrays, used to store the scale part of          */
/*   fixed point scale values with leading character 0's, up to the   */
/*   required precision.                                              */
/*                                                                    */
/*       The function accepts two parameters:                         */
/*                                                                    */
/*   s: string to be zero padded.                                     */
/*   precision:  required precision of the numeric character string.  */
/*                                                                    */
/*       String s is zero-padded in place.                            */
/*                                                                    */
/**********************************************************************/
void
PseudoFixedPoint::zeropad(char* s,long precision)
{
  long i = 0;
  long j,offset;

  while (s[i] != '\0') /* find end of string array s */
    i++;

  if (i > precision) /* error - string length must be less than or */
  {                /* equal to the declared precision */
    printf("datetime value exceeded the allowed precision");
    exit(10);
  }

  if (i < precision) /* need to zero pad */
  {
    offset = precision - i;  /* compute number of 0's to add */
    for (j = precision; i >= 0; i--,j--)  /* right shift string */
      s[j] = s[i];
    for (i= 0; i < offset; i++)  /* add required number of 0's */
      s[i] = '0';
  } /* end if string needs to be zero padded */

} /* end zeropad */


//<pb>
//=============================================================================
//  Return next pseudo random, fixed point value represented as a null
// terminated character array.
//
// Input:
//  none
//
// Output:
//  pCharArray -- a pseudo random, fixed point value represented as a null
//                  terminated character array
//
// Return:
//  none
//
// Exceptions:
//  none
//
//=============================================================================
void
PseudoFixedPoint::getNextPseudo(char* pCharArray)
{
  long mag,scale;
  char charMag[9],charScale[3];

  long randomNum;
  randomNum = rand15() - 1;

  mag = (randomNum % modulus_) / 100;
  scale = (randomNum % modulus_) % 100;
  ltoa(mag,charMag);
  ltoa(scale,charScale);
  zeropad(charScale,2);
  strcpy(pCharArray,charMag);
  strcat(pCharArray,".");
  strcat(pCharArray,charScale);

} // PseudoFixedPoint::getNextPseudo()

