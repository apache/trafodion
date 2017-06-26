///////////////////////////////////////////////////////////////////////////////
//
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
//
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include "zookeeper/zookeeper.h"
#include "type2str.h"

const char *ZooConnectionTypeStr( int type )
{
    if ( type == ZOO_CREATED_EVENT )
        return "ZOO_CREATED_EVENT";
    if ( type == ZOO_DELETED_EVENT )
        return "ZOO_DELETED_EVENT";
    if ( type == ZOO_CHANGED_EVENT )
        return "ZOO_CHANGED_EVENT";
    if ( type == ZOO_CHILD_EVENT )
        return "ZOO_CHILD_EVENT";
    if ( type == ZOO_SESSION_EVENT )
        return "ZOO_SESSION_EVENT";
    if ( type == ZOO_NOTWATCHING_EVENT )
        return "ZOO_NOTWATCHING_EVENT";

    return "INVALID_TYPE";
}

const char *ZooConnectionStateStr( int state )
{
    if ( state == 0 )
        return "CLOSED_STATE";
    if ( state == ZOO_EXPIRED_SESSION_STATE )
        return "EXPIRED_SESSION_STATE";
    if ( state == ZOO_AUTH_FAILED_STATE )
        return "AUTH_FAILED_STATE";
    if ( state == ZOO_CONNECTING_STATE )
        return "CONNECTING_STATE";
    if ( state == ZOO_ASSOCIATING_STATE )
        return "ASSOCIATING_STATE";
    if ( state == ZOO_CONNECTED_STATE )
        return "CONNECTED_STATE";

    return "INVALID_STATE";
}
