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

/*
* Filename    : Row.java
* Description :
*
*/

package org.trafodion.jdbc.t2;

import java.io.Serializable;
import java.sql.*;
import java.util.BitSet;
import java.util.Locale;

// Referenced classes of package sun.jdbc.rowset:
//            BaseRow

class Row extends BaseRow
    implements Serializable, Cloneable {

    private Object currentVals[];
    private BitSet colsChanged;
    private boolean deleted;
    private boolean updated;
    private boolean inserted;
    private int numCols;

    Row(int i) {
        origVals = new Object[i];
        currentVals = new Object[i];
        colsChanged = new BitSet(i);
        numCols = i;
    }

    Row(int i, Object aobj[]) {
        origVals = new Object[i];
        for(int j = 0; j < i; j++)
            origVals[j] = aobj[j];

        currentVals = new Object[i];
        colsChanged = new BitSet(i);
        numCols = i;
    }

    protected void clearDeleted() {
        deleted = false;
    }

    protected void clearInserted() {
        inserted = false;
    }

    protected void clearUpdated() {
        updated = false;
        for(int i = 0; i < numCols; i++) {
            currentVals[i] = null;
            colsChanged.clear(i);
        }

    }

    protected boolean getColUpdated(int i) {
        return colsChanged.get(i);
    }

    protected Object getColumnObject(int i) throws SQLException {
        if(getColUpdated(i - 1))
            return currentVals[i - 1];
        else
            return origVals[i - 1];
    }

    protected boolean getDeleted() {
        return deleted;
    }

    protected boolean getInserted() {
        return inserted;
    }

    protected boolean getUpdated() {
        return updated;
    }

    protected void initColumnObject(int i, Object obj) {
        origVals[i - 1] = obj;
    }

    protected void moveCurrentToOrig() {
        for(int i = 0; i < numCols; i++)
            if(getColUpdated(i)) {
                origVals[i] = currentVals[i];
                currentVals[i] = null;
                colsChanged.clear(i);
            }
	}

    private void setColUpdated(int i) {
        colsChanged.set(i);
    }

    protected void setColumnObject(int i, Object obj) {
        currentVals[i - 1] = obj;
        setColUpdated(i - 1);
    }

	protected void setLobObject(int i, Object obj) {
        currentVals[i - 1] = obj;
		origVals[i-1] = obj;
    }


    protected void setDeleted() {
        deleted = true;
    }

    protected void setInserted() {
        inserted = true;
    }

    protected void setUpdated() {
        updated = true;
    }
	
	protected void deleteRow(Locale locale, PreparedStatement deleteStmt, BitSet paramCols) throws SQLException
	{
		int i;
		int j;
		int count;

		for (i = 0, j = 1; i < numCols ; i++)
		{
			if (paramCols.get(i))
				deleteStmt.setObject(j++, origVals[i]);
		}
		count =	deleteStmt.executeUpdate();
		if (count == 0)
			throw SQLMXMessages.createSQLException(locale, "row_modified", null);
	}

	protected void updateRow(Locale locale, PreparedStatement updateStmt, BitSet paramCols, BitSet keyCols) throws SQLException
	{
		int i;
		int j;
		int count;
		Object obj;
        int numPKey=0;        
        int loc=0;           
        int pKeyCounter=1;    

        for (i = 0; i < numCols; i++ )  
        {                                
            if(keyCols.get(i))           
                numPKey++;               
        }                               
        
        loc = numCols - numPKey;         

		for (i = 0, j = 1; i < numCols ; i++)
		{
			if (keyCols.get(i))
			{
				if (getColUpdated(i))
					throw SQLMXMessages.createSQLException(locale, "primary_key_not_updateable", null);
                updateStmt.setObject((loc+pKeyCounter),getColumnObject(i+1)); 
                pKeyCounter++;  
			}
			else
			{
				{
					obj = getColumnObject((i+1));
					if (obj instanceof SQLMXLob)
					{
						if (obj == origVals[i])	// New and old Lob objects are same
						{
							updateStmt.setObject(j++, new DataWrapper((int) ((SQLMXLob)obj).dataLocator_));
							continue;
						}
					}
					updateStmt.setObject(j++, obj);
				}
			}
		}
		

        /* 
		for (i = 0 ;  i < numCols ; i++)
		{
			if (paramCols.get(i))
			{
				obj = origVals[i];
				if (obj instanceof SQLMXLob)
				{
					updateStmt.setObject(j++, new DataWrapper(((SQLMXLob)obj).dataLocator_));
					continue;
				}
				updateStmt.setObject(j++, origVals[i]);
			}
        } */
		count = updateStmt.executeUpdate();
		if (count == 0)
			throw SQLMXMessages.createSQLException(locale, "row_modified", null);
		moveCurrentToOrig();
		setUpdated();
	}
	
	protected void refreshRow(Locale locale, PreparedStatement selectStmt, BitSet selectCols, BitSet keyCols) throws SQLException
	{
		int i;
		int j;
		ResultSet rs;
		ResultSetMetaData rsmd;
		int columnCount;
		
		clearUpdated();	
	
		for (i = 0, j = 1; i < numCols ; i++)
		{
			if (keyCols.get(i))
				selectStmt.setObject(j++, origVals[i]);
		}
		rs = selectStmt.executeQuery();
		if (rs != null)
		{
			try {
				rsmd = rs.getMetaData();

				columnCount = rsmd.getColumnCount();
				for (i = 0, j = 1 ; i < numCols ; i++)
				{
					if (selectCols.get(i))
						origVals[i] = rs.getObject(j++);
				}
			} finally {
				rs.close();
			}
		}
	}

	protected void closeLobObjects() 
	{
		int i;
		SQLMXLob lob;

		for (i = 0; i < numCols ; i++)
		{
			if (currentVals[i] instanceof SQLMXLob)
			{
				lob = (SQLMXLob)currentVals[i];
				lob.close();
			}
				
		}
	}
}
