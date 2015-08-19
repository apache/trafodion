#ifndef OPT_ERROR_H
#define OPT_ERROR_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         opt_error.h
 * Description:  Binder/normalizer/optimizer error codes
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
 *
 *****************************************************************************
 */

enum OptimizerSQLErrorCode
{
  BIND_CONTROL_QUERY_SUCCESSFUL                       =  4074,

  CATALOG_HISTOGRM_HISTINTS_TABLES_CONTAIN_BAD_VALUES = -6002,
  CATALOG_HISTOGRM_HISTINTS_TABLES_CONTAIN_BAD_VALUE  =  6003,
  CATALOG_HISTINTS_TABLES_CONTAIN_BAD_VALUES	        =  6004,
  MULTI_COLUMN_STATS_NEEDED                           =  6007,
  SINGLE_COLUMN_STATS_NEEDED                          =  6008,
  OSIM_ERRORORWARNING                                 =  6009,
  MULTI_COLUMN_STATS_NEEDED_AUTO                      =  6010,
  SINGLE_COLUMN_STATS_NEEDED_AUTO                     =  6011,
  SINGLE_COLUMN_SMALL_STATS                           =  6012,
  SINGLE_COLUMN_SMALL_STATS_AUTO                      =  6013,
  CMP_OPT_WARN_FROM_DCMPASSERT                        =  6021
};

#endif /* OPT_ERROR_H */
