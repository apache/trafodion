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

#ifndef TRAFCONFIGLOG_H_
#define TRAFCONFIGLOG_H_

using namespace std;

#define TC_LOG_BUF_SIZE 256

#define TC_LOG_EMERG   0   /* system is unusable */
#define TC_LOG_ALERT   1   /* action must be taken immediately */
#define TC_LOG_CRIT    2   /* critical conditions */
#define TC_LOG_ERR     3   /* error conditions */
#define TC_LOG_WARNING 4   /* warning conditions */
#define TC_LOG_NOTICE  5   /* normal but significant condition */
#define TC_LOG_INFO    6   /* informational */
#define TC_LOG_DEBUG   7   /* debug-level messages */

int TcLogWrite(int event_type, int severity, char *evl_buf);

#endif
