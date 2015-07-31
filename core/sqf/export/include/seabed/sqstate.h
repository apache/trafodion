//------------------------------------------------------------------
//
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

//
// sqstate module
//
#ifndef __SB_SQSTATE_H_
#define __SB_SQSTATE_H_

#include "ms.h"

#include "int/sqstate.h"

//
// format of options array
//
typedef struct Sqstate_Ic_Arg_Opt_Type {
    const char *option_str;
    int         option_int;
} Sqstate_Ic_Arg_Opt_Type;

//
// sqstate-ic (interceptor) EP (entry point)
//
// usage:
//   SQSTATE_IC_EP(mymodule,a_ic,sre) {
//       // body goes here
//   }
//
#define SQSTATE_IC_EP(module,ic,sre) \
  extern "C" void INT_SQSTATE_IC_NAME_PARAMS(module,ic,sre); \
  void INT_SQSTATE_IC_NAME_PARAMS(module,ic,sre)

//
// ICs can call this to format a transid.
// Returns true if valid transid
//
SB_Export bool sqstateic_format_transid(SB_Transid_Type *transid_in,
                                        char            *transid_out,
                                        int              transid_out_len);

//
// ICs can call this to transid from ctrl.
// Returns non-NULL if there's a transid.
//
SB_Export SB_Transid_Type *sqstateic_get_transid(void *ctrl,
                                                 int   ctrl_len);


//
// sqstate-ic (interceptor) variable template for use by IC macros below.
//
typedef enum Sqstate_Ic_Var_Flags {
    SQSTATE_IC_VAR_FLAGS_NONE      = 0x00,
    SQSTATE_IC_VAR_FLAGS_ACC_MASK  = 0x0f,
    SQSTATE_IC_VAR_FLAGS_ACC_NONE  = 0x00,
    SQSTATE_IC_VAR_FLAGS_ACC_R     = 0x01,
    SQSTATE_IC_VAR_FLAGS_ACC_W     = 0x02,
    SQSTATE_IC_VAR_FLAGS_ACC_RW    = 0x03,
    SQSTATE_IC_VAR_FLAGS_TYPE_MASK = 0xf0,
    SQSTATE_IC_VAR_FLAGS_TYPE_NONE = 0x00,
    SQSTATE_IC_VAR_FLAGS_TYPE_DYN  = 0x10
} SqState_Ic_Var_Flags;
typedef struct Sqstate_Ic_Var {
    const char *var_str;    // var string
    const char *var_struct; // var struct
    void       *var_addr;   // var address
    long        var_size;   // var size
    int         var_flags;  // var flags
} Sqstate_Ic_Var;

//
// sqstate-ic (interceptor) variable
//
// usage:
//   int  var1;
//   long var2;
//   SQSTATE_IC_VAR_TABLE_BEGIN(my_var_table)
//     SQSTATE_IC_VAR_ENTRY(var1)
//     SQSTATE_IC_VAR_ENTRY(var2)
//     SQSTATE_IC_VAR_STRUCT_ENTRY(var3struct,var3)
//   SQSTATE_IC_VAR_TABLE_END(my_var_table)
//
//   sqstateic_var_add(my_var_table, sizeof(my_var_table));
//
#define SQSTATE_IC_VAR_TABLE_BEGIN(v) \
static Sqstate_Ic_Var v[] = { \
  { "begin-" #v, NULL, NULL, sizeof(Sqstate_Ic_Var), SQSTATE_IC_VAR_FLAGS_NONE },

#define SQSTATE_IC_VAR_TABLE_END(v) \
  { "end-" #v, NULL, NULL, sizeof(Sqstate_Ic_Var), SQSTATE_IC_VAR_FLAGS_NONE } \
};

#define SQSTATE_IC_VAR_ENTRY(v) \
{ #v, NULL, reinterpret_cast<void *>(&v), sizeof(v), SQSTATE_IC_VAR_FLAGS_ACC_RW }

#define SQSTATE_IC_VAR_RO_ENTRY(v) \
{ #v, NULL, reinterpret_cast<void *>(&v), sizeof(v), SQSTATE_IC_VAR_FLAGS_ACC_R }

#define SQSTATE_IC_VAR_RW_ENTRY(v) \
{ #v, NULL, reinterpret_cast<void *>(&v), sizeof(v), SQSTATE_IC_VAR_FLAGS_ACC_RW }

#define SQSTATE_IC_VAR_STRUCT_ENTRY(s,v) \
{ #v, #s, reinterpret_cast<void *>(&v), sizeof(v), SQSTATE_IC_VAR_FLAGS_ACC_RW }

#define SQSTATE_IC_VAR_STRUCT_RO_ENTRY(s,v) \
{ #v, #s, reinterpret_cast<void *>(&v), sizeof(v), SQSTATE_IC_VAR_FLAGS_ACC_R }

#define SQSTATE_IC_VAR_STRUCT_RW_ENTRY(s,v) \
{ #v, #s, reinterpret_cast<void *>(&v), sizeof(v), SQSTATE_IC_VAR_FLAGS_ACC_RW }

//
// IC format types
//
enum {
    SQSTATE_IC_FMT_TYPE_HEX = 0x1,
    SQSTATE_IC_FMT_TYPE_DEC = 0x2
};

//
// IC struct formatter signature
//
typedef int (*Sqstate_Ic_Struct_Fmt)(const char *data_name,
                                     void       *data,
                                     const char *struct_name,
                                     void       *struct_desc,
                                     int         fmt_type,
                                     char       *rsp);
//
// ICs can call this to add structs.
//
SB_Export int sqstateic_struct_add(const char            *struct_name,
                                   Sqstate_Ic_Struct_Fmt  formatter,
                                   void                  *struct_desc);

//
// ICs can call this to format a struct.
//
SB_Export int sqstateic_struct_format(const char *struct_name,
                                      const char *data_name,
                                      void       *data,
                                      int         fmt_type,
                                      char       *rsp);

//
// ICs can call this to format a struct field.
//
SB_Export int sqstateic_struct_format_field(void *data,
                                            long  data_size,
                                            long  data_off,
                                            int   fmt_type,
                                            char *rsp);

//
// ICs can call this to add variables.
//
SB_Export int sqstateic_var_add(Sqstate_Ic_Var *vars, long sizeof_vars);

//
// ICs can call this to add dynamic variable.
//
SB_Export int sqstateic_var_dyn_add(const char *var_str,
                                    void       *var_addr,
                                    long        var_size,
                                    bool        var_ro);

//
// ICs can call this to add dynamic variable.
//
SB_Export int sqstateic_var_dyn_del(const char *var_str);

//
// ICs can call this to get lookup a variable.
//
SB_Export Sqstate_Ic_Var *sqstateic_var_lookup(const char *var_str);

//
// ICs can call this to set the rsp to list of options
//
// If this function finds -h or -help in ic_argv,
// then this function will format rsp and return the size of rsp.
//
// usage:
//   Sqstate_Ic_Arg_Opt_Type options[] = {
//     { "-optxx", 0 },
//     { "-optyy", 0 },
//     { NULL,     0 }
//   };
//   int len = sqstateic_set_ic_args_options(ic_argc,
//                                           ic_argv,
//                                           rsp,
//                                           rsp_len,
//                                           options);
//   if (len)
//       return len;
//
SB_Export int sqstateic_set_ic_args_options(int                      ic_argc,
                                            char                    *ic_argv[],
                                            char                    *rsp,
                                            int                      rsp_len,
                                            Sqstate_Ic_Arg_Opt_Type *options);

//
// sqstate-pi (plug-in) EP (entry point)
//
// usage:
//   SQSTATE_PI_EP(mymodule,a_pi,node,proc.info,lib) {
//       // body goes here
//   }
//
#define SQSTATE_PI_EP(module,pi,node,proc,info,lib) \
  extern "C" void INT_SQSTATE_PI_NAME_PARAMS(module,pi,node,proc,info,lib); \
  void INT_SQSTATE_PI_NAME_PARAMS(module,pi,node,proc,info,lib)

//
// sqstate-pi-sqstate (plug-in) EP (entry point)
//
// usage:
//   SQSTATE_PI_SQSTATE_EP(mymodule,a_pi,node,proc.info,lib) {
//       // body goes here
//   }
//
#define SQSTATE_PI_SQSTATE_EP(module,pi,node,proc,info,lib,rsp,rsplen,rsplenp) \
  extern "C" void INT_SQSTATE_PI_SQSTATE_NAME_PARAMS(module,pi,node,proc,info,lib,rsp,rsplen,rsplenp); \
  void INT_SQSTATE_PI_SQSTATE_NAME_PARAMS(module,pi,node,proc,info,lib,rsp,rsplen,rsplenp)

//
// sqstate-pi-ag-begin (plug-in) EP (entry point)
//
// usage:
//   SQSTATE_PI_AG_BEGIN_EP(mymodule,a_pi,info) {
//       // body goes here
//   }
//
#define SQSTATE_PI_AG_BEGIN_EP(module,pi,info) \
  extern "C" void INT_SQSTATE_PI_AG_BEGIN_NAME_PARAMS(module,pi,info); \
  void INT_SQSTATE_PI_AG_BEGIN_NAME_PARAMS(module,pi,info)

//
// sqstate-pi-ag-end (plug-in) EP (entry point)
//
// usage:
//   SQSTATE_PI_AG_END_EP(mymodule,a_pi,info) {
//       // body goes here
//   }
//
#define SQSTATE_PI_AG_END_EP(module,pi,info) \
  extern "C" void INT_SQSTATE_PI_AG_END_NAME_PARAMS(module,pi,info); \
  void INT_SQSTATE_PI_AG_END_NAME_PARAMS(module,pi,info)

//
// sqstate-pi (plug-in) probe info
//
typedef struct Sqstate_Pi_Info_Type {
    bool   verbose;     // sqstate -v option
    bool   verbosev;    // sqstate -vv option
    bool   self;        // sqstate -self option
    int    ic_argc;     // sqstate -icarg option
    char **ic_argv;     // sqstate -icarg option
} Sqstate_Pi_Info_Type;

//
// sqstate-pi (plug-in) probe function type
//
typedef void (*Sqstate_Pi_Fun_Type) INT_SQSTATE_PI_PARAMS(node,proc,info,lib);

//
// sqstate-pi-ag (plug-in) probe function type
//
typedef void (*Sqstate_Pi_Ag_Fun_Type) INT_SQSTATE_PI_AG_PARAMS(info);

//
// sqstate-pi (plug-in) probe function type (sqstate-internal)
//
typedef void (*Sqstate_Pi_Sqstate_Fun_Type) INT_SQSTATE_PI_SQSTATE_PARAMS(node,proc,info,lib,rsp,rsplen,rsplenp);

//
// PIs can call this to set the ctrl for sending to IC.
//
SB_Export void sqstatepi_set_ctrl(char                 *ctrl,
                                  Sqstate_Pi_Info_Type *info,
                                  const char           *lib,
                                  const char           *module,
                                  const char           *call);

#endif // !__SB_SQSTATE_H_
