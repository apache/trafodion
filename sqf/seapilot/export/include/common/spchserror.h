// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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

enum SPCHSERROR = {
  // Error numbers that apply to all health checks
  SPCHSERROR_OK = 0,
  SPCHSERROR_FAILURE = 1,
  SPCHSERROR_NON_CRITICAL_FAILURE = 2,

  // Error for no node id
  SPCHSERROR_NONODEID = 10,
  
  // Error numbers that are specific to individual health checks
  SPCHSERROR_SE_GETINTERNALLOG_OTHERFAILURE = 100, 
  SPCHSERROR_SE_GETINTERNALLOG_CONNECTIONTIMEOUT = 101, 
  SPCHSERROR_SE_GETLOG_OTHERFAILURE = 102,
  SPCHSERROR_SE_GETLOG_CONNECTIONTIMEOUT = 103,
  SPCHSERROR_SE_GETADDRESSINFO_OTHERFAILURE = 104,
  SPCHSERROR_SE_GETADDRESSINFO_CONNECTIONTIMEOUT = 105,
  SPCHSERROR_SE_CHECKSPACE_OTHERFAILURE = 110,
  SPCHSERROR_SE_CHECKSPACE_CONNECTIONTIMEOUT = 111,
  SPCHSERROR_SE_UNCONNECTEDNODE = 119
}
