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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "props.h"


//
// Implement properties
//
SB_Props::SB_Props() : iv_getenv(false) {
}

SB_Props::SB_Props(bool pv_getenv) : iv_getenv(pv_getenv) {
}

SB_Props::~SB_Props() {
}

bool SB_Props::load(const char *pp_file) {
    bool lv_lret = false;
    if (pp_file == NULL)
        return lv_lret;
    FILE *lp_file = fopen(pp_file, "r");
    if (lp_file != NULL) {
        enum { LINE_SIZE = 64 * 1024 };
        char *lp_line = new char[LINE_SIZE];
        int lv_line_num = 0;
        while (fgets(lp_line, LINE_SIZE, lp_file)) {
            lv_line_num++;
            char *lp_key = lp_line;
            int lv_len = static_cast<int>(strlen(lp_line));
            if ((lv_len > 0) && lp_line[lv_len - 1] == '\n')
                lp_line[lv_len - 1] = '\0';
            char *lp_equals = NULL;
            for (int lv_inx = 0; lv_inx < lv_len; lv_inx++) {
                if (lp_line[lv_inx] == '=') {
                    lp_equals = &lp_line[lv_inx];
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
                lv_len = static_cast<int>(strlen(lp_line));
            while ((*lp_value) && (isspace(*lp_value)))
                lp_value++;
            char *lp_end_value = &lp_line[lv_len-1];
            while ((lp_end_value >= lp_value) && isspace(*lp_end_value))
                lp_end_value--;
            if (lp_end_value < lp_value)
                continue;  // no value
            lp_end_key[1] = '\0';
            lp_end_value[1] = '\0';
            put(lp_key, lp_value);
        }
        delete [] lp_line;
        lv_lret = true;
        fclose(lp_file);
    }
    return lv_lret;
}

bool SB_Props::repl_var(char *pp_value) {
    int  lv_copy1_len;
    int  lv_copy2_len;
    bool lv_ret;

    lv_ret = false;
    while (*pp_value) {
        if (*pp_value == '\'') {
            // replace single-quoted-value with value
            char *lp_start = &pp_value[1];
            char *lp_end_quote = strchr(lp_start, '\'');
            if (lp_end_quote == NULL) {
                // no end-quote - copy whole thing
                lv_copy1_len = static_cast<int>(strlen(pp_value));
                memmove(pp_value, lp_start, lv_copy1_len);
            } else {
                // end-quote - copy first and second parts
                lv_copy1_len = static_cast<int>(lp_end_quote - lp_start);
                memmove(pp_value, lp_start, lv_copy1_len);
                lv_copy2_len = static_cast<int>(strlen(&lp_end_quote[1])) + 1;
                memmove(&pp_value[lv_copy1_len],
                        &lp_end_quote[1],
                        lv_copy2_len);
            }
            pp_value += lv_copy1_len;
        } else if (*pp_value == '$') {
            // replace variable with variable's value
            char *lp_end_value = &pp_value[1];
            while (isalnum(*lp_end_value) || (*lp_end_value == '_'))
                lp_end_value++;
            char lv_save = *lp_end_value;
            *lp_end_value = '\0';
            int lv_var_len = static_cast<int>(strlen(pp_value));
            const char *lp_var = get(&pp_value[1]);
            if ((lp_var == NULL) && iv_getenv)
                lp_var = getenv(&pp_value[1]);
            *lp_end_value = lv_save;
            if (lp_var == NULL)
                lp_var = "";
            int lv_var_value_len = static_cast<int>(strlen(lp_var));
            if (lv_var_value_len > 0)
                lv_ret = true;
            if (lv_var_value_len <= lv_var_len) {
                // variable fits: fill in var, move rest-of-line in after var
                memcpy(pp_value, lp_var, lv_var_value_len);
                memmove(&pp_value[lv_var_value_len],
                        lp_end_value,
                        strlen(lp_end_value) + 1);
            } else {
                // variable does not fit: make room and fill in var
                memmove(&pp_value[lv_var_value_len],
                        lp_end_value,
                        strlen(lp_end_value) + 1);
                memcpy(pp_value, lp_var, lv_var_value_len);
            }
        } else
            pp_value++;
    }
    return lv_ret;
}

bool SB_Props::store(const char *pp_file) {
    bool lv_sret = false;
    if (pp_file == NULL)
        return lv_sret;
    FILE *lp_file = fopen(pp_file, "w");
    if (lp_file != NULL) {
        enum { LINE_SIZE = 64 * 1024 };
        char *lp_line = new char[LINE_SIZE];
        SB_Smap_Enum lv_enum(this);
        while (lv_enum.more()) {
            char *lp_key = lv_enum.next();
            const char *lp_value = get(lp_key);
            sprintf(lp_line, "%s=%s\n", lp_key, lp_value);
            fputs(lp_line, lp_file);
        }
        delete [] lp_line;
        lv_sret = true;
        fclose(lp_file);
    }
    return lv_sret;
}

