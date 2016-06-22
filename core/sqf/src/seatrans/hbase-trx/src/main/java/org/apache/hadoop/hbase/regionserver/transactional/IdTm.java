/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/

package org.apache.hadoop.hbase.regionserver.transactional;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.hadoop.hbase.util.Bytes;

/**
 * IdTm
 *
 * id tm
 */
public class IdTm implements IdTmCb {
    private static final int TO = 1000;
    private static final Log LOG = LogFactory.getLog(IdTm.class);

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
        if (LOG.isDebugEnabled()) LOG.debug("IdTm begin, loop=" + loop);

        for (int inx = 0; inx < loop; inx++) {
            try {
                cli.ping(TO);
            } catch (IdTmException exc) {
                LOG.error("ping threw exc=", exc);
            }
            try {
                IdTmId id = new IdTmId();
                cli.id(TO, id);
                if (LOG.isDebugEnabled()) LOG.debug("id ret=0x" + Long.toHexString(id.val));
            } catch (IdTmException exc) {
                LOG.error("id threw exc=", exc);
            }
        }
        if (LOG.isDebugEnabled()) LOG.debug("IdTm done");
    }

    public IdTm(boolean cb) {
        if (cb) {
            try {
                reg_hash_cb(this);
            } catch (IdTmException exc) {
                LOG.error("reg_hash_cb threw exc=" + exc);
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
        if (LOG.isDebugEnabled()) LOG.debug("cb called nid=" + nid + ",pid=" + pid + ",servers=" + servers);
        for (int inx = 0; inx < servers.length; inx++) {
            if (LOG.isDebugEnabled()) LOG.debug("cb server[" + inx + "]=" + servers[inx]);
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
        if (LOG.isDebugEnabled()) LOG.debug("id begin");
        try {
           int err = native_id(timeout, id);
           if (LOG.isDebugEnabled()) LOG.debug("id returned: " + id.val + ", error: " + err);
           if (err != 0) {
              LOG.error("native_id returned: " + err + " Throwing IdTmException");
              throw new IdTmException("ferr=" + err);
           }
           if (id.val == 0) {
              LOG.error("native_id returned id: " + id.val + " err: " + err + ", Throwing IdTmException");
              throw new IdTmException("ferr=" + err);
           }

        } catch (Throwable t) {
           LOG.error("id threw:", t);
           throw new IdTmException("id threw:" + t);
        }
    }

    /**
     * idToStr server id_to_string
     *
     * @param timeout timeout in ms
     * @param id id to convert to string
     * @param idString string output of id conversion
     * @exception IdTmException exception
     */
    public void idToStr(int timeout, long id, byte [] idString) throws IdTmException {
      if (LOG.isDebugEnabled()) LOG.debug("idToStr begin, id: " + Long.toHexString(id));

      try {
           int err = native_id_to_string(timeout, id, idString);
           if (err != 0) {
              LOG.error("native_id_to_string returned: " + err + " Throwing IdTmException");
              throw new IdTmException("ferr=" + err);
           }
        } catch (Throwable t) {
           LOG.error("idToStr threw:", t);
           throw new IdTmException("idToStr threw:" + t);
        }
    }

    /**
     * strToId server string_to_id
     *
     * @param timeout timeout in ms
     * @param id id
     * @param idString string id to convert
     * @exception IdTmException exception
     */
    public void strToId(int timeout, IdTmId id, String idString) throws IdTmException {
        if (LOG.isDebugEnabled()) LOG.debug("strToId begin " + idString);

        try {
           int err = native_string_to_id(timeout, id, Bytes.toBytes(idString), idString.length());
           if (LOG.isDebugEnabled()) LOG.debug("strToId returned: " + id.val
                                           + ", hex: " + Long.toHexString(id.val) + " error: " + err);

           if (err != 0) {
              LOG.error("native_string_to_id returned: " + err + " Throwing IdTmException");
              throw new IdTmException("native_string_to_id returned: ferr=" + err);
           }
        } catch (Throwable t) {
           LOG.error("strToId threw:", t);
           throw new IdTmException("strToId threw:" + t);
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
            LOG.error("ping returned: " + err + " Throwing IdTmException");
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
            LOG.error("reg_hash_cb returned: " + err + " Throwing IdTmException");
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
     * id server id to string
     *
     * @param timeout timeout in ms
     * @param id id to convert to string
     * @param idString string output of id conversion
     * @return file error
     */
    private native int native_id_to_string(int timeout, long id, byte [] idString);

    /**
     * id server string to id
     *
     * @param timeout timeout in ms
     * @param idString string to convert
     * @param id id
     * @return file error
     */
    private native int native_string_to_id(int timeout, IdTmId id, byte [] idString, int len);

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
