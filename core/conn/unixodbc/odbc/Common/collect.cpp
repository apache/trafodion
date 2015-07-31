/*************************************************************************
*
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

#include <windows.h>
#include <stdlib.h>
#include "collect.h"

typedef struct _node_tag  {
	DWORD dwKey;
	PVOID *pData;
	struct _node_tag *pNext;
} NODE, *PNODE;

static NODE Head = {(DWORD)-1, NULL, NULL};

BOOL GetEntry (DWORD dwKey, PVOID *ppData)
{
	PNODE pCurrent;

	pCurrent = Head.pNext;
	while (NULL != pCurrent) {
		if (dwKey == pCurrent->dwKey)  {
			*ppData = pCurrent->pData;
			return(TRUE);
		}
		pCurrent = pCurrent->pNext;
	}

	return(FALSE);
}

BOOL AddEntry (DWORD dwKey, PVOID pData)
{
	PNODE pTemp;

	pTemp = (PNODE) malloc (sizeof (NODE));
	if (NULL == pTemp)  {
		return(FALSE);
	}

	pTemp->dwKey = dwKey;
	pTemp->pData = (void**)pData;
	pTemp->pNext = Head.pNext;
	Head.pNext = pTemp;

	return(TRUE);
}	

BOOL DeleteEntry (DWORD dwKey, PVOID *ppData)
{
	PNODE pCurrent, pTemp;

	pTemp = &Head;
	pCurrent = Head.pNext;

	while (NULL != pCurrent) {
		if (dwKey == pCurrent->dwKey)  {
			pTemp->pNext = pCurrent->pNext;
			*ppData = pCurrent->pData;
			free (pCurrent);
			return(TRUE);
		}
		else {
			pTemp = pCurrent;
			pCurrent = pCurrent->pNext;
		}
	}

	return(FALSE);
}	
