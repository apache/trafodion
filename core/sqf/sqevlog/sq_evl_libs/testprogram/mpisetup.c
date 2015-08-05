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
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "seabed/ms.h"
#include "seabed/fs.h"

#define TRUE  1
#define FALSE 0


void
my_mpi_close()
{
#ifdef MPI_
  msg_mon_close_process(msg_get_phandle("$SYSTEM"));
  msg_mon_close_process(msg_get_phandle("$DATA"));
  msg_mon_process_shutdown();
#endif
}

void
my_mpi_fclose()
{
#ifdef MPI_
  msg_mon_close_process(msg_get_phandle("$SYSTEM"));
  msg_mon_close_process(msg_get_phandle("$DATA"));
  file_mon_process_shutdown();
#endif
}

short my_mpi_setup (int argc, char* argv[] )
{
#ifdef MPI_
  file_init_attach(&argc,&argv,TRUE,"");
  file_mon_process_startup(FALSE);
#endif

  msg_debug_hook("NGG", "ngg.hook");

  return 0;
}
