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

/*
* Start of xa.h header
**
This header is reproduced from the X/Open XA specification.
*/

#ifndef XA_H
#define XA_H

#if __WORDSIZE == 64
  #define int64 long
#else
  #define int64 long long
#endif

/*
* Transaction branch identification: XID and NULLXID:
*/

#define XIDDATASIZE 128 /* size in bytes */
#define MAXGTRIDSIZE 64 /* maximum size in bytes of gtrid */
#define MAXBQUALSIZE 64 /* maximum size in bytes of bqual */

struct xid_t {
int formatID; /* format identifier */
int gtrid_length; /* value from 1 through 64 */
int bqual_length; /* value from 1 through 64 */
char data[XIDDATASIZE];
};
typedef struct xid_t XID;

/*
* A value of -1 in formatID means that the XID is null.
*/

/*
* Declarations of routines by which RMs call TMs:
*/
#ifdef __STDC__
extern int ax_reg(int, XID *, int64);
extern int ax_unreg(int, int64);
#else /* ifndef __STDC__ */
extern int ax_reg();
extern int ax_unreg();
#endif /* ifndef __STDC__ */

/*
* XA Switch Data Structure
*/

#define RMNAMESZ 32 /* length of resource manager name, */

/* including the null terminator */

#define MAXINFOSIZE 256 /* maximum size in bytes of xa_info */

/* strings, including the null terminator*/
struct xa_switch_t {
char name[RMNAMESZ]; /* name of resource manager */
int64 flags; /* options specific to the resource manager */
int64 version; /* must be 0 */

#ifdef __STDC__
int (*xa_open_entry)(char *, int, int64); /* xa_open function pointer*/
int (*xa_close_entry)(char *, int, int64); /* xa_close function pointer*/
int (*xa_start_entry)(XID *, int, int64); /* xa_start function pointer*/
int (*xa_end_entry)(XID *, int, int64); /* xa_end function pointer*/
int (*xa_rollback_entry)(XID *, int, int64);

/* xa_rollback function pointer*/
int (*xa_prepare_entry)(XID *, int, int64);/* xa_prepare function pointer*/
int (*xa_commit_entry)(XID *, int, int64); /* xa_commit function pointer*/
int (*xa_recover_entry)(XID *, int64, int, int64);

/* xa_recover function pointer*/
int (*xa_forget_entry)(XID *, int, int64); /* xa_forget function pointer*/
int (*xa_complete_entry)(int *, int *, int, int64);

/* xa_complete function pointer*/

#else /* ifndef __STDC__ */
int (*xa_open_entry)(); /* xa_open function pointer */
int (*xa_close_entry)(); /* xa_close function pointer */
int (*xa_start_entry)(); /* xa_start function pointer */
int (*xa_end_entry)(); /* xa_end function pointer */
int (*xa_rollback_entry)(); /* xa_rollback function pointer*/
int (*xa_prepare_entry)(); /* xa_prepare function pointer */
int (*xa_commit_entry)(); /* xa_commit function pointer */
int (*xa_recover_entry)(); /* xa_recover function pointer */
int (*xa_forget_entry)(); /* xa_forget function pointer */
int (*xa_complete_entry)(); /* xa_complete function pointer*/
#endif /* ifndef __STDC__ */

};

/*
* Flag definitions for the RM switch
*/
#define TMNOFLAGS 0x00000000LL /* no resource manager features
            selected */
#define TMREGISTER 0x00000001LL /* resource manager dynamically
            registers */
 #define TMUSEASYNC 0x00000004LL /* resource manager supports
            asynchronous operations */

/*
* Flag definitions for xa_ and ax_ routines
*/

/* use TMNOFLAGS, defined above, when not specifying other flags */
#define TMASYNC 0x80000000LL /* perform routine asynchronously */
#define TMONEPHASE 0x40000000LL /* caller is using one-phase commit
            optimisation */
#define TMFAIL 0x20000000LL /* dissociates caller and marks
            transaction branch rollback-only */
#define TMNOWAIT 0x10000000LL /* return if blocking condition exists */
#define TMRESUME 0x08000000LL /* caller is resuming association
            with suspended transaction branch */
#define TMSUCCESS 0x04000000LL /* dissociate caller from transaction
            branch*/
#define TMSUSPEND 0x02000000LL /* caller is suspending, not ending,
            association */
#define TMSTARTRSCAN  0x01000000LL /* start a recovery scan */
#define TMENDRSCAN    0x00800000LL /* end a recovery scan */
#define TMRESENDRSCAN 0x00010000LL
#define TMRMFAILRSCAN 0x00001000LL
#define TMMULTIPLE 0x00400000LL /* wait for any asynchronous operation */
#define TMJOIN 0x00200000LL /* caller is joining existing transaction
            branch */
#define TMMIGRATE 0x00100000LL /* caller intends to perform migration */

/* Seaquest specific!! These can only be used between TM and TSE. */
#define TMNOUNDO  0x00000002LL /* Do not undo transaction on rollback */

/*
* ax_() return codes (transaction manager reports to resource manager)
*/
#define TM_JOIN 2 /* caller is joining existing transaction
         branch */
#define TM_RESUME 1 /* caller is resuming association with
         suspended transaction branch */
#define TM_OK 0 /* normal execution */
#define TMER_TMERR -1 /* an error occurred in the transaction
         manager */
#define TMER_INVAL -2 /* invalid arguments were given */
#define TMER_PROTO -3 /* routine invoked in an improper context */

/*
* xa_() return codes (resource manager reports to transaction manager)
*/
#define XA_RBBASE 100 /* the inclusive lower bound of the
         rollback codes */
#define XA_RBROLLBACK XA_RBBASE /* the rollback was caused by an
         unspecified reason */
#define XA_RBCOMMFAIL XA_RBBASE+1 /* the rollback was caused by a
         communication failure */
#define XA_RBDEADLOCK XA_RBBASE+2 /* a deadlock was detected */
#define XA_RBINTEGRITY XA_RBBASE+3 /* a condition that violates the
         integrity of the resources was detected */
#define XA_RBOTHER XA_RBBASE+4 /* the resource manager rolled back
         the transaction branch for
         a reason not on this list */
#define XA_RBPROTO XA_RBBASE+5 /* a protocol error occurred in the
         resource manager */
#define XA_RBTIMEOUT XA_RBBASE+6 /* a transaction branch took too
         long */
#define XA_RBTRANSIENT XA_RBBASE+7 /* may retry the transaction branch */
#define XA_RBEND XA_RBTRANSIENT /* the inclusive upper bound of the
         rollback codes */
#define XA_NOMIGRATE 9 /* resumption must occur where
         suspension occurred */
#define XA_HEURHAZ 8 /* the transaction branch may have
         been heuristically completed */
#define XA_HEURCOM 7 /* the transaction branch has been
         heuristically committed */
#define XA_HEURRB 6 /* the transaction branch has been
         heuristically rolled back */
#define XA_HEURMIX 5 /* the transaction branch has been
         heuristically committed and rolled back */
#define XA_RETRY 4 /* routine returned with no effect and
         may be reissued */
#define XA_RDONLY 3 /* the transaction branch was read-only and
         has been committed */
#define XA_OK 0 /* normal execution */
#define XAER_ASYNC -2 /* asynchronous operation already outstanding */
#define XAER_RMERR -3 /* a resource manager error occurred in the
         transaction branch */
#define XAER_NOTA -4 /* the XID is not valid */
#define XAER_INVAL -5 /* invalid arguments were given */
#define XAER_PROTO -6 /* routine invoked in an improper context */
#define XAER_RMFAIL -7 /* resource manager unavailable */
#define XAER_DUPID -8 /* the XID already exists */
#define XAER_OUTSIDE -9 /* resource manager doing work outside */

/* global transaction */
#endif /* ifndef XA_H */

/*
* End of xa.h header
*/
