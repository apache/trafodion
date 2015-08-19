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

import java.io.IOException;
import java.util.List;

import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;


public class ResultIterator {
	ResultScanner   scanner;
	Result[]        resultSet;
	Result          row = null;
	scanFetchStep   step;
	List<KeyValue>  kvList;
	int 			listIndex = 0;
	int             cellIndex;
	int				numKVs;
	boolean         isSingleRow = false;
	
	private enum scanFetchStep {
		SCAN_FETCH_NEXT_ROW,
		SCAN_FETCH_NEXT_COL,
		SCAN_FETCH_CLOSE
	} ;

	public ResultIterator(ResultScanner scanner) {
		this.scanner = scanner;
		resultSet = null;
		step = scanFetchStep.SCAN_FETCH_NEXT_ROW;
	}
	
	public ResultIterator(Result[] results) {
		this.scanner = null;
		resultSet = results;
		step = scanFetchStep.SCAN_FETCH_NEXT_ROW;
	}
	
	public ResultIterator(Result result) {
		this.scanner = null;
		resultSet = null;
		row = result;
		isSingleRow = true;
		step = scanFetchStep.SCAN_FETCH_NEXT_ROW;
	}
	
	KeyValue nextCell() throws IOException {
		while (true)
		{
			switch (step)
			{
				case SCAN_FETCH_NEXT_ROW:
				{
				        if (isSingleRow == false) {				        
        					if (scanner != null)
        						row = scanner.next();
        					else {
        						if (listIndex == resultSet.length) {
        							step = scanFetchStep.SCAN_FETCH_CLOSE;
        							break;
        						}							
        						row = resultSet[listIndex];
        						listIndex++;
        					}
        				}
					
					if (row == null || row.isEmpty()) {
						step = scanFetchStep.SCAN_FETCH_CLOSE;
						break;
					}
					
					kvList = row.list();
					cellIndex = 0;
					numKVs = kvList.size();
	
					step = scanFetchStep.SCAN_FETCH_NEXT_COL;
				}
				break;
	
				case SCAN_FETCH_NEXT_COL:
				{
					KeyValue kv = kvList.get(cellIndex);
					cellIndex++;
					if (kv == null) {
					        if (isSingleRow)
						        step = scanFetchStep.SCAN_FETCH_CLOSE;
						else
						        step = scanFetchStep.SCAN_FETCH_NEXT_ROW;
						break;
					}
	
					if (cellIndex == numKVs)
					        if (isSingleRow)
						        step = scanFetchStep.SCAN_FETCH_CLOSE;
						else
						        step = scanFetchStep.SCAN_FETCH_NEXT_ROW;
	
					return kv;
				}
				
				case SCAN_FETCH_CLOSE:
				{
					return null;
				}
	
			}// switch
		} // while
		
	}
	
}
