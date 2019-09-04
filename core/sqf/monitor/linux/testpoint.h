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

#ifndef TESTPOINT_H_
#define TESTPOINT_H_

//
// Defines of env vars used to set a test point value
//

// Startup in real cluster with down node simulation
#define TP001_NODE_DOWN  "MON_TP_NODE_DOWN"     // down node name at startup
#define TRAF_EXCLUDE_LIST  "TRAF_EXCLUDE_LIST"     // exclude node
// Node re-integration failure testpoints
//   Roles:
//     new      - new monitor created by shell 
//     creator  - existing monitor where node up request originates
//     existing - existing monitor where node up request does NOT originates 
#define TP010_NODE_UP  "MON_TP010_NODE_UP"  // new before MPI_Comm_connect()
#define TP011_NODE_UP  "MON_TP011_NODE_UP"  // new before sending port info
                                            // to creator
#define TP012_NODE_UP  "MON_TP012_NODE_UP"  // new before MPI_Intercomm_merge()

#define TP013_NODE_UP  "MON_TP013_NODE_UP"  // new before connecting to existing
#define TP014_NODE_UP  "MON_TP014_NODE_UP"  // new before sending port info
                                            // to existing
#define TP015_NODE_UP  "MON_TP015_NODE_UP"  // new before sending "ready" to
                                            // creator
#define TP016_NODE_UP  "MON_TP016_NODE_UP"  // new after connection to 1/2
                                            // existing monitors



#ifdef USE_TESTPOINTS
#define TEST_POINT( TPVAR ) \
    { \
       const char *tp = getenv( TPVAR );\
       if ( tp != NULL )\
       {\
           char buf[MON_STRING_BUF_SIZE];\
           snprintf(buf, sizeof(buf), "[%s], Test point: %s, aborting\n",\
                    method_name, TPVAR);\
           mon_log_write(MON_CLUSTER_REINTEGRATE_10, SQ_LOG_ERR, buf);    \
           mon_failure_exit(true);\
       }\
    }

#else
#define TEST_POINT(tpnum)
#endif

#endif /*TESTPOINT_H_*/
