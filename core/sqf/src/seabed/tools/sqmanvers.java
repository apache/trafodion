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

package com.hp.sq.sqmanvers;

import java.io.IOException;

import java.util.Arrays;
import java.util.Set;

import java.util.jar.Attributes;
import java.util.jar.JarFile;
import java.util.jar.Manifest;

public class sqmanvers {
    static String  IV =  "Implementation-Version-";
    static String  IV1 = IV + "1";
    static String  IV2 = IV + "2";
    static String  IV3 = IV + "3";
    static String  IV4 = IV + "4";
    static String  IV5 = IV + "5";
    static String  IV6 = IV + "6";

    static boolean cv_all     = false;
    static boolean cv_version = false;

    private static void exit(int pv_exit) {
        Runtime.getRuntime().exit(pv_exit);
    }

    private static void jar_man_vers(String pp_jar) {
        Object[]   la_jf_mf_uns_keys;
        String[]   la_jf_mf_sorted_keys;
        JarFile    lp_jf;
        Manifest   lp_jf_mf;
        Attributes lp_jf_mf_attrs;
        Set        lp_jf_mf_keys;
        String     lp_v1;
        String     lp_v2;
        String     lp_v3;
        String     lp_v4;
        String     lp_v5;
        String     lp_v6;
        int        lv_inx;

        try {
            // get jarfile manifest keys
            lp_jf = new JarFile(pp_jar);
            lp_jf_mf = lp_jf.getManifest();
            lp_jf_mf_attrs = lp_jf_mf.getMainAttributes();
            lp_jf_mf_keys = lp_jf_mf_attrs.entrySet();

            if (cv_all) {
                la_jf_mf_uns_keys = lp_jf_mf_keys.toArray();
                la_jf_mf_sorted_keys = new String[la_jf_mf_uns_keys.length];
                for (lv_inx = 0; lv_inx < la_jf_mf_uns_keys.length; lv_inx++) {
                    la_jf_mf_sorted_keys[lv_inx] =
                      la_jf_mf_uns_keys[lv_inx].toString();
                }
                Arrays.sort(la_jf_mf_sorted_keys);
                for (lv_inx = 0;
                     lv_inx < la_jf_mf_sorted_keys.length;
                     lv_inx++) {
                    System.out.println(la_jf_mf_sorted_keys[lv_inx]);
                }
            }

            // v1=Version <comp-vers>
            // v2=Release <prod-vers>
            // v3=Build <flavor>
            // v4=[xxx]
            // v5=branch 12345-release/seaquest
            // v6=date <date>

            lp_v1 = lp_jf_mf_attrs.getValue(IV1);
            lp_v2 = lp_jf_mf_attrs.getValue(IV2);
            lp_v3 = lp_jf_mf_attrs.getValue(IV3);
            lp_v4 = lp_jf_mf_attrs.getValue(IV4);
            lp_v5 = lp_jf_mf_attrs.getValue(IV5);
            lp_v6 = lp_jf_mf_attrs.getValue(IV6);
            if ((lp_v1 != null) &&
                (lp_v2 != null) &&
                (lp_v3 != null) &&
                (lp_v4 != null) &&
                (lp_v5 != null) &&
                (lp_v6 != null)) {
                lp_v1 = lp_v1.trim();
                lp_v2 = lp_v2.trim();
                lp_v3 = lp_v3.trim();
                lp_v4 = lp_v4.trim();
                lp_v5 = lp_v5.trim();
                lp_v6 = lp_v6.trim();
                System.out.println(lp_v1 + " " +
                                   lp_v2 + " (" +
                                   lp_v3 + " " +
                                   lp_v4 + ", " +
                                   lp_v5 + ", " +
                                   lp_v6 + ")");
            } else {
                System.out.println("Jar file '" +
                                   pp_jar +
                                   " missing manifest version");
                exit(1);
            }
        } catch (IOException lp_ioe) {
            System.out.println("Error reading jar file '" +
                               pp_jar +
                               "', error " + lp_ioe);
            exit(1);
        }
    }

    private static void usage(String pp_err) {
        System.out.println(pp_err);
        System.out.println("usage: sqjavavers [-a] [-v] <jar>...");
        exit(1);
    }

    private static void version() {
        String lp_jar;
        String lp_sqroot;

        lp_sqroot = System.getenv("TRAF_HOME");
        lp_jar = lp_sqroot + "/export/lib/sqmanvers.jar";
        jar_man_vers(lp_jar);
    }

    public static void main(String[] pa_args) {
        String  lp_arg;
        boolean lv_file_seen;
        int     lv_inx;

        lv_file_seen = false;
        for (lv_inx = 0; lv_inx < pa_args.length; lv_inx++) {
            lp_arg = pa_args[lv_inx];
            if (lp_arg.startsWith("-")) {
                if (lv_file_seen)
                    usage("options must preceed jar files");
                if (lp_arg.equals("-a"))
                    cv_all = true;
                else if (lp_arg.equals("-v")) {
                    cv_version = true;
                    version();
                } else
                    usage("unknown option '" + lp_arg + "'");
            } else {
                lv_file_seen = true;
                jar_man_vers(lp_arg);
            }
        }
    }
}
