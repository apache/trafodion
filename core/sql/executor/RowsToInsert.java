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

import java.util.Vector;

public class RowsToInsert  extends Vector<RowsToInsert.RowInfo> {

    public class RowInfo {
	public byte[] rowId;
	public Vector<RowsToInsert.ColToInsert> columns;
    }

    public class ColToInsert {
	public byte[] qualName;
	public byte[] colValue;
    }

    private static final long serialVersionUID = 5066470006717527863L;

    public void addRowId(byte[] rowId) {
	RowInfo rowInfo = new RowInfo();
	rowInfo.rowId = rowId;
	rowInfo.columns = new Vector<RowsToInsert.ColToInsert>();
	rowInfo.columns.clear();
	add(rowInfo);
    }

    public void addColumn(byte[] name, byte[] value) {
	ColToInsert col = new ColToInsert();
	col.qualName = name;
	col.colValue = value;
	if (size() > 0)
	    get(size()-1).columns.add(col);
	//	RowInfo.columns.add(col);
    }

}
