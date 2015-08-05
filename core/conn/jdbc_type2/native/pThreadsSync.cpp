/**************************************************************************
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
**************************************************************************/
//
// MODULE: pThreadsSync.cpp
//
//
/* ************************************************************************
   *
   * Description: This file contains all of the pthread function calls used
   *              by the JDBC/MX driver. They were all moved to this file
   *              in order to make it easier to make pthread changes in the
   *              future.
   *
   *              In some cases wrappers were writting to hide the pthread
   *              call.
   *
   ************************************************************************ */

#include "thread_safe_extended.h"
#include "pThreadsSync.h"
#include "CSrvrStmt.h"
#include "Debug.h"
#include "tslxExt.h"

/* ****************************************************************
   *
   * Function Name:pseudoFileCallBack
   *
   * Parameters: filenum         - the Gaurdian file number for which no-wait
   *                               I/O has completed.
   *             tag             - tag of completed I/O.
   *             completionCount - tranfer count of completed I/O
   *             fserror         - Guardian error number for completed I/O
   *             msgBuf          - Address of user data area.
   *
   * Description: Due to the way that Gaurdian does no-wait I/O. More
   *              than one I/O can be queued. This routine is called
   *              by the pthread system, when one of these I/O completes
   *
   * Returns: none
   *
   ******************************************************************/


void pseudoFileCallBack(const short filenum,
                        const long  tag,
                        const long  completionCount,
                        const long  fserror,
                        void       *msgBuf )
{
	
	FUNCTION_ENTRY("pseudoFileCallBack",("..."));

	SRVR_STMT_HDL *pSrvrStmt=NULL;
	int retcode;
#if defined(TAG64)
	int _ptr32 *tempPtr;

	
	if ( tag != NULL) 
	{
		if(tempStmtIdMap.find(tag) != tempStmtIdMap.end())
		{
			pSrvrStmt=(SRVR_STMT_HDL * )tempStmtIdMap.find(tag)->second;
			if (pSrvrStmt!=NULL)
				{
					pSrvrStmt->nowaitRetcode = fserror;
					retcode = tslx_ext_cond_signal(&pSrvrStmt->cond);
				}
			

		}
	}
#else
	pSrvrStmt = (SRVR_STMT_HDL *)tag;

    if (pSrvrStmt != NULL) {
		pSrvrStmt->nowaitRetcode = fserror;
		retcode = tslx_ext_cond_signal(&pSrvrStmt->cond);
	}
#endif
		

				
	
	FUNCTION_RETURN_VOID(("fserror=%ld",fserror));
}

/* ****************************************************************
   *
   * Function Name:registerPseudoFileIO
   *
   * Parameters: filenum - The Gaurdian file number of the file that
   *                       nowait I/O is to be performed on.
   *
   * Description: This function helps to remove calling the ptheards
   *              from the driver code. Any time that the ptheards
   *              library calls are changed, all of thse changes are
   *              made here.
   *
   *              This routine registers a filenumber with the pthread
   *              software and will be managed through the use of a
   *              callback. The callback is required due to the way that
   *              Gaurdian does nowait I/O
   *
   * Returns: SPT_SUCCESS - file number was registered.
   *          SPT_ERROR   - bad file number, file number alread
   *                        registered, or FILE_COMPLETE_SET returned
   *                        non-zero.
   *
   ******************************************************************/

int registerPseudoFileIO(const short filenum) {
	FUNCTION_ENTRY("registerPseudoFileIO",("..."));
	int retcode = tslx_ext_regFileIOHandler(filenum, pseudoFileCallBack);
	FUNCTION_RETURN_NUMERIC(retcode,(NULL));
}

/* ****************************************************************
   *
   * Function Name:initStmtForNowait
   *
   * Parameters: cond  - A condition variable that is used as a
   *                     synchronization device that allows threads
   *                     to suspend execution and relinquish the
   *                     processor until some predicate on shared
   *                     data is satisfied.
   *             mutex - A mutex is a MUTual EXclusion device, and is
   *                     useful for protecting shared data structures
   *                     from concurrent modifications, and implementing
   *                     critical sections and monitors.
   *
   * Description: This routine creates and initilizes both the mutex
   *              and condition variable.
   *
   * Returns: returns a zero (0) if the initilization was sucessfull.
   *
   ******************************************************************/

int initStmtForNowait(_TSLX_cond_t *cond, _TSLX_mutex_t *mutex) //venu for TSLX
{
	FUNCTION_ENTRY("initStmtForNowait",("..."));
    int retcode;
	retcode = tslx_ext_cond_init(cond,NULL); //venu for TSLX
	if (retcode == 0)
        retcode = tslx_ext_mutex_init(mutex, NULL); //venu for TSLX
	FUNCTION_RETURN_NUMERIC(retcode,(NULL));
}

int mutexCondDestroy(_TSLX_cond_t *cond, _TSLX_mutex_t *mutex) //venu for tslx
{
	FUNCTION_ENTRY("mutexCondDestroy",("..."));
    int retcode;
    retcode =  tslx_ext_cond_destroy(cond);
    retcode += tslx_ext_mutex_destroy(mutex); //venu for tslx
    FUNCTION_RETURN_NUMERIC(retcode,(NULL));
}

/* ****************************************************************
   *
   * Function Name:WaitForCompletion
   *
   * Parameters: pSrvrStmt
   *             cond      - The condition variable that will be
   *                         used to suspend execution, until a
   *                         I/O has been completed.
   *             mutex     - The mutex that will lock the shared
   *                         resource. In this case the function
   *                         call WaitForCompletion is the resource.
   *
   * Description: The thread execution is suspended and does not
   *              consume any CPU time until the condition variable
   *              is signaled. The mutex is used to prevent the
   *              condition variable from being used by another thread.
   *              The condition variable can have only one cond wait
   *              being performed on it at the same time.
   *
   * Returns: return 0 on sucess and a non-zero error code on error
   *
   ******************************************************************/

int WaitForCompletion(SRVR_STMT_HDL   *pSrvrStmt,
                      _TSLX_cond_t  *cond,
                      _TSLX_mutex_t *mutex) //venu for tslx
{
	FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_CLI,"WaitForCompletion",("pSrvrStmt=0x%08x, cond=0x%08x, mutex=0x%08x",
		pSrvrStmt,
		cond,
		mutex));

	int retcode;

	pSrvrStmt->nowaitRetcode = 9999;
    retcode                  = tslx_ext_mutex_lock(mutex); // mutex must be lock before calling pthread_cond_wait venu for tslx

    if (retcode == 0) {
	         retcode = tslx_ext_cond_wait(cond, mutex);        // Wait for the cond variable to be signaled
	     if (retcode == 0) {                              // The must allways be unlock after the cond_wait call
	
            retcode = tslx_ext_mutex_unlock(mutex);       // If cond_wait was good check return code of unlock
        }
        else {
	
            tslx_ext_mutex_unlock(mutex);                // If cond_wait failed use its return and do not check unlock
        }
    }
	
    FUNCTION_RETURN_NUMERIC(retcode,(NULL));
}

short abortTransaction(void)
{
	FUNCTION_ENTRY("abortTransaction",("..."));
	short retcode = tslx_ext_ABORTTRANSACTION();
	FUNCTION_RETURN_NUMERIC(retcode,(NULL));
}

short beginTransaction(long *transTag)
{
	FUNCTION_ENTRY("beginTransaction",("transTag address=0x%08x",transTag));
    short retcode = tslx_ext_BEGINTRANSACTION(transTag);

	// Display txn id in external (ASCII) form
	DEBUG_TRANSTAG();

	FUNCTION_RETURN_NUMERIC(retcode,("transTag=0x%08x",*transTag));
}

short resumeTransaction(long transTag) 
{
	FUNCTION_ENTRY("resumeTransaction",("transTag=0x%08x",transTag));
    short retcode = tslx_ext_RESUMETRANSACTION(transTag);

	// Display txn id in external (ASCII) form or txn status if transTag = 0
	DEBUG_TRANSTAG();

	FUNCTION_RETURN_NUMERIC(retcode,(NULL));
}

short endTransaction(void)
{
	FUNCTION_ENTRY("endTransaction",("..."));
    short retcode = tslx_ext_ENDTRANSACTION();
	FUNCTION_RETURN_NUMERIC(retcode,(NULL));
}

//Added for R3.0 Transaction issue sol. 10-100430-9906
short tmfInit(void)
{
	FUNCTION_ENTRY("tmfInit",("..."));
    short retcode = tslx_ext_TMF_Init();
	FUNCTION_RETURN_NUMERIC(retcode,(NULL));
}
