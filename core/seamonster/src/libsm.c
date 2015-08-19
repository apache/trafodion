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

#include "sm.h"

int
SM_init(int channel, int node, int verifier)
{
    return SM_ERR_INTERNAL;
}

int
SM_finalize(int channel)
{
    return SM_ERR_INTERNAL;
}

int
SM_register(int channel, sm_id_t id)
{
    return SM_ERR_INTERNAL;
}

int
SM_cancel(int channel, sm_id_t id)
{
    return SM_ERR_INTERNAL;
}

int
SM_put(int channel, int nchunks, sm_chunk_t *chunks)
{
    return SM_ERR_INTERNAL;
}

int
SM_get(int channel, sm_chunk_t *chunks[], int *nchunks, sm_handle_t *handle)
{
    return SM_ERR_INTERNAL;
}

int
SM_get_done(int channel, sm_handle_t handle)
{
    return SM_ERR_INTERNAL;
}

int
SM_ctl(int channel, int cmd, void *ptr)
{
    return SM_ERR_INTERNAL;
}

int
SM_strerror_r(int errnum, char *buf, size_t buflen)
{
    buf[0] = 0;
    return 0;
}

int
SM_get_stats(pid_t spid, int fl_clear)
{
    return 0;
}
