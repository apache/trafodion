///////////////////////////////////////////////////////////////////////////////
//
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
//
///////////////////////////////////////////////////////////////////////////////

using namespace std;

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "props.h"

//
// Implement string map
//
MON_Smap::MON_Smap() : iv_count(0) {
    for (int lv_hash = 0; lv_hash <= TABLE_SIZE; lv_hash++)
        iv_HT[lv_hash] = NULL;
}

MON_Smap::~MON_Smap() {
    if (iv_count != 0) {
        for (int lv_hash = 0; lv_hash < TABLE_SIZE; lv_hash++) {
            while (iv_HT[lv_hash] != NULL) {
                SML_Type *lp_item = iv_HT[lv_hash];
                SML_Type *lp_next = (SML_Type *) lp_item->iv_link.ip_next;
                delete [] lp_item->ip_key;
                delete [] lp_item->ip_value;
                delete lp_item;
                iv_HT[lv_hash] = lp_next;
                iv_count--;
            }
            if (iv_count == 0)
                break;
        }
    }
}

bool MON_Smap::empty() {
    return (iv_count == 0);
}

const char *MON_Smap::get(const char *pp_key) {
    assert(pp_key != NULL);
    int lv_hash = hash(pp_key);
    SML_Type *lp_item = iv_HT[lv_hash];
    while (lp_item != NULL) {
        if (strcmp(pp_key, lp_item->ip_key) == 0)
            return lp_item->ip_value;
        lp_item = (SML_Type *) lp_item->iv_link.ip_next;
    }
    return NULL;
}

void *MON_Smap::getv(const char *pp_key) {
    assert(pp_key != NULL);
    int lv_hash = hash(pp_key);
    SML_Type *lp_item = iv_HT[lv_hash];
    while (lp_item != NULL) {
        if (strcmp(pp_key, lp_item->ip_key) == 0)
            return lp_item->ip_vvalue;
        lp_item = (SML_Type *) lp_item->iv_link.ip_next;
    }
    return NULL;
}

int MON_Smap::hash(const char *pp_key) {
    int lv_len = (int) strlen(pp_key);
    int lv_hash = lv_len;
    for (int lv_inx = 0; lv_inx < lv_len; lv_inx++)
        lv_hash += pp_key[lv_inx];
    lv_hash = ((unsigned int) lv_hash % TABLE_SIZE);
    return lv_hash;
}

MON_Smap_Enum *MON_Smap::keys() {
    return new MON_Smap_Enum(this);
}

void MON_Smap::printself(bool pv_traverse) {
    printf("this=%p, size=%d\n", (void *) this, iv_count);
    if (pv_traverse) {
        int inx = 0;
        for (int lv_hash = 0; lv_hash < TABLE_SIZE; lv_hash++) {
            SML_Type *lp_item = iv_HT[lv_hash];
            for (;
                 lp_item != NULL;
                 lp_item = (SML_Type *) lp_item->iv_link.ip_next) {
                if (lp_item->iv_use_vvalue)
                    printf("  inx=%d, hash=%d, key='%s', value=%p\n",
                           inx, lv_hash, lp_item->ip_key, lp_item->ip_vvalue);
                else
                    printf("  inx=%d, hash=%d, key='%s', value='%s'\n",
                           inx, lv_hash, lp_item->ip_key, lp_item->ip_value);
                inx++;
            }
        }
    }
}

void MON_Smap::put(const char *pp_key, const char *pp_value) {
    assert(pp_key != NULL);
    assert(pp_value != NULL);
    int lv_hash = hash(pp_key);
    SML_Type *lp_item = new SML_Type;
    lp_item->iv_link.ip_next = (MON_QL_Type *) iv_HT[lv_hash];
    int lv_len = (int) strlen(pp_key);
    lp_item->ip_key = new char[lv_len+1];
    strcpy(lp_item->ip_key, pp_key);
    lv_len = (int) strlen(pp_value);
    lp_item->ip_value = new char[lv_len+1];
    lp_item->iv_use_vvalue = false;
    strcpy(lp_item->ip_value, pp_value);
    iv_HT[lv_hash] = lp_item;
    iv_count++;
}

void MON_Smap::putv(const char *pp_key, void *pp_value) {
    assert(pp_key != NULL);
    assert(pp_value != NULL);
    int lv_hash = hash(pp_key);
    SML_Type *lp_item = new SML_Type;
    lp_item->iv_link.ip_next = (MON_QL_Type *) iv_HT[lv_hash];
    int lv_len = (int) strlen(pp_key);
    lp_item->ip_key = new char[lv_len+1];
    strcpy(lp_item->ip_key, pp_key);
    lp_item->ip_vvalue = pp_value;
    lp_item->iv_use_vvalue = true;
    iv_HT[lv_hash] = lp_item;
    iv_count++;
}

void MON_Smap::remove(const char *pp_key, char *pp_value) {
    assert(pp_key != NULL);
    int lv_hash = hash(pp_key);
    SML_Type *lp_item = iv_HT[lv_hash];
    SML_Type *lp_prev = NULL;
    while (lp_item != NULL) {
        if (strcmp(pp_key, lp_item->ip_key) == 0) {
            if (lp_prev == NULL)
                iv_HT[lv_hash] = (SML_Type *) lp_item->iv_link.ip_next;
            else
                lp_prev->iv_link.ip_next = lp_item->iv_link.ip_next;
            iv_count--;
            strcpy(pp_value, lp_item->ip_value);
            delete [] lp_item->ip_key;
            delete [] lp_item->ip_value;
            delete lp_item;
            break;
        }
        lp_prev = lp_item;
        lp_item = (SML_Type *) lp_item->iv_link.ip_next;
    }
}

void *MON_Smap::removev(const char *pp_key) {
    assert(pp_key != NULL);
    int lv_hash = hash(pp_key);
    SML_Type *lp_item = iv_HT[lv_hash];
    SML_Type *lp_prev = NULL;
    while (lp_item != NULL) {
        if (strcmp(pp_key, lp_item->ip_key) == 0) {
            if (lp_prev == NULL)
                iv_HT[lv_hash] = (SML_Type *) lp_item->iv_link.ip_next;
            else
                lp_prev->iv_link.ip_next = lp_item->iv_link.ip_next;
            iv_count--;
            void *lp_ret = lp_item->ip_vvalue;
            delete [] lp_item->ip_key;
            delete lp_item;
            return lp_ret;
        }
        lp_prev = lp_item;
        lp_item = (SML_Type *) lp_item->iv_link.ip_next;
    }
    return NULL;
}

int MON_Smap::size() {
    return iv_count;
}

MON_Smap_Enum::MON_Smap_Enum(MON_Smap *pp_map)
: ip_item(pp_map->iv_HT[0]), ip_map(pp_map), iv_hash(0) {
}

MON_Smap_Enum::~MON_Smap_Enum() {
}

bool MON_Smap_Enum::more() {
    MON_Smap::SML_Type *lp_item = ip_item;
    int lv_hash = iv_hash;
    for (; lv_hash < MON_Smap::TABLE_SIZE; lv_hash++) {
        if (lp_item != NULL)
            break;
        lp_item = ip_map->iv_HT[lv_hash+1];
    }
    ip_item = lp_item;
    iv_hash = lv_hash;
    return (lp_item != NULL);
}

char *MON_Smap_Enum::next() {
    MON_Smap::SML_Type *lp_item = ip_item;
    int lv_hash = iv_hash;
    for (; lv_hash < MON_Smap::TABLE_SIZE; lv_hash++) {
        if (lp_item != NULL)
            break;
        lp_item = ip_map->iv_HT[lv_hash+1];
    }
    ip_item = lp_item;
    iv_hash = lv_hash;
    if (lp_item != NULL) {
        ip_item = (MON_Smap::SML_Type *) lp_item->iv_link.ip_next;
        return lp_item->ip_key;
    }
    return NULL;
}
//
// Implement properties
//
MON_Props::MON_Props() : iv_getenv(false) {
}

MON_Props::MON_Props(bool pv_getenv) : iv_getenv(pv_getenv) {
}

MON_Props::~MON_Props() {
}

bool MON_Props::load(const char *pp_file) {
    bool lv_lret = false;
    if (pp_file == NULL)
        return lv_lret;
    FILE *lp_file = fopen(pp_file, "r");
    if (lp_file != NULL) {
        char la_line[BUFSIZ];
        while (fgets(la_line, sizeof(la_line), lp_file)) {
            char *lp_key = la_line;
            int lv_len = (int) strlen(la_line);
            char *lp_equals = NULL;
            for (int lv_inx = 0; lv_inx < lv_len; lv_inx++) {
                if (la_line[lv_inx] == '=') {
                    lp_equals = &la_line[lv_inx];
                    break;
                }
            }
            if (lp_equals == NULL)
                continue;
            while (isspace(*lp_key))
                lp_key++;
            if (*lp_key == '#')
                continue;  // comment
            char *lp_end_key = &lp_equals[-1];
            while ((lp_end_key >= lp_key) && isspace(*lp_end_key))
                lp_end_key--;
            if (lp_end_key < lp_key)
                continue;  // no key
            char *lp_value = &lp_equals[1];
            if (repl_var(lp_value))
                lv_len = (int) strlen(la_line);
            while ((*lp_value) && (isspace(*lp_value)))
                lp_value++;
            char *lp_end_value = &la_line[lv_len-1];
            while ((lp_end_value >= lp_value) && isspace(*lp_end_value))
                lp_end_value--;
            if (lp_end_value < lp_value)
                continue;  // no value
            lp_end_key[1] = '\0';
            lp_end_value[1] = '\0';
            put(lp_key, lp_value);
        }
        lv_lret = true;
        fclose(lp_file);
    }
    return lv_lret;
}

bool MON_Props::repl_var(char *pp_value) {
    bool lv_ret;

    lv_ret = false;
    while (*pp_value) {
        if (*pp_value == '$') {
            char *pp_end_value = &pp_value[1];
            while (isalnum(*pp_end_value) || (*pp_end_value == '_'))
                pp_end_value++;
            char lv_save = *pp_end_value;
            *pp_end_value = '\0';
            int lv_var_len = (int) strlen(pp_value);
            const char *lp_var = get(&pp_value[1]);
            if ((lp_var == NULL) && iv_getenv)
                lp_var = getenv(&pp_value[1]);
            *pp_end_value = lv_save;
            if (lp_var == NULL)
                lp_var = "";
            int lv_var_value_len = (int) strlen(lp_var);
            if (lv_var_value_len > 0)
                lv_ret = true;
            if (lv_var_value_len <= lv_var_len) {
                // variable fits: fill in var, move rest-of-line in after var
                memcpy(pp_value, lp_var, lv_var_value_len);
                strcpy(&pp_value[lv_var_value_len], pp_end_value);
            } else {
                // variable does not fit: make room and fill in var
                memmove(&pp_value[lv_var_value_len], pp_end_value, strlen(pp_end_value) + 1);
                memcpy(pp_value, lp_var, lv_var_value_len);
            }
        } else
            pp_value++;
    }
    return lv_ret;
}

bool MON_Props::store(const char *pp_file) {
    bool lv_sret = false;
    if (pp_file == NULL)
        return lv_sret;
    FILE *lp_file = fopen(pp_file, "w");
    if (lp_file != NULL) {
        MON_Smap_Enum lv_enum(this);
        while (lv_enum.more()) {
            char *lp_key = lv_enum.next();
            const char *lp_value = get(lp_key);
            char la_line[BUFSIZ];
            sprintf(la_line, "%s=%s\n", lp_key, lp_value);
            fputs(la_line, lp_file);
        }
        lv_sret = true;
        fclose(lp_file);
    }
    return lv_sret;
}

