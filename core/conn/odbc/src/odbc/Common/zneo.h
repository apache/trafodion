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
/* SCHEMA PRODUCED DATE - TIME : 2/17/2006 - 10:38:39 */
#pragma section systemtypeinfo
/* Constant ZNEO-VAL-PTYPE-UNDEFINED created on 02/17/2006 at 10:38 */
#define ZNEO_VAL_PTYPE_UNDEFINED 0
/* Constant ZNEO-VAL-PTYPE-NSPROPER created on 02/17/2006 at 10:38 */
#define ZNEO_VAL_PTYPE_NSPROPER 1
/* Constant ZNEO-VAL-PTYPE-TASKSERV created on 02/17/2006 at 10:38 */
#define ZNEO_VAL_PTYPE_TASKSERV 2
/* Constant ZNEO-VAL-PUSE-UNDEFINED created on 02/17/2006 at 10:38 */
#define ZNEO_VAL_PUSE_UNDEFINED 0
/* Constant ZNEO-VAL-PUSE-NEO created on 02/17/2006 at 10:38 */
#define ZNEO_VAL_PUSE_NEO 1
/* Constant ZNEO-VAL-PUSE-LEO created on 02/17/2006 at 10:38 */
#define ZNEO_VAL_PUSE_LEO 2
/* Definition ZNEO-DDL-SYSTYPE-INFO created on 02/17/2006 at 10:38 */
#pragma fieldalign shared2 __zneo_ddl_systype_info
typedef struct __zneo_ddl_systype_info
{
   union
   {
      long long                       zsysinfo;
      struct
      {
         zspi_ddl_int_def                zplatformtype;
         zspi_ddl_int_def                zplatformusage;
         zspi_ddl_int_def                zreserved1;
         zspi_ddl_int_def                zreserved2;
      } zinfo;
   } u_zsysinfo;
} zneo_ddl_systype_info_def;
#define zneo_ddl_systype_info_def_Size 8
