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

#include "tmaudit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dtm/tm_util.h"
#include "dtm/tmtransid.h"
//#include "tmtx.h"

// read audit trail
int main ()
{
    FILE *lp_audit_file = fopen ("tm_audit", "r");
    char la_audit_buf[REC_SIZE];
    int lv_num_cp = 0;
    TM_Transid lv_transid;

    if (!lp_audit_file)
    {
        printf ("\n No Audit file to read.  Exiting.\n"); 
        return 1;
    }

    while (true)
    {
        Audit_Header *lv_header;
        int64 lv_num_bytes = fread (la_audit_buf, 1, REC_SIZE, lp_audit_file);
        if (!lv_num_bytes)
            break;

        lv_header = (Audit_Header *)la_audit_buf;
        lv_transid = lv_header->iv_transid;
        printf("\nTransid (%d,%d,%d) :", 
           lv_transid.get_node(), lv_transid.get_seq_num(), lv_transid.get_incarnation_num());
        printf(" Audit record type : (%d)", lv_header->iv_type);
        switch (lv_header->iv_type)
        {
        case TM_Transaction_State:
        {
            printf("Transaction State record");
            Audit_Transaction_State *lv_audit_rec;
            lv_audit_rec = (Audit_Transaction_State *)la_audit_buf;
            printf ("\nAudit record state ");
            printf("(DTM generated) : ");
            switch (lv_audit_rec->iv_state) {

               case Active_Trans_State:
                printf ("ACTIVE ");
                break;

               case Committed_Trans_State:
                printf ("COMMITTED ");
                break;

               case HungCommitted_Trans_State:
                printf ("HUNG COMMITTED ");
                break;

               case Aborting_Trans_State:
                printf ("ABORTING ");
                break;

               case Aborted_Trans_State:
                printf ("ABORTED ");
                break;

               case HungAborted_Trans_State:
                printf ("HUNG ABORTED ");
                break;

               case Forgotten_Trans_State:
                printf ("FORGOTTEN ");
                break;
               default:
                printf ("UNKNOWN ");
                break;
            }

            if ((lv_transid.get_type_flags() & TM_TT_NO_UNDO) == TM_TT_NO_UNDO)
               printf("NO-UNDO ");
            if ((lv_transid.get_type_flags() & TM_TT_FORCE_CONSISTENCY) == TM_TT_FORCE_CONSISTENCY)
               printf("FORCE CONSISTENCY ");

            TM_Txid_Internal *lv_txid = (TM_Txid_Internal *)
                                      &lv_audit_rec->iv_hdr.iv_transid;
            printf ("\n    [nid]    [seq number]    [Version]    [Checksum]\n");        
            printf ("    [%d]    [%d]        [%d]        [%d]\n\n",lv_txid->iv_node, 
              lv_txid->iv_seqnum, lv_txid->iv_version, lv_txid->iv_check_sum);

            break;
       }
       case TM_Control_Point :
       {
           lv_num_cp++;
           printf("Control Point\n");
           break;
       }
       case TM_Shutdown :
      {
          printf("Shutdown Record\n");
          break;
      }
    }  
    }
    printf("\nHit %d Control Points\n", lv_num_cp);
}
