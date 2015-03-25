//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2015 Hewlett-Packard Development Company, L.P.
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
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "idtmjni.h"

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "idtmsrv.h"

static bool            gv_inited  = false;
static jobject         gv_j_cb    = NULL;
static pthread_mutex_t gv_mutex   = PTHREAD_MUTEX_INITIALIZER;
static int             gv_oid     = -1;
static SB_Phandle_Type gv_phandle;
static bool            gv_verbose = false;


static int do_cb(JNIEnv *pp_j_env, char **ppp_snames, int pv_count) {
    jstring      *lp_str;
    jclass        lv_cb_class;
    jmethodID     lv_id_cb;
    int           lv_inx;
    int           lv_nid;
    int           lv_pid;
    jobjectArray  lv_str_array;
    jclass        lv_str_class;

    lv_cb_class = pp_j_env->GetObjectClass(gv_j_cb);
    assert(lv_cb_class != 0);
    lv_id_cb = pp_j_env->GetMethodID(lv_cb_class, "cb", "(II[Ljava/lang/String;)I");
    assert(lv_id_cb != 0);
    lv_str_class = pp_j_env->FindClass("java/lang/String");
    assert(lv_str_class != 0);
    lv_str_array = pp_j_env->NewObjectArray(pv_count, lv_str_class, NULL);
    assert(lv_str_array != 0);
    lp_str = new jstring[pv_count];
    for (lv_inx = 0; lv_inx < pv_count; lv_inx++) {
        lp_str[lv_inx] = pp_j_env->NewStringUTF(ppp_snames[lv_inx]);
        assert(lp_str[lv_inx] != 0);
        pp_j_env->SetObjectArrayElement(lv_str_array, lv_inx, lp_str[lv_inx]);
    }
    msg_mon_get_process_info(NULL, &lv_nid, &lv_pid);
    pp_j_env->CallIntMethod(gv_j_cb,
                            lv_id_cb,
                            lv_nid,
                            lv_pid,
                            lv_str_array);
    for (lv_inx = 0; lv_inx < pv_count; lv_inx++) {
        pp_j_env->DeleteLocalRef(lp_str[lv_inx]);
    }
    pp_j_env->DeleteLocalRef(lv_str_array);
    return 0;
}

#include "idtmclicom.h"


//
// initialize.
//
// if first call, attach and startup.
// open server.
//
// return file error.
//
static int do_init(JNIEnv *pp_j_env) {
    char *la_argv[1];
    int   lv_argc;
    int   lv_ferr;
    int   lv_perr;

    lv_perr = pthread_mutex_lock(&gv_mutex);
    assert(lv_perr == 0);
    if (gv_inited)
        lv_ferr = XZFIL_ERR_OK;
    else {
        do_reg_hash_cb(NULL); // ref to make cmplr happy
        lv_argc = 0;
        la_argv[0] = NULL;
        gv_inited = true;
        try {
            lv_ferr = msg_init_attach(&lv_argc, (char ***) &la_argv, 0, NULL);
        } catch (SB_Fatal_Excep &fatal_exc) {
            if (gv_verbose)
                printf("cli: msg_init_attach threw exc=%s, setting PATHDOWN\n",
                       fatal_exc.what());
            lv_ferr = XZFIL_ERR_PATHDOWN;
        } catch (...) {
            if (gv_verbose)
                printf("cli: msg_init_attach threw unknown exc, setting PATHDOWN\n");
            lv_ferr = XZFIL_ERR_PATHDOWN;
        }
        if (lv_ferr == XZFIL_ERR_OK) {
            try {
                lv_ferr = msg_mon_process_startup(false);
            } catch (SB_Fatal_Excep &fatal_exc) {
                if (gv_verbose)
                    printf("cli: msg_mon_process_startup threw exc=%s, setting PATHDOWN\n",
                           fatal_exc.what());
                lv_ferr = XZFIL_ERR_PATHDOWN;
            } catch (...) {
                if (gv_verbose)
                    printf("cli: msg_mon_process_startup threw unknown exc, setting PATHDOWN\n");
                lv_ferr = XZFIL_ERR_PATHDOWN;
            }
        }
    }
    if (lv_ferr == XZFIL_ERR_OK) {
        if (gv_oid < 0) {
            lv_ferr = do_cli_open(pp_j_env, &gv_phandle, &gv_oid);
            if (gv_verbose)
                printf("cli: open-err=%d\n", lv_ferr);
        }
    }
    lv_perr = pthread_mutex_unlock(&gv_mutex);
    assert(lv_perr == 0);

    return lv_ferr;
}

//
// org.apache.hadoop.hbase.regionserver.transactional.idTm.native_id(j_timeout, j_id)
//
// initialize.
// call do_cli_id() and set j_id.val to returned id from do_cli_id()
//
// return file error
//
jint Java_org_apache_hadoop_hbase_regionserver_transactional_IdTm_native_1id(JNIEnv *pp_j_env, jobject, jint j_timeout, jobject j_id) {
    int      lv_ferr;
    long     lv_id;
    jclass   lv_id_class;
    jfieldID lv_id_val;

    lv_ferr = do_init(pp_j_env);
    lv_id = 0;
    if (lv_ferr == XZFIL_ERR_OK) {
        lv_ferr = do_cli_id(&gv_phandle, j_timeout, &lv_id);
        if (lv_ferr == XZFIL_ERR_OK) {
            lv_id_class = pp_j_env->GetObjectClass(j_id);
            assert(lv_id_class != 0);
            lv_id_val = pp_j_env->GetFieldID(lv_id_class, "val", "J");
            assert(lv_id_val != 0);
            pp_j_env->SetLongField(j_id, lv_id_val, lv_id);
        }
    }
    if (gv_verbose)
        printf("cli: id() err=%d, id=0x%lx\n", lv_ferr, lv_id);

    return lv_ferr;
}

//
// org.apache.hadoop.hbase.regionserver.transactional.idTm.native_ping(j_timeout)
//
// initialize.
// call do_cli_ping().
//
// return file error
//
jint Java_org_apache_hadoop_hbase_regionserver_transactional_IdTm_native_1ping(JNIEnv *pp_j_env, jobject, jint j_timeout) {
    int lv_ferr;

    lv_ferr = do_init(pp_j_env);
    if (lv_ferr == XZFIL_ERR_OK) {
        lv_ferr = do_cli_ping(&gv_phandle, j_timeout);
    }
    if (gv_verbose)
        printf("cli: ping() err=%d\n", lv_ferr);

    return lv_ferr;
}

//
// org.apache.hadoop.hbase.regionserver.transactional.idTm.native_reg_hash_cb(j_cb_in)
//
// register cb
//
// return file error
//
jint Java_org_apache_hadoop_hbase_regionserver_transactional_IdTm_native_1reg_1hash_1cb(JNIEnv *pp_j_env, jobject, jobject j_cb_in) {
    int lv_ferr;

    gv_j_cb = pp_j_env->NewGlobalRef(j_cb_in);

    lv_ferr = XZFIL_ERR_OK;

    return lv_ferr;
}
