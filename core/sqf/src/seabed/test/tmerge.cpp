//------------------------------------------------------------------
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

#include <assert.h>
#include <ctype.h>
#include <stdio.h>

#include "tversbin.h"

VERS_BIN(seabed_test_tmerge)

DEFINE_COMP_DOVERS(seabed_test_tmerge)

bool delta = false;
bool verbose = false;

class CFile {
public:
    CFile(char *file_str, int indent_col);
    virtual ~CFile();

    long  get_delta_ts(CFile &f);
    long  get_ts();
    bool  lt(CFile &f);
    bool  is_ok();
    long  peek_ts();
    void  print_line();
    void  print_line_ts(long delta_ts);
    void  read_line();

private:
    enum { MAX_PEEK = 50 };
    void  set_ok(bool ok_in);
    bool  ts_ok();
    void  ts_set(char *line, long *ts_ret);

    FILE *fp;
    char *indent;
    char *lp;
    bool  ok;
    char *peekp[MAX_PEEK];
    int   peekp_inx;
    long  ts;
};

CFile::CFile(char *file_str, int indent_col) {
    fp = fopen(file_str, "r");
    lp = NULL;
    ok = (fp != NULL);
    peekp_inx = -1;
    for (int inx = 0; inx < MAX_PEEK; inx++)
        peekp[inx] = NULL;
    indent = new char[indent_col+1];
    for (int inx = 0; inx < indent_col; inx++)
        indent[inx] = ' ';
    indent[indent_col] = '\0';
}

CFile::~CFile() {
    if (lp != NULL)
        free(lp);
    for (int inx = 0; inx < MAX_PEEK; inx++)
        if (peekp[inx] != NULL)
            free(peekp[inx]);
    if (fp != NULL)
        fclose(fp);
    delete [] indent;
}

long CFile::get_delta_ts(CFile &f) {
    long delta_ts = f.get_ts() - get_ts();
    long next_ts = peek_ts() - get_ts();
    if (next_ts > 0) {
        if (next_ts < delta_ts)
            delta_ts = next_ts;
        else if (delta_ts < 0)
            delta_ts = next_ts;
    }
    return delta_ts;
}

long CFile::get_ts() {
    return ts;
}

bool CFile::is_ok() {
    return ok;
}

bool CFile::lt(CFile &f) {
    bool ret;

    ret = false;
    if (is_ok() && f.is_ok()) {
        if (ts_ok()) {
            ret = (ts < f.ts);
        } else
            ret = true;
    }
    if (verbose)
        printf("lt=%d, ts1=%ld, t2s=%ld\n",
               ret, ts, f.ts);
    return ret;
}

long CFile::peek_ts() {
    long ret = 0;
    size_t len;
    for (int inx = 0; inx <= peekp_inx; inx++) {
        ts_set(peekp[inx], &ret);
        if (ret != 0)
            break;
    }
    if (ret == 0) {
        do {
            peekp_inx++;
            assert(peekp_inx < MAX_PEEK);
            len = 0;
            ssize_t gl_ret = getline(&peekp[peekp_inx], &len, fp);
            if (gl_ret != -1) {
                if (gl_ret > 0) {
                    if (peekp[peekp_inx][gl_ret - 1] == '\n')
                        peekp[peekp_inx][gl_ret - 1] = '\0';
                }
                ts_set(peekp[peekp_inx], &ret);
            } else {
                set_ok(false);
                ret = 0;
                break;
            }
        } while (ret == 0);
    }
    return ret;
}

void CFile::print_line() {
    if (lp != NULL)
        printf("%s%s\n", indent, lp);
}

void CFile::print_line_ts(long delta_ts) {
    if ((lp != NULL) && ts_ok()) {
        // 0123456789012345
        // 21:14:59.019.699
        lp[16] = '\0';
        char delta[20];
        if (delta_ts < 0)
            strcpy(delta, "           ");
        else
            sprintf(delta, "(>%8ld)", delta_ts);
        printf("%s%s%s %s\n", indent, lp, delta, &lp[17]);
    } else if (lp != NULL) {
        printf("%s%s\n", indent, lp);
    }
}

void CFile::read_line() {
    if (peekp_inx < 0) {
        if (lp != NULL) {
            free(lp);
            lp = NULL;
        }
        size_t len = 0;
        ssize_t gl_ret = getline(&lp, &len, fp);
        if (gl_ret != -1) {
            if (gl_ret > 0) {
                if (lp[gl_ret - 1] == '\n')
                    lp[gl_ret - 1] = '\0';
            }
            ts_set(lp, NULL);
        } else
            set_ok(false);
    } else {
        assert(peekp_inx >= 0);
        lp = peekp[0];
        for (int inx = 0; inx < peekp_inx; inx++)
            peekp[inx] = peekp[inx+1];
        peekp[peekp_inx] = NULL;
        peekp_inx--;
        ts_set(lp, NULL);
    }
}

void CFile::set_ok(bool ok_in) {
    ok = ok_in;
}

bool CFile::ts_ok() {
    bool ret;
    ret = (ts > 0);
    return ret;
}

void CFile::ts_set(char *line, long *ts_ret) {
    long lts;
    // 0123456789012345
    // 21:14:59.019.699
    if ((line[2] == ':') && 
        (line[5] == ':') && 
        (line[8] == '.') && 
        (line[12] == '.') &&
        ((line[0] >= '0') && (line[0] <= '2')) &&
        isdigit(line[1]) &&
        ((line[3] >= '0') && (line[3] <= '5')) &&
        isdigit(line[4]) &&
        ((line[6] >= '0') && (line[6] <= '5')) &&
        isdigit(line[7]) &&
        isdigit(line[9]) &&
        isdigit(line[10]) &&
        isdigit(line[11]) &&
        isdigit(line[13]) &&
        isdigit(line[14]) &&
        isdigit(line[15])) {
        long hrs = atoi(&line[0]);
        long min = atoi(&line[3]);
        long sec = atoi(&line[6]);
        long ms = atoi(&line[9]);
        long us = atoi(&line[13]);
        lts = hrs * 3600L * 1000000L +
              min * 60L * 1000000L +
              sec * 1000000L +
              ms * 1000L +
              us;
    } else {
         lts = 0;
    }
    if (ts_ret == NULL)
        ts = lts;
    else
        *ts_ret = lts;
}

void do_merge(char *flstr, char *frstr) {
    CFile fl(flstr, 0);
    CFile fr(frstr, 17);
    long delta_ts;

    fl.read_line();
    fr.read_line();

    do {    
        while (fl.lt(fr) || (fl.is_ok() && !fr.is_ok())) {
            if (delta) {
                delta_ts = fl.get_delta_ts(fr);
                fl.print_line_ts(delta_ts);
            } else
                fl.print_line();
            fl.read_line();
        }
        if (delta) {
            delta_ts = fr.get_delta_ts(fl);
            fr.print_line_ts(delta_ts);
        } else
            fr.print_line();
        fr.read_line();
    } while (fl.is_ok() || fr.is_ok());
}

void print_usage(char *argv[]) {
    printf("usage: %s [-d] [-v] fileL fileR\n", argv[0]);
}

int main(int argc, char *argv[]) {
    char *flstr = NULL;
    char *frstr = NULL;

    CALL_COMP_DOVERS(seabed_test_tmerge, argc, argv);
    for (int arg = 1; arg < argc; arg++) {
        char *p = argv[arg];
        if (*p == '-') {
            if (strcmp(p, "-d") == 0)
                delta = true;
            else if (strcmp(p, "-v") == 0)
                verbose = true;
            else {
                printf("invalid option\n");
                print_usage(argv);
                return 1;
            }
        } else {
            if (flstr == NULL)
                flstr = p;
            else if (frstr == NULL)
                frstr = p;
            else {
                printf("too many files\n");
                print_usage(argv);
                return 1;
            }
        }
    }
    if ((flstr == NULL) || (frstr == NULL)) {
        printf("two files needed\n");
        print_usage(argv);
        return 1;
    }

    do_merge(flstr, frstr);

    return 0;
}

