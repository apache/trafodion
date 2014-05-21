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

//
// Implement tree
//

#ifndef __SB_PTREE_H_
#define __SB_PTREE_H_

#include "ttree.h"

#include "seabed/ms.h"

class SB_Process_Node : public SB_Gen_Tree_Node<SB_Process_Node> {
public:
    SB_Process_Node(int pv_inx, MS_Mon_Process_Info_Type *pp_info);
    virtual ~SB_Process_Node();

    MS_Mon_Process_Info_Type *ip_info;
    SB_Process_Node          *ip_next;
    int                       iv_inx;
};

class SB_Process_Tree : public SB_Gen_Tree<SB_Process_Node> {
public:
    SB_Process_Tree();
    virtual ~SB_Process_Tree();

    virtual void traverse_print_node(FILE *f, SB_Process_Node *pp_node);
    virtual void traverse_print_root(FILE *f, SB_Process_Node *pp_node);
    void         tree_build(MS_Mon_Process_Info_Type *pp_info, int pv_count);

    int                        iv_count;
    SB_Process_Node          **ipp_nodes;
    MS_Mon_Process_Info_Type  *ip_info;
};


#include "ptree.inl"

#endif // !__SB_PTREE_H_
