#include <string>
#include <iostream>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "generated/WmsService_types.h"
#include "Wms.h"

using namespace trafodion::wms::thrift;

string zkhost;
int zkport;

static bool parse_arguments( int argc, char *argv[] )
{
	zkhost = "localhost";
	zkport = 2182;

	char arguments[] = "i:p:";
	int character;
	optarg = NULL;

	while ( ( character = getopt( argc, argv, arguments ) ) != -1 ){
		switch ( character )
		{
		case 'p':
			zkport = atoi( optarg );
			break;
		case 'i':
			zkhost = optarg;
			break;
		default :
			;
		}
	}
	if (zkhost.length() == 0 || zkport == 0 )
		return false;

	return true;
}

int main(int argc, char **argv)
{
	if(false == parse_arguments(argc, argv )){
		cout << "parse_arguments returned false" << endl;
	}

	cout << "zkhost " << zkhost.c_str() << "," << "zkport " << zkport << endl;

	int result = wmsOpen(zkhost.c_str(), zkport);
	if(result) {
		cout << "wmsOpen failed=" << wmsGetLastErrorText() << endl;
		exit(EXIT_FAILURE);
	}
	else
		cout << "wmsOpen succeeded, result=" << result << endl;

	timeval tv;
	gettimeofday (&tv, NULL);
	long currentTimestamp = tv.tv_sec * 1000 + tv.tv_usec/1000;

	map <string,KeyValue> keyValues;
	KeyValue kv;

	kv.__set_intValue(Operation::OPERATION_BEGIN);
	keyValues.insert ( std::pair<string,KeyValue>("operation",kv));
	kv.__set_stringValue("RUNNING");
	keyValues.insert ( std::pair<string,KeyValue>("state",kv));
	kv.__set_stringValue("BEGIN");
	keyValues.insert ( std::pair<string,KeyValue>("subState",kv));
	kv.__set_longValue(currentTimestamp);
	keyValues.insert ( std::pair<string,KeyValue>("beginTimestamp",kv));
	kv.__set_longValue(currentTimestamp);
	keyValues.insert ( std::pair<string,KeyValue>("endTimestamp",kv));
	kv.__set_stringValue("trafodion");
	keyValues.insert ( std::pair<string,KeyValue>("type",kv));
	kv.__set_stringValue("Select * from manageability.nwms_schema.services;");
	keyValues.insert ( std::pair<string,KeyValue>("queryText",kv));
	kv.__set_stringValue("MXID11000001075212235857042874154000000000106U6553500_4_SQL_DATASOURCE_Q8");
 	keyValues.insert ( std::pair<string,KeyValue>("queryId",kv));
 	kv.__set_longValue(9010203);
	keyValues.insert ( std::pair<string,KeyValue>("deltaNumRows",kv));
	kv.__set_longValue(1000);
	keyValues.insert ( std::pair<string,KeyValue>("deltaRowsRetrieved",kv));
	kv.__set_longValue(5000);
	keyValues.insert ( std::pair<string,KeyValue>("deltaRowsAccessed",kv));
	kv.__set_longValue(1000000);
	keyValues.insert ( std::pair<string,KeyValue>("aggrRowsRetrieved",kv));
	kv.__set_longValue(400);
	keyValues.insert ( std::pair<string,KeyValue>("aggrRowsAccessed",kv));
	kv.__set_stringValue("wms_test");
	keyValues.insert ( std::pair<string,KeyValue>("applicationId",kv));
	kv.__set_floatValue(600000);
	keyValues.insert ( std::pair<string,KeyValue>("aggrEstimatedRowsUsed",kv));
	kv.__set_longValue(66600000);
	keyValues.insert ( std::pair<string,KeyValue>("aggrNumRowsIUD",kv));
	kv.__set_stringValue("SQL_CUR_3");
	keyValues.insert ( std::pair<string,KeyValue>("sessionId",kv));
	kv.__set_stringValue("trafuser");
	keyValues.insert ( std::pair<string,KeyValue>("userName",kv));
	kv.__set_floatValue(10000);
	keyValues.insert ( std::pair<string,KeyValue>("deltaEstimatedRowsUsed",kv));
	kv.__set_floatValue(60987653);
	keyValues.insert ( std::pair<string,KeyValue>("deltaEstimatedRowsAccessed",kv));
	kv.__set_floatValue(2000000);
	keyValues.insert ( std::pair<string,KeyValue>("aggrEstimatedRowsAccessed",kv));

	Data request;
	Data response;

	request.__set_keyValues(keyValues);
	result = wmsWriteRead(request,response);
	if(result) {
		cout << "wmsWriteRead failed =" << wmsGetLastErrorText() << endl;
		exit(EXIT_FAILURE);
	} else {
		cout << "wmsWriteRead succeeded, result=" << result << endl;
		//map <string, string>::iterator it;
		//for (it = response.begin(); it != response.end(); ++it) {
		//	cout << it->first << "=" << it->second << endl;
		//}
	}

	keyValues.clear();
/*
	request.__set_operation(Operation::OPERATION_UPDATE);
	request.__set_state("EXECUTING");
	request.__set_subState("UPDATE");
	request.__set_workloadId(response.workloadId);
	request.__set_beginTimestamp(currentTimestamp);
	request.__set_endTimestamp(currentTimestamp);
*/
	result = wmsWriteRead(request,response);
	if(result) {
		cout << "wmsWriteRead failed =" << wmsGetLastErrorText() << endl;
		exit(EXIT_FAILURE);
	} else {
		cout << "wmsWriteRead succeeded, result=" << result << endl;
		//map <string, string>::iterator it;
		//for (it = response.begin(); it != response.end(); ++it) {
		//	cout << it->first << "=" << it->second << endl;
		//}
	}
/*
	request.__set_operation(Operation::OPERATION_END);
	request.__set_state("COMPLETED");
	request.__set_subState("SUCCEEDED");
	request.__set_beginTimestamp(currentTimestamp);
	request.__set_endTimestamp(currentTimestamp);
*/
	result = wmsWriteRead(request,response);
	if(result) {
		cout << "wmsWriteRead failed =" << wmsGetLastErrorText() << endl;
		exit(EXIT_FAILURE);
	} else {
		cout << "wmsWriteRead succeeded, result=" << result << endl;
		//map <string, string>::iterator it;
		//for (it = response.begin(); it != response.end(); ++it) {
		//	cout << it->first << "=" << it->second << endl;
		//}
	}

	result = wmsClose();
	if(result) {
		cout << "wmsClose failed = " << wmsGetLastErrorText() << endl;
		exit(EXIT_FAILURE);
	}
	else
		cout << "wmsClose succeeded, result=" << result << endl;

	return 0;
}

