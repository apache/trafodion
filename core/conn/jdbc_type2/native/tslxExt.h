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

/*
 * HISTORY
 * $Log: spt_extensions.h,v $
 *
 * 2010/09/28
 * Fix Soln: 10-100526-0628. OSS - pthreads  SPT_xxx wrapper functions.
 * Changes: Changed the byte count parameters (read_count, write_count & *count_read)
 * from short to unsigned short in the thread aware wrappers to match the current 
 * definitions of the NSK I/O functions.
 *
 * $EndLog$
 */


#ifndef _TSLX_EXT_H /* { */
#define _TSLX_EXT_H

#include <thread_safe_extended.h> nolist

typedef void (*TSLX_func_ptr)(void);

typedef void (*spt_FileIOHandler_p)(const short filenum, const long	tag, const long count_transferred, const long error, void *userdata);

enum tslx_error {
	TSLXE_SUCCESS, TSLXE_ERROR, TSLXE_INTERRUPTED, TSLXE_TIMEDOUT 

};

_TSLX_t tslx_ext_pthread_self(); //pthread_self
int tslx_ext_cond_signal( _TSLX_cond_t *); //pthread_cond_signal
int tslx_ext_cond_wait(_TSLX_cond_t *, _TSLX_mutex_t *);
int tslx_ext_cond_init( _TSLX_cond_t *, const _TSLX_condattr_t *); //pthread_cond_init
int tslx_ext_cond_destroy( _TSLX_cond_t * ); //pthread_cond_destroy
int tslx_ext_regFileIOHandler(const short,const spt_FileIOHandler_p ); //spt_regFileIOHandler
short tslx_ext_TMF_Init(); //SPT_TMF_Init
int tslx_ext_mutex_init ( _TSLX_mutex_t *, int );
int tslx_ext_mutex_destroy ( _TSLX_mutex_t * );
int tslx_ext_mutex_lock ( _TSLX_mutex_t * );
int tslx_ext_mutex_unlock ( _TSLX_mutex_t * );
short tslx_ext_ABORTTRANSACTION ( void );
short tslx_ext_BEGINTRANSACTION ( long * );
short tslx_ext_RESUMETRANSACTION ( long );
short tslx_ext_ENDTRANSACTION ( void );

#endif	/* } _TSLX_EXT_H */

