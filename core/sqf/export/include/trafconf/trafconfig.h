/**********************************************************************
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
********************************************************************/
#ifndef TRAFCONFIG_H_
#define TRAFCONFIG_H_

//
// On Windows, this would actually do something like:
// #define TRAF_Export __declspec(dllexport)
//
#define TC_Export

#ifdef USE_TRAFCONF_DIAGS
#define TC_DIAG_UNUSED __attribute__((warn_unused_result))
#else
#define TC_DIAG_UNUSED
#endif

#ifdef USE_TRAFCONF_DEPRECATED
#define TC_DIAG_DEPRECATED __attribute__((deprecated))
#else
#define TC_DIAG_DEPRECATED
#endif

#define TC_PROCESSOR_NAME_MAX          128

#define TC_REGISTRY_KEY_MAX             64
#define TC_REGISTRY_VALUE_MAX         4096
#define TC_PERSIST_PROCESSOR_NAME_MAX  TC_PROCESSOR_NAME_MAX
#define TC_PERSIST_ROLES_MAX           128
#define TC_PERSIST_KEY_MAX              64
#define TC_PERSIST_VALUE_MAX          4096
#define TC_PERSIST_KEYS_VALUE_MAX     4096
#define TC_NODES_MAX                  1536 // This can be higher when needed and
                                           // will have performance implications
                                           // in the monitor process.
                                           // NOTE: Must increment by 64, see
                                           //       monitor's msgdef.h MAX_NODES
#define TC_SPARE_NODES_MAX             256
#define TC_UNIQUE_STRING_VALUE_MAX    4096

#define TC_STORE_MYSQL             "MYSQL"
#define TC_STORE_POSTGRESQL        "POSTGRESQL"
#define TC_STORE_SQLITE            "SQLITE"

#define PERSIST_PROCESS_KEYS       "PERSIST_PROCESS_KEYS"
#define PERSIST_PROCESS_NAME_KEY   "PROCESS_NAME"
#define PERSIST_PROCESS_TYPE_KEY   "PROCESS_TYPE"
#define PERSIST_PROGRAM_NAME_KEY   "PROGRAM_NAME"
#define PERSIST_PROGRAM_ARGS_KEY   "PROGRAM_ARGS"
#define PERSIST_REQUIRES_DTM       "REQUIRES_DTM"
#define PERSIST_STDOUT_KEY         "STDOUT"
#define PERSIST_RETRIES_KEY        "PERSIST_RETRIES"
#define PERSIST_ZONES_KEY          "PERSIST_ZONES"

#define TC_ROOT_NODE               "/trafodion"
#define TC_INSTANCE_NODE           "/instance"

typedef enum {
    ProcessType_Undefined=0,                // No process type as been defined
    ProcessType_TSE,                        // Identifies a Table Storage Engine (DP2)
    ProcessType_DTM,                        // Identifies a Distributed Transaction Monitor process
    ProcessType_ASE,                        // Identifies a Audit Storage Engine (ADP)
    ProcessType_Generic,                    // Identifies a generic process
    ProcessType_NameServer,                 // Identifies a nameserver processes
    ProcessType_Watchdog,                   // Identifies the monitor's watchdog processes
    ProcessType_AMP,                        // Identifies a AMP process
    ProcessType_Backout,                    // Identifies a Backout process
    ProcessType_VolumeRecovery,             // Identifies a Volume Recovery process
    ProcessType_MXOSRVR,                    // Identifies a MXOSRVR process
    ProcessType_SPX,                        // Identifies a SeaPilot ProXy process
    ProcessType_SSMP,                       // Identifies a SQL Statistics Merge Process (SSMP)
    ProcessType_PSD,                        // Identifies the monitor's process start daemon processes
    ProcessType_SMS,                        // Identifies a SeaMonster Service process
    ProcessType_TMID,                       // Identifies a Transaction Management ID process
    ProcessType_PERSIST,                    // Identifies a generic persistent process

    ProcessType_Invalid                     // marks the end of the process
                                            // types, add any new process
                                            // types before this one
} TcProcessType_t;

typedef enum {
    ZoneType_Undefined   = 0x0000,          // No zone type defined
    ZoneType_Edge        = 0x0001,          // Zone of service only nodes
    ZoneType_Aggregation = 0x0002,          // Zone of compute only nodes
    ZoneType_Storage     = 0x0004,          // Zone of storage only nodes
    ZoneType_Excluded    = 0x0010,          // Excluded cores
    ZoneType_Any         = ( ZoneType_Edge | ZoneType_Aggregation | ZoneType_Storage ),
    ZoneType_Frontend    = ( ZoneType_Edge | ZoneType_Aggregation ),
    ZoneType_Backend     = ( ZoneType_Aggregation | ZoneType_Storage )
} TcZoneType_t;

typedef enum {
      TCDBSTOREUNDEFINED = 0     
    , TCDBMYSQL          = 1 // MySQL Database
    , TCDBPOSTGRESQL     = 2 // PostgresQL Database   [TBD]
    , TCDBSQLITE         = 3 // Sqlite Database
} TcStorageType_t;

typedef enum {
      TCSUCCESS = 0         // Successful operation
    , TCNOTIMPLEMENTED = -1 // Not implemented
    , TCNOTINIT = -2        // Database not open
    , TCALREADYINIT = -3    // Database already opened
    , TCDBOPERROR = -4      // Database operation failed
    , TCDBNOEXIST = -5      // Database operation yielded non-existent data
    , TCDBTRUNCATE = -6     // Database operation returned less data than available
    , TCDBCORRUPT = -7      // Internal processing error or database corruption
} TcError_t;

typedef struct TcNodeConfiguration_s
{
    int  nid;                                   // Node Id (logical)
    int  pnid;                                  // Physical Node ID
    char node_name[TC_PERSIST_PROCESSOR_NAME_MAX]; // short hostname
    char domain_name[TC_PERSIST_PROCESSOR_NAME_MAX]; // domain name
    int  excluded_first_core;                   // First or only core assigned
    int  excluded_last_core;                    // Last core assigned or -1
    int  first_core;                            // First or only core assigned
    int  last_core;                             // Last core assigned or -1
    int  processors;                            // Number logical processors
    int  roles;                                 // Role assigment
} TcNodeConfiguration_t;

typedef struct TcPhysicalNodeConfiguration_s
{
    int  pnid;                                  // Physical Node ID
    char node_name[TC_PERSIST_PROCESSOR_NAME_MAX]; // short hostname
    char domain_name[TC_PERSIST_PROCESSOR_NAME_MAX]; // domain name
    int  excluded_first_core;                   // First or only core assigned
    int  excluded_last_core;                    // Last core assigned or -1
    int  spare_count;                           // Number of entries in spare_pnid[]
    int  spare_pnid[TC_SPARE_NODES_MAX];           // list of pnids for which this node can be a spare 
} TcPhysicalNodeConfiguration_t;

typedef struct TcRegistryConfiguration_s
{
    char scope[TC_REGISTRY_KEY_MAX];
    char key[TC_REGISTRY_KEY_MAX];
    char value[TC_REGISTRY_VALUE_MAX];
} TcRegistryConfiguration_t;

typedef struct TcPersistConfiguration_s
{
    char persist_prefix[TC_PERSIST_KEY_MAX]; // DTM, TMID, or ... (PERSIST_PROCESS_KEYS)
    char process_name[TC_PERSIST_VALUE_MAX]; // Process name {<prefix>[<format>]}
    char process_type[TC_PERSIST_VALUE_MAX]; // DTM, TMID, PERSIST, ...
    char program_name[TC_PERSIST_VALUE_MAX]; // Program executable name (no path in name)
    char program_args[TC_PERSIST_VALUE_MAX]; // Program program arguments (runtime options)
    char std_out[TC_PERSIST_VALUE_MAX];      // STDOUT {<prefix>[<format>]}
    bool requires_DTM;                       // True when process requires transaction support
    int  persist_retries;                    // Process create retries
    int  persist_window;                     // Process create retries window (seconds)
    char persist_zones[TC_PERSIST_VALUE_MAX]; // Process creation zones {<format>}
} TcPersistConfiguration_t;


TC_Export int tc_close( void )
TC_DIAG_UNUSED;

TC_Export const char *tc_errmsg( int err )
TC_DIAG_UNUSED;

TC_Export int tc_initialize( bool traceEnabled
                           , const char *traceFileName = NULL
                           , const char *instanceNode = NULL
                           , const char *rootNode = NULL )
TC_DIAG_UNUSED;

TC_Export int tc_delete_nameserver( const char *node_name )
TC_DIAG_UNUSED;

TC_Export int tc_get_nameservers( int   *count
                                , int    max
                                , char **nodeNames )
TC_DIAG_UNUSED;

TC_Export int tc_get_nameserver( const char *node_name )
TC_DIAG_UNUSED;

TC_Export int tc_put_nameserver( const char *node_name )
TC_DIAG_UNUSED;

TC_Export TcStorageType_t tc_get_storage_type( void )
TC_DIAG_UNUSED;

TC_Export int tc_delete_node( int nid
                            , const char *node_name )
TC_DIAG_UNUSED;

TC_Export int tc_get_node( const char *node_name
                         , TcNodeConfiguration_t *node_config )
TC_DIAG_UNUSED;

TC_Export int tc_put_node( TcNodeConfiguration_t *node_config )
TC_DIAG_UNUSED;


TC_Export int tc_get_pnode( const char *node_name
                          , TcPhysicalNodeConfiguration_t *pnode_config )
TC_DIAG_UNUSED;

TC_Export int tc_put_pnode( TcPhysicalNodeConfiguration_t *pnode_config )
TC_DIAG_UNUSED;

//
// Call this to get all configured logical nodes
//
// count:                   is number of node_config array entries returned
// max:                     is size of the node_config array
// node_config:             is array of logical node configuration entries
//
// if count would be greater than max, an error is returned
// if node_config is null, max is ignored and
//    number of node_config entries is returned in count
//
TC_Export int tc_get_nodes( int           *count
                          , int            max
                          , TcNodeConfiguration_t *node_config )
TC_DIAG_UNUSED;

//
// Call this to get all configured spare nodes
//
// count:                   is number of pnode_config array entries returned
// max:                     is size of the pnode_config array
// pnode_config:            is array of spare node configuration entries
//
// if count would be greater than max, an error is returned
// if pnode_config is null, max is ignored and
//    number of pnode_config array entries is returned in count
//
TC_Export int tc_get_snodes( int                 *count
                           , int                  max
                           , TcPhysicalNodeConfiguration_t *pnode_config )
TC_DIAG_UNUSED;


TC_Export int tc_delete_persist_keys( void )
TC_DIAG_UNUSED;

TC_Export int tc_get_persist_keys( const char *persist_keys )
TC_DIAG_UNUSED;

TC_Export int tc_put_persist_keys( const char *persist_keys )
TC_DIAG_UNUSED;


TC_Export int tc_delete_persist_process( const char *persist_key_prefix )
TC_DIAG_UNUSED;

TC_Export int tc_get_persist_process( const char *persist_key_prefix
                                    , TcPersistConfiguration_t *persist_config )
TC_DIAG_UNUSED;

TC_Export int tc_put_persist_process( const char *persist_key_prefix
                                    , TcPersistConfiguration_t *persist_config )
TC_DIAG_UNUSED;


//
// Call this to get all cluster scope configuration registry entries
//
// count:                   is number of registry_config array entries returned
// max:                     is size of the registry_config array
// registry_configuration:  is the array of cluster registry entries
//
// if count would be greater than max, an error is returned
// if registry_config is null, max is ignored and
//    number of registry entries is returned in count
//
TC_Export int tc_get_registry_cluster_set( int *count
                                         , int  max
                                         , TcRegistryConfiguration_t *registry_config )
TC_DIAG_UNUSED;

//
// Call this to get all process scope configuration registry entries
//
// count:                   is number of registry_config array entries returned
// max:                     is size of the registry_config array
// registry_configuration:  is the array of process registry entries
//
// if count would be greater than max, an error is returned
// if registry_config is null, max is ignored and
//    number of registry entries entries is returned in count
//
TC_Export int tc_get_registry_process_set( int *count
                                         , int  max
                                         , TcRegistryConfiguration_t *registry_config )
TC_DIAG_UNUSED;


TC_Export int tc_get_registry_key( const char *key )
TC_DIAG_UNUSED;

TC_Export int tc_put_registry_key( const char *key )
TC_DIAG_UNUSED;


TC_Export int tc_get_registry_process( const char *process_name )
TC_DIAG_UNUSED;

TC_Export int tc_put_registry_process( const char *process_name )
TC_DIAG_UNUSED;


TC_Export int tc_get_registry_cluster_data( const char *key
                                          , const char *data )
TC_DIAG_UNUSED;

TC_Export int tc_put_registry_cluster_data( const char *key
                                          , const char *data )
TC_DIAG_UNUSED;


TC_Export int tc_get_registry_process_data( const char *process_name
                                          , const char *key
                                          , const char *data )
TC_DIAG_UNUSED;

TC_Export int tc_put_registry_process_data( const char *process_name
                                          , const char *key
                                          , const char *data )
TC_DIAG_UNUSED;


TC_Export TcStorageType_t tc_get_storage_type( void )
TC_DIAG_UNUSED;


TC_Export int tc_delete_unique_strings( int nid )
TC_DIAG_UNUSED;

TC_Export int tc_get_unique_string( int nid, int id, const char *unique_string )
TC_DIAG_UNUSED;

TC_Export int tc_put_unique_string( int nid, int id, const char *unique_string )
TC_DIAG_UNUSED;


TC_Export int tc_get_unique_string_id( int nid, const char *unique_string, int *id )
TC_DIAG_UNUSED;

TC_Export int tc_get_unique_string_id_max( int nid, int *id )
TC_DIAG_UNUSED;

#endif // TRAFCONFIG_H_
