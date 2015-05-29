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
#ifndef LIBWMS_WMS_H
#define LIBWMS_WMS_H

using namespace std;
using namespace trafodion::wms::thrift;

// +++ Temp fix for ZK session expiry issue. Will use zk handle if sent as input
int wmsOpen(const char* zkhost, int zkport, void *zhIn=NULL);
int wmsWriteRead(Data &,Data &);
void resetKeyValue();
int wmsClose();
const char* wmsGetLastErrorText();
int getServer();

void setKeyValue(const string& key, const short value);
void setKeyValue(const string& key, const int value);
void setKeyValue(const string& key, const long value);
void setKeyValue(const string& key, const double value);
void setKeyValue(const string& key, const string value);
void setKeyValue(const string& key, const char *value);
//
void getKeyValue(const string& key, short& value);
void getKeyValue(const string& key, int& value);
void getKeyValue(const string& key, long& value);
void getKeyValue(const string& key, double& value);
void getKeyValue(const string& key, string& value);

#endif /*LIBWMS_WMS_H*/


