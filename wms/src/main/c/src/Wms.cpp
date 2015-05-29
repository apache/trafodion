/**
 *(C) Copyright 2013 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
using namespace std;

#include "GlobalHeader.h"
#include "WmsException.h"
#include "WmsZookeeper.h"
#include "generated/WmsService.h"
#include "generated/WmsService_types.h"
#include "Wms.h"

using namespace trafodion::wms::thrift;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

boost::shared_ptr<TTransport> tsocket_ptr;
boost::shared_ptr<TTransport> ttransport_ptr;
boost::shared_ptr<TProtocol> tprotocol_ptr;

Header header;
Request request;
Response response;

map <string,KeyValue> _keyValues;

const char*
wmsGetLastErrorText(){
	return lastErrorText.str().c_str();
}

int
wmsOpen(const char* zkhost, int zkport, void *zhIn){
	try	{
		if(isOpen==true)
			return 0;

		// +++ Temp fix for ZK session expiry issue. Will use zk handle if sent as input
		if( zhIn )
		{
			zh = (zhandle_t *)zhIn;
			myZkHandle = false;
		}
		else
		{
			if(zkhost == 0)
				zkhost = "localhost";
			if(zkport == 0)
				zkport = 2181;

			myZkHandle = true;
		}

		zk_ip_port << zkhost << ":" << zkport;

		getServer();
		isOpen=true;
	} catch( WmsException e) {
		lastErrorType = e.m_errorType;
		lastErrorNum = e.m_errorNum;
		lastErrorText.str("");
		lastErrorText << e.m_errorText;
		return -1;
	}
	return 0;
}

int
wmsWriteRead(Data &requestData,Data &responseData){
	try	{
		if(isOpen==false)
			throw WmsException(ERROR_WRITEREAD, 0, (char *)"Connection is not open");

		try {

			requestData.__set_keyValues(_keyValues);
			request.__set_header(header);
			request.__set_data(requestData);
			header.__set_clientUserName(getenv("USER"));
			header.__set_clientTimestamp(connectionInfo);

			WmsServiceClient client(tprotocol_ptr);
			client.writeread(response,request);

			request.header.__set_serverLastUpdated(response.header.serverLastUpdated);

			responseData = response.data;
			_keyValues.clear();
			_keyValues = responseData.keyValues;

		} catch (TException &tx) {
			wmsClose();
			throw WmsException(ERROR_WRITEREAD, 0, (char *)tx.what());
		} catch (WmsException e){
			wmsClose();
			throw WmsException(ERROR_WRITEREAD, 0, (char *)e.what());
		}
	} catch( WmsException e) {
		lastErrorType = e.m_errorType;
		lastErrorNum = e.m_errorNum;
		lastErrorText.str("");
		lastErrorText << e.m_errorText;
		return -1;
	}
	return 0;
}

void resetKeyValue()
{
	_keyValues.clear();
}

void setKeyValue(const string& key, const short value)
{
	KeyValue kv;
	kv.__set_shortValue(value);
	_keyValues.insert( std::pair<string,KeyValue>(key,kv));
}

void setKeyValue(const string& key, const int value)
{
	KeyValue kv;
	kv.__set_intValue(value);
	_keyValues.insert( std::pair<string,KeyValue>(key,kv));
}

void setKeyValue(const string& key, const long value)
{
	KeyValue kv;
	kv.__set_longValue(value);
	_keyValues.insert( std::pair<string,KeyValue>(key,kv));
}

void setKeyValue(const string& key, const double value)
{
	KeyValue kv;
	kv.__set_floatValue(value);
	_keyValues.insert( std::pair<string,KeyValue>(key,kv));
}

void setKeyValue(const string& key, const string value)
{
	KeyValue kv;
	kv.__set_stringValue(value);
	_keyValues.insert( std::pair<string,KeyValue>(key,kv));
}

void setKeyValue(const string& key, const char *value)
{
	KeyValue kv;

	if(value) {
		kv.__set_stringValue(value);
		_keyValues.insert( std::pair<string,KeyValue>(key,kv));
	}
}

void getKeyValue(const string& key, short& value)
{
	KeyValue kv;
	kv = _keyValues[key];
	value = kv.shortValue;
}

void getKeyValue(const string& key, int& value)
{
	KeyValue kv;
	kv = _keyValues[key];
	value = kv.intValue;
}

void getKeyValue(const string& key, long& value)
{
	KeyValue kv;
	kv = _keyValues[key];
	value = kv.longValue;
}

void getKeyValue(const string& key, double& value)
{
	KeyValue kv;
	kv = _keyValues[key];
	value = kv.floatValue;
}

void getKeyValue(const string& key, string& value)
{
	KeyValue kv;
	kv = _keyValues[key];
	value = kv.stringValue;
}

int
wmsClose(){
	try	{
		if( isOpen == false )
			return 0;

		ttransport_ptr->close();
		ttransport_ptr.reset();
		tsocket_ptr.reset();
		tprotocol_ptr.reset();
		wmsServers.clear();
		closeZkSession();
		isOpen=false;
	} catch( WmsException e) {
		lastErrorType = e.m_errorType;
		lastErrorNum = e.m_errorNum;
		lastErrorText.str("");
		lastErrorText << e.m_errorText;
		return -1;
	}
	return 0;
}

int
getServer() {
	timeval tv;
	gettimeofday (&tv, NULL);
	connectionInfo = tv.tv_sec * 1000 + tv.tv_usec/1000;
//	unsigned int roundRobinIndex=0;
//	unsigned int roundRobinMax;
//	static bool bSetSeed = true;

//	if (bSetSeed == true){
//		bSetSeed = false;
//		srand ( time(NULL) );
//	}

	getZkServerList();

	if (wmsServers.empty())
		throw WmsException(ERROR_OPEN, 0, (char *)"No running WMS servers found");

//	roundRobinMax=rand() % 10 + 1;

//	for(unsigned int j=0; j < roundRobinMax; j++)
//		roundRobinIndex = rand() % (wmsServers.size() - 1) + 1;

	char *token,*lasts;
	for(unsigned i=0; i < wmsServers.size(); ++i) {
//	for(unsigned i=0; i < wmsServers.size(); ++i, ++roundRobinIndex){
//		roundRobinIndex = roundRobinIndex % wmsServers.size();

		string host = wmsServers.front();
//		string host = wmsServers[roundRobinIndex];

		if ((token = strtok_r((char*)host.c_str(), ":", &lasts)) != NULL)//ip addr
			wmshost = token;
		if ((token = strtok_r(NULL, ":", &lasts)) != NULL)//skip server instance id
		if ((token = strtok_r(NULL, ":", &lasts)) != NULL)//thrift port
			wmsport = atoi(token);

		cout << "wmshost:" << wmshost << ",wmsport:" << wmsport << endl;

		try {
			boost::shared_ptr<TTransport> socket(new TSocket(wmshost, wmsport));
			//boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
			boost::shared_ptr<TTransport> transport(new TFramedTransport(socket));
			boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

			WmsServiceClient client(protocol);
			transport->open();
			client.ping(connectionInfo);
			tsocket_ptr = socket;
			ttransport_ptr = transport;
			tprotocol_ptr = protocol;
			return 0;
		} catch (TException &tx) {
			wmsServers.pop_front();
			wmsServers.push_back(host);
			lastErrorType = ERROR_OPEN;
			lastErrorNum = 0;
			lastErrorText.str("");
			lastErrorText << tx.what();
		}
	}

	throw WmsException(ERROR_OPEN, 0, (char *)"No active WMS servers found");
}

