/**********************************************************************
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
**********************************************************************/


#include "NLSConversion.h"
#include "charinfo.h"
#include "nawstring.h"

NAWString *charToUnicode(Lng32 charset, const char *s, Int32 len, CollHeap *h)
{
  NAString str(s, len);

  NAWString *ws = NULL;
  if ( h )
    ws = new (h) NAWString(charset, str.data(), len, h);
  else
    ws = new NAWString(charset, str.data(), len);

  return ws;
}


NAString* 
unicodeToChar(const NAWchar *wstr, Int32 wlen, Lng32 charset, CollHeap *h, NABoolean allowInvalidChar)
{
  Int32 slen;
  switch ( charset ) {
   case CharInfo::ISO88591:
   case CharInfo::SJIS:
   case CharInfo::EUCJP:
   case CharInfo::GB18030:
   case CharInfo::GB2312:
   case CharInfo::GBK:
   case CharInfo::KSC5601:
   case CharInfo::BIG5:
   case CharInfo::UTF8:
     slen = CharInfo::maxBytesPerChar((CharInfo::CharSet)charset)*wlen;
     break;
   default:
     return NULL; //We should never get here (unless a new charset is only partially implemented.)
     break;
  }
  char* target = NULL;

  if ( h ) {
    target = new (h) char[slen];
    if ( target == NULL ) return 0;
  } else
    target = new char[slen];

  slen = UnicodeStringToLocale(charset, wstr, wlen, target, slen, FALSE, allowInvalidChar);

  NAString *res = NULL;

  if ( h ) {
     // Only create a return NAString if the conversion was successful.
     if (slen != 0 || wlen == 0)
        res = new (h) NAString(target, slen, h);
     NADELETEBASIC(target, h);
  }
  else {
     // Only create a return NAString if the conversion was successful.
     if (slen != 0 || wlen == 0)
       res = new NAString(target, slen);
     delete [] target;
  }

  return res ;
}

NAString *charToChar(Lng32 targetCS, const char *s, Int32 sLenInBytes, Lng32 sourceCS, 
                     NAMemory *h /* = NULL */, NABoolean allowInvalidChar /* = FALSE */)
{
  NAString *res = NULL;
  if (s == NULL || sourceCS == (Lng32)CharInfo::UnknownCharSet || targetCS == (Lng32)CharInfo::UnknownCharSet)
  {
    return NULL; // error
  }
  if (sLenInBytes == 0)
  {
    if (h)
      res = new (h) NAString(h); // empty string
    else
      res = new NAString;
    return res;
  }
  if (targetCS == sourceCS)
  {
    if (h)
      res = new (h) NAString(s, sLenInBytes, h); // deep copy
    else
      res = new NAString(s, sLenInBytes); // deep copy

    return res;
  }

  // targetCS != sourceCS

  if ((CharInfo::CharSet)sourceCS == CharInfo::UCS2)
  {
    res = unicodeToChar ( (const NAWchar *)s              // source string
                        , sLenInBytes / BYTES_PER_NAWCHAR // src len in NAWchars
                        , targetCS
                        , h
                        , allowInvalidChar
                        );
    return res;
  }

  // sourceCS != CharInfo::UCS2

  NAWString * wstr = charToUnicode ( sourceCS     // src char set
                                   , s            // src str
                                   , sLenInBytes  // src str len in bytes
                                   , h            // heap for allocated target str
                                   );
  if (wstr == NULL) // conversion failed
  {
    return NULL; // error
  }
  if ((CharInfo::CharSet)targetCS == CharInfo::UCS2)
  {
    if (h)
      res = new (h) NAString ( (const char *)wstr->data()         // source string
                             , wstr->length() * BYTES_PER_NAWCHAR // source len in bytes
                             , h
                             );
    else
      res = new NAString ( (const char *)wstr->data()         // source string
                         , wstr->length() * BYTES_PER_NAWCHAR // source len in bytes
                         );

    delete wstr;
    return res;
  }

  // targetCS != CharInfo::UCS2
  
  res = unicodeToChar ( wstr->data()
                      , wstr->length() // in NAWchars
                      , targetCS
                      , h
                      , allowInvalidChar
                      );
  delete wstr;
  return res;
}

