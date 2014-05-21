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

#ifndef __SB_PTREE_INL_
#define __SB_PTREE_INL_

#include <string.h>

SB_Process_Tree::SB_Process_Tree() : SB_Gen_Tree<SB_Process_Node>() {
    iv_count = 0;
    ip_info = NULL;
}

SB_Process_Tree::~SB_Process_Tree() {
    int lv_inx;

    if (ip_root != NULL)
        delete ip_root;
    if (ipp_nodes != NULL) {
        for (lv_inx = 0; lv_inx < iv_count; lv_inx++)
            delete ipp_nodes[lv_inx];
        delete [] ipp_nodes;
    }
}

void SB_Process_Tree::traverse_print_node(FILE            *pp_f,
                                          SB_Process_Node *pp_node) {
    fprintf(pp_f,
            "+--%s, p-id=%d/%d, prog=%s\n",
            pp_node->ip_info->process_name,
            pp_node->ip_info->nid,
            pp_node->ip_info->pid,
            pp_node->ip_info->program);
}

void SB_Process_Tree::traverse_print_root(FILE            *pp_f,
                                          SB_Process_Node *pp_node) {
    pp_node = pp_node; // touch
    fprintf(pp_f, "monitor\n");
}

void SB_Process_Tree::tree_build(MS_Mon_Process_Info_Type *pp_info,
                                 int                       pv_count) {
    char             la_programt[MS_MON_MAX_PROCESS_PATH];
    SB_Process_Node *lp_node1;
    SB_Process_Node *lp_node2;
    char            *lp_program;
    int              lv_inx1;
    int              lv_inx2;

    iv_count = pv_count;
    ip_info = pp_info;

    // make nodes
    ip_root = new SB_Process_Node(0, NULL); // monitor node
    ipp_nodes = new SB_Process_Node *[pv_count];
    for (lv_inx1 = 0; lv_inx1 < pv_count; lv_inx1++)
        ipp_nodes[lv_inx1] = new SB_Process_Node(lv_inx1, &pp_info[lv_inx1]);

    // link nodes
    for (lv_inx1 = 0; lv_inx1 < pv_count; lv_inx1++) {
        lp_node1 = ipp_nodes[lv_inx1];
        lp_program = basename(lp_node1->ip_info->program);
        strcpy(la_programt, lp_program);
        strcpy(lp_node1->ip_info->program, la_programt);
    }
    for (lv_inx1 = 0; lv_inx1 < pv_count; lv_inx1++) {
        lp_node1 = ipp_nodes[lv_inx1];
        lp_program = lp_node1->ip_info->program;
        if (strcmp(lp_program, "shell") == 0)
            continue;
        if (strcmp(lp_program, "sp_wrapper") == 0)
            continue;
        for (lv_inx2 = 0; lv_inx2 < pv_count; lv_inx2++) {
            lp_node2 = ipp_nodes[lv_inx2];
            if (lv_inx2 == lv_inx1)
                continue;
            lp_program = lp_node2->ip_info->program;
            if (strcmp(lp_program, "shell") == 0)
                continue;
            if (strcmp(lp_program, "sp_wrapper") == 0)
                continue;
            if ((lp_node2->ip_info->parent_nid == lp_node1->ip_info->nid) &&
                (lp_node2->ip_info->parent_pid == lp_node1->ip_info->pid)) {
                set_child(lp_node1, lp_node2);
            }
        }
    }

    // link any parent-less node to monitor
    for (lv_inx1 = 0; lv_inx1 < pv_count; lv_inx1++) {
        lp_node1 = ipp_nodes[lv_inx1];
        if (lp_node1->ip_parent == NULL)
            set_child(ip_root, lp_node1);
    }

    // fix levels
    traverse_level(ip_root, 0);
}

SB_Process_Node::SB_Process_Node(int pv_inx, MS_Mon_Process_Info_Type *pp_info)
: SB_Gen_Tree_Node<SB_Process_Node>(),
  ip_info(pp_info), ip_next(NULL), iv_inx(pv_inx) {
}

SB_Process_Node::~SB_Process_Node() {
}

#endif // !__SB_PTREE_INL_
