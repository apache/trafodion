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

#ifndef TMMSG_H_
#define TMMSG_H_
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "tmlibmsg.h"

const int32 NULL_MSGID = 0;

// EID must be a maximum of EAD_SIZE characters
//                              123456789.12
const char EID_CTmMessage[] = {"CTmMessage_"}; 

class CTmMessage
{
protected: 
   char ia_EID[EID_SIZE+1];
   Tm_Req_Msg_Type      iv_req;

public:
   CTmMessage(Tm_Req_Msg_Type * pp_req)
   {
      if (!pp_req)
         abort();
      memset(&iv_req, 0, sizeof(Tm_Req_Msg_Type));
      memcpy(&iv_req, pp_req, sizeof(Tm_Req_Msg_Type));
      memset(ia_EID, 0, EID_SIZE+1);
      //set_EID();
   }
   CTmMessage(short pv_reqType)
   {
      memset(&iv_req, 0, sizeof(Tm_Req_Msg_Type));
      requestType(pv_reqType);
      memset(ia_EID, 0, EID_SIZE+1);
      //set_EID();
   }
   CTmMessage(CTmMessage *pp_msg)
   {
      memcpy(&iv_req, pp_msg->request(), sizeof(iv_req));
      memset(ia_EID, 0, EID_SIZE+1);
      //set_EID();
   }
   ~CTmMessage() {}

   Tm_Req_Msg_Type * request() {return &iv_req;}

   short requestType() {return request()->iv_msg_hdr.rr_type.request_type;}
   void requestType(short pv_reqType) {request()->iv_msg_hdr.rr_type.request_type = pv_reqType;}

   void set_EID() 
   {
       strcpy((char *) &ia_EID, (char *) EID_CTmMessage);
   }
    void validate()
    {
       if (strcmp((char *) &ia_EID, (char *) &EID_CTmMessage) != 0)
       {
          ;// abort();
       }
    } //validate
};

#endif //TMMSG_H_
