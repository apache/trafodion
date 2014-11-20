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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"

// forward
void print_spaces(int len);

void print_info_entry(int                          count,
                      MS_Mon_Node_Info_Entry_Type *info) {
    char hdr[50];
    int  len;
    int  node;

    printf("cluster-nodes=%d\n",
           count);
    for (node = 0; node < count; node++) {
        sprintf(hdr, "node[%d].", node);
        len = (int) strlen(hdr);
        printf("%snid=%d, state=%d, type=%d, processors=%d, procs=%d, pnid=%d, pstate=%d, spare=%d, cores=%d\n",
               hdr,
               info[node].nid,
               info[node].state,
               info[node].type,
               info[node].processors,
               info[node].process_count,
               info[node].pnid,
               info[node].pstate,
               info[node].spare_node,
               info[node]._fill);
        print_spaces(len);
        printf("memtotal=%d, memfree=%d, swapfree=%d, cachefree=%d, memactive=%d\n",
               info[node].memory_total,
               info[node].memory_free,
               info[node].swap_free,
               info[node].cache_free,
               info[node].memory_active);
        print_spaces(len);
        printf("meminactive=%d, memdirty=%d, memwriteback=%d, memvmallocuse=%d\n",
               info[node].memory_inactive,
               info[node].memory_dirty,
               info[node].memory_writeback,
               info[node].memory_vm_alloc_used);
        print_spaces(len);
        printf("cpuuser=%lld, cpunice=%lld, cpusystem=%lld, cpuidle=%lld, cpuiowait=%lld, cpuirq=%lld\n",
               info[node].cpu_user,
               info[node].cpu_nice,
               info[node].cpu_system,
               info[node].cpu_idle,
               info[node].cpu_iowait,
               info[node].cpu_irq);
        print_spaces(len);
        printf("cpusoftirq=%lld, btime=%d, name=%s\n",
               info[node].cpu_soft_irq,
               info[node].btime,
               info[node].node_name);
    }
}

void print_info_entry2(int                          count,
                       MS_Mon_Node_Info_Entry_Type *info,
                       int                          node_count,
                       int                          pnode_count,
                       int                          spares_count,
                       int                          available_spares_count) {

    char hdr[50];
    int  len;
    int  node;

    printf("cluster-nodes=%d, node-count=%d, pnode-count=%d, spares-count=%d, available-spares-count=%d\n",
           count, node_count, pnode_count, spares_count, available_spares_count);
    for (node = 0; node < count; node++) {
        sprintf(hdr, "node[%d].", node);
        len = (int) strlen(hdr);
        printf("%snid=%d, state=%d, type=%d, processors=%d, procs=%d, pnid=%d, pstate=%d, spare=%d, cores=%d\n",
               hdr,
               info[node].nid,
               info[node].state,
               info[node].type,
               info[node].processors,
               info[node].process_count,
               info[node].pnid,
               info[node].pstate,
               info[node].spare_node,
               info[node]._fill);
        print_spaces(len);
        printf("memtotal=%d, memfree=%d, swapfree=%d, cachefree=%d, memactive=%d\n",
               info[node].memory_total,
               info[node].memory_free,
               info[node].swap_free,
               info[node].cache_free,
               info[node].memory_active);
        print_spaces(len);
        printf("meminactive=%d, memdirty=%d, memwriteback=%d, memvmallocuse=%d\n",
               info[node].memory_inactive,
               info[node].memory_dirty,
               info[node].memory_writeback,
               info[node].memory_vm_alloc_used);
        print_spaces(len);
        printf("cpuuser=%lld, cpunice=%lld, cpusystem=%lld, cpuidle=%lld, cpuiowait=%lld, cpuirq=%lld\n",
               info[node].cpu_user,
               info[node].cpu_nice,
               info[node].cpu_system,
               info[node].cpu_idle,
               info[node].cpu_iowait,
               info[node].cpu_irq);
        print_spaces(len);
        printf("cpusoftirq=%lld, btime=%d, name=%s\n",
               info[node].cpu_soft_irq,
               info[node].btime,
               info[node].node_name);
    }
}


void print_info(MS_Mon_Node_Info_Type *info) {
    char hdr[50];
    int  len;
    int  node;

    printf("cluster-nodes=%d, returned-nodes=%d\n",
           info->num_nodes,
           info->num_returned);
    for (node = 0; node < info->num_returned; node++) {
        sprintf(hdr, "node[%d].", node);
        len = (int) strlen(hdr);
        printf("%snid=%d, state=%d, type=%d, processors=%d, procs=%d, pnid=%d, pstate=%d, spare=%d, cores=%d\n",
               hdr,
               info->node[node].nid,
               info->node[node].state,
               info->node[node].type,
               info->node[node].processors,
               info->node[node].process_count,
               info->node[node].pnid,
               info->node[node].pstate,
               info->node[node].spare_node,
               info->node[node]._fill);
        print_spaces(len);
        printf("memtotal=%d, memfree=%d, swapfree=%d, cachefree=%d, memactive=%d\n",
               info->node[node].memory_total,
               info->node[node].memory_free,
               info->node[node].swap_free,
               info->node[node].cache_free,
               info->node[node].memory_active);
        print_spaces(len);
        printf("meminactive=%d, memdirty=%d, memwriteback=%d, memvmallocuse=%d\n",
               info->node[node].memory_inactive,
               info->node[node].memory_dirty,
               info->node[node].memory_writeback,
               info->node[node].memory_vm_alloc_used);
        print_spaces(len);
        printf("cpuuser=%lld, cpunice=%lld, cpusystem=%lld, cpuidle=%lld, cpuiowait=%lld, cpuirq=%lld\n",
               info->node[node].cpu_user,
               info->node[node].cpu_nice,
               info->node[node].cpu_system,
               info->node[node].cpu_idle,
               info->node[node].cpu_iowait,
               info->node[node].cpu_irq);
        print_spaces(len);
        printf("cpusoftirq=%lld, btime=%d, name=%s\n",
               info->node[node].cpu_soft_irq,
               info->node[node].btime,
               info->node[node].node_name);
    }
}

void print_spaces(int len) {
    int inx;

    for (inx = 0; inx < len; inx++)
        printf(" ");
}

int main(int argc, char *argv[]) {
    int                   ferr;
    MS_Mon_Node_Info_Type info;

    msfs_util_init(&argc, &argv, msg_debug_hook);
    util_test_start(true);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    int available_spares_count2 = 0;
    int count = 0;
    int count2 = 0;
    int node_count2 = 0;
    int node_info_max = 0;
    int node_info_max2 = 0;
    int pnode_count2 = 0;
    int spares_count2 = 0;
    MS_Mon_Node_Info_Entry_Type *node_info = NULL;
    MS_Mon_Node_Info_Entry_Type *node_info2 = NULL;

    ferr = msg_mon_get_node_info(&count, 0, NULL);
    TEST_CHK_FEOK(ferr);
    node_info_max = count;
    node_info = (MS_Mon_Node_Info_Entry_Type *)malloc(count * sizeof(MS_Mon_Node_Info_Entry_Type));
    ferr = msg_mon_get_node_info(&count, node_info_max, node_info);
    TEST_CHK_FEOK(ferr);
    print_info_entry(count, node_info);
    free(node_info);

    ferr = msg_mon_get_node_info2(&count2, 0, NULL, &node_count2, &pnode_count2, &spares_count2, &available_spares_count2);
    TEST_CHK_FEOK(ferr);
    node_info_max2 = count2;
    node_info2 = (MS_Mon_Node_Info_Entry_Type *)malloc(count2 * sizeof(MS_Mon_Node_Info_Entry_Type));
    ferr = msg_mon_get_node_info2(&count2, node_info_max2, node_info2, &node_count2, &pnode_count2, &spares_count2, &available_spares_count2);
    TEST_CHK_FEOK(ferr);
    print_info_entry2(count2, node_info2, node_count2, pnode_count2, spares_count2, available_spares_count2);
    free(node_info2);

    ferr = msg_mon_get_node_info_all(&info);
    TEST_CHK_FEOK(ferr);
    print_info(&info);
    ferr = msg_mon_get_node_info_detail(0, &info);
    TEST_CHK_FEOK(ferr);
    print_info(&info);
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
