// ===============================================================================================
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
// ===============================================================================================
#ifndef WMS_EVENTS_UTIL_DLL
#define WMS_EVENTS_UTIL_DLL

int64 getValidJulianTimestamp(const int64 usec_timestamp);
int64  utc2lct_useconds(const int64 utcTimestamp);
time_t utc2lct_seconds(const time_t utcTimestamp);
int64  utc2lct_useconds_jts(const int64 utcTimestamp);
int64  lct2utc_useconds(const int64 lctTimestamp);
time_t lct2utc_seconds(const time_t lctTimestamp);
int64  lct2utc_useconds_jts(const int64 lctTimestamp);
char * _itoa(int n, char *buff, int base);

extern bool isWms;
bool findString(char *p, int *idx, string msg);
int getComponentId();
long getLongEventId(int type, int component_id, int event_id, int max_len);

#endif
