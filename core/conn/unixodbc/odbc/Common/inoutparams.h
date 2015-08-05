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

#ifndef INOUTPARAMS_H
#define INOUTPARAMS_H

#define GetObjRefHdl_in_params			4
#define GetObjRefHdl_out_params			6
#define UpdateSrvrState_in_params		3
#define UpdateSrvrState_out_params		5
#define StopSrvr_in_params				4
#define StopSrvr_out_params				1
#define InitializeDialogue_in_params	3
#define InitializeDialogue_out_params	2
#define TerminateDialogue_in_params		1
#define TerminateDialogue_out_params	1
#define SetConnectionOption_in_params	4
#define SetConnectionOption_out_params	2
#define Prepare_in_params				7
#define Prepare_out_params				5
#define ExecDirect_in_params			9
#define ExecDirect_out_params			5
#define PrepareRowset_in_params			9
#define PrepareRowset_out_params		5
#define ExecuteN_in_params				8
#define ExecuteN_out_params				3
#define ExecuteRowset_in_params			8
#define ExecuteRowset_out_params		3
#define ExecDirectRowset_in_params		10
#define ExecDirectRowset_out_params		5
#define FetchRowset_in_params			6
#define FetchRowset_out_params			4
#define FetchPerf_in_params				6
#define FetchPerf_out_params			4
#define Close_in_params					3
#define Close_out_params				3
#define EndTransaction_in_params		2
#define EndTransaction_out_params		2
#define GetSQLCatalogs_in_params		18
#define GetSQLCatalogs_out_params		4
#define ExecuteCall_in_params			8
#define ExecuteCall_out_params			4
#define MonitorCall_in_params			1
#define MonitorCall_out_params			1

#define RegProcess_in_params				4
#define RegProcess_out_params				2
#define WouldLikeToLive_in_params			2
#define WouldLikeToLive_out_params			2
#define StartAS_out_params					2
#define StopAS_in_params					2
#define StopAS_out_params					1
#define StartDS_in_params					1
#define StartDS_out_params					2
#define StopDS_in_params					3
#define StopDS_out_params					1
#define StatusAS_out_params					2
#define StatusDS_in_params					1
#define StatusDS_out_params					2
#define StatusDSDetail_in_params			1
#define StatusDSDetail_out_params			2
#define StatusDSAll_out_params				2
#define StatusSrvrAll_out_params			2
#define DataSourceConfigChanged_in_params	3
#define DataSourceConfigChanged_out_params	1
#define EnableTrace_in_params				4
#define EnableTrace_out_params				1
#define DisableTrace_in_params				4
#define DisableTrace_out_params				1

#define GetDataSourceValues_in_params		1
#define SetDSStatus_in_params				3
#define SetASStatus_in_params				2
#define StopServer_in_params				3
#define StopServer_out_params				1
#define EnableServerTrace_in_params			2
#define EnableServerTrace_out_params		1
#define DisableServerTrace_out_params		1
#define EnableServerStatistics_in_params	2
#define EnableServerStatistics_out_params	1
#define DisableServerStatistics_in_params	1
#define DisableServerStatistics_out_params	1
#define UpdateServerContext_in_params		1
#define UpdateServerContext_out_params		1

#define GetObjectNameList_in_params			1
#define GetObjectNameList_out_params		3
#define GetDataSource_in_params				1
#define GetDataSource_out_params			3
#define GetStartupConfigValues_out_params	5
#define SetASStatus_out_params				2
#define AddNewDataSource_in_params			1
#define AddNewDataSource_out_params			2
#define CheckDataSourceName_in_params		1
#define CheckDataSourceName_out_params		2
#define DropDataSource_in_params			1
#define DropDataSource_out_params			2
#define GetDSNControl_in_params				1
#define GetDSNControl_out_params			4
#define GetDataSourceValues_in_params		1
#define GetDataSourceValues_out_params		3
#define GetEnvironmentValues_in_params		2
#define GetEnvironmentValues_out_params		3
#define GetResourceValues_in_params			2
#define GetResourceValues_out_params		3
#define SetDSNControl_in_params				2
#define SetDSNControl_out_params			2
#define SetDSStatus_out_params				2
#define SetDataSource_in_params				1
#define SetDataSource_out_params			2
#define SetEnvironmentValues_in_params		3
#define SetEnvironmentValues_out_params		2
#define SetResourceValues_in_params			3
#define SetResourceValues_out_params		2
#define UserAuthenticate_out_params			5
#define ChangePassword_out_params			3

#endif
