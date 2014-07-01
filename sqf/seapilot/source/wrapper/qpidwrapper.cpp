// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
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
#ifndef WIN32
#include <netdb.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include "common/common.info_header.pb.h"
#else
#include <process.h>
#include "win/common.info_header.pb.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "qpidwrapper.h"
#include "common/sp_errors.h"
#include "common/evl_sqlog_eventnum.h"

#ifndef WIN32 // Linux
extern "C" const char* sp_libwrapper_vers_str();
#endif


// private
// ----------------------------------------------------------------------
//
// qpidWrapper
// Purpose : Constructor
//
// ----------------------------------------------------------------------
qpidWrapper::qpidWrapper()
{
    activeConnection = activeSession = activeSender=false;
    sequenceNumber_ = 0;

    iPAddress_[0] = '\0';
    portNumber_ = -1;	

    pthread_mutex_init(&mutex, NULL);

}

// ---------------------------------------------------------------------
//
// ~qpidWrapper
// Purpose : Destructor
//
// ---------------------------------------------------------------------
qpidWrapper::~qpidWrapper()
{
    // Since the qpidWrapper object is a global object, we have no
    // control over when its destructor is called relative to when
    // destructors are called for other global objects. (This is one
    // reason that global variables are bad, but that's a soap box
//     // for another day.) It turns out Qpid itself has some global
    // objects (qpid::sys::Mutex objects), and we were seeing cases
    // where these objects would be destroyed before this destructor
    // was called. And then when we'd try to destroy our connection
    // and session objects, qpid would try to access those now-destroyed
    // Mutexes, and would throw an assert. Sometimes the race turns out
    // differently and we see looping behavior instead. The workaround,
    // it seems, is to avoid accessing any qpid objects at all within
    // this destructor. So, we just let them leak and hope that Qpid
    // does appropriate cleanup. We are in process termination, so a
    // memory leak is of no importance.

    // We do need to make sure that some racing thread that might access
    // this object after our destructor call does not cause bad behavior;
    // fortunately setting spDisabled to true makes all method calls to
    // this object no-ops. This will work so long as callers check spDisabled
    // outside of mutex. This is safe because spDisabled is initially true
    // in the constructor, then is set to false only under mutex.

        pthread_mutex_lock( &mutex );

#ifndef MONITOR_PROCESS
        try
        {            
                 if ((activeConnection)&& (connection.isOpen()) )   
                {
                    connection.close();
                }          
        }
        catch(const std::exception& error)
        {
            // ignore the exception, we're in a destructor!
        }
#endif       
        activeConnection = activeSession = activeSender=false;
        pthread_mutex_unlock( &mutex );

    pthread_mutex_destroy(&mutex);

}

// -----------------------------------------------------------------------
//
// closeQpidConnection
// Purpose - explicitly close a connection
//
// ----------------------------------------------------------------------
int qpidWrapper::closeQpidConnection()
{
#ifndef MONITOR_PROCESS
    int random = 0;
    int retries = 0;

    pthread_mutex_lock( &mutex );
    while ((activeConnection) && (retries <= MAX_RETRIES))
    {
        try
        {//All a connections sessions can be closed by a call to Connection::close().
            if ((activeConnection)&& (connection.isOpen()) )   
            {
                    connection.close();
            }        
	    activeConnection = activeSession = activeSender=false;  
	    receiver_map_.clear();
        }
        catch(const std::exception& error)
        {
           // avoid bogus sleep
           if (++retries <= MAX_RETRIES)
           {
               random = (rand()%(1+MAX_RAND))*10000; //sleep between 10 and 50 ms)
#ifndef WIN32
               usleep(random);
#else
			   random = random/1000;  //micro to milli
			   Sleep(random);
#endif
           }
        }
    }
    pthread_mutex_unlock( &mutex );

   if (activeConnection)
      return SP_UNKNOWN;
   else
#endif
      return SP_SUCCESS;
}
// -----------------------------------------------------------------------
//
// createQpidConnection
// Purpose - explicitly create a connection
//
// ----------------------------------------------------------------------
int qpidWrapper::createQpidConnection(const char *ipAddress, int portNumber,
									  const char *user, const char *password, const char *mode)
{

   // probably don't have the configuration yet, so
   // we'll shoot out of createConnection if
   // sp is disabled
   int error = SP_SUCCESS;  // assume success
#ifndef MONITOR_PROCESS
   pthread_mutex_lock( &mutex );
   if (activeConnection)
   {
       pthread_mutex_unlock( &mutex );
       return SP_SUCCESS;
   } 
   error = createConnection(true,  ipAddress, portNumber, user, password, mode);
   pthread_mutex_unlock( &mutex );
#endif

   return error;
}

// ------------------------------------------------------------------------
//
// initInfoHeader
// Purpose - initialize the header fields
//
// -------------------------------------------------------------------------
int qpidWrapper::initInfoHeader (void *header, int componentId, unsigned int instanceId, std::string& ipaddr, unsigned int host_id, const void *field
#ifdef SEAQUEST_PROCESS
, char * processName, int nodeId, int pnodeId
#endif
)
{

    if (header == NULL)
        return SP_BAD_PARAM;

    if (field == NULL)
    {
        common::info_header *infoHeader = (common::info_header *)header;
        common::qpid_header *qpidHeader = (common::qpid_header *)infoHeader->mutable_header();

        initQpidHeader(qpidHeader, componentId, instanceId, ipaddr, host_id, field
#ifdef SEAQUEST_PROCESS
                   , processName, nodeId, pnodeId
#endif
        );
#ifdef SEAQUEST_PROCESS
        if (processName)
            infoHeader->set_info_process_name(qpidHeader->process_name());
        if (nodeId != -1)
            infoHeader->set_info_node_id(qpidHeader->node_id());
        if (pnodeId != -1)
            infoHeader->set_info_pnid_id(qpidHeader->pnid_id());
#endif
        infoHeader->set_info_ip_address_id(qpidHeader->ip_address_id());
        infoHeader->set_info_host_id(qpidHeader->host_id());
        infoHeader->set_info_generation_time_ts_utc(qpidHeader->generation_time_ts_utc());
        infoHeader->set_info_generation_time_ts_lct(qpidHeader->generation_time_ts_lct());
        infoHeader->set_info_process_id(qpidHeader->process_id());
        infoHeader->set_info_thread_id(qpidHeader->thread_id());
        infoHeader->set_info_sequence_num(qpidHeader->sequence_num());
        infoHeader->set_info_component_id(componentId);

        // will populate these when available, will change to call the qpidHeader methods
        // once thsee are meaningful.  Until, save the pathlength...
        infoHeader->set_info_version (1);
        infoHeader->set_info_tenant_id(0);
        infoHeader->set_info_domain_id(0);
        infoHeader->set_info_subdomain_id(0);
        infoHeader->set_info_cluster_id(0);
        infoHeader->set_info_instance_id(qpidHeader->instance_id());
    }
	else
	{
		// reflection flavor: use the reflection interface to assign the contents
		google::protobuf::Message *infoM = (google::protobuf::Message *)header;
		const google::protobuf::FieldDescriptor *infoF = (const google::protobuf::FieldDescriptor *)field;
		const google::protobuf::Reflection *infoR = infoM->GetReflection();
		const google::protobuf::Descriptor *infoD = infoF->message_type();
		const google::protobuf::FieldDescriptor *qpidF = infoD->FindFieldByName("header");
		if(qpidF != NULL)
		{
			google::protobuf::Message *qpidM = infoR->MutableMessage(infoM, qpidF);
			initQpidHeader(qpidM, componentId, instanceId, ipaddr, host_id, qpidF
#ifdef SEAQUEST_PROCESS
                                       , processName, nodeId, pnodeId
#endif
);
			const google::protobuf::Descriptor *qpidD = qpidF->message_type();
			const google::protobuf::Reflection *qpidR = qpidM->GetReflection();

			const google::protobuf::FieldDescriptor *infoFF = infoD->FindFieldByName("info_generation_time_ts_utc");
			const google::protobuf::FieldDescriptor *qpidFF = qpidD->FindFieldByName("generation_time_ts_utc");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetInt64(infoM, infoFF, qpidR->GetInt64(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;

			infoFF = infoD->FindFieldByName("info_generation_time_ts_lct");
			qpidFF = qpidD->FindFieldByName("generation_time_ts_lct");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetInt64(infoM, infoFF, qpidR->GetInt64(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;

			infoFF = infoD->FindFieldByName("info_version");
			qpidFF = qpidD->FindFieldByName("version");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetUInt32(infoM, infoFF, qpidR->GetUInt32(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;

			infoFF = infoD->FindFieldByName("info_cluster_id");
			qpidFF = qpidD->FindFieldByName("cluster_id");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetUInt32(infoM, infoFF, qpidR->GetUInt32(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;

			infoFF = infoD->FindFieldByName("info_domain_id");
			qpidFF = qpidD->FindFieldByName("domain_id");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetUInt32(infoM, infoFF, qpidR->GetUInt32(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;

			infoFF = infoD->FindFieldByName("info_subdomain_id");
			qpidFF = qpidD->FindFieldByName("subdomain_id");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetUInt32(infoM, infoFF, qpidR->GetUInt32(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;

			infoFF = infoD->FindFieldByName("info_instance_id");
			qpidFF = qpidD->FindFieldByName("instance_id");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetUInt32(infoM, infoFF, qpidR->GetUInt32(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;

			infoFF = infoD->FindFieldByName("info_tenant_id");
			qpidFF = qpidD->FindFieldByName("tenant_id");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetUInt32(infoM, infoFF, qpidR->GetUInt32(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;

			infoFF = infoD->FindFieldByName("info_component_id");
			qpidFF = qpidD->FindFieldByName("component_id");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetUInt32(infoM, infoFF, qpidR->GetUInt32(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;

			infoFF = infoD->FindFieldByName("info_process_id");
			qpidFF = qpidD->FindFieldByName("process_id");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetInt32(infoM, infoFF, qpidR->GetInt32(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;

			infoFF = infoD->FindFieldByName("info_thread_id");
			qpidFF = qpidD->FindFieldByName("thread_id");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetUInt32(infoM, infoFF, qpidR->GetUInt32(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;

#ifdef SEAQUEST_PROCESS
			infoFF = infoD->FindFieldByName("info_node_id");
			qpidFF = qpidD->FindFieldByName("node_id");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetUInt32(infoM, infoFF, qpidR->GetUInt32(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;

			infoFF = infoD->FindFieldByName("info_pnid_id");
			qpidFF = qpidD->FindFieldByName("pnid_id");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetUInt32(infoM, infoFF, qpidR->GetUInt32(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;
#endif

			infoFF = infoD->FindFieldByName("info_ip_address_id");
			qpidFF = qpidD->FindFieldByName("ip_address_id");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetString(infoM, infoFF, qpidR->GetString(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;

			infoFF = infoD->FindFieldByName("info_sequence_num");
			qpidFF = qpidD->FindFieldByName("sequence_num");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetUInt32(infoM, infoFF, qpidR->GetUInt32(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;

                         infoFF = infoD->FindFieldByName("info_host_id");
                         qpidFF = qpidD->FindFieldByName("host_id");
                         if((infoFF != NULL) && (qpidFF != NULL))
                             infoR->SetUInt32(infoM, infoFF, qpidR->GetUInt32(*qpidM, qpidFF));
                         else
                             return SP_NOT_FOUND;

#ifdef SEAQUEST_PROCESS
			infoFF = infoD->FindFieldByName("info_process_name");
			qpidFF = qpidD->FindFieldByName("process_name");
			if((infoFF != NULL) && (qpidFF != NULL))
				infoR->SetString(infoM, infoFF, qpidR->GetString(*qpidM, qpidFF));
			else
				return SP_NOT_FOUND;
#endif

		}
		else
			return SP_NOT_FOUND;
	}

    return SP_SUCCESS;
}

// ------------------------------------------------------------------------
//
// initQpidHeader
// Purpose - initialize the header fields
//
// -------------------------------------------------------------------------
int qpidWrapper::initQpidHeader (void *header, int componentId, unsigned int instanceId, std::string& ipaddr, unsigned int host_id, const void *field
#ifdef SEAQUEST_PROCESS
, char *processName, int nodeId, int pnodeId
#endif
)
{
    if (header == NULL)
        return SP_BAD_PARAM;

	    struct timeval utcTimeStamp;
		time_t localTimeStamp;

#ifndef WIN32
    int pid = getpid();
    uint tid = pthread_self();
	    gettimeofday(&utcTimeStamp,NULL); // seconds and microseconds since Jan. 1, 1970 in UTC

    tm localTime;
    localtime_r(&utcTimeStamp.tv_sec,&localTime /* out */);  // converts to local time
    localTimeStamp = timegm(&localTime);  // seconds since Jan 1, 1970, LCT
#else
	int pid = _getpid();
	uint tid = 0;
  
	time_t seconds;
	tm * utc_time;
	tm * ltc_time;

	seconds = time (NULL);
	utc_time = gmtime ( &seconds );
	ltc_time = localtime (&seconds);

	utcTimeStamp.tv_sec = 0; //mktime(utc_time);
	utcTimeStamp.tv_usec = 0;

	localTimeStamp = mktime(ltc_time);

#endif

    if (field == NULL)
    {
        common::qpid_header *qpidHeader =  (common::qpid_header *)header;

        qpidHeader->set_generation_time_ts_utc(1000000*(int64_t)utcTimeStamp.tv_sec
                                           + utcTimeStamp.tv_usec);
        qpidHeader->set_generation_time_ts_lct(1000000*(int64_t)localTimeStamp
                                           + utcTimeStamp.tv_usec);
        qpidHeader->set_component_id(componentId);
        qpidHeader->set_process_id(pid);
        qpidHeader->set_thread_id(tid);
        qpidHeader->set_sequence_num(sequenceNumber_++);

        // will populate these when available
        qpidHeader->set_version (1);
        qpidHeader->set_tenant_id(0);
        qpidHeader->set_domain_id(0);
        qpidHeader->set_subdomain_id(0);
        qpidHeader->set_cluster_id(0);
        qpidHeader->set_instance_id(instanceId);

#ifdef SEAQUEST_PROCESS
        if (nodeId > -1)
           qpidHeader->set_node_id(nodeId);
        if (pnodeId > -1)
           qpidHeader->set_pnid_id(pnodeId);
        if (processName)
            qpidHeader->set_process_name(processName);
#endif
        qpidHeader->set_ip_address_id(ipaddr);
        qpidHeader->set_host_id(host_id);

#ifndef WIN32
        qpidHeader->set_system_version(sp_libwrapper_vers_str());
#endif
	}
    else
    {
	// reflection flavor: use the reflection interface to assign the contents
	google::protobuf::Message *qpidHeader = (google::protobuf::Message *)header;
	const google::protobuf::FieldDescriptor *f = (const google::protobuf::FieldDescriptor *)field;
	const google::protobuf::Descriptor *desc = f->message_type();
	const google::protobuf::Reflection *reflect = qpidHeader->GetReflection();
 
	const google::protobuf::FieldDescriptor *field = desc->FindFieldByName("generation_time_ts_utc");
	if(field != NULL)
		reflect->SetInt64(qpidHeader, field, 1000000*(int64_t)utcTimeStamp.tv_sec + utcTimeStamp.tv_usec);
	else
		return SP_NOT_FOUND;
	field = desc->FindFieldByName("generation_time_ts_lct");
	if(field != NULL)
		reflect->SetInt64(qpidHeader, field, 1000000*(int64_t)localTimeStamp + utcTimeStamp.tv_usec);
	else
		return SP_NOT_FOUND;
	field = desc->FindFieldByName("version");
	if(field != NULL)
		reflect->SetUInt32(qpidHeader, field, 1);
	else
		return SP_NOT_FOUND;
	field = desc->FindFieldByName("cluster_id");
	if(field != NULL)
		reflect->SetUInt32(qpidHeader, field, 0);
	else
		return SP_NOT_FOUND;
	field = desc->FindFieldByName("domain_id");
	if(field != NULL)
		reflect->SetUInt32(qpidHeader, field, 0);
	else
		return SP_NOT_FOUND;
	field = desc->FindFieldByName("subdomain_id");
	if(field != NULL)
		reflect->SetUInt32(qpidHeader, field, 0);
	else
		return SP_NOT_FOUND;
	field = desc->FindFieldByName("instance_id");
	if(field != NULL)
		reflect->SetUInt32(qpidHeader, field, instanceId);
	else
		return SP_NOT_FOUND;
	field = desc->FindFieldByName("tenant_id");
	if(field != NULL)
		reflect->SetUInt32(qpidHeader, field, 0);
	else
		return SP_NOT_FOUND;
	field = desc->FindFieldByName("component_id");
	if(field != NULL)
		reflect->SetUInt32(qpidHeader, field, componentId);
	else
		return SP_NOT_FOUND;
	field = desc->FindFieldByName("process_id");
	if(field != NULL)
		reflect->SetInt32(qpidHeader, field, pid);
	else
		return SP_NOT_FOUND;
	field = desc->FindFieldByName("thread_id");
	if(field != NULL)
		reflect->SetUInt32(qpidHeader, field, tid);
	else
		return SP_NOT_FOUND;
#ifdef SEAQUEST_PROCESS
	field = desc->FindFieldByName("node_id");
	if(field != NULL && nodeId != -1)
	    reflect->SetUInt32(qpidHeader, field, nodeId);

	field = desc->FindFieldByName("pnid_id");
	if(field != NULL &&  pnodeId != -1)
	    reflect->SetUInt32(qpidHeader, field, pnodeId);
#endif
	field = desc->FindFieldByName("ip_address_id");
	if(field != NULL)
		reflect->SetString(qpidHeader, field, ipaddr);
	else
		return SP_NOT_FOUND;
	field = desc->FindFieldByName("sequence_num");
	if(field != NULL)
		reflect->SetUInt32(qpidHeader, field, sequenceNumber_++);
	else
		return SP_NOT_FOUND;
        field = desc->FindFieldByName("host_id");
        if(field != NULL)
                reflect->SetUInt32(qpidHeader, field, host_id);
        else
                return SP_NOT_FOUND;

#ifdef SEAQUEST_PROCESS
	field = desc->FindFieldByName("process_name");
	if(field != NULL)
		reflect->SetString(qpidHeader, field, processName);
	else
		return SP_NOT_FOUND;
#endif

#ifndef WIN32

        field = desc->FindFieldByName("system_version");
        if(field != NULL)
            reflect->SetString(qpidHeader, field, sp_libwrapper_vers_str());
        else
            return SP_NOT_FOUND;
#endif
    }

    return SP_SUCCESS;
}

// ------------------------------------------------------------------------
//
// setInfoHeaderSequenceNumber
// Purpose - initialize the header fields
//
// -------------------------------------------------------------------------
int qpidWrapper::setInfoHeaderSequenceNumber (void *header)
{
    if (header == NULL)
        return SP_BAD_PARAM;

    common::info_header *infoHeader = (common::info_header *)header;
    common::qpid_header *qpidHeader = (common::qpid_header *)infoHeader->mutable_header();

    setQpidHeaderSequenceNumber(qpidHeader);

    infoHeader->set_info_sequence_num(qpidHeader->sequence_num());

    return SP_SUCCESS;
}

// ------------------------------------------------------------------------
//
// setQpidHeaderSequenceNumber
// Purpose - update sequence number field in a (reused) qpid header
//
// -------------------------------------------------------------------------
int qpidWrapper::setQpidHeaderSequenceNumber (void *header)
{

    if (header == NULL)
        return SP_BAD_PARAM;

    common::qpid_header *qpidHeader =  (common::qpid_header *)header;

    qpidHeader->set_sequence_num(sequenceNumber_++);

    return SP_SUCCESS;
}

// -------------------------------------------------------------------
//
// createConnection
// Purpose - create a qpid connection and open a session.
//
// --------------------------------------------------------------------
int qpidWrapper::createConnection (bool retry, 
								   const char *ipAddress,
                                   int portNumber,
								   const char *user,
								   const char *password,
								   const char *mode)
{
	//printf("AMQP wrapper: IP=%s, port=%d, user=%s, PW=%s mode=%s\n",
	//	   ipAddressParam, portNumberParam, user, password, mode);
	int rc = SP_SUCCESS;
#ifndef MONITOR_PROCESS
    int random = 0;
    int retries = 0;
    const char *user_fn = user;
    const char *password_fn = password;
    if (!user) user_fn = "";         // Must set to empty string for assignment to std::string.
    if (!password) password_fn = ""; // Must set to empty string for assignment to std::string.

        
    if (ipAddress)
    {       
      	strcpy(iPAddress_, ipAddress);
    }
    
    if (portNumber != -1)
    {
	portNumber_ = portNumber;
    }
    
    //url should in such format:"127.0.0.1:5672"
   char url[MAX_SP_BUFFER];
   memset(url,0,sizeof(url));
   sprintf(url,"%s:%d",iPAddress_,portNumber_);
   connection=qpid::messaging::Connection(string(url));
    
    // when this while loop is exited, either there will be an active connection AND session
    // or neither.  If createSession fails, it will close the active connection and force the
    // connection to be retried.
    while(!(activeConnection) && (retries <= MAX_RETRIES))
    {
        try 
        {
          // Attempt to connect with authentication.
          connection.setOption("username",user_fn);
          connection.setOption("password",password_fn);
          connection.setOption("tcp_nodelay",true);
          connection.open();
          activeConnection = true;
        }
        catch (const std::exception &error)
        {
          // Get error.
          std::string excText = error.what();
          if (excText.find("Authentication failed") != std::string::npos)
          {
            rc = SP_AUTHENTICATION_FAILED;
            retries = MAX_RETRIES;
            break;
          }
          // avoid bogus sleep
          else if ((retry) && ((++retries <= MAX_RETRIES)))
          {
             random = (rand()%(1+MAX_RAND))*10000;  // between 10 and 50 milliseconds
#ifndef WIN32
             usleep(random);
#else
             random = random/1000;  //micro to milli
             Sleep(random);
#endif
          }
          else
            retries = MAX_RETRIES+1;
        } catch (...)
        {
          printf("createConnection: Unknown exception caught." );
        }
       
        //if connected, try to create a session, the active data member will
        // get set in this method.
        if(activeConnection && !activeSession)
           createSession();
    }
   if ((!activeConnection || !activeSession) && rc == SP_SUCCESS)
      rc = SP_CONNECTION_CLOSED;
#endif
   return rc;
}

// -----------------------------------------------------------------------
//
// createSession
// Purpose : create a session
//
// -----------------------------------------------------------------------
int qpidWrapper::createSession()
{
#ifndef MONITOR_PROCESS

    if (activeConnection && activeSession)
       return SP_SUCCESS;

    if (activeConnection)
    { 
       try
       {
            session = connection.createSession();            
            activeSession = true;
	     activeSender=false;
       }
       catch (const std::exception &error)
       {
            //if we fail, close connection
            connection.close();
            activeConnection = activeSession = activeSender=false;            
        }
    }

    if (!activeConnection)
        return SP_CONNECTION_CLOSED;

#endif
    return SP_SUCCESS;
}

int qpidWrapper::createProducer(std::string &exchange)
{
	//address should be in following 2 formats:
	//"my-new-topic/usa.new;{create: always, node:{type:topic}}"
	//"my-new-topic/"+ routingkey_str+";{create: always, node:{type:topic}}";
	//or "my-new-topic; {create: always, node:{type:topic}}"
	int ret=SP_SUCCESS;
	string address=exchange+address_option;
    try
    { 
        /*@exception ResolutionError if there is an error in resolving
         * the address
         *
         * @exception MalformedAddress if the syntax of address is not
         * valid*/
	  	sender = session.createSender(address);
		activeSender=true;
		ret=SP_SUCCESS;
     }
	 catch(const std::exception& error)
	 {	 
		ret=SP_BAD_PARAM;
		activeSender=false;
	 }  
         return ret;
}           


// --------------------------------------------------------------------------
// sendMessage
// Purpose : send a Qpid Message.  If the async flag is set, it will send
//           an async message
//
// ---------------------------------------------------------------------------
int qpidWrapper::sendMessage(bool retry, const std::string& messageText, const std::string& contentType,
                             AMQPRoutingKey& routingKey, std::string &exchange, bool asyncMsg)
{
    int  error = SP_SUCCESS;
#ifndef MONITOR_PROCESS
    bool done = false;
    qpid::messaging::Message message;
    int  random = 0;
    int  retries = 0;
    string routingkey_str=routingKey.GetAsString();  

    pthread_mutex_lock( &mutex );

    // retry logic is within the connection logic, return if we can't get a connection
    // or a session
    if( (!connection.isOpen()) || (!activeSession))
    {
        error = createConnection (retry);
        if (error != SP_SUCCESS)
        {
            pthread_mutex_unlock( &mutex );
            return error;
        }
    }
    // Only the first time to create sender as a procuder/sender 
    // Only producer need to call this, so put it in sendMessage, not in createSession
    if(!activeSender)
    {
    	error=createProducer(exchange);
	if(error!= SP_SUCCESS)
		return error;	
    }

    // create and initialize a qpid message object
    message.setSubject(routingkey_str);
    message.setContentType(contentType);
    message.setContent(messageText);	

    while ((retries <= MAX_RETRIES) && (!done))
    {
         try
         {
           /**
     		* Sends a message
     		* 
     		* @param message the message to send	 	
     		* @param sync if true the call will block until the server
     		* confirms receipt of the messages; if false will only block for
     		* available capacity (i.e. pending == capacity)
			  QPID_MESSAGING_EXTERN void send(const Message& message, bool sync=false);
     		*/
           	if (asyncMsg)	sender.send(message);  
	   	else sender.send(message,true);
           done = true;
           error = SP_SUCCESS;
         }
         catch(const std::exception& excep)
         {
            activeSession = false;

             // lost our connection, try again, otherwise just
             // retry the message
             if (!connection.isOpen())
                 activeConnection = false;

             if ((retry) && (++retries <= MAX_RETRIES))
             {
                 random = (rand()%(1+MAX_RAND))*10000;  // between 10 and 50 milliseconds
#ifndef WIN32
               usleep(random);
#else
			   random = random/1000;  //micro to milli
			   Sleep(random);
#endif

                // presume that session is lost.  Someday qpid will give
                // us this information, but for now we just dispose and
                // assume.  WE only retry the connection 1 time here.  If it succeeds
                // we'll retry the message.  If not, then we exit the while loop.
                error = createConnection (retry);
                if (error != SP_SUCCESS)
                {
                      done = true;
                      error = SP_CONNECTION_CLOSED;
                }
            }
            // else belongs to the if "retry"
            else
            {
                 done = true;
                 error = SP_SEND_FAILED;
            }
         }
     }

     pthread_mutex_unlock( &mutex );
#endif
     return error;
}

// -------------------------------------------------------------------------------
//
// syncMessages
// Purpose : for users of async sends, this will make sure they are sent.
//           USER BEWARE - this may take a while....
//
// -------------------------------------------------------------------------------
int qpidWrapper::syncMessages()
{
    int  error = SP_SUCCESS;
#ifndef MONITOR_PROCESS
    bool done = false;
    int  random = 0;
    int  retries = 0;

    // no need to sync if our connection was lost
    if(!activeConnection)
        return SP_CONNECTION_CLOSED;

    pthread_mutex_lock( &mutex );
    while ((retries <= MAX_RETRIES) && (!done))
    try
    {
        session.sync();
        done = true;
        error = SP_SUCCESS;
    }
    catch (const std::exception& excep)
    {
        error = SP_SYNC_FAILED;
        if (!connection.isOpen())
        {
           error = SP_CONNECTION_CLOSED;
           done = true;
        }
        else if (++retries <= MAX_RETRIES)
        {
            random = (rand()%(1+MAX_RAND))*10000;  // between 10 and 50 milliseconds
#ifndef WIN32
               usleep(random);
#else
			   random = random/1000;  //micro to milli
			   Sleep(random);
#endif
        }
    }
    pthread_mutex_unlock( &mutex );
#endif
    return error;
}
string qpidWrapper::createAddress(std::string exchange,std::string routingkey)
{
	//address should be in following 2 formats:
	//"my-new-topic/usa.new;{create: always, node:{type:topic}}"
	//"my-new-topic/"+ routingkey+";{create: always, node:{type:topic}}";
	//or "my-new-topic; {create: always, node:{type:topic}}"
	string address=exchange+"/"+routingkey+address_option;
	return address;
}

//If the program need to read messages from many sources,it can create multiple Consumer/receivers ,
//Every consumer/receiver bind to an unique address=exchange +routingkey.
int qpidWrapper::createConsumer(std::string exchange,std::string routingkey,uint32_t capacity)
{	
	string address=createAddress( exchange,routingkey);
        try
        {
       	 /**
     		* Create a new receiver through which messages can be received
     		* from the specified address.
     		*
     		* @exception ResolutionError if there is an error in resolving
     		* the address
     		*
     		* @exception MalformedAddress if the syntax of address is not
     		* valid
     		*/
	  	receiver_= session.createReceiver(address);		
	       receiver_.setCapacity(CAPACITY);
		string name=receiver_.getName();
		receiver_map_[address]=name;
		//receiver_map_.insert(valType(address,receiver));
        }
        catch(const std::exception& error)
        {
		return SP_BAD_PARAM;		
        }
        return SP_SUCCESS;
}   
int qpidWrapper::deleteConsumer(std::string exchange,std::string routingkey)
{
	 string address=createAddress( exchange,routingkey);
	  if (receiver_map_.count( address))  
	  {
	  	string name=receiver_map_[address];
		receiver_=session.getReceiver(name);
		receiver_.close();
		receiver_map_.erase (address);
		return SP_SUCCESS;
          }
	 return SP_BAD_PARAM;		
 }

//If the program have multiple Consumers/receivers,call this retrieve method
//A receiver can only read from one source, but many programs need to be able to read messages from many sources. 
//In the Qpid Messaging API, a program can ask a session for the ¡°next receiver¡±; 
//that is, the receiver that is responsible for the next available message. 
bool qpidWrapper::retrieveNextMessage(qpid::messaging::Message &message,qpid::messaging::Duration timeout)
   { 
     /**
     * Returns the receiver for the next available message. If there
     * are no available messages at present the call will block for up
     * to the specified timeout waiting for one to arrive.     
     */
    //QPID_MESSAGING_EXTERN Receiver nextReceiver(Duration timeout=Duration::FOREVER);
    /**
     * Retrieves a message for this receivers subscription or waits
     * for up to the specified timeout for one to become
     * available. 
     * @return false if there is no message to give after
     * waiting for the specified timeout, or if the Receiver is
     * closed, in which case isClose() will be true.
     */
     //receiver.fetch(Message& message, Duration timeout=Duration::FOREVER);
     //currently ,use the default timeout =Duration::FOREVER,if need ,developer can set the timeout in new feature in future.	        

    bool ret= session.nextReceiver().fetch(message,RECEIVER_TIMEOUT) ;	    
    session.acknowledge();
    return ret;
   }

void qpidWrapper::getMessageData(qpid::messaging::Message& message,string & content,string &routingkey)
{
	content=message.getContent();
	routingkey=message.getSubject();
	return;
    }

//Currently,the acknowledge method won't be used externally.
void qpidWrapper::acknowledge(qpid::messaging::Message &message)
   {
	/**
     * Acknowledges the specified message.
     */
    session.acknowledge(message, false);	
    }
void qpidWrapper::acknowledge()
    {
	/**
     * Acknowledges the specified message.
     */
    session.acknowledge(false);	
    }





