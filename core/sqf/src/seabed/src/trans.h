//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2015 Hewlett-Packard Development Company, L.P.
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

#ifndef __SB_TRANS_H_
#define __SB_TRANS_H_

#define TRANSID_COPY(dest,src) { \
  dest.id[0] = src.id[0]; \
  dest.id[1] = src.id[1]; \
  dest.id[2] = src.id[2]; \
  dest.id[3] = src.id[3]; \
}
#define TRANSID_COPY_MON(dest,src,destid,srcid) { \
  dest.destid[0] = src.srcid[0]; \
  dest.destid[1] = src.srcid[1]; \
  dest.destid[2] = src.srcid[2]; \
  dest.destid[3] = src.srcid[3]; \
}
#define TRANSID_COPY_MON_FROM(dest,src) TRANSID_COPY_MON(dest,src,id,txid)
#define TRANSID_COPY_MON_TO(dest,src) TRANSID_COPY_MON(dest,src,txid,id)
#define TRANSID_IS_INVALID(idin) (idin.id[0] == -1)
#define TRANSID_IS_VALID(idin) (idin.id[0] != 0)
#define TRANSID_SET_INVALID(id) TRANSID_SET_VALUE(id, -1)
#define TRANSID_SET_NULL(id) TRANSID_SET_VALUE(id, 0)
#define TRANSID_SET_VALUE(idin,val) { \
  idin.id[0] = val; \
  idin.id[1] = val; \
  idin.id[2] = val; \
  idin.id[3] = val; \
}

#define TRANSSEQ_COPY(dest,src) { \
  dest = src; \
}
#define TRANSSEQ_SET_VALUE(idin,val) { \
  idin = val; \
}
#define TRANSSEQ_SET_NULL(id) TRANSSEQ_SET_VALUE(id, 0)


#endif // !__SB_TRANS_H_
