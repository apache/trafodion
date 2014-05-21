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

#ifndef __SB_TTREE_H_
#define __SB_TTREE_H_

#include "seabed/int/opts.h"

#include <stdio.h>

template <class T>
class SB_Gen_Tree_Node {
public:
    SB_Gen_Tree_Node();
    virtual ~SB_Gen_Tree_Node();

    T   *ip_child;
    T   *ip_parent;
    T   *ip_sibling;
    int  iv_level;
};

template <class T>
class SB_Gen_Tree {
public:
    SB_Gen_Tree();
    virtual ~SB_Gen_Tree();

    virtual bool is_child(T *pp_parent, T *pp_child);
    virtual void set_child(T *pp_parent, T *pp_child);
    virtual void tree_print(FILE *pp_f);

    T *ip_root;

protected:
    void         traverse_level(T *pp_node, int pv_level);
    void         traverse_print(FILE *pp_f, T *pp_node);
    virtual void traverse_print_node(FILE *pp_f, T *pp_node);
    virtual void traverse_print_root(FILE *pp_f, T *pp_node);
};

#include "ttree.inl"

#endif // !__SB_TTREE_H_
