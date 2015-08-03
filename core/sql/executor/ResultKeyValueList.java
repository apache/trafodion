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

package org.trafodion.sql.HBaseAccess;

import java.util.List;
import java.io.*;

import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.client.Result;
import java.nio.*;

public class ResultKeyValueList {
	Result result;
	List<KeyValue> kvList;

	public ResultKeyValueList(Result result) {
		super();
		this.result = result;
		kvList = result.list();
	}

	byte[] getRowID() {
	        if (result == null)
	                return null;
	        else
		        return result.getRow();
	}

	byte[] getAllKeyValues() {
        if (kvList == null)
           return null;
        int numCols = kvList.size();
        byte[] rowID = result.getRow();
        int bufSize = rowID.length;
        bufSize += (64 * numCols);
        for (int i=0; i<numCols; i++) {
          bufSize += kvList.get(i).getLength();
        }
        ByteBuffer buf = ByteBuffer.allocate(bufSize);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        // move in numCols
        buf.putInt(numCols);
        // move in rowID length and rowID
        buf.putInt(rowID.length);
        buf.put(rowID);;
        // move in all descriptors
        for (int i=0; i<numCols; i++) {
          copyKVs(buf, kvList.get(i));
        }
        return buf.array();
    }

	void copyKVs(ByteBuffer buf, KeyValue kv)
	{
	    buf.putInt(kv.getLength());
        int offset = kv.getOffset();
		buf.putInt(kv.getValueLength());
		buf.putInt(kv.getValueOffset() - offset);
		buf.putInt(kv.getQualifierLength());
		buf.putInt(kv.getQualifierOffset() - offset);
		buf.putInt(kv.getFamilyLength());
		buf.putInt(kv.getFamilyOffset() - offset);
		buf.putLong(kv.getTimestamp());
		buf.put(kv.getBuffer(), kv.getOffset(), kv.getLength());
	}


	int getSize() {
	        if (kvList == null)
	                return 0;
	        else
		        return kvList.size();
	}

	KeyValue getEntry(int i) {
	        if (kvList == null)
	                return null;
	        else
		        return kvList.get(i);
	}
}
