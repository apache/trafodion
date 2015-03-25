//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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

package com.hp.traf;

/**
 * t284cli
 *
 * test id server
 */
public class t284cli implements t284cb {
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


        t284cli cli = new t284cli(cb);
        System.out.println("t284cli(j) begin, loop=" + loop);

        for (int inx = 0; inx < loop; inx++) {
            try {
                cli.ping(TO);
            } catch (t284exc exc) {
                System.out.println("ping threw exc=" + exc);
            }
            try {
                t284id id = new t284id();
                cli.id(TO, id);
                System.out.println("id ret=0x" + Long.toHexString(id.val));
            } catch (t284exc exc) {
                System.out.println("id threw exc=" + exc);
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
