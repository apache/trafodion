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
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

DLLEXPORT void
tfds_init_ (char* version)
{
  return;
}

DLLEXPORT void
TFDS_MESSAGE_PARAMS_(int flags, char* des_msg1, char* des_msg2,
  char* des_msg3, char* des_msg4, char* des_msg5)
{
  return;
}


DLLEXPORT
int PROCESS_DIP (
        int dip_id,
        int severity,
        int action,
        void (*file_proc_ptr)(char*),
 void (*reg_proc_ptr)(char*,char*,char*,char*)
)
{
  return 0;
}

DLLEXPORT void
TFDS_SetException_Handler( void (*callback_funct)(),
                                int  dump_flag,
                                int  severity,
                                void (*file_proc_ptr)(char*),
                                void (*reg_proc_ptr) (char*,char*,char*,char*))
{
  return;
}
