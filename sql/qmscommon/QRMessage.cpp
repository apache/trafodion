// **********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2014 Hewlett-Packard Development Company, L.P.
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
// **********************************************************************

#include "string.h"
#include "QRMessage.h"
#include "QRLogger.h"

using namespace QR;

const char* const REQUEST_COMMENT_DELIM = "#";

static const QRMessage::ReqNames reqNamesArray_[] =
{
  { INITIALIZE_REQUEST, "INITIALIZE"},
  { ALLOCATE_REQUEST,   "ALLOCATE"  },
  { PUBLISH_REQUEST,    "PUBLISH"   },
  { MATCH_REQUEST,      "MATCH"     },
  { CHECK_REQUEST,      "CHECK"     },
  { CLEANUP_REQUEST,    "CLEANUP"   },
  { DEFAULTS_REQUEST,   "DEFAULTS"  },
  { COMMENT_REQUEST,    "COMMENT"   },
  { WORKLOAD_REQUEST,   "WORKLOAD"  },
  { ERROR_REQUEST,      "Invalid"   }
};

QRMessageTypeEnum QRMessage::resolveRequestName(char* name)
{
  // Check for comment first. Treat as a string instead of single character in
  // case the delimiter is changed to >1 char.
  if (!strncmp(name, REQUEST_COMMENT_DELIM, strlen(REQUEST_COMMENT_DELIM)))
    return COMMENT_REQUEST;

  for (Int32 i=0; i<sizeof reqNamesArray_ / sizeof reqNamesArray_[0]; i++)
  {
    if (!strcmp(name, reqNamesArray_[i].name))
      return reqNamesArray_[i].type;
  }

  return ERROR_REQUEST;
}

const char* QRMessage::getRequestName(QRMessageTypeEnum type)
{
  Int32 index = type - IPC_MSG_QR_FIRST - 1;
  // Verify that the array contents is in sync with its index.
  assertLogAndThrow(CAT_SQL_COMP_QR_IPC, LL_MVQR_FAIL,
                    reqNamesArray_[index].type == type, QRLogicException, 
		    "reqNamesArray_ is out of sync.");

  return reqNamesArray_[index].name;
}
