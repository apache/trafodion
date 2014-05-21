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

enum SPOBJECTTYPE
{
    SPOBJECTTYPE_NULL = 0
  , SPOBJECTTYPE_UNKNOWN = 1

  /* Hardware-oriented objects */
  , SPOBJECTTYPE_PHYSICALNODE = 101
  , SPOBJECTTYPE_PHYSICALCORE = 102
  , SPOBJECTTYPE_ADAPTER = 103
  , SPOBJECTTYPE_CONTROLLER = 104
  , SPOBJECTTYPE_DEVICE = 105
  , SPOBJECTTYPE_DISK = 106
  , SPOBJECTTYPE_NETWORK = 107
  , SPOBJECTTYPE_PORT = 108

  /* OS-oriented objects */
  , SPOBJECTTYPE_PROCESS = 201
  , SPOBJECTTYPE_CONNECTION = 202
  , SPOBJECTTYPE_APPLICATION = 203
  , SPOBJECTTYPE_DOMAIN = 204
  , SPOBJECTTYPE_ENDPOINT = 205
  , SPOBJECTTYPE_FILE = 206
  , SPOBJECTTYPE_FILESYSTEM = 207
  , SPOBJECTTYPE_GROUP = 208
  , SPOBJECTTYPE_USER = 209
  , SPOBJECTTYPE_SERVICE = 210
  , SPOBJECTTYPE_SUBNET = 211
  , SPOBJECTTYPE_TRAPDEST = 212
  , SPOBJECTTYPE_SESSION = 213

  /* Foundation-oriented objects */
  , SPOBJECTTYPE_LOGICALNODE = 301
  , SPOBJECTTYPE_MONITOR = 302
  , SPOBJECTTYPE_SE = 303
  , SPOBJECTTYPE_INSTANCE = 304
  , SPOBJECTTYPE_TRANSACTION = 305
  , SPOBJECTTYPE_MANAGER = 306
  , SPOBJECTTYPE_AUDITTRAIL = 307

  /* SQL-oriented objects*/
  , SPOBJECTTYPE_TABLE = 401
  , SPOBJECTTYPE_INDEX = 402
  , SPOBJECTTYPE_VIEW = 403
  , SPOBJECTTYPE_MATERIALIZEDVIEWS = 404
  , SPOBJECTTYPE_PRODECURE = 405
  , SPOBJECTTYPE_TENANT = 406
  , SPOBJECTTYPE_SQLOBJECT = 407
  , SPOBJECTTYPE_SQLSOFTWARE = 408
  , SPOBJECTTYPE_SQLSUBSYSTEM = 409
  

  /* Access-oriented objects */
  , SPOBJECTTYPE_RULE = 501

}
