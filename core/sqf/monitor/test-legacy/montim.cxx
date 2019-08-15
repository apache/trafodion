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

#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <map>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

class MyTime
{
public:
  int    thOfSec;  // 1/100000 of second
  int    secs;
  bool   isComm;
  int    type;
  int    reqt;
  int    inType;
  int    inReq;
  void   *addr;
};

class ReqInfo
{
public:
  string       reqType;
  string       reqName;
  int          count;
  int          locnt;
  MyTime       totalTime;
  MyTime       locTime;
};

typedef map<string,MyTime*> NameToTime;
typedef map<string,ReqInfo*> StringToReqInfo;

class ProcessInfo
{
public:
  string          procName;
  string          lastReq;
  string          start;
  string          exit;
  MyTime          totalTime;
  MyTime          totalWait;
  MyTime          locTime;
  int             count;
  int             locnt;
  bool            useMPI;
  list<MyTime>    timeList;
  StringToReqInfo reqInfoMap;
  void            *addr;
};

typedef map<string,ProcessInfo*> StringToProcInfo;

static const string GetMessage = "CMonitor::GetMessage";
static const string Send_Event = "CMonitor::Send_Event";
static const string Sent_Event = "CMonitor::Sent_Event";
static const string Monitor_ShareWithPeers = "CMonitor::ShareWithPeers";
static const string SharedWithPeer = "CCluster::SharedWithPeer";
static const string ShareWithPeers = "CCluster::ShareWithPeers";
static const string GetRequest = "CMonitor::GetRequest";
static const string GetRidValue = "CRid::GetRidValue";
static const string ProcessRequest = "CMonitor::ProcessRequest";
static const string processrequest = "CMonitor::processrequest";
static const string CloseProcess = "CMonitor::CloseProcess";
static const string Close1Process = "CMonitor:CloseProcess";
static const string Container_ExitProcess = "CProcessContainer::ExitProcess";
static const string ExitProcess = "CProcessContainer::ExitProcess";

static const string MsgType_Close = " - Close is completed to";
static const string ReqType_Close = " - Close";
static const string ReqType_Event = " - Event";
static const string ReqType_Exit = " - Process Exit";
static const string ReqType_Get = " - Get";
static const string ReqType_Kill = " - Kill";
static const string ReqType_Mount = " - Mount";
static const string ReqType_NodeInfo = " - NodeInfo";
static const string ReqType_NewProcess = " - NewProcess";
static const string ReqType_NodeAdd = " - Add node";
static const string ReqType_NodeDelete = " - Delete node";
static const string ReqType_NodeDown = " - Down node";
static const string ReqType_NodeUp = " - Up node";
static const string ReqType_Notify = " - Notify";
static const string ReqType_Open = " - Open";
static const string ReqType_OpenInfo = " - OpenInfo";
static const string ReqType_ProcessInfo = " - ProcessInfo";
static const string ReqType_Set = " - Set";
static const string ReqType_Shutdown = " - Shutdown";
static const string ReqType_Startup = " - Startup";
static const string ReqType_TmLeader = " - TmLeader";
static const string ReqType_TmSync = " - TmSync";
static const string ReqType_Unknown = " - Unknown request";
static const string ReplyType_TmSync = " - Unsolicited TmSync Reply";
static const string ReplyType_Unknown = " - Unknown reply type";
static const string MsgType_Unknown = " - Unknown message type.";
static const string EndProcess = " - Exit";

list<string*>      procNameList;

bool
inProcNameList( string *procName )
{
  list<string*>::iterator pos;
  if (procNameList.empty()) return true;
  for (pos = procNameList.begin(); pos != procNameList.end(); pos++)
  {
    if (**pos == *procName) return true;
  }
  return false;
}

bool more_than(const MyTime & a, const MyTime & b) 
{
  if (a.thOfSec > b.thOfSec) return true;
  return false;
}

bool pi_more_than(const ProcessInfo & a, const ProcessInfo & b) 
{
  if (a.totalTime.secs > b.totalTime.secs ||
      (a.totalTime.secs == b.totalTime.secs &&
       a.totalTime.thOfSec > b.totalTime.thOfSec)) return true;
  return false;
}

bool pic_more_than(const ProcessInfo & a, const ProcessInfo & b) 
{
  if (a.timeList.size() > b.timeList.size())
    return true;
  return false;
}

MyTime *
calcTime( string begTime, string endTime, MyTime *total = NULL )
{
  MyTime *theTime = new MyTime;
  //theTime->begTime = begTime;
  //theTime->endTime = endTime;
  int days = atoi( endTime.c_str() ) * 24 * 60 * 60;
  endTime = endTime.substr( endTime.find( ':' ) + 1 );
  int hours = atoi( endTime.c_str() ) * 60 * 60;
  endTime = endTime.substr( endTime.find( ':' ) + 1 );
  int mins = atoi( endTime.c_str() ) * 60;
  endTime = endTime.substr( endTime.find( ':' ) + 1 );
  int secs = atoi( endTime.c_str() );
  endTime = endTime.substr( endTime.find( '.' ) + 1 );
  theTime->thOfSec = atoi( endTime.c_str() );
#if 0
  cout << "End Days: " << days << ", hours: " << hours 
       << ", Mins: " << mins << ", Secs: " << secs 
       << ", Microsecs: " << theTime->thOfSec << endl;
#endif
  theTime->secs = days + hours + mins + secs;

  days = atoi( begTime.c_str() ) * 24 * 60 * 60;
  begTime = begTime.substr( begTime.find( ':' ) + 1 );
  hours = atoi( begTime.c_str() ) * 60 * 60;
  begTime = begTime.substr( begTime.find( ':' ) + 1 );
  mins = atoi( begTime.c_str() ) * 60;
  begTime = begTime.substr( begTime.find( ':' ) + 1 );
  secs = atoi( begTime.c_str() );
  begTime = begTime.substr( begTime.find( '.' ) + 1 );
  int thOfSec = atoi( begTime.c_str() );
#if 0
  cout << "Beg Days: " << days << ", hours: " << hours 
       << ", Mins: " << mins << ", Secs: " << secs 
       << ", Microsecs: " << thOfSec << endl;
#endif
  theTime->secs -= (days + hours + mins + secs);
  if (theTime->thOfSec >= thOfSec)
    theTime->thOfSec -= thOfSec;
  else
  {
    theTime->thOfSec = (theTime->thOfSec+100000)-thOfSec;
    theTime->secs--;
  }
  if (theTime->secs < 0)
  {
    delete theTime;
    return (MyTime*)0;
  }
  if (total)
  {
    total->secs += theTime->secs;
    total->thOfSec += theTime->thOfSec;
    if (total->thOfSec >= 100000)
    {
      total->thOfSec = total->thOfSec % 100000;
      total->secs++;
    }
  }
  return theTime;
}

ReqInfo *
mkReqInfo( string type, string name )
{
  ReqInfo *reqInfo;

  reqInfo = new ReqInfo;
  reqInfo->reqType = type;
  reqInfo->reqName = name;
  reqInfo->count = 0;
  reqInfo->totalTime.secs = 0;
  reqInfo->totalTime.thOfSec = 0;
  reqInfo->locnt = 0;
  reqInfo->locTime.secs = 0;
  reqInfo->locTime.thOfSec = 0;
  return reqInfo;
}

void
processSpecial( ProcessInfo *proc, string intime, string line )
{
  ReqInfo *reqInfo;

  if (proc->procName == ProcessRequest)
  {
    string::size_type pos;

    //cout << "procName: " << proc->procName << ", intime: " << intime << endl;
    //cout << line << endl;
    //if ((pos = line.find("] useMPI=")) != string::npos)
    if (((pos = line.find("]")) != string::npos && pos == line.size()-1) ||
        ((pos = line.find("] useMPI=")) != string::npos))
    {
      if (pos != line.size()-1)
        pos += 9;
/*
      cout << "Start process " << ProcessRequest 
           << " with useMPI=" << line.substr( pos ) << endl;
*/
      if (pos != line.size()-1)
        proc->useMPI = (atoi( line.substr( pos ).c_str() ) == 1);
      else
        proc->useMPI = 1;
      proc->start = intime;
      if (!proc->exit.empty())
      {
        MyTime *myTime = calcTime( proc->exit, intime, &proc->totalWait );
        delete myTime;
      }
    }
    else if (line.find(MsgType_Close) != string::npos)
    {
/*
      cout << MsgType_Close << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[MsgType_Close]) == NULL)
      {
        proc->reqInfoMap[MsgType_Close] = 
           mkReqInfo( MsgType_Close, "MsgType_Close" );
      }
      proc->lastReq = MsgType_Close;
    }
    else if(line.find(ReqType_Close) != string::npos)
    {
/*
      cout << ReqType_Close << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_Close]) == NULL)
      {
        proc->reqInfoMap[ReqType_Close] = 
           mkReqInfo( ReqType_Close, "ReqType_Close" );
      }
      proc->lastReq = ReqType_Close;
    }
    else if(line.find(ReqType_Event) != string::npos)
    {
/*
      cout << ReqType_Event << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_Event]) == NULL)
      {
        proc->reqInfoMap[ReqType_Event] = 
           mkReqInfo( ReqType_Event, "ReqType_Event" );
      }
      proc->lastReq = ReqType_Event;
    }
    else if(line.find(ReqType_Exit) != string::npos)
    {
/*
      cout << ReqType_Exit << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_Exit]) == NULL)
      {
        proc->reqInfoMap[ReqType_Exit] = 
           mkReqInfo( ReqType_Exit, "ReqType_Exit" );
      }
      proc->lastReq = ReqType_Exit;
    }
    else if(line.find(ReqType_Get) != string::npos)
    {
/*
      cout << ReqType_Get << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_Get]) == NULL)
      {
        proc->reqInfoMap[ReqType_Get] = 
           mkReqInfo( ReqType_Get, "ReqType_Get" );
      }
      proc->lastReq = ReqType_Get;
    }
    else if(line.find(ReqType_Kill) != string::npos)
    {
/*
      cout << ReqType_Kill << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_Kill]) == NULL)
      {
        proc->reqInfoMap[ReqType_Kill] = 
           mkReqInfo( ReqType_Kill, "ReqType_Kill" );
      }
      proc->lastReq = ReqType_Kill;
    }
    else if(line.find(ReqType_Mount) != string::npos)
    {
/*
      cout << ReqType_Mount << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_Mount]) == NULL)
      {
        proc->reqInfoMap[ReqType_Mount] = 
           mkReqInfo( ReqType_Mount, "ReqType_Mount" );
      }
      proc->lastReq = ReqType_Mount;
    }
    else if(line.find(ReqType_NodeInfo) != string::npos)
    {
/*
      cout << ReqType_NodeInfo << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_NodeInfo]) == NULL)
      {
        proc->reqInfoMap[ReqType_NodeInfo] = 
           mkReqInfo( ReqType_NodeInfo, "ReqType_NodeInfo" );
      }
      proc->lastReq = ReqType_NodeInfo;
    }
    else if(line.find(ReqType_NewProcess) != string::npos)
    {
/*
      cout << ReqType_NewProcess << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_NewProcess]) == NULL)
      {
        proc->reqInfoMap[ReqType_NewProcess] = 
           mkReqInfo( ReqType_NewProcess, "ReqType_NewProcess" );
/*
        cout << " created map for: " << reqInfo->reqName
             << ", " << reqInfo->reqType << endl;
*/
      }
      proc->lastReq = ReqType_NewProcess;
      //cout << "set proc->lastReq: " << proc->lastReq << endl;
    }
    else if(line.find(ReqType_NodeAdd) != string::npos)
    {
/*
      cout << ReqType_NodeAdd << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_NodeAdd]) == NULL)
      {
        proc->reqInfoMap[ReqType_NodeAdd] = 
           mkReqInfo( ReqType_NodeAdd, "ReqType_NodeAdd" );
      }
      proc->lastReq = ReqType_NodeAdd;
    }
    else if(line.find(ReqType_NodeDelete) != string::npos)
    {
/*
      cout << ReqType_NodeDelete << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_NodeDelete]) == NULL)
      {
        proc->reqInfoMap[ReqType_NodeDelete] = 
           mkReqInfo( ReqType_NodeDelete, "ReqType_NodeDelete" );
      }
      proc->lastReq = ReqType_NodeDelete;
    }
    else if(line.find(ReqType_NodeDown) != string::npos)
    {
/*
      cout << ReqType_NodeDown << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_NodeDown]) == NULL)
      {
        proc->reqInfoMap[ReqType_NodeDown] = 
           mkReqInfo( ReqType_NodeDown, "ReqType_NodeDown" );
      }
      proc->lastReq = ReqType_NodeDown;
    }
    else if(line.find(ReqType_NodeUp) != string::npos)
    {
/*
      cout << ReqType_NodeUp << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_NodeUp]) == NULL)
      {
        proc->reqInfoMap[ReqType_NodeUp] = 
           mkReqInfo( ReqType_NodeUp, "ReqType_NodeUp" );
      }
      proc->lastReq = ReqType_NodeUp;
    }
    else if(line.find(ReqType_Notify) != string::npos)
    {
/*
      cout << ReqType_Notify << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_Notify]) == NULL)
      {
        proc->reqInfoMap[ReqType_Notify] = 
           mkReqInfo( ReqType_Notify, "ReqType_Notify" );
      }
      proc->lastReq = ReqType_Notify;
    }
    else if(line.find(ReqType_Open) != string::npos)
    {
/*
      cout << ReqType_Open << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_Open]) == NULL)
      {
        proc->reqInfoMap[ReqType_Open] = 
           mkReqInfo( ReqType_Open, "ReqType_Open" );
      }
      proc->lastReq = ReqType_Open;
    }
    else if(line.find(ReqType_OpenInfo) != string::npos)
    {
/*
      cout << ReqType_OpenInfo << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_OpenInfo]) == NULL)
      {
        proc->reqInfoMap[ReqType_OpenInfo] = 
           mkReqInfo( ReqType_OpenInfo, "ReqType_OpenInfo" );
      }
      proc->lastReq = ReqType_OpenInfo;
    }
    else if(line.find(ReqType_ProcessInfo) != string::npos)
    {
/*
      cout << ReqType_ProcessInfo << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_ProcessInfo]) == NULL)
      {
        proc->reqInfoMap[ReqType_ProcessInfo] = 
           mkReqInfo( ReqType_ProcessInfo, "ReqType_ProcessInfo" );
      }
      proc->lastReq = ReqType_ProcessInfo;
    }
    else if(line.find(ReqType_Set) != string::npos)
    {
/*
      cout << ReqType_Set << " was received"
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_Set]) == NULL)
      {
        proc->reqInfoMap[ReqType_Set] = 
           mkReqInfo( ReqType_Set, "ReqType_Set" );
      }
      proc->lastReq = ReqType_Set;
    }
    else if(line.find(ReqType_Shutdown) != string::npos)
    {
/*
      cout << ReqType_Shutdown << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_Shutdown]) == NULL)
      {
        proc->reqInfoMap[ReqType_Shutdown] = 
           mkReqInfo( ReqType_Shutdown, "ReqType_Shutdown" );
      }
      proc->lastReq = ReqType_Shutdown;
    }
    else if(line.find(ReqType_Startup) != string::npos)
    {
/*
      cout << ReqType_Startup << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_Startup]) == NULL)
      {
        proc->reqInfoMap[ReqType_Startup] = 
           mkReqInfo( ReqType_Startup, "ReqType_Startup" );
      }
      proc->lastReq = ReqType_Startup;
    }
    else if(line.find(ReqType_TmLeader) != string::npos)
    {
/*
      cout << ReqType_TmLeader << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_TmLeader]) == NULL)
      {
        proc->reqInfoMap[ReqType_TmLeader] = 
           mkReqInfo( ReqType_TmLeader, "ReqType_TmLeader" );
      }
      proc->lastReq = ReqType_TmLeader;
    }
    else if(line.find(ReqType_TmSync) != string::npos)
    {
/*
      cout << ReqType_TmSync << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_TmSync]) == NULL)
      {
        proc->reqInfoMap[ReqType_TmSync] = 
           mkReqInfo( ReqType_TmSync, "ReqType_TmSync" );
      }
      proc->lastReq = ReqType_TmSync;
    }
    else if(line.find(ReqType_Unknown) != string::npos)
    {
/*
      cout << ReqType_Unknown << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReqType_Unknown]) == NULL)
      {
        proc->reqInfoMap[ReqType_Unknown] = 
           mkReqInfo( ReqType_Unknown, "ReqType_Unknown" );
      }
      proc->lastReq = ReqType_Unknown;
    }
    else if(line.find(ReplyType_Unknown) != string::npos)
    {
/*
      cout << ReplyType_Unknown << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReplyType_Unknown]) == NULL)
      {
        proc->reqInfoMap[ReplyType_Unknown] = 
           mkReqInfo( ReplyType_Unknown, "ReplyType_Unknown" );
      }
      proc->lastReq = ReplyType_Unknown;
    }
    else if(line.find(MsgType_Unknown) != string::npos)
    {
/*
      cout << MsgType_Unknown << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[MsgType_Unknown]) == NULL)
      {
        proc->reqInfoMap[MsgType_Unknown] = 
           mkReqInfo( MsgType_Unknown, "MsgType_Unknown" );
      }
      proc->lastReq = MsgType_Unknown;
    }
    else if(line.find(ReplyType_TmSync) != string::npos)
    {
/*
      cout << ReplyType_TmSync << " was received" 
           << " with useMPI=" << proc->useMPI << endl;
*/
      if ((reqInfo = proc->reqInfoMap[ReplyType_TmSync]) == NULL)
      {
        proc->reqInfoMap[ReplyType_TmSync] = 
           mkReqInfo( ReplyType_TmSync, "ReplyType_TmSync" );
      }
      proc->lastReq = ReplyType_TmSync;
    }
    else if(line.find(EndProcess) != string::npos)
    {
/*
      cout << EndProcess << " was received, start: " 
           << hex << proc->start << ", stop: " << dec << intime << endl;
      cout << proc->lastReq << " with useMPI=" << proc->useMPI << endl;
      cout << line << endl;
*/
      ReqInfo *reqInfo = proc->reqInfoMap[proc->lastReq];
      proc->exit = intime;
      if (proc->useMPI)
      {
        MyTime *myTime = calcTime( proc->start, proc->exit, 
                                   &reqInfo->totalTime );
        delete myTime;
        myTime = calcTime( proc->start, proc->exit, &proc->totalTime );
        delete myTime;
        proc->count++;
        reqInfo->count++;
      }
      else
      {
        MyTime *myTime = calcTime( proc->start, proc->exit, 
                                   &reqInfo->locTime );
        delete myTime;
        myTime = calcTime( proc->start, proc->exit, &proc->locTime );
        delete myTime;
        proc->locnt++;
        reqInfo->locnt++;
      }
    }
    else
    {
/*
      cout << "Received Unknown request/reply/msg" << endl;
      cout << line << endl;
*/
    }
  }
}

int main(int argc, char *argv[])
{
  StringToProcInfo  procMap;
  NameToTime        cpsMap;
  ProcessInfo       *procInfo = NULL;
  string strLine;
  string routine; 
  string intime;
  int    c;
  bool   detail = false;
  int    top = 0;
  int    countTop = 0;
  bool   sortByCount = false;
  string *procName = NULL;
  map<string,string> stringToProc;
  string pname = "";
  string::size_type pos;
  string::size_type pos2;

  while ((c = getopt(argc, argv, "c:hl:p:s:t:")) != -1)
  {
    switch (c)
    {
      case 'c':
        sortByCount = true;
        if (optarg)
        {
          countTop = atoi(optarg);
        }
        break;
      case 'h':
        cout << argv[0] << " [-c <top hitters>][-h][-l <top detail>]"
             << " [-p <proc>][-s <proc name>][-t <top timers>]"
             << " <trace4 files>" << endl;
        cout << endl << "\tc  --  sort hits for <top hitters>" << endl;
        cout << "\th  --  display this message" << endl;
        cout << "\tl  --  detail times for <top detail> calls to proc" << endl;
        cout << "\tp  --  special processing of procedure <proc>" << endl;
        cout << "\ts  --  get info on <proc name>" << endl;
        cout << "\tt  --  top <top timers>" << endl << endl;
        cout << "This utility will allow the analysis of the monitor trace4\n"
             << "files.  The final parameter is <trace4 files> which a\n"
             << "required parameter and is a list of monitor trace4 files.\n"
             << "They must be in the current directory and in the form: \n\n" 
             << "\t*trace*.n[0-1][0-6]\n" << endl;
        cout << "Where 'n' is the letter n and [01][0-6] is the node number.\n" 
             << "Sorting defaults to time in proc and -c and -t should not be\n"
             << "used together.  The option -s can only be used with the \n"
             << "option -l, but there can be multiple -s options indicating\n"
             << "more than one <proc name>." << endl;
        cout << "The -p option processes a specific procedure.  " << endl;
        cout << "At this time only Monitor::ProcessRequest has special "
             << "processing" << endl;
        cout << endl;
        break;
      case 'l':
        detail = true;
        if (optarg)
        {
          top = atoi(optarg);
        }
        break;
      case 'p':
        if (optarg)
        {
          procInfo = new ProcessInfo;
          procInfo->procName = optarg;
          detail = false;
          top = 0;
          procNameList.clear();
          sortByCount = false;
          countTop = 0;
          procInfo->count = 0;
          procInfo->locnt = 0;
        }
        break;
      case 's':
        if (optarg)
        {
          procName = new string(optarg);
          procNameList.push_back(procName);
        }
        break;
      case 't':
        sortByCount = false;
        if (optarg)
        {
          countTop = atoi(optarg);
        }
        break;
    }
  }
  ifstream fin;
  while (optind < argc)
  {
    fin.open(argv[optind]);
    cout << "Parsing trace file " << argv[optind] << endl;
    while (getline( fin, strLine ))
    {
      string::size_type beg, end, idx;
      //cout << strLine << endl;
      if (strLine.empty() || strLine.length() < 4)
      {
        //cout << "cannot parse line : \"" << strLine << "\"" << endl;
        continue;
      }
      strLine = strLine.substr( 4 );
      if (!isdigit(strLine[0])) 
      {
        cout << strLine << endl;
        continue; //bail
      }
      //cout << strLine << endl;
      intime = strLine.substr( 0, strLine.find( '-' ));
      if (intime.length() == strLine.length()) 
      {
        //cout << "No time tag: \"" << intime << "\"" << endl;
        continue;
      }
      beg = strLine.find( '-' ) + 1;
      while (!isalpha(strLine[beg])) beg++;
      end = beg;
      while (end != string::npos && 
             (isalpha(strLine[end]) || 
              strLine[end] == '_'   ||
              strLine[end] == '~'   ||
              strLine[end] == ':'))
        end++;
      if (beg == string::npos || end == string::npos || end <= beg)
      {
        cout << "No procedure name on line:" << endl << strLine << endl;
        continue;
      }
      while (!isalpha( strLine[end-1] )) end--;
      idx = end-beg;
      //cout << "beg: " << beg << ", end: " << end << ", idx: " << idx << endl;
      //cout << strLine << endl;
      //fflush(stdout);
      routine = strLine.substr( beg, idx );
      size_t colon;
      if ((colon=routine.find(":")) == string::npos)
        continue;
      else if (routine[colon+1] != ':')
        continue;
      if (routine == processrequest) routine = ProcessRequest;
      if (procInfo)
      {
        if (procInfo->procName == routine)
        {
          processSpecial( procInfo, intime, strLine );
        }
        continue;
      }
      if (routine == Sent_Event) routine = Send_Event;
      if (routine == Close1Process) routine = CloseProcess;
      if (routine == Container_ExitProcess) routine = ExitProcess;
      if (routine == Monitor_ShareWithPeers || routine == SharedWithPeer) 
        routine = ShareWithPeers;
      if (!inProcNameList(&routine)) continue;
      //if (!procName.empty() && procName != routine) continue;
      
      ProcessInfo *thisProc;
      //cout << "\"" << routine << "\" \"" << GetMessage << "\"" << endl;
      if ((thisProc = procMap[routine]))
      {
        MyTime *theTime;
        if (thisProc->exit.empty() && routine == GetMessage)
        {
          if (strLine.length() <= GetMessage.length() + 1 ||
              strLine.find( "- No I/O" ) != string::npos)
          {
            thisProc->exit = intime;
            // calculate time here
            theTime = calcTime( thisProc->start, intime, &thisProc->totalTime );
            if (theTime)
            {
              theTime->addr = theTime;
              thisProc->timeList.push_back( *theTime );
            }
          }
        }
        else if (thisProc->exit.empty() && 
            (strLine.find( "- NumRanks" ) != string::npos ||
             strLine.find( "- rid =" ) != string::npos ||
             strLine.find( "- Count=" ) != string::npos ||
             strLine.find( "- Exit" ) != string::npos ||
             strLine.find( "- exit" ) != string::npos))
        {
          thisProc->exit = intime;
          // calculate time here
          theTime = calcTime( thisProc->start, intime, &thisProc->totalTime );
          if (theTime)
          {
            long comm = 0;
            theTime->type = 0;
            theTime->reqt = 0;
            theTime->inType = 0;
            theTime->inReq = 0;
            pname = "";
            if ((pos = strLine.find( "@comm=" )) != string::npos)
                comm = strtol( strLine.substr( pos+6 ).c_str(), NULL, 16);
            if ((pos = strLine.find( "@type=" )) != string::npos)
                theTime->type = atoi( strLine.substr( pos+6 ).c_str());
            if ((pos = strLine.find( "@reqt=" )) != string::npos)
                theTime->reqt = atoi( strLine.substr( pos+6 ).c_str());
            if ((pos = strLine.find( "@intp=" )) != string::npos)
                theTime->inType = atoi( strLine.substr( pos+6 ).c_str());
            if ((pos = strLine.find( "@inrq=" )) != string::npos)
                theTime->inReq = atoi( strLine.substr( pos+6 ).c_str());
            if ((pos = strLine.find( "@name=" )) != string::npos)
            {
                pos2 = pos + 6;
                while (pos2 != string::npos && strLine[pos2] != ' ') pos2++;
                pname = strLine.substr( pos+6, pos2 - (pos+6) );
                //cout << pname << ": finished startup" << endl;
            }
            theTime->addr = theTime;
            theTime->isComm = (comm > 0);
            if (routine == "CPS_TIME::CPS_TIME" && !pname.empty())
            {
              MyTime *myTime = new MyTime;
              myTime->thOfSec = theTime->thOfSec;
              myTime->secs = theTime->secs;
              cpsMap[pname] = myTime;
            }
            thisProc->timeList.push_back( *theTime );
          }
        }
        else 
        {
          // new start
          thisProc->exit = "";
          thisProc->start = intime;
          if (routine == GetRequest ||
              routine == GetRidValue) 
          {
            thisProc->exit = intime;
          }
          pname = "";
          if ((pos = strLine.find( "@name=" )) != string::npos)
          {
            pos2 = pos + 6;
            while (pos2 != string::npos && strLine[pos2] != ' ') pos2++;
            if (pos2 != string::npos)
            {
              pname = strLine.substr( pos+6, pos2 - (pos+6) );
              //cout << pname << ": begin startup" << endl;
            }
          }
        }
        
      }
      else
      {
        thisProc = new ProcessInfo;
        thisProc->procName = routine;
        thisProc->start = intime;
        thisProc->totalTime.secs = 0;
        thisProc->totalTime.thOfSec = 0;
        thisProc->addr = (void*)thisProc;
        if (routine == GetRequest ||
              routine == GetRidValue) 
        {
          thisProc->exit = intime;
        }
        procMap[routine] =  thisProc;
        pname = "";
        if ((pos = strLine.find( "@name=" )) != string::npos)
        {
          pos2 = pos + 6;
          while (pos2 != string::npos && strLine[pos2] != ' ') pos2++;
          if (pos2 != string::npos)
          {
            pname = strLine.substr( pos+6, pos2 - (pos+6) );
            //cout << pname << ": begin startup" << endl;
          }
        }
      }
    }
    fin.close();
    if (procInfo)
    {
      StringToReqInfo::iterator pos;
      long total = (long)procInfo->totalWait.secs*100000+
                         procInfo->totalWait.thOfSec;
      if ((procInfo->locnt + procInfo->count) > 0)
        total = total / (procInfo->locnt + procInfo->count);
      printf( "Average delay before new request: %ld.%05ld\n",
              total/100000, total % 100000 );
      for (pos = procInfo->reqInfoMap.begin(); 
           pos != procInfo->reqInfoMap.end(); pos++)
      {
        ReqInfo *reqInfo = pos->second;
        total = (long)(reqInfo->totalTime.secs+reqInfo->locTime.secs)*100000+
                      reqInfo->totalTime.thOfSec+reqInfo->locTime.thOfSec;
        if ((reqInfo->count+reqInfo->locnt) > 0)
          total = total / (reqInfo->count+reqInfo->locnt);
        printf( "Processed %d\t%s at an average time: %ld.%05ld\n",
                reqInfo->count+reqInfo->locnt, reqInfo->reqName.c_str(),
                total/100000, total %100000 );
        if (reqInfo->count)
        {
          total = (long)reqInfo->totalTime.secs*100000+
                        reqInfo->totalTime.thOfSec;
          total = total / reqInfo->count;
          printf( "\t\t%d reqs at an average time: %ld.%05ld\n",
                  reqInfo->count, total/100000, total %100000 );
        }
        if (reqInfo->locnt)
        {
          total = (long)reqInfo->locTime.secs*100000+
                        reqInfo->locTime.thOfSec;
          total = total / reqInfo->locnt;
          printf( "\t\t%d local IO at an average time: %ld.%05ld\n",
                  reqInfo->locnt, total/100000, total %100000 );
        }
        delete reqInfo;
      }
      procInfo->reqInfoMap.clear();
      optind++;
      continue;
    }
    NameToTime::iterator pos;
    for (pos = cpsMap.begin(); pos != cpsMap.end(); pos++)
    {
      MyTime *mytime = pos->second;
      string pname = pos->first;
      printf( "%s: startup time: %d.%05d\n", 
        pname.c_str(), mytime->secs, mytime->thOfSec );
      delete mytime;
    }
    cpsMap.clear();

    StringToProcInfo::iterator posInfo;
    list<ProcessInfo> procInfoList;
    for (posInfo = procMap.begin(); posInfo != procMap.end(); posInfo++)
    {
      ProcessInfo *procInfo = posInfo->second;
      procInfoList.push_back(*procInfo);
    }
    int totalProcs = procInfoList.size();
    if (sortByCount)
    {
      cout << "Sorting by number of entries into proccedures" << endl << endl;
      procInfoList.sort(pic_more_than);
      if (countTop && countTop < totalProcs) totalProcs = countTop;
    }
    else
    {
      cout << "Sorting by time in procedures" << endl << endl;
      procInfoList.sort(pi_more_than);
      if (countTop && countTop < totalProcs) totalProcs = countTop;
    }
    for (int j=0; j< totalProcs; j++)
    {
      ProcessInfo pI = procInfoList.front();
      ProcessInfo *procInfo = (ProcessInfo*)pI.addr;
      procInfoList.pop_front();
      cout << procInfo->procName << " was called " 
           << procInfo->timeList.size() << endl;
      printf( "Total time in Secs: %d.%05d\n", 
              procInfo->totalTime.secs, procInfo->totalTime.thOfSec );
      if (detail)
      {
        if (!top) top = procInfo->timeList.size();
        if (top > (int) procInfo->timeList.size())  top = procInfo->timeList.size();
        procInfo->timeList.sort(more_than);
        long mpi_time = 0, loc_time = 0;
        int  mpi_cnt = 0, loc_cnt = 0;
        long mpi_reqtime[20];
        long loc_reqtime[20];
        int  mpi_reqcnt[20];
        int  loc_reqcnt[20];
        for (int i=0; i < 20; i++)
        {
          mpi_reqtime[i] = loc_reqtime[i] = mpi_reqcnt[i] = loc_reqcnt[i] = 0;
        }
        for (int i=0; i < top; i++)
        {
          MyTime theTime = procInfo->timeList.front();
          procInfo->timeList.pop_front();
          printf( "[%d] time in Secs: %d.%05d, ", 
                  i, theTime.secs, theTime.thOfSec );
          if (theTime.isComm)
          {
            mpi_cnt++;
            mpi_time += theTime.thOfSec;
            mpi_reqtime[theTime.inReq] += theTime.thOfSec;
            mpi_reqcnt[theTime.inReq]++;
            printf( "using MPI" );
/*
            if (theTime.type)
            {
              printf( ", type: %d", theTime.type );
              if (theTime.reqt)
              {
                printf( ",%d", theTime.reqt );
                if (theTime.inType)
                {
                  printf( " %d", ,%d", 
                    theTime.type, theTime.reqt, theTime.inType, theTime.inReq );
            ]
*/
            printf( "\n" );
          }
          else
          {
            loc_cnt++;
            loc_time += theTime.thOfSec;
            loc_reqtime[theTime.inReq] += theTime.thOfSec;
            loc_reqcnt[theTime.inReq]++;
            printf( "using local IO" );
/*
                    , type: %d,%d %d,%d\n", 
                    theTime.type, theTime.reqt, theTime.inType, theTime.inReq );
*/
            printf( "\n" );
          }
          delete (MyTime*) theTime.addr;
        }
        if (mpi_cnt)
        {
          printf( "%d MPI calls, Time = %ld.%05ld, avg: %ld.%05ld\n", 
            mpi_cnt, mpi_time / 100000, mpi_time % 100000,
            (mpi_time / mpi_cnt) / 100000, (mpi_time / mpi_cnt) % 100000 );
/*
          printf( "Totals by request\n" );
          for (int i=0; i< 20; i++)
          {
            if (mpi_reqcnt[i])
              printf( "%d reqtype %d calls, Time = %d.%05d, avg: %d.%05d\n", 
                mpi_reqcnt[i], i, mpi_reqtime[i] / 100000, 
                mpi_reqtime[i] % 100000, 
                (mpi_reqtime[i] / mpi_reqcnt[i]) / 100000, 
                (mpi_reqtime[i] / mpi_reqcnt[i]) % 100000 );
       
          }
*/
        }
        if (loc_cnt)
        {
          printf( "%d LOC calls, Time = %ld.%05ld, avg: %ld.%05ld\n", 
            loc_cnt, loc_time / 100000, loc_time % 100000,
            (loc_time / loc_cnt) / 100000, (loc_time / loc_cnt) % 100000 );
/*
          printf( "Totals by request\n" );
          for (int i=0; i< 20; i++)
          {
            if (loc_reqcnt[i])
              printf( "%d reqtype %d calls, Time = %d.%05d, avg: %d.%05d\n", 
                loc_reqcnt[i], i, loc_reqtime[i] / 100000, 
                loc_reqtime[i] % 100000, 
                (loc_reqtime[i] / loc_reqcnt[i]) / 100000, 
                (loc_reqtime[i] / loc_reqcnt[i]) % 100000 );
       
          }
*/
        }
      }
      int myTimeIdx = procInfo->timeList.size();
      for (int i=0; i< myTimeIdx; i++)
      {
        MyTime theTime = procInfo->timeList.front();
        delete (MyTime*) theTime.addr;
        procInfo->timeList.pop_front();
      }
      delete procInfo;
    }
    procMap.clear();
    optind++;
    cout << endl;
    int names = procNameList.size();
    for (int i=0; i<names; i++)
    {
      string *str = procNameList.front();
      delete str;
      procNameList.pop_front();
    }
  }
  
}
