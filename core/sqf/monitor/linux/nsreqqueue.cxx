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

#include "nstype.h"

#include "reqqueue.cxx"

void CRequest::monreply(struct message_def *msg, int sockFd, int *error)
{
    const char method_name[] = "CRequest::monreply";
    TRACE_ENTRY;

    if (error)
        *error = 0;
    if (!msg->noreply) // send reply
    {
        int size = offsetof(struct message_def, u.reply.u);
        switch (msg->u.reply.type)
        {
        case ReplyType_Generic:
            size += sizeof(struct Generic_reply_def);
            if (trace_settings & (TRACE_NS | TRACE_PROCESS_DETAIL))
            {
                trace_printf("%s@%d reply type=%d(Generic), size=%d, sock=%d\n", method_name, __LINE__,
                             msg->u.reply.type, size, sockFd);
                trace_printf("%s@%d generic reply. rc=%d\n", method_name, __LINE__,
                             msg->u.reply.u.generic.return_code);
            }
            break;
        case ReplyType_DelProcessNs:
            size += sizeof(struct DelProcessNs_reply_def);
            if (trace_settings & (TRACE_NS | TRACE_PROCESS_DETAIL))
            {
                trace_printf("%s@%d reply type=%d(DelProcessNs), size=%d, sock=%d, rc=%d\n", method_name, __LINE__,
                             msg->u.reply.type, size, sockFd, msg->u.reply.u.del_process_ns.return_code);
            }
            break;
        case ReplyType_NewProcessNs:
            size += sizeof(struct NewProcessNs_reply_def);
            if (trace_settings & (TRACE_NS | TRACE_PROCESS_DETAIL))
            {
                trace_printf("%s@%d reply type=%d(NewProcessNs), size=%d, sock=%d, rc=%d\n", method_name, __LINE__,
                             msg->u.reply.type, size, sockFd, msg->u.reply.u.new_process_ns.return_code);
            }
            break;
        case ReplyType_ProcessInfo:
            size += sizeof(struct ProcessInfo_reply_def);
            if (trace_settings & (TRACE_NS | TRACE_PROCESS_DETAIL))
            {
                trace_printf("%s@%d reply type=%d(ProcessInfo), size=%d, sock=%d\n", method_name, __LINE__,
                             msg->u.reply.type, size, sockFd);
                trace_printf("%s@%d process-info reply. num_processes=%d, more_data=%d, rc=%d\n", method_name, __LINE__,
                             msg->u.reply.u.process_info.num_processes,
                             msg->u.reply.u.process_info.more_data,
                             msg->u.reply.u.process_info.return_code);
            }
            break;
        case ReplyType_ProcessInfoNs:
            size += sizeof(struct ProcessInfoNs_reply_def);
            if (trace_settings & (TRACE_PROCESS_DETAIL))
            {
                trace_printf("%s@%d reply type=%d(ProcessInfoNs), size=%d, sock=%d\n", method_name, __LINE__,
                             msg->u.reply.type, size, sockFd);
                trace_printf("%s@%d process-info reply. rc=%d\n", method_name, __LINE__,
                             msg->u.reply.u.process_info.return_code);
            }
            break;
        default:
            if (trace_settings & (TRACE_NS | TRACE_PROCESS_DETAIL))
            {
                trace_printf("%s@%d reply type=%d(?), size=%d, sock=%d\n", method_name, __LINE__,
                             msg->u.reply.type, size, sockFd);
            }
            abort();
        }
        int rc = Monitor->SendSock( (char *) &size
                                  , sizeof(size)
                                  , sockFd
                                  , method_name );
        if ( rc )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], cannot send monitor reply: %s\n"
                     , method_name, ErrorMsg(rc));
            //mon_log_write(MON_COMMACCEPT_2, SQ_LOG_ERR, buf);  // TODO
            if (error)
                *error = rc;
        } else
        {
            rc = Monitor->SendSock( (char *) msg
                                  , size
                                  , sockFd
                                  , method_name );
            if ( rc )
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf), "[%s], cannot send monitor reply: %s\n"
                         , method_name, ErrorMsg(rc));
                //mon_log_write(MON_COMMACCEPT_2, SQ_LOG_ERR, buf);  // TODO
                if (error)
                    *error = rc;
            }
        }
    }

    TRACE_EXIT;
}
