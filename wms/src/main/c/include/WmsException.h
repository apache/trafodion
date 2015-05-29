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
#ifndef LIBWMS_WMSEXCEPTION_H
#define LIBWMS_WMSEXCEPTION_H

#define ERROR_INIT						-1
#define ERROR_MEMORY_ALLOCATION			-2
#define ERROR_CREATE_WORKLOAD			-3
#define ERROR_OPEN						-4
#define ERROR_WRITEREAD					-4
#define ERROR_CLOSE						-5
#define ERROR_WORKLOAD_NULL				-7
#define ERROR_WORKLOAD_VALIDATION		-8
#define ERROR_CONF_FILE					-9
#define ERROR_OTHER						-10

class WmsException
{
public:
	WmsException(short error_type, long errorNum, const char *errorText);
	~WmsException();
	const char* what();

	short m_errorType;
	long m_errorNum;
	const char *m_errorText;

};

#endif /*LIBWMS_WMSEXCEPTION_H*/


