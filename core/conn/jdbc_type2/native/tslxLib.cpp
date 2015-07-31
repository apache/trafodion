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

#include <thread_safe_extended.h> nolist
#include "tslxExt.h" nolist  /* Includes thread_safe_extended.h internally */
#include <time.h>    nolist
#include <thread_safe_extended.h> nolist
#include <stdio.h> nolist

extern int __thread_model_ ;

/*
 *   Thread Model Independent Library transfer pointers
 */
int  (*tslxe_cond_destroy) (_TSLX_cond_t *);
int  (*tslxe_cond_init) (_TSLX_cond_t *, const _TSLX_condattr_t*);
int  (*tslxe_cond_signal) (_TSLX_cond_t *);
int  (*tslxe_cond_wait)(_TSLX_cond_t *,_TSLX_mutex_t *);
_TSLX_t (*tslxe_pthread_self)();
int (*tslxe_regFileIOHandler)(const short ,const spt_FileIOHandler_p );
short (*tslxe_TMF_Init)();

int (*tslxe_mutex_init) ( _TSLX_mutex_t *, int );
int (*tslxe_mutex_destroy) ( _TSLX_mutex_t * );
int (*tslxe_mutex_lock) ( _TSLX_mutex_t * );
int (*tslxe_mutex_unlock) ( _TSLX_mutex_t * );
short (*tslxe_ABORTTRANSACTION) ( void );
short (*tslxe_BEGINTRANSACTION) ( long * );
short (*tslxe_RESUMETRANSACTION) ( long );
short (*tslxe_ENDTRANSACTION) ( void );

int tslx_ext_mutex_init ( _TSLX_mutex_t *mutex, int value)
{
	if (__thread_model_ == Unthreaded)
      return 0;
   else /* PUT or SPT library present */
      return tslxe_mutex_init ( mutex, value);

}

int tslx_ext_mutex_destroy ( _TSLX_mutex_t *mutex )
{
if (__thread_model_ == Unthreaded)
      return 0;
   else /* PUT or SPT library present */
      return tslxe_mutex_destroy (mutex);
}

int tslx_ext_mutex_lock ( _TSLX_mutex_t *mutex)
{
	if (__thread_model_ == Unthreaded)
      return 0;
   else /* PUT or SPT library present */
      return tslxe_mutex_lock(mutex);

}

int tslx_ext_mutex_unlock ( _TSLX_mutex_t *mutex )
{
	if (__thread_model_ == Unthreaded)
      return 0;
   else /* PUT or SPT library present */
      return tslxe_mutex_unlock(mutex);
}

short tslx_ext_ABORTTRANSACTION ()
{
if (__thread_model_ == Unthreaded)
      return 0;
   else /* PUT or SPT library present */
      return tslxe_ABORTTRANSACTION ();

}

short tslx_ext_BEGINTRANSACTION ( long *transTag )
{
if (__thread_model_ == Unthreaded)
      return 0;
   else /* PUT or SPT library present */
      return tslxe_BEGINTRANSACTION (transTag);

}

short tslx_ext_RESUMETRANSACTION ( long transTag )
{
if (__thread_model_ == Unthreaded)
      return 0;
   else /* PUT or SPT library present */
      return tslxe_RESUMETRANSACTION (transTag);
	
}

short tslx_ext_ENDTRANSACTION ()
{
	if (__thread_model_ == Unthreaded)
      return 0;
   else /* PUT or SPT library present */
      return tslxe_ENDTRANSACTION ();

}


int tslx_ext_cond_destroy (_TSLX_cond_t *cond)
{
   if (__thread_model_ == Unthreaded)
      return 0;
   else /* PUT or SPT library present */
      return tslxe_cond_destroy(cond);
}


int tslx_ext_cond_wait (_TSLX_cond_t *cond, _TSLX_mutex_t *mutex)
{
   if (__thread_model_ == Unthreaded)
      return 0;
   else /* PUT or SPT library present */
      return tslxe_cond_wait(cond,mutex);
}




int tslx_ext_cond_init (_TSLX_cond_t *cond, const _TSLX_condattr_t *attr)
{

   if (__thread_model_ == Unthreaded)
   {
	 // printf("In conn init returning 0\n");
      return 0;
   }
   else /* PUT or SPT library present */
   {
	 
      return tslxe_cond_init(cond, attr);
   }
}

int tslx_ext_cond_signal (_TSLX_cond_t *cond)
{
   if (__thread_model_ == Unthreaded)
      return 0;
   else /* PUT or SPT library present */
      return tslxe_cond_signal(cond);
}


_TSLX_t tslx_ext_pthread_self ()
{
	_TSLX_t temp;
   if (__thread_model_ == Unthreaded)
      return temp;
   else /* PUT or SPT library present */
      return tslxe_pthread_self();
}

int tslx_ext_regFileIOHandler(const short filenum,const spt_FileIOHandler_p functionPtr)
{
   if (__thread_model_ == Unthreaded)
      return 0;
   else /* PUT or SPT library present */
      return tslxe_regFileIOHandler( filenum,  functionPtr);
}

short tslx_ext_TMF_Init ()
{
   if (__thread_model_ == Unthreaded)
      return 0;
   else /* PUT or SPT library present */
      return tslxe_TMF_Init();
}


