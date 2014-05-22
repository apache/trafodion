// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2014 Hewlett-Packard Development Company, L.P.
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

#include "PTPATokenizedEventHandler.h"

#include <sys/time.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>

#include "common/spptLogger.h"
#include "unc.events.pb.h"  // to get token types for errors
#include "wrapper/amqpwrapper.h"
#include "common/sp_defines.h"  // for AMQP wrapper constants
#include "sqevlog/evl_sqlog_writer.h"   // for SQEVL_SEABRIDGE constant
#include "common/evl_sqlog_eventnum.h"

using namespace std;
using namespace google;
using namespace Trinity;

PTPATokenizedEventHandler::PTPATokenizedEventHandler(const string &ip, int port, 
                                             MalMessageTextCatalog & catalog,
                                             map<string, string> &configMap) :
    messageCatalog_(catalog),
    contentType_(SP_CONTENT_TYPE_APP),
    outboundRoutingKey_(SP_EVENT /* publication type */,
                       SP_COMMONPACKAGE /* package name */,
                       SP_INSTANCE /* scope */,
                       SP_PUBLIC /* security */,
                       SP_GPBPROTOCOL /* protocol */,
                       "text_event" /* publication name */),
    repositoryTableMap_(configMap)
{
    createAMQPConnection(ip.c_str(), port);

    parameters_.setDefaultMessage("Message text not available.");

    // initialize those header fields in the text event that are constant for the
    // life of this object
    common::event_header * eventHeader = textEvent_.mutable_header();
    common::info_header * infoHeader = eventHeader->mutable_header();
    common::qpid_header * qpidHeader = infoHeader->mutable_header();

    initAMQPHeader(qpidHeader,SQEVL_SEABRIDGE /* component ID */);

    eventHeader->set_event_id(0);
    eventHeader->set_event_severity(SQ_LOG_ERR);
}

PTPATokenizedEventHandler::~PTPATokenizedEventHandler()
{
    closeAMQPConnection();
}

bool PTPATokenizedEventHandler::processMsg(const google::protobuf::Message *m, const string &routingKey)
{
    bool ret = true;
    int64_t generationTimeTsUtc = 0;
    // extract the event id from the message
    const protobuf::Reflection *reflect = m->GetReflection();
    const protobuf::Descriptor *desc = m->GetDescriptor();
    const protobuf::FieldDescriptor *f = desc->FindFieldByName("header");

    if (f)
    {
        // header is present; look inside of it for event id
        if (protobuf::FieldDescriptor::CPPTYPE_MESSAGE == f->cpp_type())
        {
            // traverse to event header message

            // note that we must get the Reflection object for the embedded message
            // and we need a handle to the embedded Message itself in order to extract
            // fields from it; if you try to do it from the outermost message, Google
            // will complain that the field is not found

            const protobuf::Message & mH = reflect->GetMessage(*m, f);
            const protobuf::Reflection *reflectH = mH.GetReflection();
            const protobuf::Descriptor *descH = mH.GetDescriptor();
            const protobuf::FieldDescriptor *fH = descH->FindFieldByName("event_id");
            if ((fH) && (protobuf::FieldDescriptor::CPPTYPE_INT32 == fH->cpp_type()))
            {
                // Info used in both the republished event and the log text.
                string sqProcessName;

                // we found the event id
                int32_t eventId = reflectH->GetInt32(mH, fH);

                // obtain text representation of message
                parameters_.setProtobuf(m);
                string messageText = messageCatalog_.getMessageText(eventId,&parameters_);

                // Republish event publication text representation as common
                // event (event.common... text_event).
                string repositoryTableName = "NONE";
                map<string, string>::iterator iter = repositoryTableMap_.find(routingKey);
                if (iter != repositoryTableMap_.end())
                    repositoryTableName = iter->second;

                textEvent_.set_text(messageText);
                textEvent_.set_tokenized_event_repos_table(repositoryTableName);

                common::event_header * eventHeader = textEvent_.mutable_header();
                common::info_header * infoHeader = eventHeader->mutable_header();
                common::qpid_header * qpidHeader = infoHeader->mutable_header();

                setAMQPHeaderSequenceNumber(qpidHeader);  // update sequence number in qpid header only

                struct timeval utcTimeStamp;

                gettimeofday(&utcTimeStamp,NULL); // seconds and microseconds since Jan. 1, 1970 in UTC
                tm localTime;
                localtime_r(&utcTimeStamp.tv_sec,&localTime /* out */);  // converts to local time
                time_t lctTimestamp = timegm(&localTime);  // seconds since Jan 1, 1970, LCT

                int64_t usUtcTimestamp = 1000000*(int64_t)utcTimeStamp.tv_sec
                        + utcTimeStamp.tv_usec;  // u-seconds since Jan. 1, 1970 UTC
                qpidHeader->set_generation_time_ts_utc(usUtcTimestamp);
                
                int64_t usLctTimestamp = 1000000*(int64_t)lctTimestamp
                        + utcTimeStamp.tv_usec;  // u-seconds since Jan. 1, 1970 LCT
                qpidHeader->set_generation_time_ts_lct(usLctTimestamp);

                eventHeader->set_event_id(eventId);

                int32_t eventSeverity = SQ_LOG_ERR;  // in case we can't get severity
                fH = descH->FindFieldByName("event_severity");
                if ((fH) && (protobuf::FieldDescriptor::CPPTYPE_INT32 == fH->cpp_type()))
                {
                    eventSeverity = reflectH->GetInt32(mH, fH);
                }
                eventHeader->set_event_severity(eventSeverity);
                
                fH = descH->FindFieldByName("header");  // get the qpid_header
                if ((fH) && (protobuf::FieldDescriptor::CPPTYPE_MESSAGE == fH->cpp_type()))
                {
                    // copy the original qpid_header into the original_qpid_header
                    // nested message; note that there doesn't seem to be an easy
                    // way to do this other than field-by-field copy (sigh)

                    const protobuf::Message & mHH = reflectH->GetMessage(mH, fH);
                    const protobuf::Reflection *reflectHH = mHH.GetReflection();
                    const protobuf::Descriptor *descHH = mHH.GetDescriptor();

                    const protobuf::FieldDescriptor *fHH =
                            descHH->FindFieldByName("info_generation_time_ts_utc");

                    generationTimeTsUtc = usUtcTimestamp; // in case we can't get it
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_INT64 == fHH->cpp_type()))
                    {
                        generationTimeTsUtc = reflectHH->GetInt64(mHH, fHH);
                    }
                    infoHeader->set_info_generation_time_ts_utc(generationTimeTsUtc);

                    fHH = descHH->FindFieldByName("info_generation_time_ts_lct");

                    int64_t generationTimeTsLct = usLctTimestamp; // in case we can't get it
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_INT64 == fHH->cpp_type()))
                    {
                        generationTimeTsLct = reflectHH->GetInt64(mHH, fHH);
                    }
                    infoHeader->set_info_generation_time_ts_lct(generationTimeTsLct);

                    uint32_t version = 1;  // in case we can't get it
                    fHH = descHH->FindFieldByName("info_version");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_UINT32 == fHH->cpp_type()))
                    {
                        version = reflectHH->GetUInt32(mHH, fHH);
                    }
                    infoHeader->set_info_version(version);

                    uint32_t clusterId = 0;  // in case we can't get it
                    fHH = descHH->FindFieldByName("info_cluster_id");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_UINT32 == fHH->cpp_type()))
                    {
                        clusterId = reflectHH->GetUInt32(mHH, fHH);
                    }
                    infoHeader->set_info_cluster_id(clusterId);

                    uint32_t domainId = 0;  // in case we can't get it
                    fHH = descHH->FindFieldByName("info_domain_id");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_UINT32 == fHH->cpp_type()))
                    {
                        domainId = reflectHH->GetUInt32(mHH, fHH);
                    }
                    infoHeader->set_info_domain_id(domainId);

                    uint32_t subdomainId = 0;  // in case we can't get it
                    fHH = descHH->FindFieldByName("info_subdomain_id");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_UINT32 == fHH->cpp_type()))
                    {
                        subdomainId = reflectHH->GetUInt32(mHH, fHH);
                    }
                    infoHeader->set_info_subdomain_id(subdomainId);

                    uint32_t instanceId = 0;  // in case we can't get it
                    fHH = descHH->FindFieldByName("info_instance_id");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_UINT32 == fHH->cpp_type()))
                    {
                        instanceId = reflectHH->GetUInt32(mHH, fHH);
                    }
                    infoHeader->set_info_instance_id(instanceId);

                    uint32_t tenantId = 0;  // in case we can't get it
                    fHH = descHH->FindFieldByName("info_tenant_id");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_UINT32 == fHH->cpp_type()))
                    {
                        tenantId = reflectHH->GetUInt32(mHH, fHH);
                    }
                    infoHeader->set_info_tenant_id(tenantId);

                    uint32_t componentId = 0;  // in case we can't get it
                    fHH = descHH->FindFieldByName("info_component_id");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_UINT32 == fHH->cpp_type()))
                    {
                        componentId = reflectHH->GetUInt32(mHH, fHH);
                    }
                    infoHeader->set_info_component_id(componentId);

                    int32_t processId = 0;  // in case we can't get it
                    fHH = descHH->FindFieldByName("info_process_id");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_INT32 == fHH->cpp_type()))
                    {
                        processId = reflectHH->GetInt32(mHH, fHH);
                    }
                    infoHeader->set_info_process_id(processId);

                    uint32_t threadId = 0;  // in case we can't get it
                    fHH = descHH->FindFieldByName("info_thread_id");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_UINT32 == fHH->cpp_type()))
                    {
                        threadId = reflectHH->GetUInt32(mHH, fHH);
                    }
                    infoHeader->set_info_thread_id(threadId);

                    // optional field
                    uint32_t nodeId = 0;  // in case we can't get it
                    fHH = descHH->FindFieldByName("info_node_id");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_UINT32 == fHH->cpp_type()))
                    {
                        if(reflectHH->HasField(mHH, fHH)) 
                        {
                            nodeId = reflectHH->GetUInt32(mHH, fHH);
                            infoHeader->set_info_node_id(nodeId);
                        }
                        else 
                            infoHeader->clear_info_node_id();
                    }
                    else
                    {
                        infoHeader->clear_info_node_id();
                    }

                    // optional field
                    uint32_t pnidId = 403;  // in case we can't get it
                    fHH = descHH->FindFieldByName("info_pnid_id");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_UINT32 == fHH->cpp_type()))
                    {
                        if(reflectHH->HasField(mHH, fHH))
                        {
                            pnidId = reflectHH->GetUInt32(mHH, fHH);
                            infoHeader->set_info_pnid_id(pnidId);
                        }  
                        else 
                            infoHeader->clear_info_pnid_id();
                    }
                    else
                    {
                        infoHeader->clear_info_pnid_id();
                    }

                    string ipAddress = " "; // in case we can't get it
                    fHH = descHH->FindFieldByName("info_ip_address_id");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_STRING == fHH->cpp_type()))
                    {
                        ipAddress = reflectHH->GetString(mHH, fHH);
                    }
                    infoHeader->set_info_ip_address_id(ipAddress);

                    uint32_t sequenceNum = 0;  // in case we can't get it
                    fHH = descHH->FindFieldByName("info_sequence_num");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_UINT32 == fHH->cpp_type()))
                    {
                        sequenceNum = reflectHH->GetUInt32(mHH, fHH);
                    }
                    infoHeader->set_info_sequence_num(sequenceNum);

                    // optional field
                    fHH = descHH->FindFieldByName("info_process_name");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_STRING == fHH->cpp_type()))
                    {
                        if(reflectHH->HasField(mHH, fHH))
                        {
                            sqProcessName = reflectHH->GetString(mHH, fHH);
                            infoHeader->set_info_process_name(sqProcessName);
                        }
                        else 
                            infoHeader->clear_info_process_name();
                    }
                    else
                    {
                        infoHeader->clear_info_process_name();
                    }

                    uint32_t hostId = 0;  // in case we can't get it
                    fHH = descHH->FindFieldByName("info_host_id");
                    if ((fHH) && (protobuf::FieldDescriptor::CPPTYPE_UINT32 == fHH->cpp_type()))
                    {
                        hostId = reflectHH->GetUInt32(mHH, fHH);
                    }
                    infoHeader->set_info_host_id(hostId);

                    // serialize the protobuf and send it
                    string message;
                    message.assign(textEvent_.SerializeAsString());
                    int errorCode = sendAMQPMessage(false, message,
                                                    contentType_,
                                                    outboundRoutingKey_,
                                                    true /* send synchronously */,
                                                    &textEvent_);

                    if (SP_SUCCESS != errorCode)
                    {
                        // if we could not send it, $$$ perhaps write to syslog?
                        // $$$ temp for testing
                        ret = false;
                        errVar_.reset();
                        errVar_.setErrorString("Tried to send text event via Qpid but failed.");
                        logError(PTPA_QPID_ERROR, errVar_);
                    }
                }
                else
                {
                    ret = false;
                    errVar_.reset();
                    errVar_.setErrorString("couldn't access qpid_header in original messge; we don't expect this");
                    logError(PTPA_PROTOBUF_ERROR, errVar_);
                }

                // Create event text string and write it to the log.   The format is:
                //   [Date time] <severity>:<severity code> <component>: <message>
                time_t gentime = generationTimeTsUtc/1000000;
                struct tm* loctime = localtime(&gentime);
                char timebuf[TLEN];
                bzero(timebuf,TLEN);
                strftime(timebuf, TLEN, "[%m/%d/%Y %T]", loctime);
                string eventLogText = timebuf;
                eventLogText += "  ";
                eventLogText += severityText(eventSeverity) + ":";
                ostringstream number1; number1 << eventSeverity;
                eventLogText += number1.str() + " ";
                if (sqProcessName != "") eventLogText += sqProcessName + " ";
                ostringstream number2; number2 << eventId;
                eventLogText += number2.str() + ":";             
                eventLogText += messageText;
                logInfo(eventLogText.data());

            }
        }
        else
        {
            ret = false;
            errVar_.reset();
            errVar_.setErrorString("header isn't a nested message; we don't expect this");
            logError(PTPA_PROTOBUF_ERROR, errVar_);
        }
    }
    else
    {
        ret = false;
        errVar_.reset();
        errVar_.setErrorString("header is absent; we don't expect this");
        logError(PTPA_PROTOBUF_ERROR, errVar_);
    }

    return ret;
}

std::string PTPATokenizedEventHandler::severityText(int32_t eventSeverity)
{
  // Return the event severity as text or "" if not found.
  std::string result;
  switch (eventSeverity)
  {
    case SQ_LOG_EMERG  : result = "EMERGENCY"; break; // system is unusable 
    case SQ_LOG_ALERT  : result = "ALERT";     break; // action must be taken immediately 
    case SQ_LOG_CRIT   : result = "CRITICAL";  break; // critical conditions 
    case SQ_LOG_ERR    : result = "ERROR";     break; // error conditions 
    case SQ_LOG_WARNING: result = "WARNING";   break; // warning conditions 
    case SQ_LOG_NOTICE : result = "NOTICE";    break; // normal but significant condition 
    case SQ_LOG_INFO   : result = "INFO";      break; // informational 
    case SQ_LOG_DEBUG  : result = "DEBUG";     break; // debug-level messages 
    default            : result = "";
  }
  return result;
}





