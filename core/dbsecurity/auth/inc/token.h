#ifndef _TOKEN_H
#define _TOKEN_H
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
#include "auth.h"
#include <stddef.h>

class TokenContents;
class TokenKey;

class Token 
{
public:
   static Token * Obtain();
   
   static Token * Verify(const char *tokenKey);

   static Token * Verify(const char *tokenKey, AuthFunction authFn);
   
   void getData(char *data) const;
   
   size_t getDataSize() const;
   
   void getTokenKey(char *tokenKey) const;
     
   void getTokenKeyAsString(char *tokenKeyString) const;
   
   size_t getTokenKeySize() const;  
   
   void reset();
   
   void setData(
      const char *data,
      size_t      length);  

private:

   TokenContents &self;

   Token();
   Token(TokenKey &tokenKey);
   
   ~Token();
   Token(Token &);
   
};

#endif /* _TOKEN_H */
