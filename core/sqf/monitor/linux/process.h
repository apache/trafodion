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

#ifndef PROCESS_H_
#define PROCESS_H_

#include <string>
#include <map>
#include <set>

using namespace std;

#include <semaphore.h>
#include <string.h>

#include "notice.h"
#include "open.h"
#include "config.h"

typedef map<string, CProcess *> nameMap_t;
typedef map<int, CProcess *> pidMap_t;

class CNode;

class CProcessContainer
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class

 public:
    CProcessContainer( void );
    CProcessContainer( bool nodeContainer );
    virtual ~CProcessContainer( void );

    void AttachProcessCheck( struct message_def *lmsg );
    void AddToList( CProcess *process );
    void AddToListL( CProcess *process );
    void AddToNameMap( CProcess *process );
    void DelFromNameMap( CProcess *process );
    void AddToPidMap(int pid, CProcess *process);
    void DelFromPidMap( CProcess *process );
    void DeleteFromList( CProcess *process );
    void RemoveFromList( CProcess *process );
    void RemoveFromListL( CProcess *process );
    void Bcast( struct message_def *msg );
    char *BuildOurName( int nid, int pid, char *name );
    bool CancelDeathNotification( int nid
                                , int pid
                                , Verifier_t verifier
                                , _TM_Txid_External trans_id );
#ifndef NAMESERVER_PROCESS
    void Child_Exit ( CProcess * parent );
    void ChildUnHooked_Exit ( CProcess * parent );
#endif
    void CleanUpProcesses( void );
    CProcess *CloneProcess( int nid, 
                            PROCESSTYPE type, 
                            int priority,
                            int backup,
                            bool unhooked,
                            char *name, 
                            char *port, 
                            int os_pid,
                            int verifier,
                            int parent_nid, 
                            int parent_pid, 
                            int parent_verifier,
                            bool event_messages,
                            bool system_messages,
#ifdef NAMESERVER_PROCESS
                            char *path, 
                            char *ldpath, 
                            char *program, 
#else
                            strId_t pathStrId,
                            strId_t ldpathStrId,
                            strId_t programStrId,
#endif
                            char *infile,
                            char *outfile,
                            struct timespec *creation_time,
                            int origPNidNs);
    void close_fds( void );
    CProcess *CompleteProcessStartup( char *process_name, 
                                      char *port, 
                                      int os_pid, 
                                      bool event_messages,
                                      bool system_messages,
                                      struct timespec *creation_time,
                                      int origPNidNs);
    CProcess *CreateProcess( CProcess *parent, 
                             int nid,
#ifdef NAMESERVER_PROCESS
                             int pid,
                             Verifier_t verifier,
                             bool event_messages,
                             bool system_messages,
#endif
                             PROCESSTYPE Type,
                             int debug,
                             int priority,
                             int backup,
                             bool unhooked,
                             char *process_name, 
#ifdef NAMESERVER_PROCESS
                            char *path, 
                            char *ldpath, 
                            char *program, 
#else
                             strId_t pathStrId,
                             strId_t ldpathStrId,
                             strId_t programStrId,
#endif
                             char *infile,
                             char *outfile
                             , void *tag
                             , int & result
                             );
#ifdef NAMESERVER_PROCESS
    void DeleteAllDown();
#endif
    bool Dump_Process( CProcess *dumper, CProcess *process, char *core_path );
    void DumpCallback( int nid, pid_t pid, int status );
    void Exit_Process( CProcess *process, bool abend, int downNode );
#ifndef NAMESERVER_PROCESS
    static CProcess *ParentNewProcReply ( CProcess *process, int result );
#else
    static CProcess *MonReply ( CProcess *process, int result );
#endif
    CProcess *GetFirstProcess( void ) { return(head_); };
    CProcess *GetLastProcess( void ) { return(tail_); };
    inline sem_t *GetMutex() { return Mutex; };
    inline int GetNumProcs( void ) { return(numProcs_); };
    CProcess *GetProcess( int pid );
    CProcess *GetProcess( const char *name, bool checkstate=true );
    CProcess *GetProcess( int pid, Verifier_t verifier, bool checkstate=true );
    CProcess *GetProcess( const char *name, Verifier_t verifier, bool checkstate=true );
    CProcess *GetProcessByType( PROCESSTYPE type );
    inline nameMap_t *GetNameMap() { return nameMap_; };
    inline pidMap_t *GetPidMap() { return pidMap_; };
    CProcess *GetProcessLByType( PROCESSTYPE type );
#ifndef NAMESERVER_PROCESS
    void KillAll( STATE node_State, CProcess *process );
    void KillAllDown();
    void KillAllDownSoft();
#endif
    char *NormalizeName( char *name );
    bool Open_Process( int nid, int pid, Verifier_t verifier, int death_notification, CProcess *process );
    int ProcessCount( void ) { CAutoLock alock(pidMapLock_.getLocker()); return(numProcs_); }
    void SetProcessState( CProcess *process, STATE state, bool abend, int downNode=-1 );
    void CheckFdState ( int fd );
    void PidHangupSet ( int pid );
    void PidHangupClear ( int pid );
    void PidHangupCheck ( time_t now );
    inline  void SetFirstProcess( CProcess *process ) { head_ = process; }
    inline  void SetLastProcess( CProcess *process ) { tail_ = process; }
    inline void SetNameMap( nameMap_t *nameMap ) { nameMap_ = nameMap; };
    inline void SetPidMap( pidMap_t *pidMap ) { pidMap_ = pidMap; };
    bool WhoOpenedMe( CProcess *process, struct message_def *msg );
    bool WhoOpenedMe( CProcess *process, struct message_def *msg, CProcess *entry );
    bool WhoOpenedMe( CProcess *process, int pid, struct message_def *msg );

protected:
    inline void SetNumProcs( int numProcs ) { numProcs_ = numProcs; };

private:
    int    numProcs_; // Number of processes in container
    sem_t *Mutex;

    bool   nodeContainer_;  // true when physical node process container
    bool   processNameFormatLong_; // when true process name format is: 
                                   // '$Zxxxxpppppp' xxxx = nid, pppppp = pid
    nameMap_t *nameMap_;
    pidMap_t *pidMap_;
    CLock pidMapLock_;
    CLock nameMapLock_;
    CProcess *head_;
    CProcess *tail_;

    // Set of process ids for which we have received a broken pipe
    // indication but have not yet processed a child death signal.
    typedef set<int>           hungupPids_t;
    hungupPids_t               hungupPids_;
    CLock                      hungupPidsLock_;

    bool RestartPersistentProcess( CProcess *process, int downNode );

    // Constant that gives the number of seconds that should elapse
    // following the receipt of a pipe hangup before checking process
    // status in PidHangupCheck.
    enum ProcessDeathInfo_t {PROCESS_DEATH_MARGIN = 10};

};

class CProcess
{
    friend void CProcessContainer::AddToList(CProcess *process);
    friend void CProcessContainer::AddToListL(CProcess *process);
    friend void CProcessContainer::DeleteFromList( CProcess *process );
    friend void CProcessContainer::RemoveFromList( CProcess *process );
    friend void CProcessContainer::RemoveFromListL( CProcess *process );
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
 public:

    CProcess( CProcess *parent, 
              int nid, 
              int pid,
#ifdef NAMESERVER_PROCESS
              Verifier_t verifier,
#endif
              PROCESSTYPE type,
              int priority,
              int backup, 
              bool debug,
              bool unhooked, 
              char *name, 
#ifdef NAMESERVER_PROCESS
              char *path,
              char *ldpath,
              char *program,
#else
              strId_t  pathStrId, 
              strId_t  ldpathStrId, 
              strId_t  programStrId,
#endif
              char *infile,
              char *outfile);
    ~CProcess( void );

    typedef struct nidPid_s { int nid; int pid; } nidPid_t;

    bool CancelDeathNotification( int nid
                                , int pid
                                , Verifier_t verifier
                                , _TM_Txid_External trans_id );
    void CompleteDump(DUMPSTATUS status, char *core_file);
    void CompleteProcessStartup( char *port, int os_pid, bool event_messages, bool system_messages, bool preclone, struct timespec *creation_time, int origPNidNs );
    void CompleteRequest( int status );
    bool Create (CProcess *parent, void* tag, int & result);
    bool Dump (CProcess *dumper, char *core_path);
    void DumpBegin(int dumperNid, int dumperPid, Verifier_t dumperVerifier, char *core_path);
    void DumpEnd(DUMPSTATUS status, char *core_file);
    void Exit( CProcess *parent );
    void GenerateEvent( int event_id, int length, char *data );
    inline int GetAbort( void ) { return abort_; }
    inline void SetAbort( bool abort ) { abort_ = abort; }
    CProcess *GetBackup( void );
    inline CProcess *GetNext( void ) { return ( next_ ); }
    inline CProcess *GetNextL( void ) { return ( nextL_ ); }
    inline int GetNid ( ) { return Nid; }
    inline void SetNid ( int nid ) { Nid = nid; }
    inline int GetPid ( ) { return Pid; }
    inline int GetPidAtFork ( ) { return PidAtFork_; }
    inline void SetPid ( pid_t pid ) { Pid = pid; }
    inline int GetVerifier ( ) { return Verifier; }
    inline void SetVerifier ( int verifier ) { Verifier = verifier; }
    inline void SetVerifier ( ); // creates a new verifier based on time
    inline const char * GetName ( ) { return Name; } 
    void SetName ( const char * name )
        { strncpy( Name, name, MAX_PROCESS_NAME);
          Name[MAX_PROCESS_NAME-1] = '\0'; }
    inline bool IsEventMessages () { return Event_messages; }
    inline bool IsSystemMessages () { return System_messages; }
    inline bool IsNowait() { return Nowait; }
    inline void SetNowait ( bool nowait ) { Nowait = nowait; }
    inline bool IsBackup() { return Backup; }
    inline void SetBackup ( bool backup ) { Backup = backup; }
    inline bool IsAttached ( ) { return Attached; }
    inline void SetAttached ( bool attached ) { Attached = attached; }
    inline bool IsDeletePending ( ) { return DeletePending; }
    inline void SetDeletePending ( bool pending ) { DeletePending = pending; }
    inline bool IsClone ( ) { return Clone; }
    inline void SetClone ( bool isClone ) { Clone = isClone; }
    inline bool IsStartupCompleted ( ) { return StartupCompleted; }
    inline bool IsOpened ( ) { return (OpenedCount > 0); }
    inline void SetStartupCompleted ( bool completed ) 
                          { StartupCompleted = completed; }
    inline bool IsFirstInstance ( ) { return firstInstance_; }
    inline void SetFirstInstance ( bool firstInstance ) { firstInstance_ = firstInstance; }
    inline bool IsPersistent ( ) { return Persistent; }
    inline void SetPersistent ( bool isPersistent ) { Persistent = isPersistent; }
    inline bool IsPaired ( ) { return Paired; }
    inline void SetPaired ( bool isPaired ) { Paired = isPaired; }
    inline bool IsUnhooked ( ) { return UnHooked; }
    inline bool IsAbended ( ) { return Abended; }
    inline void SetAbended ( bool abended ) { Abended = abended; }
    inline CProcess * GetParent ( ) { return Parent; }
    inline int GetParentPid ( ) { return Parent_Pid; }
    inline int GetParentNid ( ) { return Parent_Nid; }
    inline int GetParentVerifier( ) { return Parent_Verifier; }
    inline void SetParent ( CProcess *parent ) { Parent = parent; }
    inline void SetParentPid ( pid_t pid ) { Parent_Pid = pid; }
    inline void SetParentNid ( int nid)    { Parent_Nid = nid; }
    inline void SetParentVerifier ( int verifier) { Parent_Verifier = verifier; }
    inline int GetPairParentPid ( ) { return PairParentPid; }
    inline int GetPairParentNid ( ) { return PairParentNid; }
    inline int GetPairParentVerifier( ) { return PairParentVerifier; }
    inline void SetPairParentPid ( pid_t pid ) { PairParentPid = pid; }
    inline void SetPairParentNid ( int nid )   { PairParentNid = nid; }
    inline void SetPairParentVerifier( int verifier ) { PairParentVerifier = verifier; }
    inline int GetPriority ( ) { return Priority; }
    inline void SetTag ( long long tag ) { Tag = tag; }
    inline int GetReplyTag ( ) { return ReplyTag; }
    inline void SetReplyTag ( int tag ) { ReplyTag = tag; }
    inline const char * GetPort ( ) { return Port; }
    inline PROCESSTYPE GetType ( ) { return Type; }
    inline int GetLastNid () { return LastNid; }
    inline void SetLastNid ( int nid ) { LastNid = nid; }
    inline long GetPersistentCreateTime ( ) { return PersistentCreateTime; }
    inline void SetPersistentCreateTime ( long time ) { PersistentCreateTime = time; }
    inline int GetPersistentRetries ( ) { return PersistentRetries; }
    inline void SetPersistentRetries ( int value ) { PersistentRetries = value; }
    inline DUMPSTATE GetDumpState ( ) { return DumpState; }
    inline DUMPSTATUS GetDumpStatus ( ) { return DumpStatus; }
    inline void SetDumpState ( DUMPSTATE state ) { DumpState = state; }
    inline void SetDumpStatus ( DUMPSTATUS status ) { DumpStatus = status; }
    inline int GetDumperPid ( ) { return DumperPid; }
    inline int GetDumperNid ( ) { return DumperNid; }
    inline int GetDumperVerifier ( ) { return DumperVerifier; }
    inline const char * GetDumpFile () { return dumpFile_.c_str(); }
#ifdef NAMESERVER_PROCESS
    inline int GetMonSockFd( ) { return monSockFd_; }
    inline void SetMonSockFd( int sockFd ) { monSockFd_ = sockFd; }
    inline int GetOrigPNidNs( ) { return origPNidNs_; }
    inline void SetOrigPNidNs( int pnid ) { origPNidNs_ = pnid; }
#endif

#ifndef NAMESERVER_PROCESS
    inline CNotice * GetNoticeHead() { return NoticeHead; }
#endif

    CProcess *GetProcessByType( PROCESSTYPE type );
    inline pid_t GetPriorPid ( ) { return priorPid_; }
    inline void  SetPriorPid ( pid_t pid ) { priorPid_ = pid; }
    CProcess *GetProcessLByType( PROCESSTYPE type );
    inline STATE GetState() { return State_; }
    bool MakePrimary(void);
    bool Open( CProcess *opened_process, int death_notification );
#ifndef NAMESERVER_PROCESS
    CNotice *RegisterDeathNotification( int nid
                                      , int pid
                                      , Verifier_t verifier
                                      , const char *name
                                      , _TM_Txid_External trans_id );
    void ReplyNewProcess (struct message_def * reply_msg, CProcess * process,
                          int result);
#endif
    void SendProcessCreatedNotice(CProcess *parent, int result);
    struct timespec GetCreationTime () { return CreationTime; }
    void SetCreationTime(int os_pid);
    void SetupFifo(int attachee_nid, int attachee_pid);
    inline void SetState ( STATE new_state ) { State_ = new_state; };
    inline int  FdStdin() { return fd_stdin_; };
    inline void FdStdin(int fd) { fd_stdin_ = fd; };
    inline int  FdStdout() { return fd_stdout_; };
    inline int  FdStderr() { return fd_stderr_; };

    int argc() { return argc_; }
    int userArgvLen()         { return userArgvLen_; }
    const char *userArgv()    { return userArgv_; }

    void userArgs ( int argc, int argvLen, const char * argvList );
    void userArgs ( int argc, char user_argv[MAX_ARGS][MAX_ARG_SIZE] );
    int getUserArgs( char user_argv[MAX_ARGS][MAX_ARG_SIZE] );

#ifdef NAMESERVER_PROCESS
    const char* path()        { return path_.c_str(); };
    const char* ldpath()      { return ldpath_.c_str(); };
#else
    const char* path();
    const char* ldpath();
#endif
    const char* program()     { return program_.c_str(); };
    bool isCmpOrEsp()         { return cmpOrEsp_; }
    const char *infile()      { return infile_.c_str(); };
    const char *outfile()     { return outfile_.c_str(); };

#ifndef NAMESERVER_PROCESS
    strId_t pathStrId()       { return pathStrId_; };
    strId_t ldPathStrId()     { return ldpathStrId_; };
    strId_t programStrId()    { return programStrId_; }
#endif

    const char *fifo_stdin()  { return fifo_stdin_.c_str(); };
    const char *fifo_stdout() { return fifo_stdout_.c_str(); };
    const char *fifo_stderr() { return fifo_stderr_.c_str(); };

    bool isOwned() { return owned_; }
    void resetOwned() { owned_ = false; }
    long getOwner() { return ownerId_; }
    void setOwner( long id ) { owned_ = true; ownerId_ = id; }
    inline void incrReplRef() { ++replRefCount_; }
    inline int  decrReplRef() { --replRefCount_; return replRefCount_; }
    inline int replRefCount() { return replRefCount_; }

#ifndef NAMESERVER_PROCESS
    void parentContext (struct message_def * msg) { requestBuf_ = msg; }
    struct message_def * parentContext ( void ) { return requestBuf_; }
#else
    void SetMonContext (struct message_def * msg) { requestBuf_ = msg; }
    struct message_def * GetMonContext ( void ) { return requestBuf_; }
#endif

    void SetHangupTime () { clock_gettime(CLOCK_REALTIME, &hangupTime_); }
    time_t GetHangupTime () { return hangupTime_.tv_sec; }

#ifndef NAMESERVER_PROCESS
    void childAdd ( int nid, int pid );
    int childCount ( void );
    void childRemove ( int nid, int pid );
    bool childRemoveFirst ( nidPid_t & child );

    void childUnHookedAdd( int nid, int pid );
    int childUnHookedCount( void );
    void childUnHookedRemove( int nid, int pid );
    bool childUnHookedRemoveFirst( nidPid_t & child );
#endif

    struct message_def * GetDeathNotice ( void );
    void PutDeathNotice( struct message_def * );


    bool procExitReg(CProcess *targetProcess, _TM_Txid_External transId);
    void procExitNotifierNodes( void );
    void procExitUnregAll( _TM_Txid_External transId );

    void validateObj( void );

    struct message_def * DeathMessage( );

    void Switch( CProcess *parent );

protected:
private:
    enum PickStdFile_t {PICK_STDIN, PICK_STDOUT};
    bool PickStdfile(PickStdFile_t whichStdfile, char (&file)[MAX_PROCESS_PATH],
                     int &AncestorNid, int &AncestorPid);
    void SetupPipe(int orig_fd, int unused_pipe_fd, int pipe_fd);
    void RedirectStdFiles(int pfds_stdin[2], int pfds_stdout[2],
                          int pfds_stderr[2]);
    void setEnvStr ( char **envp, int &countEnv, const char *str );
    void setEnvStrVal ( char **envp, int &countEnv, const char *str,
                        const char *val);
    void setEnvIntVal ( char **envp, int &countEnv, const char *str, int val);
    void setEnvRegGroupVals(CConfigGroup *group, char **envp, int &countEnv);
    void setEnvFromRegistry ( char **envp, int &countEnv );

    int            Nid;
    int            Pid;
    Verifier_t     Verifier; 
    pid_t          PidAtFork_;
    PROCESSTYPE    Type;
    char           Port[MPI_MAX_PORT_NAME];
    char           Name[MAX_PROCESS_NAME];  // process name

    bool           Event_messages;   // true if process wants event messages  
    bool           System_messages;  // true if process wants system messages  
    bool           Paired;           // true if this is a functional copy from another node  
    bool           Clone;            // true if this is a nonfunctional copy from another node
    bool           Debug;            // true if needs to be started in debug mode
    bool           DeletePending;    // true if it will be deleted after replication
    bool           StartupCompleted; // true when process has completed startup protocol.
    bool           Backup;           // true if backup process, then Parent_Pid is primary 
    bool           Abended;              // true if process terminated abnormally
    bool           Attached;             // true if process was attached to monitor (not child)
    bool           abort_;               // true if terminating persistent process (no restart)
    bool           Persistent;           // true if monitor is to persist process on failure
    bool           UnHooked;             // If parent dies, this process should exit
    bool           Nowait;               // If parent request a process creation notice
    long           PersistentCreateTime; // seconds since the last recreated process
    int            PersistentRetries;    // number of time recreated the process
    long long      Tag;                  // User tag to send with process creation notice
    int            Priority;             // priority of process  
    CProcess      *Parent;
    int            Parent_Nid;           // Node ID of process that created this process
    int            Parent_Pid;           // Process ID of process that created this process
    Verifier_t     Parent_Verifier;      // Verifier of process that created this process
    int            PairParentNid;        // Node ID of process that created this process pair
    int            PairParentPid;        // Process ID of process that created this process pair
    Verifier_t     PairParentVerifier;   // Verifier of process that created this process pair
    int            ReplyTag;             // MPI message tag to use for pending reply to service request.
    int            OpenedCount;          // Num of current process opens on this proces
    int            LastNid;              // Last nid used for newprocess req from this process
    DUMPSTATE      DumpState;            // state of dump
    DUMPSTATUS     DumpStatus;           // status of core file
    int            DumperNid;            // Node ID of process requesting dump
    int            DumperPid;            // Process ID of process requesting dump
    Verifier_t     DumperVerifier;       // Verifier of process requesting dump
    string         dumpFile_ ;           // core-file
    struct timespec CreationTime;        // creation time

    pid_t        priorPid_; // for restarted persistent process, the
                            // previous process id.
    STATE        State_;

    CProcess    *next_;     // next process in logical node container list
    CProcess    *prev_;     // previous process in logial node container list
    CProcess    *nextL_;    // next process in physical node container list
    CProcess    *prevL_;    // previous process in physical node container list

    int          Last_error;

    int          argc_;
    int          userArgvLen_;
    char         *userArgv_;

    string       path_;          // process's object lookup path to program
    string       ldpath_;        // process's library load path for program
    string       program_;       // program file name
    strId_t      programStrId_;
    strId_t      pathStrId_;
    strId_t      ldpathStrId_;
    bool         firstInstance_; // reset on persistent process re-creation
    bool         cmpOrEsp_;
    string       trafRootZnode_;  // TRAF_ROOT_ZNODE passed to object file
    string       trafConf_;     // TRAF_CONF passed to object file
    string       trafHome_;     // TRAF_HOME passed to object file
    string       trafLog_;      // TRAF_LOG passed to object file
    string       trafVar_;      // TRAF_VAR passed to object file

    string       infile_;    // process's stdin
    string       outfile_;   // process's stdout
    string       corefile_;  // last core file generated

    string       fifo_stdin_;
    string       fifo_stdout_;
    string       fifo_stderr_;
    int          fd_stdin_;
    int          fd_stdout_;
    int          fd_stderr_;

    bool         owned_;   // true if owned by a request, false otherwise
    long         ownerId_; // Request id of owner
    int          replRefCount_;

    struct timespec hangupTime_;  // Timestamp for "broken stderr pipe"
    
    // Reference to parent's ReqType_NewProcess request buffer.  Used for 
    // "waited" new process requests so that reply can be sent once
    // process startup message is received.
    struct message_def * requestBuf_;

    enum  { MAX_CHILD_ENV_VARS = 300 };

#ifndef NAMESERVER_PROCESS
    // Container to keep track of this process' children created on
    // the local node.  Needed because if this process abornmally terminates
    // the children will be terminated too.
    typedef list<nidPid_t> nidPidList_t;
    nidPidList_t children_;
    nidPidList_t childrenUnHooked_;   // only used with Name Server enabled

    // Lock for children_ list.   Temporarily using a lock but should 
    // be able to eliminate for better performance.   Once lioCleanupThread
    // and syncThread uniformly queue requests to be processed by worker
    // thread this lock should not be necessary.
    CLock       childrenListLock_;
#endif

    // Container to hold dead process info to be sent as death notices
    // to an ssmp process.   This is a NULL list except when the CProcess
    // instance is an SSMP process object.
    typedef list<struct message_def *> ssmpDeath_t;
    ssmpDeath_t ssmpNotices_;
    CLock       ssmpNoticesLock_;

#ifndef NAMESERVER_PROCESS
    // Container to keep track of the processes for which this process
    // is interested in process death.  deathInterestLock_ is used to
    // protect both the deathInterest_ and CNotice list.
    typedef set<int> nidSet_t;
    nidPidList_t deathInterest_;
    nidSet_t     deathInterestNid_;
    CLock        deathInterestLock_;

    CNotice       *NoticeHead;   // List of processes requesting death notice 
    CNotice       *NoticeTail;
#endif
#ifdef NAMESERVER_PROCESS
    int            monSockFd_;
    int            origPNidNs_;
#endif
};

#endif /*PROCESS_H_*/
