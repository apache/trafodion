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
#include "tokenkey.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "openssl/evp.h"
#include "seabed/int/types.h"
#include "seabed/pctl.h"
#include "seabed/ms.h"
#include "seabed/fserr.h"

//-----------------------------------------------------------------------------
//  Structure used to describe layout of user token
//

class TokenKeyContents 
{
public:
   char ID[2];                             //Key identifier, binary values 3,4

   union 
   {
      struct {

          char processName[MS_MON_MAX_PROCESS_NAME];
          int nodeId;                     //Node Number
          int processId;                  //pin
          char rand[12];                  //random byte
      } token;
      struct {
          char b64[71];                   //base64 encoded                        
          char zero;
      } encoded;
   } u;
   void populate();
   void reset();
};

static int base64(
   const char *in, 
   size_t      inLen, 
   char        *out, 
   bool         enc);

extern "C" long random(void);
extern "C" void srandom(unsigned int);

#pragma page "TokenKey::IsA"
// *****************************************************************************
// *                                                                           *
// * Function: TokenKey::IsA                                                   *
// *                                                                           *
// *    Determines if a string matches the format of a token key.  The string  *
// * may be a token key, but no longer valid.                                  *
// *                                                                           *
// *****************************************************************************

bool TokenKey::IsA(const char * tokenKeyString)

{

// Verify caller has same size token as current implementation.
   if (tokenKeyString == NULL)
      return false;

   if (tokenKeyString[0] != USERTOKEN_ID_1 || 
       tokenKeyString[1] != USERTOKEN_ID_2)
      return false;
      
   return true;

}
//************************** End of TokenKey::IsA ******************************

#pragma page "void TokenKey::TokenKey"
// *****************************************************************************
// *                                                                           *
// * Function: TokenKey::TokenKey                                              *
// *    This function constructs a TokenKey object and its contents.           *
// *                                                                           *
// *****************************************************************************
TokenKey::TokenKey()
: self(*new TokenKeyContents())

{

  memset((char *)&self,'A',sizeof(TokenKeyContents));
  self.populate();

}
//************************ End of TokenKey::TokenKey ***************************


#pragma page "void TokenKey::TokenKey"
// *****************************************************************************
// *                                                                           *
// * Function: TokenKey::TokenKey                                              *
// *    This function constructs a TokenKey object and its contents.           *
// *                                                                           *
// *****************************************************************************
TokenKey::TokenKey(const char * tokenKeyString)
: self(*new TokenKeyContents())

{

   strncpy((char *)&self,tokenKeyString,sizeof(TokenKeyContents));
   
}
//************************ End of TokenKey::TokenKey ***************************


#pragma page "void Token::~Token"
// *****************************************************************************
// *                                                                           *
// * Function: Token::~Token                                                   *
// *    This function destroys a Token object and its contents.                *
// *                                                                           *
// *****************************************************************************
TokenKey::~TokenKey()

{

   delete &self;

}
//*************************** End of Token::~Token *****************************

#pragma page "void TokenKey::operator="
// *****************************************************************************
// *                                                                           *
// * Function: TokenKey::operator=                                             *
// *    This function assigns TokenKey contents.                               *
// *                                                                           *
// *****************************************************************************
TokenKey & TokenKey::operator=(const TokenKey &rhs)

{

   if (this == &rhs)
      return *this;

   memcpy((char *)&self,(char *)&rhs.self,sizeof(TokenKeyContents));
   
   return *this;

}
//*********************** End of TokenKey::operator= ***************************

#pragma page "void Token::operator=="
// *****************************************************************************
// *                                                                           *
// * Function: Token::operator==                                               *
// *    This function compares the contents of two TokenKey objects.           *
// *                                                                           *
// *****************************************************************************
bool TokenKey::operator==(const TokenKey &rhs) const

{

   if (this == &rhs)
      return true;

   return memcmp((char *)&self,(char *)&rhs.self,sizeof(TokenKeyContents)) == 0;

}
//*********************** End of TokenKey::operator== **************************


#pragma page "TokenKey::getTokenKey"
// *****************************************************************************
// *                                                                           *
// * Function: TokenKey::getTokenKey                                           *
// *    Returns the Token Key as an array of bytes.                            *
// *                                                                           *
// *****************************************************************************
const char * TokenKey::getTokenKey() const

{

   return (const char *)(&self);

}
//******************* End of TokenContents::getTokenKey ************************

#pragma page "TokenKey::getTokenKeyAsString"
// *****************************************************************************
// *                                                                           *
// * Function: TokenKey::getTokenKeyAsString                                   *
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
void TokenKey::getTokenKeyAsString(char *tokenKeyString) const

{

TokenKeyContents decoded; 

   base64((const char *) self.u.encoded.b64,sizeof(self.u.encoded.b64),
          (char *) decoded.u.encoded.b64,false);

int count = sprintf(tokenKeyString,
                    "ID = '0x%X,0x%X'\nNode ID = %d\nProcess ID = %d\n",
                    self.ID[0],self.ID[1],decoded.u.token.nodeId,
                    decoded.u.token.processId);

char *ptr = &tokenKeyString[count];

   strcpy(ptr,"Random value = 0x");
   ptr += strlen(ptr);

   for (size_t i = 0; i < sizeof(self.u.token.rand) / sizeof(self.u.token.rand[0]); i ++)
   {
      count = sprintf(ptr,"%X",decoded.u.token.rand[i]);
      ptr += count;
   }
   
   strcpy(ptr,"\nEncoded value = ");
   
   strcat(ptr,self.u.encoded.b64);
      
}
//*************** End of TokenContents::getTokenKeyAsString ********************



#pragma page "TokenKey::getTokenKeySize"
// *****************************************************************************
// *                                                                           *
// * Function: TokenKey::getTokenKeySize                                       *
// *    Returns the number of bytes in a Token Key.                            *
// *                                                                           *
// *****************************************************************************
size_t TokenKey::getTokenKeySize() const

{

   return sizeof(TokenKeyContents);

}
//****************** End of TokenContents::getTokenKeySize *********************


#pragma page "TokenKey::reset"
// *****************************************************************************
// *                                                                           *
// * Function: TokenKey::reset                                                 *
// *    Resets all the member values to their default.                         *
// *                                                                           *
// *****************************************************************************
void TokenKey::reset()

{

// Set/free data members as appropriate
// Use case is the parent MXOSRVR authenticating as different user.

// self.reset();

   self.populate();

// Token will get generated once the user is authenticated.

}
//*********************** End of TokenContents::reset **************************


                                       
#pragma page "TokenKey::verify"
// *****************************************************************************
// *                                                                           *
// * Function: TokenKey::verify                                                *
// *                                                                           *
// *    Determines if a token key is valid.                                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <tokenKey>                      TokenKey &                      In       *
// *    is the token key to be validated against.                              *
// *                                                                           *
// *****************************************************************************
bool TokenKey::verify(TokenKey &tokenKey) const

{

// Check to see that the process name matches with the node ID and process IDd

int retval = XZFIL_ERR_OK;
int nid, pid, decoded_nid, decoded_pid;

   if (!(*this == tokenKey))
      return false; 

//      decode the token to get the process info

TokenKeyContents decoded_token; 

   base64((const char *) self.u.encoded.b64,sizeof(self.u.encoded.b64),
          (char *) decoded_token.u.encoded.b64,false);

   retval = msg_mon_get_process_info(decoded_token.u.token.processName,&nid,&pid);
   decoded_nid = decoded_token.u.token.nodeId;
   decoded_pid = decoded_token.u.token.processId;

   if ((retval == XZFIL_ERR_OK) &&
       (nid == decoded_nid) &&
       (pid == decoded_pid))
      return true;

   return false; 
   
}                                          
//************************** End of TokenKey::verify ***************************
                                       
                                       
#pragma page "TokenKey::verifyParent"
// *****************************************************************************
// *                                                                           *
// * Function: TokenKey::verifyParent                                          *
// *                                                                           *
// *    Determines if a token key refers to our parent MXOSRVR, and if so,     *
// * fetches corresponding authentication black box.                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <blackBox>                      char *                          Out      *
// *    is the authentication data (black box) associated with this token.     *
// *                                                                           *
// *  <blackBoxLength>                size_t &                        Out      *
// *    is the number of bytes in <blackBox>.                                  *
// *                                                                           *
// *  <authFn>                        AuthFunction                    In       *
// *    is the function passed by MXOSRVR for verifying and getting            *
// *    the black box                                                          *
// *                                                                           *
// *                                                                           *
// *****************************************************************************

bool TokenKey::verifyParent(
   char          * blackBox,
   size_t        & blackBoxLength,
   AuthFunction    authFn) const

{

   blackBoxLength = 0;

TokenKeyContents decoded_token; 

// First decode the token to get the process name
   base64((const char *) self.u.encoded.b64,sizeof(self.u.encoded.b64),
          (char *) decoded_token.u.encoded.b64,false);

// Send message to MXOSRVR indicated in the token key. Called MXOSRVR presumably
// verifies we are a child MXOSRVR running as instance ID and either returns
// yes or no, and if yes, includes black box data.

// First get the verification function set by MXOSRVR
// This is set to the address of MXOSRVR_ValidateToken_pst_
// (odbc/nsksrvr/Interface/odbcmxo_drvr.cpp)

int error;

   if (authFn == NULL)
      return false;

   error = (*authFn)( decoded_token.u.token.processName,
                          sizeof(TokenKeyContents),
                          (unsigned char *)&self,
                          4096, // Max size of the BlackBox - need a define
                          (int *) &blackBoxLength,
                          (unsigned char *)blackBox );

   if (error || blackBoxLength == 0)
      return false;

   return true;
   
}                                          
//*********************** End of TokenKey::verifyParent ************************
                                       
                                       
                                          
#pragma page "TokenKeyContents::populate"
// *****************************************************************************
// *                                                                           *
// * Function: TokenKeyContents::populate                                      *
// *                                                                           *
// *    Populates fields of a token key using information from current         *
// *  process (node, PID, etc.) and encoding (base64).                         *
// *                                                                           *
// *****************************************************************************
void TokenKeyContents::populate()

{

   msg_mon_get_my_process_name(u.token.processName, MS_MON_MAX_PROCESS_NAME);
   msg_mon_get_process_info( NULL, &u.token.nodeId, &u.token.processId );
   ID[0] = USERTOKEN_ID_1;
   ID[1] = USERTOKEN_ID_2;

// Generate Random bytes

   srandom ((unsigned)time(NULL));

   for (int i=0; i < sizeof(u.token.rand); i++)
       u.token.rand[i] = (char)(random() & 0xFF);

// Base 64 encoding

char buf[sizeof(u)]= {0};

   base64((const char *)&u.token, sizeof(u.token), buf, true);
   memcpy(u.encoded.b64, buf, sizeof(u.encoded.b64));
   u.encoded.zero='\0';

}                                          

//********************* End of TokenKeyContents::populate **********************
                                       

                                       
                                          
#pragma page "TokenKeyContents::reset"
// *****************************************************************************
// *                                                                           *
// * Function: TokenKeyContents::reset                                         *
// *                                                                           *
// *  process (node, PID, etc.) and encoding (base64).                         *
// *                                                                           *
// *****************************************************************************
void TokenKeyContents::reset()

{

}                                          
//********************** End of TokenKeyContents::reset ************************
                                       

                                       

#pragma page "base64"
// *****************************************************************************
// *                                                                           *
// * Function: base64                                                          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <in>                            const char *                    In       *
// *    is the buffer to be encoded or decoded.                                *
// *                                                                           *
// *  <inLen>                         size_t                          In       *
// *    is the length (in bytes) of the buffer to be encoded or decoded.       *
// *                                                                           *
// *  <out>                           char *                          Out      *
// *    returns the result of the encoding or decoding.                        *
// *                                                                           *
// *  <env>                           bool                            In       *
// *    is true if encoding and false if decoding.                             *
// *                                                                           *
// *****************************************************************************
static int base64(
   const char *in, 
   size_t      inLen, 
   char        *out, 
   bool         enc)
   
{

int len=0;
BIO *bio, *b64;
char *ptr, *buf;

   if (enc)
   {
      // encode to base64 characters
      b64 = BIO_new(BIO_f_base64());
      bio = BIO_new(BIO_s_mem());
      bio = BIO_push(b64, bio);
      BIO_write(bio, in, inLen);
      BIO_flush(bio);

      BIO_get_mem_data(bio, &ptr);
      while (*ptr != '\n')
      {
         *out++ = *ptr++;
         len++;
      }
      *out = '\0';

      BIO_free_all(bio);
      return len;
   }

// decode from base64 characters
   buf = (char *)malloc(inLen + 1);
   memcpy(buf, in, inLen);
   buf[inLen] = '\n';
   b64 = BIO_new(BIO_f_base64());
   bio = BIO_new_mem_buf(buf, inLen+1);
   bio = BIO_push(b64, bio);
   len = BIO_read(bio, out, inLen);

   BIO_free_all(bio);
   free(buf);
   return len;
}
//******************************* End of base64 ********************************

