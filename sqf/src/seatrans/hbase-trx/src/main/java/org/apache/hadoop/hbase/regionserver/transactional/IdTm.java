/**
 * Copyright 2015 The Apache Software Foundation Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements. See the NOTICE file distributed with this work for additional information regarding
 * copyright ownership. The ASF licenses this file to you under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and limitations under the
 * License.
 */
package org.apache.hadoop.hbase.regionserver.transactional;

/**
 * IdTm
 *
 * id tm
 */
public class IdTm implements IdTmCb {
    private static final int TO = 1000;

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

        IdTm cli = new IdTm(cb);
        System.out.println("IdTm begin, loop=" + loop);

        for (int inx = 0; inx < loop; inx++) {
            try {
                cli.ping(TO);
            } catch (IdTmException exc) {
                System.out.println("ping threw exc=" + exc);
            }
            try {
                IdTmId id = new IdTmId();
                cli.id(TO, id);
                System.out.println("id ret=0x" + Long.toHexString(id.val));
            } catch (IdTmException exc) {
                System.out.println("id threw exc=" + exc);
            }
        }
        System.out.println("IdTm done");
    }

    private IdTm(boolean cb) {
        if (cb) {
            try {
                reg_hash_cb(this);
            } catch (IdTmException exc) {
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
     * @exception IdTmException exception
     */
    public void id(int timeout, IdTmId id) throws IdTmException {
        int err = native_id(timeout, id);
        if (err != 0) {
            throw new IdTmException("ferr=" + err);
        }
    }

    /**
     * ping server
     *
     * @param timeout timeout in ms
     * @exception IdTmException exception
     */
    public void ping(int timeout) throws IdTmException {
        int err = native_ping(timeout);
        if (err != 0) {
            throw new IdTmException("ferr=" + err);
        }
    }

    /**
     * register hash callback
     *
     * @param cb callback
     * @exception IdTmException exception
     */
    public void reg_hash_cb(IdTmCb cb) throws IdTmException {
        int err = native_reg_hash_cb(cb);
        if (err != 0) {
            throw new IdTmException("ferr=" + err);
        }
    }

    /**
     * id server
     *
     * @param timeout timeout in ms
     * @param id id
     * @return file error
     */
    private native int native_id(int timeout, IdTmId id);
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
    private native int native_reg_hash_cb(IdTmCb cb);

    static {
        System.loadLibrary("idtm");
    }
}
