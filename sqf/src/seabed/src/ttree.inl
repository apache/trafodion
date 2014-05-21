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

#ifndef __SB_TTREE_INL_
#define __SB_TTREE_INL_

template <class T>
SB_Gen_Tree<T>::SB_Gen_Tree() : ip_root(NULL) {
}

template <class T>
SB_Gen_Tree<T>::~SB_Gen_Tree() {
}

template <class T>
bool SB_Gen_Tree<T>::is_child(T *pp_parent, T *pp_child) {
    T    *lp_node;
    bool  lv_ret;

    if (pp_parent->ip_child == NULL)
        lv_ret = false;
    else if (pp_parent->ip_child == pp_child)
        lv_ret = true;
    else {
        lp_node = pp_parent->ip_child->ip_sibling;
        lv_ret = false;
        while (lp_node != NULL) {
            if (lp_node == pp_child) {
                lv_ret = true;
                break;
            }
            lp_node = lp_node->ip_sibling;
        }
    }

    return lv_ret;
}

template <class T>
void SB_Gen_Tree<T>::set_child(T *pp_parent, T *pp_child) {
    T    *lp_node;
    bool  lv_cycle;

    lv_cycle = false;
    if (is_child(pp_child, pp_parent)) {
        // don't create cycle
        lv_cycle = true;
    } else if (pp_parent->ip_child == NULL) {
        pp_parent->ip_child = pp_child;
    } else if (pp_parent->ip_child->ip_sibling == NULL) {
        pp_parent->ip_child->ip_sibling = pp_child;
    } else {
        lp_node = pp_parent->ip_child->ip_sibling;
        while (lp_node->ip_sibling != NULL)
            lp_node = lp_node->ip_sibling;
        lp_node->ip_sibling = pp_child;
    }
    if (!lv_cycle)
        pp_child->ip_parent = pp_parent;
}

template <class T>
void SB_Gen_Tree<T>::traverse_level(T *pp_node, int pv_level) {
    pp_node->iv_level = pv_level;
    if (pp_node->ip_child != NULL)
        traverse_level(pp_node->ip_child, pv_level + 1);
    if (pp_node->ip_sibling != NULL)
        traverse_level(pp_node->ip_sibling, pv_level);
}

template <class T>
void SB_Gen_Tree<T>::traverse_print(FILE *pp_f, T *pp_node) {
    T   *lp_sibling;
    int  lv_inx;
    int  lv_inx2;
    int  lv_level;

    lv_level = pp_node->iv_level;
    if (lv_level == 0) {
        traverse_print_root(pp_f, pp_node);
    } else {
        for (lv_inx = 1; lv_inx < lv_level; lv_inx++) {
            lp_sibling = pp_node;
            for (lv_inx2 = lv_inx; lv_inx2 < lv_level; lv_inx2++)
                lp_sibling = lp_sibling->ip_parent;
            if (lp_sibling->ip_sibling == NULL)
                fprintf(pp_f, " ");
            else
                fprintf(pp_f, "|");
            fprintf(pp_f, " ");
        }
        traverse_print_node(pp_f, pp_node);
    }
    if (pp_node->ip_child != NULL)
        traverse_print(pp_f, pp_node->ip_child);
    if (pp_node->ip_sibling != NULL)
        traverse_print(pp_f, pp_node->ip_sibling);
}

template <class T>
void SB_Gen_Tree<T>::traverse_print_node(FILE *pp_f, T *pp_node) {
    fprintf(pp_f, "+--<node>=%p\n", static_cast<void *>(pp_node));
}

template <class T>
void SB_Gen_Tree<T>::traverse_print_root(FILE *pp_f, T *pp_node) {
    fprintf(pp_f, "<root>=%p\n", static_cast<void *>(pp_node));
}

template <class T>
void SB_Gen_Tree<T>::tree_print(FILE *pp_f) {
    traverse_print(pp_f, ip_root);
}

template <class T>
SB_Gen_Tree_Node<T>::SB_Gen_Tree_Node()
: ip_child(NULL), ip_parent(NULL), ip_sibling(NULL), iv_level(0) {
}

template <class T>
SB_Gen_Tree_Node<T>::~SB_Gen_Tree_Node() {
}

#endif // !__SB_TTREE_INL_
