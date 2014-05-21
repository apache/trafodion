// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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
/*
 * =====================================================================================
 *
 *       Filename:  sqEnv.h
 *
 *    Description:  Header file for class SqEnv
 *
 *        Version:  1.0
 *        Created:  10/08/11 20:03:48
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#ifndef __SQENV_H
#define __SQENV_H

#include "seabed/ms.h"

class SqEnv
{
   public:
      SqEnv(int *argc, char ***argv, const char *pname = "")
      {
         char lv_pname[256];
         sprintf(lv_pname, "%s_%d", pname, getpid());

         msg_init_attach(argc, argv, 1, lv_pname );

         // start message monitor
         msg_mon_process_startup(1); // set to true to receive monitor messages

         // enable message
         msg_mon_enable_mon_messages(1);

         sprintf(lv_pname, "%s.hook", pname);
         msg_debug_hook(pname, lv_pname);
      }
      ~SqEnv()
      {
         // stop seabed message service
         msg_mon_process_shutdown( ); // return value ignored
      }

   private:
      SqEnv();
};

#endif
