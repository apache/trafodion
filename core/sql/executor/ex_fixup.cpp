/* -*-C++-*-
 *****************************************************************************
 *
 * File:         <file>
 * Description:  
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
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
 *
 *****************************************************************************
 */

#include "ExCollections.h"
#include "exp_stdh.h"
#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_root.h"
#include "ex_onlj.h"
#include "ex_update.h"
#include "ex_delete.h"
#include "ex_union.h"
#include "ex_tuple.h"
#include "ex_hash_grby.h"
#include "ex_sort_grby.h"
#include "ex_split_top.h"
#include "ex_split_bottom.h"
#include "ex_send_top.h"
#include "ex_send_bottom.h"
#include "ex_part_input.h"
#include "ex_hashj.h"
#include "ex_mj.h"
#include "ex_dp2exe_root.h"
#include "ex_partn_access.h"

Int32 ex_tdb::fixup(Lng32 /*base*/)
{ 
  return 0;
};


Int32 ex_root_tdb::fixup(Lng32 /*base*/)
{
  return 0;
};

Int32 ex_onlj_tdb::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ex_hashj_tdb::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ex_mj_tdb::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ex_update_tdb::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ex_delete_tdb::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ex_union_tdb::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ex_tuple_tdb::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ex_hash_grby_tdb::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ex_sort_grby_tdb::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ex_split_top_tdb::fixup(Lng32 base)
{
  if (partInputDataDesc_)
    return partInputDataDesc_->fixup(base);
  return 0;
}

Int32 ex_split_bottom_tdb::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ex_send_top_tdb::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ex_send_bottom_tdb::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ex_dp2exe_root_tdb::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ex_partn_access_tdb::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ExPartInputDataDesc::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ExHashPartInputData::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ExRoundRobinPartInputData::fixup(Lng32 /*base*/)
{
  return 0;
}

Int32 ExRangePartInputData::fixup(Lng32 /*base*/)
{
  return 0;
}

