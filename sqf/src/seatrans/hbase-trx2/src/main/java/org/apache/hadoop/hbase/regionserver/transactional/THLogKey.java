/**
 * Copyright 2009 The Apache Software Foundation Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements. See the NOTICE file distributed with this work for additional information regarding
 * copyright ownership. The ASF licenses this file to you under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and limitations under the
 * License.
 */
package org.apache.hadoop.hbase.regionserver.transactional;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import java.util.UUID;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.regionserver.wal.HLogKey;

/**
 * The Keys for the transactional WAL. These are keyed on region/transactionID such that the value may contain multiple
 * rows (unlike HLogKey).
 */
public class THLogKey extends HLogKey {

    /**
     * Type of Transactional op going into the HLog
     */
	
	private static final Log LOG = LogFactory.getLog(THLogKey.class);
    public enum TrxOp {
        /**
         * A transaction was requested to be committed. The WAL edit will have all the edits from the transaction. The
         * transaction is not committed at this time. Rather it needs to hear back the final commit call.
         */
        COMMIT_REQUEST((byte) 1),
        /** A transaction was committed. We should have a previous COMMIT_REQUEST entry in the log. */
        COMMIT((byte) 2),
        /** A transaction was aborted. We should have a previous COMMIT_REQUEST entry in the log. */
        ABORT((byte) 3),
        /** A transaction was aborted. We should have a previous COMMIT_REQUEST entry in the log. */
        FORCED_FLUSH_FILLER((byte) 4);

        private final byte opCode;

        private TrxOp(final byte opCode) {
            this.opCode = opCode;
        }

        public static TrxOp fromByte(final byte opCode) {
            for (TrxOp op : TrxOp.values()) {
                if (op.opCode == opCode) {
                    return op;
                }
            }
            return null;
        }

    }

    private byte transactionOp = -1;
    private long transactionId = -1;

    public THLogKey() {
        // For Writable
    	LOG.trace("THLogKey -- CONSTRUCT");
    }

    public THLogKey(final byte[] regionName, final byte[] tablename, final long logSeqNum, final long now,
            final TrxOp op, final long transactionId, final UUID clusterId) {
        super(regionName, tablename, logSeqNum, now, clusterId);
        this.transactionOp = op.opCode;
        this.transactionId = transactionId;
        LOG.trace("THLogKey -- CONSTRUCT");
    }

    public TrxOp getTrxOp() {
    	LOG.trace("getTrxOp");
        return TrxOp.fromByte(this.transactionOp);
    }

    public long getTransactionId() {
    	LOG.trace("getTransactionId");
        return this.transactionId;
    }

    @Override
    public void write(final DataOutput out) throws IOException {
        super.write(out);
        out.writeByte(transactionOp);
        out.writeLong(transactionId);
        LOG.trace("write");
    }

    @Override
    public void readFields(final DataInput in) throws IOException {
        super.readFields(in);
        this.transactionOp = in.readByte();
        this.transactionId = in.readLong();
        LOG.trace("readFields");
    }
}
