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

package com.hp.traf;

/**
 * t284cli
 *
 * test id server
 */
public class t284cli implements t284cb {
    private static final int TO = 1000;
    private static byte      asciiTime [];
    /**
     * main
     *
     * @param args arguments
     */
    public static void main(String[] args) {
        boolean cb = false;
        int     loop = 1;
        
        for (int arg = 0; arg < args.length; arg++) {
            String sarg = args[arg];
            if (sarg.equals("-cb")) {
                cb = true;
            } else if (sarg.equals("-loop")) {
                if ((arg + 1) < args.length) {
                    arg++;
                    sarg = args[arg];
                    loop = Integer.parseInt(sarg);
                } else {
                    System.out.println("expecting <loop-count>");
                    Runtime.getRuntime().exit(1);
                }
            }
        }

        asciiTime = new byte[40];
        t284cli cli = new t284cli(cb);
        System.out.println("t284cli(j) begin, loop=" + loop);

        for (int inx = 0; inx < loop; inx++) {
            try {
                cli.ping(TO);
            } catch (t284exc exc) {
                System.out.println("ping threw exc=" + exc);
            }
            long idVal = 0;
            try {
                t284id id = new t284id();
                cli.id(TO, id);
                System.out.println("id ret=0x" + Long.toHexString(id.val));
                idVal = id.val;
            } catch (t284exc exc) {
                System.out.println("id threw exc=" + exc);
            }
            try{
                cli.id_to_string(TO, idVal, asciiTime);
                System.out.println("id_to_string ret= " + new String(asciiTime));
            }
            catch (t284exc exc){
                System.out.println("id_to_string threw exc=" + exc);
            }
            try{
                t284id id2 = new t284id();
                cli.string_to_id(TO, id2, asciiTime);
                System.out.println("string_to_id ret=0x" + Long.toHexString(id2.val));
                idVal = id2.val;
            }
            catch (t284exc exc){
                System.out.println("string_to_id threw exc=" + exc);
            }
 
        }
        System.out.println("t284cli(j) done");
    }

    private t284cli(boolean cb) {
        if (cb) {
            try {
                reg_hash_cb(this);
            } catch (t284exc exc) {
                System.out.println("reg_hash_cb threw exc=" + exc);
            }
        }
    }

    /**
     * callback
     *
     * @param nid nid
     * @param pid pid
     * @param servers servers
     * @return error
     */
    public int cb(int nid, int pid, String[] servers) {
        System.out.println("cb called nid=" + nid + ",pid=" + pid + ",servers=" + servers);
        for (int inx = 0; inx < servers.length; inx++) {
            System.out.println("cb server[" + inx + "]=" + servers[inx]);
        }
        return 0;
    }

    /**
     * id server
     *
     * @param timeout timeout in ms
     * @param id id
     * @exception t284exc exception
     */
    public void id(int timeout, t284id id) throws t284exc {
        int err = native_id(timeout, id);
        if (err != 0) {
            throw new t284exc("ferr=" + err);
        }
    }

   /**
    * id server id_to_string
    *
    * @param timeout timeout in ms
    * @param id id
    * @param id_string output string
    * @exception t284exc exception
    */
    public void id_to_string(int timeout, long id, byte [] id_string) throws t284exc {
        int err = native_id_to_string(timeout, id, id_string);
        if (err != 0) {
            throw new t284exc("id_to_string ferr=" + err);
        }
    }

   /**
    * id server string_to_id
    *
    * @param timeout timeout in ms
    * @param id output id from converted string
    * @param id_string string to convert
    * @exception t284exc exception
    */
    public void string_to_id(int timeout, t284id id, byte [] id_string) throws t284exc {
        int err = native_string_to_id(timeout, id, id_string);
        if (err != 0) {
            throw new t284exc("string_to_id ferr=" + err);
        }
    }

    /**
     * ping server
     *
     * @param timeout timeout in ms
     * @exception t284exc exception
     */
    public void ping(int timeout) throws t284exc {
        int err = native_ping(timeout);
        if (err != 0) {
            throw new t284exc("ferr=" + err);
        }
    }

    /**
     * register hash callback
     *
     * @param cb callback
     * @exception t284exc exception
     */
    public void reg_hash_cb(t284cb cb) throws t284exc {
        int err = native_reg_hash_cb(cb);
        if (err != 0) {
            throw new t284exc("ferr=" + err);
        }
    }

    /**
     * id server
     *
     * @param timeout timeout in ms
     * @param id id
     * @return file error
     */
    private native int native_id(int timeout, t284id id);
 
    /**
     * id_to_string server
     *
     * @param timeout timeout in ms
     * @param id id
     * @param asciiTimeString
     * @return file error
     */
    private native int native_id_to_string(int timeout, long id, byte[] id_string );

    /**
     * id_to_string server
     *
     * @param timeout timeout in ms
     * @param id id
     * @param asciiTimeString
     * @return file error
     */
    private native int native_string_to_id(int timeout, t284id id, byte[] id_string );

    /**
     * ping server
     *
     * @param timeout timeout in ms
     * @return file error
     */
    private native int native_ping(int timeout);

    /**
     * register hash callback
     *
     * @param cb callback
     * @return file error
     */
    private native int native_reg_hash_cb(t284cb cb);

    static {
        System.loadLibrary("sbzt284");
    }
}
