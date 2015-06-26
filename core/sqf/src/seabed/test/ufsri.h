//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef __UFSRI_H_
#define __UFSRI_H_

//
// Utilities: Request info
//
typedef FS_Receiveinfo_Type RI_Type;

bool     do_printri = false;
short    ri_buf[sizeof(RI_Type)/sizeof(short)];

static void printri(RI_Type *ri);

void getri(RI_Type *ri) {
    int ferr;

    ferr = XFILE_GETRECEIVEINFO_(ri, NULL);
    assert(ferr == XZFIL_ERR_OK);
    if (do_printri)
        printri(ri);
}

void printri(RI_Type *ri) {
    long long  *sender;
    const char *iotype;

    sender = (long long *) TPT_REF(ri->sender);
    printf("ri\n");
    switch (ri->io_type) {
    case XZSYS_VAL_RCV_IOTYPE_WRITE:
        iotype = "write";
        break;
    case XZSYS_VAL_RCV_IOTYPE_READ:
        iotype = "read";
        break;
    case XZSYS_VAL_RCV_IOTYPE_WRITEREAD:
        iotype = "writeread";
        break;
    default:
        iotype = "?";
        break;
    }
    printf("  io_type......... %d (%s)\n", ri->io_type, iotype);
    printf("  max_reply_count. %d\n", ri->max_reply_count);
    printf("  message_tag..... %d\n", ri->message_tag);
    printf("  file_number..... %d\n", ri->file_number);
    printf("  sync_id......... %d\n", ri->sync_id);
    printf("  sender.......... %llx.%llx.%llx.%llx.%llx.%llx.%llx.%llx\n",
           sender[0], sender[1], sender[2], sender[3],
           sender[4], sender[5], sender[6], sender[7]);
    printf("  open_label...... %d\n", ri->open_label);
    printf("  user_id......... %d\n", ri->user_id);
}

#endif // !__UFSRI_H_
