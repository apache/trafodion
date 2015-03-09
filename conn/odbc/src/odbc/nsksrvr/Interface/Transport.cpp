// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2015 Hewlett-Packard Development Company, L.P.
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

#include <platform_ndcs.h>
#include <idltype.h>
#include "ceercv.h"
#include "Global.h"
#include "odbcCommon.h"

#include "Transport.h"
#include "Listener_srvr.h"
#include "FileSystemDrvr.h"
#include "FileSystemSrvr.h"
#include "TCPIPSystemSrvr.h"

CTransport::CTransport()
{
       SRVRTRACE_ENTER(FILE_TNSPT+1);

       error = 0;
       error_message[0]=0;

       m_listener = new CNSKListenerSrvr();
       m_FSystemSrvr_list = new CFSystemSrvr_list();
       m_FSystemDrvr_list = new CFSystemDrvr_list();
       m_TCPIPSystemSrvr_list = new CTCPIPSystemSrvr_list();
       m_Timer_list = new CTimer_list();

       if (m_listener == NULL ||
                       m_FSystemSrvr_list == NULL ||
                               m_FSystemDrvr_list == NULL ||
                                       m_TCPIPSystemSrvr_list ==NULL ||
                                               m_Timer_list == NULL )
       {
               strcpy(error_message,"CTransport::CTransport():Memory allocation error");
               goto bailout;
       }

       m_asFSystemDrvr = NULL;

bailout:
       SRVRTRACE_EXIT(FILE_TNSPT+1);

}
CTransport::~CTransport()
{
       SRVRTRACE_ENTER(FILE_TNSPT+2);
       if (m_listener != NULL)
               delete m_listener;
       if (m_FSystemSrvr_list != NULL)
               delete m_FSystemSrvr_list;
       if (m_FSystemDrvr_list != NULL)
               delete m_FSystemDrvr_list;
       if (m_TCPIPSystemSrvr_list != NULL)
               delete m_TCPIPSystemSrvr_list;
       if (m_Timer_list != NULL)
               delete m_Timer_list;

       m_Timer_list                    = NULL;
       m_listener                              = NULL;
       m_FSystemSrvr_list              = NULL;
       m_FSystemDrvr_list              = NULL;
       m_TCPIPSystemSrvr_list  = NULL;
       m_asFSystemDrvr                 = NULL;
       SRVRTRACE_EXIT(FILE_TNSPT+2);
}

void CTransport::initialize(void)
{
       if ((error = PROCESSHANDLE_GETMINE_(TPT_REF(myHandle))) != 0)
       {
               sprintf(error_message,"CTransport::CTransport():PROCESSHANDLE_GETMINE_ failed, error %d",error);
               goto bailout;
       }

       short nodename_len, procname_len, program_len, pathname_len;
    int nodeNumber;
       if ((error = PROCESSHANDLE_DECOMPOSE_ (
                               TPT_REF(myHandle)
                               ,&myCpu                         //[ short *cpu ]
                               ,&myProcessId           //[ short *pin ]
                               ,&nodeNumber            //[ long *nodenumber ]
                               ,myNodename                     //[ char *nodename ]
                               ,sizeof(myNodename)     //[ short maxlen ]
                               ,&nodename_len          //[ short *nodename-length ]
                               ,myProcname                     //[ char *procname ]
                               ,sizeof(myProcname)     //[ short maxlen ]
                               ,&procname_len          //[ short *procname-length ]
                               ,OMITREF                        //[ long long *sequence-number ]
                               )) != 0)
       {
               sprintf(error_message,"CTransport::CTransport():PROCESSHANDLE_DECOMPOSE_ failed, error %d",error);
               goto bailout;
       }
    myNodenumber = nodeNumber;

#ifdef NSK_PLATFORM
       if ((error = PROCESS_GETINFO_ (
                               TPT_REF(myHandle)
                               ,                                       //[ char *proc-fname ] /* o 2 */
                               ,                                       //[ short maxlen ] /* o 2 */
                               ,                                       //[ short *proc-fname-len ] /* o 3 */
                               ,                                       //[ short *priority ] /* i 4 */
                               ,                                       //[ short *moms-processhandle ] /*i 5 */
                               ,                                       //[ char *hometerm ] /* i 6 */
                               ,                                       //[ short maxlen ] /* i 6 */
                               ,                                       //[ short *hometerm-len ] /* i 7 */
                               ,                                       //[ long long *process-time ]/* i 8 */
                               ,                                       //[ short *creator-access-id ]/*i 9 */
                               ,                                       //[ short *process-access-id ]/*i 10 */
                               ,                                       //[ short *gmoms-processhandle ]/* i11 */
                               ,                                       //[ short *jobid ] /* i 12 */
                               ,myProgramFile          //[ char *program-file ] /* i 13 */
                               ,sizeof(myProgramFile)  //[ short maxlen ] /* i 13 */
                               ,&program_len           //[ short *program-len ] /* i 14 */
                               ,                                       //[ char *swap-file ] /* i 15 */
                               ,                                       //[ short maxlen ] /* i 15 */
                               ,                                       //[ short *swap-len ] /* i 16 */
                               ,                                       //[ short *error-detail ] /* i 17 */
                               ,                                       //[ short *proc-type ] /* i 18 */
                               ,                                       //[ __int32_t *oss-pid ]  /* i 19*/
                               )) != 0)
       {
               sprintf(error_message,"CTransport::CTransport():PROCESS_GETINFO_ failed, error %d",error);
               goto bailout;
       }
       if ((error = FILENAME_TO_PATHNAME_(myProgramFile
                                                       ,program_len
                                                       ,myPathname
                                                       ,sizeof(myPathname)
                                                       ,&pathname_len)) != 0)
       {
               sprintf(error_message,"CTransport::CTransport():FILENAME_TO_PATHNAME_ failed, error %d",error);
               goto bailout;
       }
#else
   MS_Mon_Process_Info_Type  proc_info;

   error = msg_mon_get_process_info_detail(myProcname, &proc_info);
   if (error != XZFIL_ERR_OK )
   {
      sprintf(error_message,"CTransport::CTransport():msg_mon_get_process_info_detail failed, error %d",error);
      goto bailout;
   }

   strcpy(myProgramFile, proc_info.program);
   program_len = strlen(myProgramFile);
   strcpy(myPathname, proc_info.program);
   pathname_len = strlen(myPathname);

#endif

       pathname_len = pathname_len - sizeof(ODBCMX_ODBC_EXE_NM);
       myNodename[nodename_len] = 0;
       myProcname[procname_len] = 0;
       myProgramFile[program_len] = 0;
       myPathname[pathname_len] = 0;
       strupr(myNodename);
       strupr(myProcname);
       strupr(myProgramFile);
       strupr(myPathname);
bailout:
    ;
}

void CTransport::log_error(CError* ierror)
{
       SRVRTRACE_ENTER(FILE_TNSPT+3);
       LOG_ERROR(ierror);
       SRVRTRACE_EXIT(FILE_TNSPT+3);
}

void CTransport::log_info(CError* ierror)
{
       SRVRTRACE_ENTER(FILE_TNSPT+4);
       LOG_INFO(ierror);
       SRVRTRACE_EXIT(FILE_TNSPT+4);
}

void CTransport::log_warning(CError* ierror)
{
       SRVRTRACE_ENTER(FILE_TNSPT+5);
       LOG_WARNING(ierror);
       SRVRTRACE_EXIT(FILE_TNSPT+5);
}

CTransport GTransport;

char*
ALLOC_ERROR_BUFFER()
{
       static char buffer[500];
       return buffer;
}

#ifdef NSK_PLATFORM
short CTransport::AWAITIOX(short* filenum,short* wcount, long* tag, long wtimeout)
{
       return m_listener->AWAITIOX(filenum,wcount, tag, wtimeout);
}
#endif
