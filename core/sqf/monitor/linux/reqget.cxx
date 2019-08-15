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
#include "reqqueue.h"
#include "montrace.h"
#include "monsonar.h"
#include "monlogging.h"

extern CMonStats *MonStats;
extern CNode *MyNode;
extern CConfigContainer *Config;

CExtGetReq::CExtGetReq (reqQueueMsg_t msgType, int pid,
                        struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEE", 4);
}

CExtGetReq::~CExtGetReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqee", 4);
}

void CExtGetReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld "
              "requester(name=%s/nid=%d/pid=%d/os_pid=%d/verifier=%d) "
              "(type=%d/group=%s/key=%s/next%d)"
            , CReqQueue::svcReqType[reqType_], getId()
            , msg_->u.request.u.get.process_name
            , msg_->u.request.u.get.nid
            , msg_->u.request.u.get.pid
            , pid_
            , msg_->u.request.u.get.verifier
            , msg_->u.request.u.get.type
            , msg_->u.request.u.get.group
            , msg_->u.request.u.get.key
            , msg_->u.request.u.get.next);
    requestString_.assign( strBuf );
}


void CExtGetReq::performRequest()
{
    int i;
    CConfigGroup *group;
    CConfigKey *key;

    const char method_name[] = "CExtGetReq::performRequest";
    TRACE_ENTRY;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_get_Incr();


    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d request #%ld: Get, requester %s (%d, %d:%d) "
                      ", type=%d, group=%s, key=%s, next=%d\n"
                    , method_name, __LINE__, id_
                    , msg_->u.request.u.get.process_name
                    , msg_->u.request.u.get.nid
                    , msg_->u.request.u.get.pid
                    , msg_->u.request.u.get.verifier
                    , msg_->u.request.u.get.type
                    , msg_->u.request.u.get.group
                    , msg_->u.request.u.get.key
                    , msg_->u.request.u.get.next);
    }

    CProcess *requester;

    nid_ = msg_->u.request.u.get.nid;
    verifier_ = msg_->u.request.u.get.verifier;
    processName_ = msg_->u.request.u.get.process_name;

    if ( processName_.size() )
    { // find by name
        requester = MyNode->GetProcess( processName_.c_str()
                                      , verifier_ );
    }
    else
    { // find by pid
        requester = MyNode->GetProcess( pid_
                                      , verifier_ );
    }

    if ( requester )
    {
        // Process the request
        switch (msg_->u.request.u.get.type)
        {
            case ConfigType_Cluster:
                group = Config->GetClusterGroup();
                break;
            case ConfigType_Node:
                if (*msg_->u.request.u.get.group == '\0')
                {
                    sprintf(msg_->u.request.u.get.group,"NODE%d",msg_->u.request.u.get.nid);
                }
            case ConfigType_Process:
                group = Config->GetGroup(msg_->u.request.u.get.group);
                break;
            default:
                char buf[MON_STRING_BUF_SIZE];
                sprintf( buf, "%s@%d - Invalid ConfigType=%d.\n"
                       , method_name, __LINE__
                       , msg_->u.request.u.get.type);
                mon_log_write(MON_MONITOR_GETCONF_1, SQ_LOG_ERR, buf);
                group = NULL;
        }
        if (group)
        {
            if (*msg_->u.request.u.get.key)
            {
                key = group->GetKey(msg_->u.request.u.get.key);
                if (key==NULL)
                {
                    msg_->u.reply.type = ReplyType_Get;
                    msg_->u.reply.u.get.type = ConfigType_Undefined;
                    msg_->u.reply.u.get.group[0] = '\0';
                    msg_->u.reply.u.get.num_keys = 0;
                    msg_->u.reply.u.get.num_returned = 0;
                }
                else
                {
                    if (msg_->u.request.u.get.next)
                    {
                        key = key->GetNext();
                        if (key)
                        {
                            if (msg_->u.request.u.get.next)
                            {
                                // We are return next set of key/value pairs associated with group.
                                msg_->u.reply.type = ReplyType_Get;
                                msg_->u.reply.u.get.type = group->GetType();
                                STRCPY (msg_->u.reply.u.get.group, group->GetName());
                                i = 0;
                                while (key && i < MAX_KEY_LIST)
                                {
                                    STRCPY(msg_->u.reply.u.get.list[i].key, key->GetName());
                                    STRCPY(msg_->u.reply.u.get.list[i].value, key->GetValue());
                                    key = key->GetNext();
                                    i++;
                                }    
                                msg_->u.reply.u.get.num_keys = group->GetNumKeys();
                                msg_->u.reply.u.get.num_returned = i;
                            }
                        }
                    }
                    else
                    {
                        // We are getting only the value associated with key.
                        msg_->u.reply.type = ReplyType_Get;
                        msg_->u.reply.u.get.type = group->GetType();
                        STRCPY (msg_->u.reply.u.get.group, group->GetName());
                        msg_->u.reply.u.get.num_keys = 1;
                        msg_->u.reply.u.get.num_returned = 1;
                        STRCPY(msg_->u.reply.u.get.list[0].key, key->GetName());
                        STRCPY(msg_->u.reply.u.get.list[0].value, key->GetValue());
                    }
                }
            }
            else
            {
                // We are returning the 1st set of key/value pairs associated with group.
                msg_->u.reply.type = ReplyType_Get;
                msg_->u.reply.u.get.type = group->GetType();
                STRCPY (msg_->u.reply.u.get.group, group->GetName());
                i = 0;
                key = group->GetKey((char *) "");
                while (key && i < MAX_KEY_LIST)
                {
                    STRCPY(msg_->u.reply.u.get.list[i].key, key->GetName());
                    STRCPY(msg_->u.reply.u.get.list[i].value, key->GetValue());
                    key = key->GetNext();
                    i++;
                }    
                msg_->u.reply.u.get.num_keys = group->GetNumKeys();
                msg_->u.reply.u.get.num_returned = i;
            }
        }
        else
        {
            msg_->u.reply.type = ReplyType_Get;
            msg_->u.reply.u.get.type = ConfigType_Undefined;
            msg_->u.reply.u.get.group[0] = '\0';
            msg_->u.reply.u.get.num_keys = 0;
            msg_->u.reply.u.get.num_returned = 0;
        }    

        // Send reply to requester
        lioreply(msg_, pid_);
    }
    else
    {
        // the requester already exited and the LIO buffer will be cleaned up
    }

    TRACE_EXIT;
}
