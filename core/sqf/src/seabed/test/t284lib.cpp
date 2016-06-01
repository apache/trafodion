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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "com_hp_traf_t284cli.h"

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tms.h"
#include "t284.h"
#include "tverslib.h"

static bool            inited  = false;
static jobject         j_cb    = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int             oid     = -1;
static SB_Phandle_Type phandle;
static bool            verbose = false;

VERS_LIB(libsbzt284)

static int do_cb(JNIEnv *j_env, char **snames, int count) {
    jclass        cb_class;
    jmethodID     id_cb;
    int           inx;
    int           nid;
    int           pid;
    jstring      *str;
    jobjectArray  str_array;
    jclass        str_class;

    cb_class = j_env->GetObjectClass(j_cb);
    assert(cb_class != 0);
    id_cb = j_env->GetMethodID(cb_class, "cb", "(II[Ljava/lang/String;)I");
    assert(id_cb != 0);
    str_class = j_env->FindClass("java/lang/String");
    assert(str_class != 0);
    str_array = j_env->NewObjectArray(count, str_class, NULL);
    assert(str_array != 0);
    str = new jstring[count];
    for (inx = 0; inx < count; inx++) {
        str[inx] = j_env->NewStringUTF(snames[inx]);
        assert(str[inx] != 0);
        j_env->SetObjectArrayElement(str_array, inx, str[inx]);
    }
    msg_mon_get_process_info(NULL, &nid, &pid);
    j_env->CallIntMethod(j_cb,
                         id_cb,
                         nid,
                         pid,
                         str_array);
    for (inx = 0; inx < count; inx++) {
        j_env->DeleteLocalRef(str[inx]);
    }
    j_env->DeleteLocalRef(str_array);
    return 0;
}

#include "t284clicom.h"


//
// initialize.
//
// if first call, attach and startup.
// open server.
//
// return file error.
//
static int do_init(JNIEnv *j_env) {
    int   argc;
    char *argv[1];
    int   ferr;
    int   perr;

    perr = pthread_mutex_lock(&mutex);
    assert(perr == 0);
    if (inited)
        ferr = XZFIL_ERR_OK;
    else {
        do_reg_hash_cb(NULL); // ref to make cmplr happy
        argc = 0;
        argv[0] = NULL;
        inited = true;
        try {
            ferr = msg_init_attach(&argc, (char ***) &argv, 0, NULL);
        } catch (SB_Fatal_Excep &fatal_exc) {
            if (verbose)
                printf("cli: msg_init_attach threw exc=%s, setting PATHDOWN\n",
                       fatal_exc.what());
            ferr = XZFIL_ERR_PATHDOWN;
        } catch (...) {
            if (verbose)
                printf("cli: msg_init_attach threw unknown exc, setting PATHDOWN\n");
            ferr = XZFIL_ERR_PATHDOWN;
        }
        if (ferr == XZFIL_ERR_OK) {
            try {
                ferr = msg_mon_process_startup(false);
            } catch (SB_Fatal_Excep &fatal_exc) {
                if (verbose)
                    printf("cli: msg_mon_process_startup threw exc=%s, setting PATHDOWN\n",
                           fatal_exc.what());
                ferr = XZFIL_ERR_PATHDOWN;
            } catch (...) {
                if (verbose)
                    printf("cli: msg_mon_process_startup threw unknown exc, setting PATHDOWN\n");
                ferr = XZFIL_ERR_PATHDOWN;
            }
        }
    }
    if (ferr == XZFIL_ERR_OK) {
        if (oid < 0) {
            ferr = do_cli_open(j_env, &phandle, &oid);
            if (verbose)
                printf("cli: open-err=%d\n", ferr);
        }
    }
    perr = pthread_mutex_unlock(&mutex);
    assert(perr == 0);

    return ferr;
}

//
// com.hp.traf.t284cli.native_id(j_timeout, j_id)
//
// initialize.
// call do_cli_id() and set j_id.val to returned id from do_cli_id()
//
// return file error
//
jint Java_com_hp_traf_t284cli_native_1id(JNIEnv *j_env, jobject, jint j_timeout, jobject j_id) {
    int      ferr;
    long     id;
    jclass   id_class;
    jfieldID id_val;

    ferr = do_init(j_env);
    id = 0;
    if (ferr == XZFIL_ERR_OK) {
        ferr = do_cli_id(&phandle, j_timeout, &id);
        if (ferr == XZFIL_ERR_OK) {
            id_class = j_env->GetObjectClass(j_id);
            assert(id_class != 0);
            id_val = j_env->GetFieldID(id_class, "val", "J");
            assert(id_val != 0);
            j_env->SetLongField(j_id, id_val, id);
        }
    }
    if (verbose)
        printf("cli: id() err=%d, id=0x%lx\n", ferr, id);

    return ferr;
}

//
// com.hp.traf.t284cli.native_ping(j_timeout)
//
// initialize.
// call do_cli_ping().
//
// return file error
//
jint Java_com_hp_traf_t284cli_native_1ping(JNIEnv *j_env, jobject, jint j_timeout) {
    int ferr;

    ferr = do_init(j_env);
    if (ferr == XZFIL_ERR_OK) {
        ferr = do_cli_ping(&phandle, j_timeout);
    }
    if (verbose)
        printf("cli: ping() err=%d\n", ferr);

    return ferr;
}

//
// com.hp.traf.t284cli.native_reg_hash_cb(j_cb)
//
// register cb
//
// return file error
//
jint Java_com_hp_traf_t284cli_native_1reg_1hash_1cb(JNIEnv *j_env, jobject, jobject j_cb_in) {
    int ferr;

    j_cb = j_env->NewGlobalRef(j_cb_in);

    ferr = XZFIL_ERR_OK;

    return ferr;
}
