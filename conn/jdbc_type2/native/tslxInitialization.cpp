// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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



#include <dlfcn.h>   nolist
#include "tslxExt.h" nolist       /* Includes thread_safe_extended.h */
#include <stdio.h>   nolist

int __thread_model_ ;

/*
 *   Fiction to simplify initialization logic
 */

extern TSLX_func_ptr tslxe_cond_destroy;
extern TSLX_func_ptr tslxe_cond_init;
extern TSLX_func_ptr tslxe_cond_wait;
extern TSLX_func_ptr tslxe_cond_signal;
extern TSLX_func_ptr tslxe_pthread_self;
extern TSLX_func_ptr tslxe_regFileIOHandler;
extern TSLX_func_ptr tslxe_TMF_Init;
extern TSLX_func_ptr tslxe_mutex_init;
extern TSLX_func_ptr tslxe_mutex_destroy;
extern TSLX_func_ptr tslxe_mutex_lock;
extern TSLX_func_ptr tslxe_mutex_unlock;
extern TSLX_func_ptr tslxe_ABORTTRANSACTION;
extern TSLX_func_ptr tslxe_BEGINTRANSACTION;
extern TSLX_func_ptr tslxe_RESUMETRANSACTION;
extern TSLX_func_ptr tslxe_ENDTRANSACTION;



dlHandle TSLX_dlopen_handle;


int get_ptr ( TSLX_func_ptr *ptr_to_ptr, const char *symbol )
{
   void  *function_ptr;

   function_ptr = dlsym(TSLX_dlopen_handle, symbol);
   if (function_ptr == NULL) {
      dlclose(TSLX_dlopen_handle);
//	  printf("Function not found %s\n",symbol);
      return 1;                        /* Function not found */

   }
   *ptr_to_ptr = (TSLX_func_ptr)function_ptr;
 //  printf("Function found %s\n",symbol);
   return 0;                           /* Function found */
}


/*
 *   Initialize PUT/SPT transfer pointers
 */
int initialize_for_H23 ( void )
{
  // TSLX_dlopen_handle = dlopen("/G/SYSTEM/SYSTEM/ZSPTDLL", RTLD_NOW | RTLD_VERBOSE(1));
  // if (TSLX_dlopen_handle == NULL) {
/*    printf("dlopen() result is %s (octal %o)\n", dlerror(), dlresultcode());*/
	//   printf("error in dlopen SPT\n");
    //  return 0;                        /* Initialization impossible */
   //}
  if (get_ptr(&tslxe_cond_destroy, "pthread_cond_destroy"))
      return 0;
   if (get_ptr(&tslxe_cond_init, "pthread_cond_init"))
	   return 0;
   if (get_ptr(&tslxe_cond_wait, "pthread_cond_wait"))
	   return 0;
   if (get_ptr(&tslxe_cond_signal, "pthread_cond_signal"))
      return 0;
   if (get_ptr(&tslxe_pthread_self, "pthread_self"))
      return 0;


   if (get_ptr(&tslxe_mutex_init, "pthread_mutex_init"))
      return 0;
   if (get_ptr(&tslxe_mutex_destroy, "pthread_mutex_destroy"))
	   return 0;
   if (get_ptr(&tslxe_mutex_lock, "pthread_mutex_lock"))
	   return 0;
   if (get_ptr(&tslxe_mutex_unlock, "pthread_mutex_unlock"))
      return 0;


if (get_ptr(&tslxe_ABORTTRANSACTION, "SPT_ABORTTRANSACTION"))
      return 0;
   if (get_ptr(&tslxe_BEGINTRANSACTION, "SPT_BEGINTRANSACTION"))
	   return 0;
   if (get_ptr(&tslxe_RESUMETRANSACTION, "SPT_RESUMETRANSACTION"))
	   return 0;
   if (get_ptr(&tslxe_ENDTRANSACTION, "SPT_ENDTRANSACTION"))
      return 0;

   if (get_ptr(&tslxe_regFileIOHandler, "spt_regFileIOHandler"))
         return 0;
      if (get_ptr(&tslxe_TMF_Init, "SPT_TMF_Init"))
         return 0;
  //  printf("Success SPT\n");
   return 1;                           /* Initialization succeeded */
}



/*
 *   Initialize PUT/SPT transfer pointers
 */
int initialize_for_thread_library ( void )
{


   if (get_ptr(&tslxe_cond_destroy, "pthread_cond_destroy"))
      return 0;
   if (get_ptr(&tslxe_cond_init, "pthread_cond_init"))
	   return 0;
   if (get_ptr(&tslxe_cond_wait, "pthread_cond_wait"))
	   return 0;
   if (get_ptr(&tslxe_cond_signal, "pthread_cond_signal"))
      return 0;
   if (get_ptr(&tslxe_pthread_self, "pthread_self"))
      return 0;


   if (get_ptr(&tslxe_mutex_init, "_TSLX_mutex_init"))
      return 0;
   if (get_ptr(&tslxe_mutex_destroy, "_TSLX_mutex_destroy"))
	   return 0;
   if (get_ptr(&tslxe_mutex_lock, "_TSLX_mutex_lock"))
	   return 0;
   if (get_ptr(&tslxe_mutex_unlock, "_TSLX_mutex_unlock"))
      return 0;


if (get_ptr(&tslxe_ABORTTRANSACTION, "_TSLX_ABORTTRANSACTION"))
      return 0;
   if (get_ptr(&tslxe_BEGINTRANSACTION, "_TSLX_BEGINTRANSACTION"))
	   return 0;
   if (get_ptr(&tslxe_RESUMETRANSACTION, "_TSLX_RESUMETRANSACTION"))
	   return 0;
   if (get_ptr(&tslxe_ENDTRANSACTION, "_TSLX_ENDTRANSACTION"))
      return 0;

   if (_CMATHREADS) {
	   /*Place holder for PUT*/
	   /*Since it is not supported now, we will initialize to un threaded*/
//	   printf("CMATHREADS cannot initialize\n");
	   if (get_ptr(&tslxe_regFileIOHandler, "put_regFileIOHandler"))
         return 0;
      if (get_ptr(&tslxe_TMF_Init, "PUT_TMF_Init"))

        return 0;

   } else if (_SPTHREADS) {
//	   printf("Initializing with SPT threads\n");
      if (get_ptr(&tslxe_regFileIOHandler, "spt_regFileIOHandler"))
         return 0;
      if (get_ptr(&tslxe_TMF_Init, "SPT_TMF_Init"))
         return 0;
   } else if(_KTHREADS)
   {
         /*Place holder for kernel threads*/
	   /*Since it is not supported now, we will initialize to un threaded*/
	   return 0;                         /* Should never happen! */
//	 printf("Unknown threads\n");
   }
   else
   {
     return 0;                         /* Should never happen! */
	// printf("Unknown threads\n");
   }

   return 1;                           /* Initialization succeeded */
}


int client_initialization ( void )
{
   int Result;
   void  *test_ptr;
      TSLX_dlopen_handle = dlopen(NULL, RTLD_NOW | RTLD_VERBOSE(1));
   if (TSLX_dlopen_handle == NULL) {
/*    printf("dlopen() result is %s (octal %o)\n", dlerror(), dlresultcode());*/
	   printf("error in dlopen \n");
      return 0;                        /* Initialization impossible */
   }

	test_ptr = dlsym(TSLX_dlopen_handle, "_TSLX_mutex_lock");
   if (test_ptr == NULL) {
    //  dlclose(TSLX_dlopen_handle);
	  __thread_model_ =1;
//		  printf("TSLX symbols not found\n");
	  Result=initialize_for_H23();

     // return 1;                        /* Function not found */

   }
   else
   {
	   //printf("Found TSLX symbols\n");
		//  printf("In client initialization\n");
		if (_CMATHREADS) {
		//	printf("PUT threads\n");
		//__thread_model = PUT_threads;
		//  printf("CMATHREADS\n");
		__thread_model_ =2;
		Result = initialize_for_thread_library();
		/* if Result == 0, what to do? */
		} else if (_SPTHREADS) {
		//	printf("SPT threads\n");
		//__thread_model = SPT_threads;
		//printf("SPT THREADS\n");
		__thread_model_ =1;
		Result = initialize_for_thread_library();
		/* if Result == 0, what to do? */
		}else if(_KTHREADS)
		{
		//printf("SPT threads\n");
		/*Place holder for kernel threads*/
		/*Since it is not supported now, we will initialize to un threaded*/
	    __thread_model_ =0;
		Result = 1;

		}
		else
		{
		//	printf("Unthreaded threads\n");
		//__thread_model = Unthreaded;
		//printf("Unknown thread library\n");
		__thread_model_ =0;
		Result = 1;
		}

   }
   return Result;
}
