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

#include "MemLeak.h"

//LCOV_EXCL_START

#ifdef NSK_QS
	#ifdef TRACE_MEMORY_LEAK_QS
		#define TRACE_MEMORY_LEAK
	#else
		#undef TRACE_MEMORY_LEAK
	#endif
#endif

#ifdef NSK_STATS
	#ifdef TRACE_MEMORY_LEAK_STATS
		#define TRACE_MEMORY_LEAK
	#else
		#undef TRACE_MEMORY_LEAK
	#endif
#endif

#ifdef NSK_COM
	#ifdef TRACE_MEMORY_LEAK_COM
		#define TRACE_MEMORY_LEAK
	#else
		#undef TRACE_MEMORY_LEAK
	#endif
#endif

#ifdef NSK_RULE
	#ifdef TRACE_MEMORY_LEAK_RULE
		#define TRACE_MEMORY_LEAK
	#else
		#undef TRACE_MEMORY_LEAK
	#endif
#endif

#ifdef NSK_SYNC
	#ifdef TRACE_MEMORY_LEAK_SYNC
		#define TRACE_MEMORY_LEAK
	#else
		#undef TRACE_MEMORY_LEAK
	#endif
#endif

#ifdef NSK_OFFNDR
	#ifdef TRACE_MEMORY_LEAK_OFFNDR
		#define TRACE_MEMORY_LEAK
	#else
		#undef TRACE_MEMORY_LEAK
	#endif
#endif

#ifndef _DEBUG
#undef TRACE_MEMORY_LEAK
#endif

#ifdef TRACE_MEMORY_LEAK

#include <new>
#include <histry.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern long GetCurrentProcessId(void);
extern "C" int RTL_heap_getattribute_(int, size_t *);

typedef struct node 
{
	void* p;
	size_t size;
	bool bAlreadyPrinted;
	long id;
	node* next;
	char buff[512];
} node;

typedef struct nlist 
{
	node* list;
	nlist()
	{
		list=NULL;
	}
	~nlist()
	{
		node* cnode = list;
		node* nnode;
		while( cnode != NULL )
		{
			nnode = cnode->next;
			free(cnode);
			cnode = nnode;
		}
		list=NULL;
	}

	bool ins_node( void* p, long id, size_t size, char* txt)
	{
		node* cnode = list;
		node* pnode = list;
		node* nnode;

		while(cnode!=NULL )
		{
			pnode=cnode;
			cnode=cnode->next;
		}
		if((nnode = (node*)malloc(sizeof(node)))!=NULL)
		{
			nnode->id = id;
			nnode->p = p;
			nnode->size = size;
			strcpy(nnode->buff, txt);
			nnode->bAlreadyPrinted = false;

			nnode->next = cnode;
			if(pnode!=NULL) 
				pnode->next = nnode;
			else
				list = nnode;
		}
		return (nnode == NULL)?false:true;
	}
	bool del_node(void* p )
	{
		node* cnode = list;
		node* pnode = list;
		while( cnode!= NULL )
		{
			if ( p == cnode->p )
				break;
			pnode = cnode;
			cnode = cnode->next;
		}
		if( cnode==NULL)
			return false;
		if (pnode == list && cnode == list)
			list = cnode->next;
		else
			pnode->next = cnode->next;
		free(cnode);
		return true;
	}
	size_t find_node(void* p )
	{
		node* cnode = list;
		while( cnode != NULL )
		{
			if ( p == cnode->p )
				break;
			cnode = cnode->next;
		}
		if( cnode==NULL)
			return 0;
		return cnode->size;
	}

	void list_node(char* description, bool bskip)
	{
		char fileName[80];

		sprintf( fileName, "ML%d",GetCurrentProcessId());
		FILE *fp = fopen_guardian( fileName,"a+");
		if (fp != NULL)
		{
			fprintf(fp,"---- %s ----\n",description);
			fflush(fp);
			int activeHeap;
			long total = 0,lp=0;
			node* cnode = list;
			node* nnode;
			while( cnode != NULL ){
				nnode = cnode->next;
				cnode->bAlreadyPrinted = true;
				total += cnode->size;
				if (bskip == false)
				{
					fprintf(fp,"%d id = %d, size = %d, trace=%s\n",++lp,cnode->id,cnode->size,cnode->buff);
					fflush(fp);
				}
				cnode = nnode;
			}
			RTL_heap_getattribute_(11, &activeHeap);
			fprintf(fp,"Total memory %d, heap = %d\n",total, activeHeap);
			fflush(fp);
			fclose(fp);
		}
	}

	void list_added_node(char* description, bool bskip)
	{
		char fileName[80];

		sprintf( fileName, "ML%d",GetCurrentProcessId());
		FILE *fp = fopen_guardian( fileName,"a+");
		if (fp != NULL)
		{
			fprintf(fp,"---- %s ----\n",description);
			int activeHeap;
			long total = 0,lp=0;
			node* cnode = list;
			node* nnode;
			while( cnode != NULL ){
				nnode = cnode->next;
				if (false == cnode->bAlreadyPrinted)
				{
					cnode->bAlreadyPrinted = true;
					total += cnode->size;
					if (bskip == false)
						fprintf(fp,"%d id = %d, size = %d, trace=%s\n",++lp,cnode->id,cnode->size,cnode->buff);
				}
				cnode = nnode;
			}
			RTL_heap_getattribute_(11, &activeHeap);
			fprintf(fp,"Total memory %d, heap = %d\n",total, activeHeap);
			fclose(fp);
		}
	}


} nlist;

nlist memList;
static long id = 1;
unsigned long MTotal;
NSK_histWorkspace mhws;

char gfile[51];
char gfunction[51];
long gline = 0;

void Awrite(void* p, size_t size, char* tmp)
{
	MTotal += size;
	memList.ins_node(p,id++,size, tmp);
}

void Dwrite(void* p)
{
	size_t size = memList.find_node(p );
	MTotal -= size;
	memList.del_node(p);
}

void* operator_new(size_t nSize)
{
	char buf[129];
	char tmp[512] = {0};
	int numtry = 0;
	int error;

	if (gfile[0] != 0)
	{
		strcat(tmp,gfile);
		strcat(tmp, "::");
		gfile[0] = 0;
	}
	if (gfunction[0] != 0)
	{
		strcat(tmp,gfunction);
		strcat(tmp, "::");
		gfunction[0] = 0;
	}
	if (gline != 0)
	{
		sprintf(buf,"%d",gline);
		strcat(tmp, buf);
		gline = 0;
	}
	if (tmp[0] == 0)
	{
		error = HIST_INIT_ (&mhws, HistVersion1, HO_Init_Here | HO_OneLine, NULL);
		if (error == HIST_OK)
			do 
			{
				int len, tlen=strlen(tmp);
				numtry++;
				while ((len = HIST_FORMAT_ (&mhws, buf, 79)) > 0) 
				{
					buf[ len ] = 0;
					tlen += (len + 2);
					if (tlen > sizeof(tmp)) break;
					strcat(tmp, buf);
					strcat(tmp, "|");
				}
			} while (((error = HIST_GETPRIOR_ (&mhws)) == HIST_OK) && numtry <= 50);
			else
				sprintf (tmp, "HIST_INIT_ error: %d. Unable to trace\n", error);
	}

	void* p=malloc(nSize);
	Awrite(p,nSize,tmp);
	return p;
}

//scalar, throwing new and it matching delete
void* operator new (std::size_t size) throw(std::bad_alloc) {
	return operator_new(size);
}
 
void operator delete (void* ptr) throw() {
	Dwrite(ptr);
	free(ptr);
}

//scalar, nothrow new and it matching delete
void* operator new (std::size_t size,const std::nothrow_t&) throw() {
	return operator_new(size);
}

void operator delete (void* ptr, const std::nothrow_t&) throw() {
	Dwrite(ptr);
	free(ptr);
}

//array throwing new and matching delete[]
void* operator new [](std::size_t size) throw(std::bad_alloc) {
	return operator_new(size);
}

void operator delete[](void* ptr) throw() {
	Dwrite(ptr);
	free(ptr);
}

//array, nothrow new and matching delete[]
void* operator new [](std::size_t size, const std::nothrow_t&) throw() {
	return operator_new(size);
}

void operator delete[](void* ptr, const std::nothrow_t&) throw() {
	Dwrite(ptr);
	free(ptr);
}

void ListAll(char* description, bool bskip)
{
    memList.list_node(description, bskip);
}

void ListLastAdded(char* description, bool bskip)
{
	memList.list_added_node(description, bskip);
}

void markNewLine(char* file,char* function, long line)
{
	if(file != NULL)
		strncpy(gfile, file, sizeof(gfile)-1);
	if(function != NULL)
		strncpy(gfunction, function, sizeof(gfunction)-1);
	gline = line;
}

#else

void ListAll(char* description, bool bskip)
{
}

void ListLastAdded(char* decription, bool bskip)
{
}

void markNewLine(char* file,char* function, long line)
{
}

#endif
//LCOV_EXCL_STOP
