//******************************************************************************
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
//******************************************************************************
#include "token.h"
#include "tokenkey.h"
#include <stddef.h>
#include <string.h>

class TokenContents {
public:
   TokenContents();
   void reset();
   TokenKey        tokenKey;
   size_t          dataLength;
   char           *data;
};

static Token * me = NULL; 

#pragma page "Token::Obtain"
// *****************************************************************************
// *                                                                           *
// * Function: Token::Obtain                                                   *
// *    Constructs or returns the singleton instance of a Token object.        *
// *                                                                           *
// *****************************************************************************

Token * Token::Obtain()

{

   if (me != NULL)
      return me;
 
   try
   {
      me = new Token();
  //    if (me == NULL)
  //       throw exception();   // Memory exception
      if (me == NULL)
         return NULL;
                            
      return me;
   }
   catch ( ... )
   {
      throw;
   }

}
//************************** End of Token::Obtain ******************************


#pragma page "Token::Verify (local only)"
// *****************************************************************************
// *                                                                           *
// * Function: Token::Verify                                                   *
// *                                                                           *
// *    Determines if a token key string represents a valid token key in the   *
// *  context of this process.  If so, a token is returned.  Otherwise, NULL   *
// *  is returned.                                                             *
// *                                                                           *
// *     This function is called when a parent mxosrvr needs to verify         *
// *  that the token passed by child mxosrvr is valid                          *
// *                                                                           *
// *     This function is also called during CLI authentication                *
// *                                                                           *
// *****************************************************************************

Token * Token::Verify(const char * tokenKeyString)

{

   if (tokenKeyString == NULL)
      return NULL;  
 
//
// Is the string in the format of a token key?  
            
   if (!TokenKey::IsA(tokenKeyString))
      return NULL;


// *****************************************************************************
// *                                                                           *
// *   OK, it *looks* like a Token Key.                                        *
// *                                                                           *
// *   Check to see if it matches the local MXOSRVR                            *
// *                                                                           *
// *****************************************************************************

TokenKey tokenKey(tokenKeyString);

//
// First, determine if this is a token key for this MXOSRVR
//
  
   if (me != NULL)
   {
      // Token key refers to this MXOSRVR process and is a valid token key.
      // Return pointer to the singleton instance of Token.
      if (tokenKey.verify(me->self.tokenKey))
         return me;

      return NULL;
   }

   return me;
}
//*************************** End of Token::Verify (local only) ****************
    
#pragma page "Token::Verify"
// *****************************************************************************
// *                                                                           *
// * Function: Token::Verify                                                   *
// *                                                                           *
// *    Determines if a token key string represents a valid token key in the   *
// *  context of this process.  If so, a token is returned.  Otherwise, NULL   *
// *  is returned.                                                             *
// *                                                                           *
// *     This function is called during MXOSRVR authentication                 *
// *  The credentials could be a token if the MXOSRVR has been invoked by      *
// *  another program, which itself received the token from a parent MXOSRVR.  *
// *                                                                           *
// *****************************************************************************

Token * Token::Verify(const char * tokenKeyString, AuthFunction authFn)

{

   if (tokenKeyString == NULL)
      return NULL;  
 
//
// Is the string in the format of a token key?  If not, this is probably the
// initial call and the string contains the user password.  This probably not
// an error; caller will proceed with authenticating user normally. 
            
   if (!TokenKey::IsA(tokenKeyString))
      return NULL;


// *****************************************************************************
// *                                                                           *
// *   OK, it *looks* like a Token Key.                                        *
// *                                                                           *
// *  There are three possibilities:                                           *
// *                                                                           *
// *     1) Token key is local to this MXOSRVR                                 *
// *     2) Token key refers to a parent MXOSRVR                               *
// *     3) Token key is not valid                                             *
// *                                                                           *
// *****************************************************************************

TokenKey tokenKey(tokenKeyString);

//
// First, determine if this is a token key for this MXOSRVR
//
  
   if (me != NULL)
   {
      // Token key refers to this MXOSRVR process and is a valid token key.
      // Return pointer to the singleton instance of Token.
      if (tokenKey.verify(me->self.tokenKey))
         return me;
   }
    
// Token key not local.  See if token refers to another MXOSRVR, which could  
// be our parent MXOSRVR.  Fetch token from the specified MXOSRVR in the token  
// key.  (Note, MXOSRVR should only accept the fetch token request from the 
// instance ID.
// If unable to contact parent MXOSRVR or parent MXOSRVR says token key is no
// good, return NULL (token key verification failed).

char blackBox[4096];  // Guess for now on maximum
size_t blackBoxLength;

   if (!tokenKey.verifyParent(blackBox,blackBoxLength,authFn))
   {
      return NULL;      
   }

   if (me != NULL)
      delete me;
      
   me = new Token(tokenKey);
            
   me->setData(blackBox,blackBoxLength);
         
// We now have a local copy of Token   
         
   return me; 
      
}
//*************************** End of Token::Verify *****************************



#pragma page "void Token::Token"
// *****************************************************************************
// *                                                                           *
// * Function: Token::Token                                                    *
// *    This function constructs a Token object and its contents.              *
// *                                                                           *
// *****************************************************************************
Token::Token()
: self(*new TokenContents())

{

}
//*************************** End of Token::Token ******************************


#pragma page "void Token::Token"
// *****************************************************************************
// *                                                                           *
// * Function: Token::Token                                                    *
// *    This function constructs a Token object and its contents.              *
// *                                                                           *
// *****************************************************************************
Token::Token(TokenKey &tokenKey)
: self(*new TokenContents())

{

   self.tokenKey = tokenKey; 

}
//*************************** End of Token::Token ******************************


#pragma page "void Token::~Token"
// *****************************************************************************
// *                                                                           *
// * Function: Token::~Token                                                   *
// *    This function destroys a Token object and its contents.                *
// *                                                                           *
// *****************************************************************************
Token::~Token()

{

   delete &self;

}
//*************************** End of Token::~Token *****************************



#pragma page "Token::getData"
// *****************************************************************************
// *                                                                           *
// * Function: Token::getData                                                  *
// *                                                                           *
// *    Gets authentication data (black box) associated with a token.          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <data>                          char *                          Out      *
// *    is the authentication data (black box) associated with a token.        *
// *                                                                           *
// *****************************************************************************
void Token::getData(char *data) const  

{

   memcpy(data,self.data,self.dataLength);

}                                          
//***************************** End of Token:getData ***************************
      
      
#pragma page "Token::getDataSize"
// *****************************************************************************
// *                                                                           *
// * Function: Token::getDataSize                                              *
// *                                                                           *
// *    Returns the size of the authentication data (black box) associated     *
// * with a token.                                                             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <data>                          char *                          Out      *
// *    is the LDAP username.                                                  *
// *                                                                           *
// *  <length>                        size_t &                        Out      *
// *    is the number of bytes in <data>.                                      *
// *                                                                           *
// *****************************************************************************
size_t Token::getDataSize() const   

{

   return self.dataLength;

}                                          
//************************** End of Token:getDataSize **************************
      
      
#pragma page "Token::getTokenKey"
// *****************************************************************************
// *                                                                           *
// * Function: Token::getTokenKey                                              *
// *                                                                           *
// *    Gets the token key for this token.                                     *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <tokenKey>                      char *                          Out      *
// *    is the token key for this token.                                       *
// *                                                                           *
// *****************************************************************************
void Token::getTokenKey(char *tokenKey) const  

{

   memcpy(tokenKey,self.tokenKey.getTokenKey(),self.tokenKey.getTokenKeySize());

}                                          
//************************** End of Token:getTokenKey **************************
      
      
#pragma page "Token::getTokenKeyAsString"
// *****************************************************************************
// *                                                                           *
// * Function: Token::getTokenKeyAsString                                      *
// *    Returns the Token Key as a formatted ASCII string.                     *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <tokenKeyString>                char *                          Out      *
// *    returns the Token Key as a formatted ASCII string.                     *
// *                                                                           *
// *****************************************************************************
void Token::getTokenKeyAsString(char *tokenKeyString) const

{

   self.tokenKey.getTokenKeyAsString(tokenKeyString);
   
}
//******************* End of Token::getTokenKeyAsString ************************
      
      
      
#pragma page "Token::getTokenKeySize"
// *****************************************************************************
// *                                                                           *
// * Function: Token::getTokenKeySize                                          *
// *                                                                           *
// *    Returns the token key size.                                            *
// *                                                                           *
// *****************************************************************************
size_t Token::getTokenKeySize() const  

{

   return self.tokenKey.getTokenKeySize();

}                                          
//*********************** End of Token:getTokenKeySize *************************
      
      
#pragma page "Token::reset"
// *****************************************************************************
// *                                                                           *
// * Function: Token::reset                                                    *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
void Token::reset()   

{
   
   self.reset();

}                                          
//***************************** End of Token:reset *****************************
      
                                       
#pragma page "Token::setData"
// *****************************************************************************
// *                                                                           *
// * Function: Token::setData                                                  *
// *                                                                           *
// *    Sets authentication data (black box) associated with a token.          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <data>                          const char *                    In       *
// *    is the authentication data (black box) associated with this token.     *
// *                                                                           *
// *  <length>                        size_t                          In       *
// *    is the number of bytes in <data>.                                      *
// *                                                                           *
// *****************************************************************************
void Token::setData(
   const char       *data,
   size_t            length)   

{

   if (self.data)
      delete [] self.data;
      
   self.data = new char[length];

   memcpy(self.data,data,length);
   self.dataLength = length;
   
}                                          
//***************************** End of Token:setData ***************************
                                       
                                       
#pragma page "TokenContents::TokenContents"
// *****************************************************************************
// *                                                                           *
// * Function: TokenContents::TokenContents                                    *
// *                                                                           *
// *****************************************************************************
TokenContents::TokenContents()

{

   data = NULL;
   dataLength = 0;
     
}
//****************** End of TokenContents::TokenContents ***********************
   

#pragma page "TokenContents::reset"
// *****************************************************************************
// *                                                                           *
// * Function: TokenContents::reset                                            *
// *    Resets all the member values to their default.                         *
// *                                                                           *
// *****************************************************************************
void TokenContents::reset()

{

   tokenKey.reset();

   if (data)
      delete [] data;
   
   dataLength = 0;
   data = NULL;
    
}
//*********************** End of TokenContents::reset **************************
                                          

