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

/* -*-java-*-
 * Filename		: DataWrapper.java
 * Description	: Provides an object to pass numeric data across the JNI.
 */
 /*
  * Methods Changed: convertStringToNumeric()
 */
package org.apache.trafodion.jdbc.t2;


import java.math.BigDecimal;
import java.math.BigInteger;
import java.sql.Blob;
import java.sql.Clob;
import java.sql.SQLException;
import java.util.BitSet;
import java.util.Locale;

public class DataWrapper
{
	// Types of data the wrapper can store
	static final byte UNKNOWN		=  0;
	static final byte BYTE			=  1;
	static final byte SHORT			=  2;
	static final byte INTEGER		=  3;
	static final byte LONG			=  4;
	static final byte FLOAT			=  5;
	static final byte DOUBLE		=  6;
	static final byte BOOLEAN		=  7;
	static final byte STRING		=  8;
	static final byte BYTES			=  9;
	static final byte BLOB			= 10;
	static final byte CLOB			= 11;
	static final byte BIG_DECIMAL	= 12;
	static final byte OBJECT		= 13;
	static private final int[] powersOfTen = { 10, 100, 1000, 10000 };

	// Data fields.  These are read directly rather than using
	//   getters for performance in the Java and JNI code.
	byte[]					byteValue;
	short[]					shortValue;
	int[]					intValue;
	long[]					longValue;
	float[]					floatValue;
	double[]				doubleValue;
	boolean[]				booleanValue;
	byte[]					dataType;
	byte[][]				bytesValue;
	Object[]				objectValue;
	Object[]				origValue;
	boolean[]				isNullValue;
	boolean[]				numericValid;
	boolean[]				setNeeded;
    byte[][]                SQLbytesValue;

	private boolean			updated;
	private boolean			deleted;
	private boolean			inserted;
	private boolean			isInsertRow;
	private int				totalColumns;
	private boolean[] dataTruncation;

	private static final double FLOAT_MAX = 3.4028235E38;
	private static final BigDecimal MinLong = BigDecimal.valueOf(Long.MIN_VALUE);
	private static final BigDecimal MaxLong = BigDecimal.valueOf(Long.MAX_VALUE);
	private static final BigDecimal MinDouble = new BigDecimal(-Double.MAX_VALUE);
	private static final BigDecimal MaxDouble = new BigDecimal(Double.MAX_VALUE);

	// Constructor
	// Creates a Data Wrapper that can contain an entire row of data or statement parameters
	public DataWrapper(int total_cols)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_DataWrapper].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_DataWrapper].methodParameters(
										  "total_cols = " + total_cols);
		try
		{
			initDataWrapper(total_cols);
			isInsertRow = false;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_DataWrapper].methodExit();
		}
	}

	private void initDataWrapper(int total_cols)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_initDataWrapper_I].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_initDataWrapper_I].methodParameters(
										  "total_cols = " + total_cols);
		try
		{
			totalColumns = total_cols;
			initDataWrapper();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_initDataWrapper_I].methodExit();
		}
	}

	void initDataWrapper()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_initDataWrapper].methodEntry();
		try
		{
			byteValue = new byte[totalColumns];
			shortValue = new short[totalColumns];
			intValue = new int[totalColumns];
			longValue = new long[totalColumns];
			floatValue = new float[totalColumns];
			doubleValue = new double[totalColumns];
			booleanValue = new boolean[totalColumns];
			dataType = new byte[totalColumns];
			isNullValue = new boolean[totalColumns];
			numericValid = new boolean[totalColumns];
			setNeeded = new boolean[totalColumns];
			dataTruncation = new boolean[totalColumns];

			// The bytes, object and orig value arrays are only allocated on demand.
			// Creating an array is actually an object itself and requires extra overhead.
			// To avoid wasting time, every time the arrays are accessed, they are checked
			//    to see if they are allocated and if needed, they are allocated at that time.
			// A null array implies that all the elements are null.
			// The orig array is special in that if the array is null, it means that
			//    no original values have been stored.  If it is not null, to avoid having to
			//    create and use another array to tell if the value has not been set verses
			//    so a null value really means that the value was stored and is null.
			bytesValue = null;
			objectValue = null;
			origValue = null;

			updated = false;
			deleted = false;
			inserted = false;
			
            SQLbytesValue = null; 
            
			for (int col_idx=0; col_idx<totalColumns; col_idx++) clearColumn(col_idx+1);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_initDataWrapper].methodExit();
		}
	}

	void setInsertRow()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setInsertRow].methodEntry();
		try
		{
			isInsertRow = true;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setInsertRow].methodExit();
		}
	}

	void copyRows(DataWrapper source) throws SQLException
	{
		// Copy a data wrapper into the current
		if (JdbcDebugCfg.entryActive) debug[methodId_copyRows].methodEntry();
		try
		{
			initDataWrapper(source.totalColumns);
			for (int col_idx=0; col_idx<totalColumns; col_idx++)
			{
				if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
						"col_idx = " + col_idx);

				// If original object is the data wrapper, it is not set
				if ((source.origValue!=null) && (source.origValue[col_idx]!=source))
				{
					if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
						"Using orig value from source.origValue[" + col_idx + "] = " + source.origValue[col_idx]);

					// Need to use original value from source
					setObject(col_idx,source.origValue[col_idx]);
				}
				else
				{
					// Use the current value from the source
					dataType[col_idx] = source.dataType[col_idx];
					if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
							"dataType[" + col_idx + "] = " + dataType[col_idx]);

					// Save the numeric types
					byteValue[col_idx] = source.byteValue[col_idx];
					if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
							"byteValue[" + col_idx + "] = " + byteValue[col_idx]);

					shortValue[col_idx] = source.shortValue[col_idx];
					if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
							"shortValue[" + col_idx + "] = " + shortValue[col_idx]);

					intValue[col_idx] = source.intValue[col_idx];
					if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
							"intValue[" + col_idx + "] = " + intValue[col_idx]);

					longValue[col_idx] = source.longValue[col_idx];
					if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
							"longValue[" + col_idx + "] = " + longValue[col_idx]);

					floatValue[col_idx] = source.floatValue[col_idx];
					if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
							"floatValue[" + col_idx + "] = " + floatValue[col_idx]);

					doubleValue[col_idx] = source.doubleValue[col_idx];
					if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
							"doubleValue[" + col_idx + "] = " + doubleValue[col_idx]);

					booleanValue[col_idx] = source.booleanValue[col_idx];
					if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
							"booleanValue[" + col_idx + "] = " + booleanValue[col_idx]);

					isNullValue[col_idx] = source.isNullValue[col_idx];
					if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
							"isNullValue[" + col_idx + "] = " + isNullValue[col_idx]);

					numericValid[col_idx] = source.numericValid[col_idx];
					if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
							"numericValid[" + col_idx + "] = " + numericValid[col_idx]);

					setNeeded[col_idx] = source.setNeeded[col_idx];
					if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
							"setNeeded[" + col_idx + "] = " + setNeeded[col_idx]);

					// Save the object types if needed
					if (source.objectValue==null)
					{
						// Object is null
						if (objectValue!=null) objectValue[col_idx] = null;
						if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
								"source.objectValue is null");
					}
					else
					{
						// Save the object
						if (objectValue==null) setupObjects();
						objectValue[col_idx] = source.objectValue[col_idx];
						if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
								"objectValue[" + col_idx + "] is : " + objectValue[col_idx]);

					}

					if (source.bytesValue==null)
					{
						if (bytesValue!=null) bytesValue[col_idx] = null;
						if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
								"source.bytesValue is null");
					}
					else
					{
						if (bytesValue==null) setupBytes();
						bytesValue[col_idx] = source.bytesValue[col_idx];
						if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
								"bytesValue[" + col_idx + "] is : " + bytesValue[col_idx]);
					}
                    if (source.SQLbytesValue==null)
                    {
                        if (SQLbytesValue!=null) SQLbytesValue[col_idx] = null;
						if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
								"source.SQLbytesValue is null");
                    }
                    else
                    {
                        if (SQLbytesValue==null) setupSQLBytes();
                        SQLbytesValue[col_idx] = source.SQLbytesValue[col_idx];
						if (JdbcDebugCfg.traceActive) debug[methodId_copyRows].methodParameters(
								"SQLbytesValue[" + col_idx + "] is : " + SQLbytesValue[col_idx]);
                    }
//---------------------
				}
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_copyRows].methodExit();
		}
	}

	private void setOrig(int col_num, Object obj)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setOrig].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setOrig].methodParameters(
										  "col_num = " + col_num);
		try
		{
			if (obj==(Object)this)
			{
				// Set orig to not updated
				if (origValue!=null) origValue[col_num-1] = this;
			}
			else
			{
				if (origValue==null) setupOrig();
				origValue[col_num-1] = obj;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setOrig].methodExit();
		}
	}

	private void updateOrig(int col_num) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateOrig].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_updateOrig].methodParameters(
										  "col_num = " + col_num);
		try
		{
			// Only update if current value was set before
			if (dataType[col_num-1]!=UNKNOWN)
			{
				// Only update if original has not been set before
				if (origValue==null) setupOrig();
				if (origValue[col_num-1]==this)
				{
					origValue[col_num-1] = getObject(col_num);
				}
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateOrig].methodExit();
		}
	}

	void clearColumn(int col_num)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_clearColumn].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_clearColumn].methodParameters(
										  "col_num = " + col_num);
		try
		{
			// set the column back to unknown
			dataType[col_num-1] = UNKNOWN;
			isNullValue[col_num-1] = true;
			numericValid[col_num-1] = false;
			setNeeded[col_num-1] = false;
			if (bytesValue!=null) bytesValue[col_num-1] = null;
			if (objectValue!=null) objectValue[col_num-1] = null;
			clearOrig(col_num);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_clearColumn].methodExit();
		}
	}

	private void clearOrig(int col_num)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_clearOrig].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_clearOrig].methodParameters(
										  "col_num = " + col_num);
		try
		{
			if (origValue!=null)
			{
				origValue[col_num-1] = this;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_clearOrig].methodExit();
		}
	}

	private Object getOrigValue(int col_num)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getOrigValue].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getOrigValue].methodParameters(
										  "col_num = " + col_num);
		try
		{
			// Return the origValue object
			if (origValue==null) return this;
			return origValue[col_num-1];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getOrigValue].methodExit();
		}
	}

	private Object getOriginalObject(int col_num)  throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getOriginalObject].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getOriginalObject].methodParameters(
										  "col_num = " + col_num);
		try
		{
			// Return the object that represents the original value
			// If original is saved, return it, otherwise return the current
			Object obj = getOrigValue(col_num);
			if (obj==this)
			{
				obj = getObject(col_num);
			}
			return obj;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getOriginalObject].methodExit();
		}
	}

	// updateLong() sets all fields and is used by all constructors,
	//   so no conversions are required when a specific data type
	//   is required.
	private void updateLong(int col_num, long value) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateLong].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_updateLong].methodParameters(
										  "col_num = " + col_num + ", value=" + value);
		try
		{
			updateOrig(col_num);
			byteValue[col_num-1] = (byte) value;
			shortValue[col_num-1] = (short) value;
			intValue[col_num-1] = (int) value;
			longValue[col_num-1] = value;
			floatValue[col_num-1] = (float) value;
			doubleValue[col_num-1] = (double) value;
			booleanValue[col_num-1] = (value != 0);
			isNullValue[col_num-1] = false;
			if (objectValue!=null) objectValue[col_num-1] = null;
			if (bytesValue!=null) bytesValue[col_num-1] = null;
			numericValid[col_num-1] = true;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateLong].methodExit();
		}
	}

	// Getters for all supported data types
	// Original data type is saved for string and
	//   Java wrapper object conversion.
	void setByte(int col_num, byte value) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setByte].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setByte].methodParameters(
										  "col_num = " + col_num + ", value=" + value);
		try
		{
			updateLong(col_num, value);
			dataType[col_num-1] = BYTE;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setByte].methodExit();
		}
	}

	void setShort(int col_num, short value) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setShort].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setShort].methodParameters(
										  "col_num = " + col_num + ", value=" + value);
		try
		{
			updateLong(col_num, value);
			dataType[col_num-1] = SHORT;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setShort].methodExit();
		}
	}

	void setInt(int col_num, int value) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setInt].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setInt].methodParameters(
										  "col_num = " + col_num + ", value=" + value);
		try
		{
			updateLong(col_num, value);
			dataType[col_num-1] = INTEGER;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setInt].methodExit();
		}
	}

	void setLong(int col_num, long value) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setLong].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setLong].methodParameters(
										  "col_num = " + col_num + ", value=" + value);
		try
		{
			updateLong(col_num, value);
			dataType[col_num-1] = LONG;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setLong].methodExit();
		}
	}

	void setFloat(int col_num, float value) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setFloat].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setFloat].methodParameters(
										  "col_num = " + col_num + ", value=" + value);
		try
		{
			// Need to override truncation for floating point values
			updateLong(col_num, (long) value);
			floatValue[col_num-1] = value;
			/* 
	            * Description: float is properly converted to double before assigning to double data type
 			*/
			if (value==Float.POSITIVE_INFINITY) doubleValue[col_num-1] = Double.POSITIVE_INFINITY;
			else if (value==Float.NEGATIVE_INFINITY) doubleValue[col_num-1] = Double.NEGATIVE_INFINITY;
			/*else doubleValue[col_num-1] = value;  */
			else doubleValue[col_num-1] = Double.parseDouble(Float.toString(value));
			dataType[col_num-1] = FLOAT;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setFloat].methodExit();
		}
	}

	void setDouble(int col_num, double value) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setDouble].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setDouble].methodParameters(
										  "col_num = " + col_num + ", value=" + value);
		try
		{
			// Need to override truncation for floating point values
			updateLong(col_num, (long) value);
			if (value==Double.POSITIVE_INFINITY) floatValue[col_num-1] = Float.POSITIVE_INFINITY;
			else if (value==Double.NEGATIVE_INFINITY) floatValue[col_num-1] = Float.NEGATIVE_INFINITY;
			else floatValue[col_num-1] = (float) value;
			doubleValue[col_num-1] = value;
			dataType[col_num-1] = DOUBLE;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setDouble].methodExit();
		}
	}

	void setBoolean(int col_num, boolean value) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setBoolean].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setBoolean].methodParameters(
										  "col_num = " + col_num + ", value=" + value);
		try
		{
			if (value) updateLong(col_num, 1);
			else updateLong(col_num, 0);
			dataType[col_num-1] = BOOLEAN;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setBoolean].methodExit();
		}
	}

	boolean isNull(int col_num) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isNull].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_isNull].methodParameters(
										  "col_num = " + col_num);
		try
		{
			if (setNeeded[col_num-1]) updateColumn(col_num);
			return isNullValue[col_num-1];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isNull].methodExit();
		}
	}

	void setNull(int col_num) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setNull].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setNull].methodParameters(
										  "col_num = " + col_num);
		try
		{
			updateOrig(col_num);
			isNullValue[col_num-1] = true;
			if (objectValue!=null) objectValue[col_num-1] = null;
			if (bytesValue!=null) bytesValue[col_num-1] = null;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setNull].methodExit();
		}
	}

	private void setupOrig()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setupOrig].methodEntry();
		try
		{
			origValue = new Object[totalColumns];
			for (int i=0; i<totalColumns; i++) clearOrig(i+1);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setupOrig].methodExit();
		}
	}

	private void setupBytes()
	{
		// This method is implemented in the JNI also.  Any changes need to be made in both places
		if (JdbcDebugCfg.entryActive) debug[methodId_setupBytes].methodEntry();
		try
		{
			bytesValue = new byte[totalColumns][];
			for (int col_idx=0; col_idx<totalColumns; col_idx++) bytesValue[col_idx] = null;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setupBytes].methodExit();
		}
	}
    private void setupSQLBytes()
    {
        // This method is implemented in the JNI also.  Any changes need to be made in both places
        SQLbytesValue = new byte[totalColumns][];
        for (int col_idx=0; col_idx<totalColumns; col_idx++) SQLbytesValue[col_idx] = null;
    }

	private void setupObjects()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setupObjects].methodEntry();
		try
		{
			objectValue = new Object[totalColumns];
			for (int col_idx=0; col_idx<totalColumns; col_idx++) objectValue[col_idx] = null;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setupObjects].methodExit();
		}
	}

	void setBlob(int col_num, Blob b) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setBlob].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setBlob].methodParameters(
										  "col_num = " + col_num);
		try
		{
			setNull(col_num);
			if (objectValue==null) setupObjects();
			objectValue[col_num-1] = (Object) b;
			isNullValue[col_num-1] = (b==null);
			dataType[col_num-1] = BLOB;
			clearOrig(col_num);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setBlob].methodExit();
		}
	}

	void setClob(int col_num, Clob c) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setClob].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setClob].methodParameters(
										  "col_num = " + col_num);
		try
		{
			setNull(col_num);
			if (objectValue==null) setupObjects();
			objectValue[col_num-1] = (Object) c;
			isNullValue[col_num-1] = (c==null);
			dataType[col_num-1] = CLOB;
			clearOrig(col_num);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setClob].methodExit();
		}
	}

	void setBigDecimal(int col_num, BigDecimal big) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setBigDecimal].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setBigDecimal].methodParameters(
										  "col_num = " + col_num);
		try
		{
			setNull(col_num);
			if (objectValue==null) setupObjects();
			objectValue[col_num-1] = (Object) big;
			isNullValue[col_num-1] = (big==null);
			dataType[col_num-1] = BIG_DECIMAL;
			numericValid[col_num-1] = false;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setBigDecimal].methodExit();
		}
	}
	static int extractUShort(byte[] array, int offset, boolean swap) {
		int value;
		if (swap) {
			value = ((array[offset]) & 0x00ff) | ((array[offset + 1] << 8) & 0xff00);
		} else {
			value = ((array[offset + 1]) & 0x00ff) | ((array[offset] << 8) & 0xff00);
		}
		return value & 0xffff;
	}
	public String convertSQLBigNumToBigDecimal2(byte[] sourceData,
			int scale) {
		String strVal = ""; // our final String
		boolean negative = ((sourceData[sourceData.length - 2] & 0x80) > 0);
		sourceData[sourceData.length - 2] &= 0x7F; // force sign to 0, continue
		// normally

		// we need the data in an array which can hold UNSIGNED 16 bit values
		// in java we dont have unsigned datatypes so 32-bit signed is the best
		// we can do
		int[] dataInShorts = new int[sourceData.length / 2];
		for (int i = 0; i < dataInShorts.length; i++)
			dataInShorts[i] = DataWrapper.extractUShort(sourceData, i * 2, false); // copy
		// the
		// data
		int curPos = dataInShorts.length - 1; // start at the end
		while (curPos >= 0 && dataInShorts[curPos] == 0)
		// get rid of any trailing 0's
			curPos--;
		int remainder = 0;
		long temp; // we need to use a LONG since we will have to hold up to
		// 32-bit UNSIGNED values

		// we now have the huge value stored in 2 bytes chunks
		// we will divide by 10000 many times, converting the remainder to
		// String
		// when we are left with a single chunk <10000 we will handle it using a
		// special case
		while (curPos >= 0 || dataInShorts[0] >= 10000) {
		// start on the right, divide the 16 bit value by 10000
		// use the remainder as the upper 16 bits for the next division
			for (int j = curPos; j >= 0; j--) {
				// these operations got messy when java tried to infer what size
				// to store the value in
				// leave these as separate operations for now...always casting
				// back to a 64 bit value to avoid sign problems
				temp = remainder;
				temp &= 0xFFFF;
				temp = temp << 16;
				temp += dataInShorts[j];
				dataInShorts[j] = (int) (temp / 10000);
				remainder = (int) (temp % 10000);
			}
			// if we are done with the current 16bits, move on
			if (dataInShorts[curPos] == 0)
				curPos--;

				// go through the remainder and add each digit to the final String
			for (int j = 0; j < 4; j++) {
				strVal = (remainder % 10) + strVal;
				remainder /= 10;
			}
		}
		// when we finish the above loop we still have 1 <10000 value to include
		remainder = dataInShorts[0];
		for (int j = 0; j < 4; j++) {
			strVal = (remainder % 10) + strVal;
			remainder /= 10;
		}
		BigInteger bi = new BigInteger(strVal); // create a java BigInt
		if (negative)
			bi = bi.negate();
		return new BigDecimal(bi, scale).toString(); // create a new BigDecimal with the
		// descriptor's scale
	}
	public BigDecimal convertSQLBigNumToBigDecimal(byte[] sourceData,
			int scale) {
		String strVal = ""; // our final String
		boolean negative = ((sourceData[sourceData.length - 2] & 0x80) > 0);
		sourceData[sourceData.length - 2] &= 0x7F; // force sign to 0, continue
		// normally

		// we need the data in an array which can hold UNSIGNED 16 bit values
		// in java we dont have unsigned datatypes so 32-bit signed is the best
		// we can do
		int[] dataInShorts = new int[sourceData.length / 2];
		for (int i = 0; i < dataInShorts.length; i++)
			dataInShorts[i] = DataWrapper.extractUShort(sourceData, i * 2, false); // copy
		// the
		// data
		int curPos = dataInShorts.length - 1; // start at the end
		while (curPos >= 0 && dataInShorts[curPos] == 0)
			// get rid of any trailing 0's
			curPos--;
		int remainder = 0;
		long temp; // we need to use a LONG since we will have to hold up to
		// 32-bit UNSIGNED values

		// we now have the huge value stored in 2 bytes chunks
		// we will divide by 10000 many times, converting the remainder to
		// String
		// when we are left with a single chunk <10000 we will handle it using a
		// special case
		while (curPos >= 0 || dataInShorts[0] >= 10000) {
			// start on the right, divide the 16 bit value by 10000
			// use the remainder as the upper 16 bits for the next division
			for (int j = curPos; j >= 0; j--) {
				// these operations got messy when java tried to infer what size
				// to store the value in
				// leave these as separate operations for now...always casting
				// back to a 64 bit value to avoid sign problems
				temp = remainder;
				temp &= 0xFFFF;
				temp = temp << 16;
				temp += dataInShorts[j];
				dataInShorts[j] = (int) (temp / 10000);
				remainder = (int) (temp % 10000);
			}
			// if we are done with the current 16bits, move on
			if (dataInShorts[curPos] == 0)
				curPos--;
			// go through the remainder and add each digit to the final String
			for (int j = 0; j < 4; j++) {
				strVal = (remainder % 10) + strVal;
				remainder /= 10;
			}
		}
		// when we finish the above loop we still have 1 <10000 value to include
		remainder = dataInShorts[0];
		for (int j = 0; j < 4; j++) {
			strVal = (remainder % 10) + strVal;
			remainder /= 10;
		}

		BigInteger bi = new BigInteger(strVal); // create a java BigInt
		if (negative)
			bi = bi.negate();
		return new BigDecimal(bi, scale); // create a new BigDecimal with the
		// descriptor's scale
	}
	byte [] convertBigDecimalToSQLBigNum(int col_num, int scale, int targetLength) throws SQLException {
		if(objectValue == null)  throw new SQLException("No value of BigDecimal set on this column number: " + col_num);
		if(objectValue.length <  col_num) throw new SQLException("Invalid Column index specified: " + col_num);
		if(!(objectValue[col_num - 1] instanceof BigDecimal)) throw new SQLException("Value at specified index is not of a BigDecimal type");
		((BigDecimal)objectValue[col_num - 1]).setScale(scale, BigDecimal.ROUND_DOWN).unscaledValue().toString().getBytes();
		byte[] sourceData = ((BigDecimal)objectValue[col_num - 1]).setScale(scale, BigDecimal.ROUND_DOWN).unscaledValue().toString().getBytes();
		// trailing
		// 0s,
		// remove decimal point,
		// get the chars
		byte[] targetData = new byte[targetLength];
		int[] targetInShorts = new int[targetLength / 2];
		int length;
		int temp;
		int tarPos = 1;
		// remove leading 0s and sign character
		int zeros = 0;
		while (zeros < sourceData.length
				&& (sourceData[zeros] == '0' || sourceData[zeros] == '-'))
			zeros++;
		// convert from characters to values
		for (int i = zeros; i < sourceData.length; i++)
			sourceData[i] -= '0';
		length = sourceData.length - zeros; // we have a new length
		// iterate through 4 bytes at a time
		for (int i = 0; i < length; i += 4) {
			int temp1 = 0;
			int j = 0;
			// get 4 bytes worth of data or as much that is left
			for (j = 0; j < 4 && i + j < length; j++)
				temp1 = temp1 * 10 + sourceData[zeros + i + j];
			int power = powersOfTen[j - 1]; // get the power of ten based on how
			// many digits we got
			temp = targetInShorts[0] * power + temp1; // move the current
			// digits over and then
			// add our new value in
			targetInShorts[0] = temp & 0xFFFF; // we save only up to 16bits --
			// the rest gets carried over

			// we do the same thing for the rest of the digits now that we have
			// an upper bound
			for (j = 1; j < targetInShorts.length; j++) {
				int t = (temp & 0xFFFF0000) >> 16;
				temp = targetInShorts[j] * power + t;
				targetInShorts[j] = temp & 0xFFFF;
			}
			int carry = (temp & 0xFFFF0000) >> 16;
			if (carry > 0) {
				targetInShorts[tarPos++] = carry;
			}
		}
		for (int i = 0; i < targetInShorts.length; i++) {
			targetData[i * 2] = (byte) ((targetInShorts[i] & 0xFF00) >> 8);
			targetData[i * 2 + 1] = (byte) (targetInShorts[i] & 0xFF);
		}
		// add sign
		if ((((BigDecimal)objectValue[col_num - 1]).signum() < 0))
			targetData[targetData.length - 2] |= 0x80;
		return targetData;
	}

	void setString(int col_num, String s) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setString].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setString].methodParameters(
										  "col_num = " + col_num + " s='" + s + "'");
		try
		{
			setNull(col_num);
			if (objectValue==null) setupObjects();
			objectValue[col_num-1] = (Object) s;
			isNullValue[col_num-1] = (s==null);
			dataType[col_num-1] = STRING;
			numericValid[col_num-1] = false;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setString].methodExit();
		}
	}

	void setBytes(int col_num, byte[] b) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setBytes].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setBytes].methodParameters(
										  "col_num = " + col_num);
		try
		{
			setNull(col_num);
			if (bytesValue==null) setupBytes();
			bytesValue[col_num-1] = b;
			isNullValue[col_num-1] = (b==null);
			dataType[col_num-1] = BYTES;
			numericValid[col_num-1] = false;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setBytes].methodExit();
		}
	}
    void setSQLBytes(int col_num, byte[] b) throws SQLException
    {
        if (SQLbytesValue==null) setupSQLBytes();
        SQLbytesValue[col_num-1] = b;
    }

	byte getDataType(int col_num)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getDataType].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getDataType].methodParameters(
										  "col_num = " + col_num);
		try
		{
			return dataType[col_num-1];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getDataType].methodExit();
		}
	}

	private void updateColumn(int col_num) throws SQLException
	{
		// Update the column if it was set by only the value and type variables
		// This is done by the JNI for speed
		if (JdbcDebugCfg.entryActive) debug[methodId_updateColumn].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_updateColumn].methodParameters(
										  "col_num = " + col_num);
		try
		{
            setSQLBytes(col_num,SQLbytesValue[col_num-1]); 

			if (setNeeded[col_num-1])
			{
				setNeeded[col_num-1] = false;
				byte data_type = dataType[col_num-1];
				dataType[col_num-1] = UNKNOWN;
				switch (data_type)
				{
					case UNKNOWN:
						return;
					case BYTE:
						setByte(col_num,byteValue[col_num-1]);
						return;
					case SHORT:
						setShort(col_num,shortValue[col_num-1]);
						return;
					case INTEGER:
						setInt(col_num,intValue[col_num-1]);
						return;
					case LONG:
						setLong(col_num,longValue[col_num-1]);
						return;
					case FLOAT:
						setFloat(col_num,floatValue[col_num-1]);
						return;
					case DOUBLE:
						setDouble(col_num,doubleValue[col_num-1]);
						return;
					case BOOLEAN:
						setBoolean(col_num,booleanValue[col_num-1]);
						return;
					case STRING:
						setString(col_num,(String)objectValue[col_num-1]);
						return;
					case BYTES:
						setBytes(col_num,bytesValue[col_num-1]);
						return;
					case BLOB:
						setBlob(col_num,(Blob)objectValue[col_num-1]);
						return;
					case CLOB:
						setClob(col_num,(Clob)objectValue[col_num-1]);
						return;
					case BIG_DECIMAL:
						setBigDecimal(col_num,(BigDecimal)objectValue[col_num-1]);
						return;
					case OBJECT:
						setObject(col_num,objectValue[col_num-1]);
						return;
					default:
						// Leave as UNKNOWN.
						return;
				}
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateColumn].methodExit();
		}
	}

	String getString(int col_num) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getString].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getString].methodParameters(
										  "col_num = " + col_num);
		try
		{
			if (setNeeded[col_num-1]) updateColumn(col_num);
			if (isNullValue[col_num-1])
			{
				if (JdbcDebugCfg.traceActive)
					debug[methodId_getString].traceOut(JdbcDebug.debugLevelEntry,"isNull true");
				return(null);
			}

			// We will be accessing the object value of the column, so
			//    create the object array now if it has not been created yet
			//    so we know that the array exists in the later logic.
			if (objectValue==null) setupObjects();

			// Convert values to string.
			switch (dataType[col_num-1])
			{
				case STRING:
					// Just return the string
					break;
				case BYTE:
				case SHORT:
				case INTEGER:
				case LONG:
					// If the object is set for numeric types, it was converted before.
					// If it is not set, need to convert the value to a string.
					if (objectValue[col_num-1]==null)
					{
						objectValue[col_num-1] = Long.toString(longValue[col_num-1]);
					}
					break;
				case BOOLEAN:
					// Return numeric for boolean rather than
					//   "true" or "false" since numeric is used
					//   more in driver.
					if (objectValue[col_num-1]==null)
					{
						if (booleanValue[col_num-1]) objectValue[col_num-1] = "1";
						else objectValue[col_num-1] = "0";
					}
					break;
				case DOUBLE:
					// Return floating point value
					if (objectValue[col_num-1]==null)
					{
						objectValue[col_num-1] = Double.toString(doubleValue[col_num-1]);
					}
					break;
				case FLOAT:
					// Return floating point value
					if (objectValue[col_num-1]==null)
					{
						objectValue[col_num-1] = Float.toString(floatValue[col_num-1]);
					}
					break;
				case BYTES:
					{
					String rc = new String(bytesValue[col_num-1]);
					if (JdbcDebugCfg.traceActive)
						debug[methodId_getString].traceOut(JdbcDebug.debugLevelEntry,"Byte Array = " + rc);
					return rc;
				}
				case BIG_DECIMAL:
					{
					String rc = ((BigDecimal) objectValue[col_num-1]).toString();
					if (JdbcDebugCfg.traceActive)
						debug[methodId_getString].traceOut(JdbcDebug.debugLevelEntry,"BigDecimal = " + rc);
					return rc;
				}
				default:
					// No supported conversion
					if (JdbcDebugCfg.traceActive)
						debug[methodId_getString].traceOut(JdbcDebug.debugLevelEntry,"Unsupported type");
					return null;
			}
			if (JdbcDebugCfg.traceActive)
				debug[methodId_getString].traceOut(JdbcDebug.debugLevelEntry,"String = " + (String) objectValue[col_num-1]);
			return (String) objectValue[col_num-1];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getString].methodExit();
		}
	}

	byte[] getBytes(int col_num)  throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBytes].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getBytes].methodParameters(
										  "col_num = " + col_num);
		try
		{
			if (setNeeded[col_num-1]) updateColumn(col_num);
			if (isNullValue[col_num-1]) return(null);

			if (dataType[col_num-1]==STRING) return(((String) objectValue[col_num-1]).getBytes());

			return(bytesValue[col_num-1]);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBytes].methodExit();
		}
	}

	boolean isNumeric(int col_num)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_isNumeric].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_isNumeric].methodParameters(
										  "col_num = " + col_num);
		try
		{
			switch (dataType[col_num-1])
			{
				case BYTE:
				case SHORT:
				case INTEGER:
				case LONG:
				case FLOAT:
				case DOUBLE:
				case BOOLEAN:
					return(true);
			}
			return(false);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_isNumeric].methodExit();
		}
	}

	Object getObject(int col_num) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getObject].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getObject].methodParameters(
										  "col_num = " + col_num);
		try
		{
			if (setNeeded[col_num-1]) updateColumn(col_num);
			if (isInsertRow && (dataType[col_num-1]==UNKNOWN))
				throw new SQLException("No value has been inserted");
			if (isNullValue[col_num-1]) return(null);

			// Returns the Java wrapper equivalent of the
			//   data wrapper
			Object rc = null;
			switch (dataType[col_num-1])
			{
				case BYTE:
					rc = new Byte(byteValue[col_num-1]);
					break;
				case SHORT:
					rc = new Short(shortValue[col_num-1]);
					break;
				case INTEGER:
					rc = new Integer(intValue[col_num-1]);
					break;
				case LONG:
					rc = new Long(longValue[col_num-1]);
					break;
				case FLOAT:
					rc = new Float(floatValue[col_num-1]);
					break;
				case DOUBLE:
					rc = new Double(doubleValue[col_num-1]);
					break;
				case BOOLEAN:
					rc = new Boolean(booleanValue[col_num-1]);
					break;
				case BYTES:
					if (bytesValue!=null) rc = bytesValue[col_num-1];
					break;
				default:
					if (objectValue!=null) rc = objectValue[col_num-1];
			}
			if (JdbcDebugCfg.traceActive)
			{
				if (rc==null) debug[methodId_getString].traceOut(JdbcDebug.debugLevelEntry,"Returning null for " + getDataTypeString(col_num));
				else debug[methodId_getString].traceOut(JdbcDebug.debugLevelEntry,"Returning " + rc.toString() + " for " + getDataTypeString(col_num));
			}
			return(rc);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getObject].methodExit();
		}
	}

	void setObject(int col_num, Object obj) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setObject].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_setObject].methodParameters(
										  "col_num = " + col_num + ", object=" + obj.toString());
		try
		{
			if (obj==null) setNull(col_num);
			else if (obj instanceof Byte) setByte(col_num, ((Byte)obj).byteValue());
			else if (obj instanceof Short) setShort(col_num, ((Short)obj).shortValue());
			else if (obj instanceof Integer) setInt(col_num, ((Integer)obj).intValue());
			else if (obj instanceof Long) setLong(col_num, ((Long)obj).longValue());
			else if (obj instanceof Float) setFloat(col_num, ((Float)obj).floatValue());
			else if (obj instanceof Double) setDouble(col_num, ((Double)obj).doubleValue());
			else if (obj instanceof Boolean) setBoolean(col_num, ((Boolean)obj).booleanValue());
			else if (obj instanceof String) setString(col_num, (String) obj);
			else if (obj instanceof Blob) setBlob(col_num, (Blob) obj);
			else if (obj instanceof Clob) setClob(col_num, (Clob) obj);
			else if (obj instanceof BigDecimal) setBigDecimal(col_num, (BigDecimal) obj);
			else if (obj instanceof byte[]) setBytes(col_num,(byte[]) obj);
			else if (obj instanceof DataWrapper)
			{
				DataWrapper wrapper = (DataWrapper) obj;
				setNull(col_num);
				clearOrig(col_num);
				dataType[col_num-1] = wrapper.dataType[0];
				byteValue[col_num-1] = wrapper.byteValue[0];
				shortValue[col_num-1] = wrapper.shortValue[0];
				intValue[col_num-1] = wrapper.intValue[0];
				longValue[col_num-1] = wrapper.longValue[0];
				floatValue[col_num-1] = wrapper.floatValue[0];
				doubleValue[col_num-1] = wrapper.doubleValue[0];
				booleanValue[col_num-1] = wrapper.booleanValue[0];
				numericValid[col_num-1] = wrapper.numericValid[0];
				if (wrapper.objectValue!=null)
				{
					if (wrapper.objectValue[0]!=null)
					{
						if (objectValue==null) setupObjects();
						objectValue[col_num-1] = wrapper.objectValue[0];
					}
				}
				if (wrapper.bytesValue!=null)
				{
					if (wrapper.bytesValue[col_num-1]!=null)
					{
						if (bytesValue==null) setupBytes();
						bytesValue[col_num-1] = wrapper.bytesValue[0];
					}
				}
				isNullValue[col_num-1] = wrapper.isNullValue[0];
                if (wrapper.SQLbytesValue!=null)
                {
                    if (wrapper.SQLbytesValue[col_num-1]!=null)
                    {
                        if (SQLbytesValue==null) setupSQLBytes();
                        SQLbytesValue[col_num-1] = wrapper.SQLbytesValue[0];
                    }
                }

			}
			else
			{
				setNull(col_num);
				if (objectValue==null) setupObjects();
				objectValue[col_num-1] = obj;
				/* A check for isNullValue is added here, which is set true in setNull method. This
				 will set the value depending on the obj value */
				isNullValue[col_num-1] = (obj==null);
				dataType[col_num-1] = OBJECT;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setObject].methodExit();
		}
	}


	private void convertStringToNumeric(int col_num, boolean allowBoolean, Locale locale, String value) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_convertStringToNumeric].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_convertStringToNumeric].methodParameters(
										  "col_num = " + col_num +
										  " value=" + value);
		try
		{
			String trim_value = value.trim();
			boolean convert = false;
			if (allowBoolean)
			{
				// If allowing boolean, check for 'true' and 'false' first
				if (trim_value.compareToIgnoreCase("true")==0) setDouble(col_num,1);
				else if (trim_value.compareToIgnoreCase("false")==0) setDouble(col_num,0);
				else convert = true;
			}
			else convert = true;

			if (convert)
			{
				// Try to convert the string into a double
				double dbl_value;
				long long_value;

				float float_value = 0;
				BigDecimal tmpVal;
				try
				{
					//dbl_value = parseDouble(locale,trim_value);
					//long_value = (long) dbl_value;
					tmpVal = new BigDecimal(trim_value);
					dbl_value = tmpVal.doubleValue();
					long_value = tmpVal.longValue();
					float_value = tmpVal.floatValue();

				}
				catch (NumberFormatException e)
				{
					/*  removing try,catch since this is not required
					 *

					// Cannot convert value to a double.  See if it is a
					//   bad numeric or out of range.
					try
					{
						BigDecimal big = new BigDecimal(trim_value);
						// Value is valid but out of range of a double
						// BigDecimal will return either positive or negative
						//   infinity.
						dbl_value = big.doubleValue();
						long_value = big.longValue();
					}
					catch (NumberFormatException e1)
					{
					*/	// Bad numeric
						throw Messages.createSQLException(locale,
							"invalid_cast_specification", null);
				}
				setLong(col_num,long_value);
				doubleValue[col_num-1] = dbl_value;
				//floatValue[col_num-1] = (float) dbl_value;
				floatValue[col_num-1] = float_value;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_convertStringToNumeric].methodExit();
		}
	}

	private void convertToNumeric(int col_num, boolean allowBoolean, Locale locale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_convertToNumeric].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_convertToNumeric].methodParameters(
										  "col_num = " + col_num);
		try
		{
			// For character types, convert to numeric and update the numeric column values
			if (!isNullValue[col_num-1])
			{
				String save_string;
				Object save_orig;
				switch (dataType[col_num-1])
				{
					case STRING:
						// Convert the string to a double and set it
						save_orig = getOrigValue(col_num);
						save_string = (String) objectValue[col_num-1];
						convertStringToNumeric(col_num, allowBoolean, locale, save_string);
						// Now the numeric values are up to date, make the column
						//   back into a string
						objectValue[col_num-1] = (Object) save_string;
						dataType[col_num-1] = STRING;
						setOrig(col_num,save_orig);
						break;
					case BYTES:
						// Convert the byte array into a double and set it
						save_orig = getOrigValue(col_num);
						byte[] save_bytes = bytesValue[col_num-1];
						convertStringToNumeric(col_num, allowBoolean, locale, new String(save_bytes));
						// Now the numeric values are up to date, make the column
						//   back into a byte array
						bytesValue[col_num-1] = save_bytes;
						dataType[col_num-1] = BYTES;
						setOrig(col_num,save_orig);
						break;
					case BIG_DECIMAL:
						save_orig = getOrigValue(col_num);
						BigDecimal save_big = (BigDecimal) objectValue[col_num-1];
						setDouble(col_num, save_big.doubleValue());
						setLong(col_num, save_big.longValue());
						doubleValue[col_num-1] = save_big.doubleValue();
						floatValue[col_num-1] = (float) doubleValue[col_num-1];
						objectValue[col_num-1] = save_big;
						dataType[col_num-1] = BIG_DECIMAL;
						setOrig(col_num,save_orig);
						break;
				}
				numericValid[col_num-1] = true;
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_convertToNumeric].methodExit();
		}
	}

	boolean getBoolean(int col_num, Locale locale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getBoolean].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getBoolean].methodParameters(
										  "col_num = " + col_num);
		try
		{
			if (setNeeded[col_num-1]) updateColumn(col_num);
			if (isNullValue[col_num-1]) return false;
			if (!numericValid[col_num-1]) convertToNumeric(col_num,true,locale);
			if (doubleValue[col_num-1]==0) return false;
			if (doubleValue[col_num-1]==1) return true;
			throw Messages.createSQLException(locale,
				"numeric_out_of_range", null);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getBoolean].methodExit();
		}
	}

	static boolean parseBoolean(Locale locale, String s) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_parseBoolean].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_parseBoolean].methodParameters(
										  "s = " + s);
		try
		{
			s = s.trim();
			if (s.equalsIgnoreCase("true")) return true;
			if (s.equalsIgnoreCase("false")) return false;
			try
			{
				double doubleValue = Double.parseDouble(s);
				if (doubleValue==0) return false;
				if (doubleValue==1) return true;
				throw Messages.createSQLException(locale,
					"numeric_out_of_range", null);
			}
			catch (NumberFormatException e)
			{
				throw Messages.createSQLException(locale,
					"invalid_cast_specification", null);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_parseBoolean].methodExit();
		}
	}

	boolean outOfRange(int col_num, long min_value, long max_value)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_outOfRange].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_outOfRange].methodParameters(
										  "col_num = " + col_num + ", min_value = " + min_value + ", max_value = " + max_value +
										  " value=" + doubleValue[col_num-1]);
		try
		{
			if (dataType[col_num-1] == DOUBLE)
				return ((doubleValue[col_num-1] < (double)min_value) ||
					(doubleValue[col_num-1] > (double)max_value));
			if (dataType[col_num-1] == FLOAT)
				return ((floatValue[col_num-1] < (float)min_value) ||
					(floatValue[col_num-1] > (float)max_value));
			return ((longValue[col_num-1] < min_value) ||
				(longValue[col_num-1] > max_value));
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_outOfRange].methodExit();
		}
	}

	byte getByte(int col_num, Locale locale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getByte].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getByte].methodParameters(
										  "col_num = " + col_num);
		try
		{
			if (setNeeded[col_num-1]) updateColumn(col_num);
			if (isNullValue[col_num-1]) return 0;
			if (!numericValid[col_num-1]) convertToNumeric(col_num,false,locale);
			if (outOfRange(col_num, Byte.MIN_VALUE, Byte.MAX_VALUE))
				throw Messages.createSQLException(locale,
					"numeric_out_of_range", null);
			dataTruncation[col_num-1] = ((double)byteValue[col_num-1] != doubleValue[col_num-1]);
			if (JdbcDebugCfg.traceActive) debug[methodId_getByte].methodReturn("byteValue[" + col_num + "]= " + byteValue[col_num-1] +
											  "dataTruncation=" + dataTruncation);
			return byteValue[col_num-1];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getByte].methodExit();
		}
	}

	byte parseByte(Locale locale, String s) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_parseByte].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_parseByte].methodParameters(
										  "s = " + s);
		try
		{
			double dvalue;
			try
			{
				dvalue = Double.parseDouble(s.trim());
			}
			catch (NumberFormatException e)
			{
				throw Messages.createSQLException(locale,
					"invalid_cast_specification", null);
			}
			if ((dvalue < (double)Byte.MIN_VALUE) ||
				(dvalue > (double)Byte.MAX_VALUE))
				throw Messages.createSQLException(locale, "numeric_out_of_range", null);
			byte bvalue = (byte) dvalue;
			dataTruncation[0] = ((double)bvalue != dvalue);
			if (JdbcDebugCfg.traceActive) debug[methodId_parseByte].methodReturn("bvalue=" + bvalue +
											  "dataTruncation=" + dataTruncation[0]);
			return bvalue;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_parseByte].methodExit();
		}
	}

	short getShort(int col_num, Locale locale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getShort].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getShort].methodParameters(
										  "col_num = " + col_num);
		try
		{
			if (setNeeded[col_num-1]) updateColumn(col_num);
			if (isNullValue[col_num-1]) return 0;
			if (!numericValid[col_num-1]) convertToNumeric(col_num,false,locale);
			if (outOfRange(col_num, Short.MIN_VALUE, Short.MAX_VALUE))
				throw Messages.createSQLException(locale,
					"numeric_out_of_range", null);
			dataTruncation[col_num-1] = ((double)shortValue[col_num-1] != doubleValue[col_num-1]);
			if (JdbcDebugCfg.traceActive) debug[methodId_getShort].methodReturn("shortValue[" + col_num + "]= " + shortValue[col_num-1] +
											  "dataTruncation=" + dataTruncation[col_num-1]);
			return shortValue[col_num-1];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getShort].methodExit();
		}
	}

	short parseShort(Locale locale, String s) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_parseShort].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_parseShort].methodParameters(
										  "s = " + s);
		try
		{
			try
			{
				double dvalue = Double.parseDouble(s.trim());
				if ((dvalue < (double)Short.MIN_VALUE) ||
					(dvalue > (double)Short.MAX_VALUE))
					throw Messages.createSQLException(locale,
						"numeric_out_of_range", null);
				short svalue = (short) dvalue;
				boolean isTruncated = ((double)svalue != dvalue);
				if (JdbcDebugCfg.traceActive) debug[methodId_parseShort].methodReturn("svalue=" + svalue +
												  "dataTruncation=" + isTruncated);
				return svalue;
			}
			catch (NumberFormatException e)
			{
				throw Messages.createSQLException(locale,
					"invalid_cast_specification", null);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_parseShort].methodExit();
		}
	}

	int getInt(int col_num, Locale locale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getInt].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getInt].methodParameters(
										  "col_num = " + col_num);
		try
		{
			if (setNeeded[col_num-1]) updateColumn(col_num);
			if (isNullValue[col_num-1]) return 0;
			if (!numericValid[col_num-1]) convertToNumeric(col_num,false,locale);
			if (outOfRange(col_num, Integer.MIN_VALUE, Integer.MAX_VALUE))
				throw Messages.createSQLException(locale,
					"numeric_out_of_range", null);
			dataTruncation[col_num-1] = ((double)intValue[col_num-1] != doubleValue[col_num-1]);
			if (JdbcDebugCfg.traceActive) debug[methodId_getInt].methodReturn("intValue[" + (col_num-1) + "]= " + intValue[col_num-1] +
											  " dataTruncation=" + dataTruncation[col_num-1]);
			return intValue[col_num-1];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getInt].methodExit();
		}
	}

	int parseInt(Locale locale, String s) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_parseInt].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_parseInt].methodParameters(
										  "s = " + s);
		try
		{
			try
			{
				double dvalue = Double.parseDouble(s.trim());
				if ((dvalue < (double)Integer.MIN_VALUE) ||
					(dvalue > (double)Integer.MAX_VALUE))
					throw Messages.createSQLException(locale,
						"numeric_out_of_range", null);
				int ivalue = (int) dvalue;
				boolean isTruncated = ((double)ivalue != dvalue);
				if (JdbcDebugCfg.traceActive) debug[methodId_parseInt].methodReturn("ivalue=" + ivalue +
												  "dataTruncation=" + isTruncated);
				return ivalue;
			}
			catch (NumberFormatException e)
			{
				throw Messages.createSQLException(locale,
					"invalid_cast_specification", null);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_parseInt].methodExit();
		}
	}

	long getLong(int col_num, Locale locale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getLong].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getLong].methodParameters(
										  "col_num = " + col_num);
		try
		{
			if (setNeeded[col_num-1]) updateColumn(col_num);
			if (isNullValue[col_num-1]) return 0;
			if (!numericValid[col_num-1]) convertToNumeric(col_num,false,locale);
			switch (dataType[col_num-1])
			{
				case STRING:
				case BYTES:
				case BIG_DECIMAL:
				case DOUBLE:
					{
					// Check range for the double value
					if ((doubleValue[col_num-1] > (double) Long.MAX_VALUE) ||
						(doubleValue[col_num-1] < (double) Long.MIN_VALUE))
					{
						Object[] errorParmList = new Object[2];
						errorParmList[0]	   = getString(col_num);
						errorParmList[1]	   = new Integer(col_num);
						throw Messages.createSQLException(locale,
							"numeric_value_out_of_range", errorParmList);
					}
					dataTruncation[col_num-1] = (doubleValue[col_num-1] != longValue[col_num-1]);
					break;
				}


				case FLOAT:
					{
					if ((floatValue[col_num-1] > (double) Long.MAX_VALUE) ||
						(floatValue[col_num-1] < (double) Long.MIN_VALUE))
					{
						Object[] errorParmList = new Object[2];
						errorParmList[0]	   = getString(col_num);
						errorParmList[1]	   = new Integer(col_num);
						throw Messages.createSQLException(locale,
							"numeric_value_out_of_range", errorParmList);
					}
					dataTruncation[col_num-1] = (floatValue[col_num-1] != longValue[col_num-1]);
					break;
				}
				default:
					dataTruncation[col_num-1] = false;
			}
			if (JdbcDebugCfg.traceActive) debug[methodId_getLong].methodReturn("longValue[" + (col_num-1) + "]= " + longValue[col_num-1] +
											  " dataTruncation=" + dataTruncation[col_num-1]);
			return longValue[col_num-1];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getLong].methodExit();
		}
	}

	long parseLong(Locale locale, String s, int columnIndex) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_parseLong].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_parseLong].methodParameters(
										  "s = " + s + ", columnIndex = " + columnIndex);
		try
		{
			long retValue;

			boolean isTruncated;
			// Try to use the Long parser to catch errors.  If the string will be truncated or
			//    is out of range, the parse will fail and we will have to use BigDecimal to
			//    determine the result.
			try
			{
				retValue = Long.parseLong(s.trim());
			}
			catch (NumberFormatException e)
			{
				BigDecimal bigValue;
				try
				{
					bigValue = new BigDecimal(s.trim());
				}
				catch (NumberFormatException e1)
				{
					Object[] columnNumber = new Object[1];
					columnNumber[0]		  = new Integer(columnIndex);
					throw Messages.createSQLException(locale,
						"sql_conversion_not_allowed",
						columnNumber);
				}
				if ((bigValue.compareTo(MinLong) < 0) ||
					(bigValue.compareTo(MaxLong) > 0))
				{
					Object[] errorParmList = new Object[2];
					errorParmList[0]	   = s.trim();
					errorParmList[1]	   = new Integer(columnIndex);
					throw Messages.createSQLException(locale,
						"numeric_value_out_of_range", errorParmList);
				}
				retValue = bigValue.longValue();
				isTruncated = (retValue != bigValue.doubleValue());
				if (JdbcDebugCfg.traceActive) debug[methodId_getLong].methodReturn("parseValue=" + retValue +
												  "dataTruncation=" + isTruncated);
				return retValue;
			}
			isTruncated = false;
			if (JdbcDebugCfg.traceActive) debug[methodId_getLong].methodReturn("parseValue=" + retValue +
											  "dataTruncation=" + isTruncated);
			return retValue;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_parseLong].methodExit();
		}
	}

	double getDouble(int col_num, Locale locale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getDouble].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getDouble].methodParameters(
										  "col_num = " + col_num);
		try
		{
			if (setNeeded[col_num-1]) updateColumn(col_num);
			if (isNullValue[col_num-1]) return 0.0;
			if (!numericValid[col_num-1]) convertToNumeric(col_num,false,locale);
			if (JdbcDebugCfg.traceActive) debug[methodId_getDouble].methodReturn("doubleValue[" + col_num + "]= " + doubleValue[col_num-1]);
			if ((doubleValue[col_num-1]==Double.POSITIVE_INFINITY) ||
				(doubleValue[col_num-1]==Double.NEGATIVE_INFINITY))
			{
				// If infinity, may be out of range string or BigDecimal
				// Need to check.
				BigDecimal big = null;
				if (dataType[col_num-1]==BIG_DECIMAL)
				{
					big = (BigDecimal) objectValue[col_num-1];
				}
				else if (dataType[col_num-1]==STRING)
				{
					big = new BigDecimal((String) objectValue[col_num-1]);
				}
				else if (dataType[col_num-1]==BYTES)
				{
					big = new BigDecimal(new String(bytesValue[col_num-1]));
				}
				if (big!=null)
				{
					// We have the wrapper value as a BigDecimal.
					// We need to check range.
					if ((big.compareTo(MinDouble)<0) ||
						(big.compareTo(MaxDouble)>0))
					{
						Object[] errorParmList = new Object[2];
						errorParmList[0]	   = big.toString();
						errorParmList[1]	   = new Integer(col_num);
						throw Messages.createSQLException(locale,
							"numeric_value_out_of_range", errorParmList);
					}
				}
			}
			return doubleValue[col_num-1];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getDouble].methodExit();
		}
	}

	static double parseDouble(Locale locale, String s) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_parseDouble].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_parseDouble].methodParameters(
										  "s = " + s);
		try
		{
			s = s.trim();
			if (s.equals("Inf")) return Double.POSITIVE_INFINITY;
			if (s.equals("-Inf")) return Double.NEGATIVE_INFINITY;
			try
			{
				double dvalue = Double.parseDouble(s);
				if (JdbcDebugCfg.traceActive) debug[methodId_parseDouble].methodReturn("dvalue=" + dvalue);
				return dvalue;
			}
			catch (NumberFormatException e)
			{
				throw Messages.createSQLException(locale,
					"invalid_cast_specification", null);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_parseDouble].methodExit();
		}
	}

	float getFloat(int col_num, Locale locale) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getFloat].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_getFloat].methodParameters(
										  "col_num = " + col_num);
		try
		{
			if (setNeeded[col_num-1]) updateColumn(col_num);
			if (isNullValue[col_num-1]) return 0.0f;
			if (!numericValid[col_num-1]) convertToNumeric(col_num,false,locale);
			if ((floatValue[col_num-1]==Float.POSITIVE_INFINITY) ||
				(floatValue[col_num-1]==Float.NEGATIVE_INFINITY))
			{
				if (JdbcDebugCfg.traceActive) debug[methodId_getFloat].methodReturn("floatValue=" + floatValue[col_num-1]);
				return floatValue[col_num-1];
			}

			if (Math.abs(doubleValue[col_num-1]) > FLOAT_MAX)
				throw Messages.createSQLException(locale,
					"numeric_out_of_range", null);

			if (JdbcDebugCfg.traceActive) debug[methodId_getFloat].methodReturn("floatValue=" + floatValue[col_num-1]);
			return floatValue[col_num-1];
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getFloat].methodExit();
		}
	}

	static float parseFloat(Locale locale, String s) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_parseFloat].methodEntry();
		if (JdbcDebugCfg.traceActive) debug[methodId_parseFloat].methodParameters(
										  "s = " + s);
		try
		{
			s = s.trim();
			if (s.equals("Inf")) return Float.POSITIVE_INFINITY;
			if (s.equals("-Inf")) return Float.NEGATIVE_INFINITY;
			try
			{
				double dvalue = Double.parseDouble(s);
				if (Math.abs(dvalue) > FLOAT_MAX)
					throw Messages.createSQLException(locale,
						"numeric_out_of_range", null);
				float fvalue = (float) dvalue;
				if (JdbcDebugCfg.traceActive) debug[methodId_parseFloat].methodReturn("value=" + fvalue);
				return fvalue;
			}
			catch (NumberFormatException e)
			{
				throw Messages.createSQLException(locale,
					"invalid_cast_specification", null);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_parseFloat].methodExit();
		}
	}

	boolean getUpdated()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getUpdated].methodEntry();
		try
		{
			return updated;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getUpdated].methodExit();
		}
	}

	void clearUpdated() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_clearUpdated].methodEntry();
		try
		{
			updated = false;
			for(int col_idx = 0; col_idx < totalColumns; col_idx++)
			{
				// Restore original value
				Object orig = getOrigValue(col_idx+1);
				if (orig!=this)
				{
					// Value was changed.  Set back to original.
					setObject(col_idx+1,orig);
					// Clear original value since it is the same as current
					clearOrig(col_idx+1);
				}
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_clearUpdated].methodExit();
		}
	}

	boolean getDeleted()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getDeleted].methodEntry();
		try
		{
			return deleted;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getDeleted].methodExit();
		}
	}

	boolean getInserted()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_getInserted].methodEntry();
		try
		{
			return inserted;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_getInserted].methodExit();
		}
	}

	void setInserted()
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_setInserted].methodEntry();
		try
		{
			inserted = true;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_setInserted].methodExit();
		}
	}

	void deleteRow(Locale locale, SQLMXPreparedStatement deleteStmt, BitSet paramCols) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_deleteRow].methodEntry();
		try
		{
			int col_idx;
			int param_idx;
			int count;

			for (col_idx = 0, param_idx = 0; col_idx < totalColumns ; col_idx++)
			{
				if (paramCols.get(col_idx))
					deleteStmt.setObject(++param_idx, getOriginalObject(col_idx+1));
			}
			count =	deleteStmt.executeUpdate();
			if (count == 0)
				throw Messages.createSQLException(locale, "row_modified", null);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_deleteRow].methodExit();
		}
	}

	void insertRow(SQLMXPreparedStatement insertStmt, BitSet paramCols) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_insertRow].methodEntry();
		try
		{
			int col_idx;
			int param_idx;

			for (col_idx = 0, param_idx = 0; col_idx < totalColumns ; col_idx++)
			{
				if (paramCols.get(col_idx))
					insertStmt.setObject(++param_idx, getOriginalObject(col_idx+1));
			}
			insertStmt.execute();
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_insertRow].methodExit();
		}

	}

	void updateRow(Locale locale, SQLMXPreparedStatement updateStmt, BitSet paramCols, BitSet keyCols) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_updateRow].methodEntry();
		try
		{
			int col_idx;
			int param_idx;
			int count;
			Object obj;
			int numPKey=0;
			int loc=0;
			int pKeyCounter=1;

			for (col_idx = 0; col_idx < totalColumns; col_idx++)
			{
				if(keyCols.get(col_idx)) numPKey++;
			}

			loc = totalColumns - numPKey;

			for (col_idx = 0, param_idx = 0; col_idx < totalColumns; col_idx++)
			{
				Object orig_obj = getOrigValue(col_idx+1);
				if (keyCols.get(col_idx))
				{
					// If original object has been saved, the column was updated
					if (orig_obj!=this)
						throw Messages.createSQLException(locale, "primary_key_not_updateable", null);
					updateStmt.setObject((loc+pKeyCounter),getObject(col_idx+1));
					pKeyCounter++;
				}
				else
				{
					obj = getObject(col_idx+1);
/* TODO: Selva
					if (obj instanceof SQLMXLob)
					{
						if (orig_obj==this)
						{
							// LOB has not been updated
							updateStmt.setLong(++param_idx, ((SQLMXLob)obj).dataLocator_);
							continue;
						}
					}
*/
					updateStmt.setObject(++param_idx, obj);
				}
			}

			count = updateStmt.executeUpdate();
			if (count == 0)
				throw Messages.createSQLException(locale, "row_modified", null);

			// Clearing original makes the current the only value
			for(col_idx = 0; col_idx < totalColumns; col_idx++) clearOrig(col_idx+1);
			updated = true;
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_updateRow].methodExit();
		}
	}

	void refreshRow(Locale locale, SQLMXPreparedStatement selectStmt, BitSet selectCols, BitSet keyCols) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_refreshRow].methodEntry();
		try
		{
			int col_idx;
			int param_idx;
			SQLMXResultSet rs;
			//SQLMXResultSetMetaData rsmd;

			clearUpdated();

			for (col_idx = 0, param_idx = 0; col_idx < totalColumns ; col_idx++)
			{
				if (keyCols.get(col_idx))
					selectStmt.setObject(++param_idx, getOriginalObject(col_idx+1));
			}

			rs = (SQLMXResultSet) selectStmt.executeQuery();
			if (rs != null)
			{
				try
				{
					//rsmd = (SQLMXResultSetMetaData) rs.getMetaData();
					rs.next();
					for (col_idx = 0, param_idx = 0 ; col_idx < totalColumns ; col_idx++)
					{
						// Update the column value with the refreshed value
						if (selectCols.get(col_idx))
						{
							// Set the new value
							setObject(col_idx+1,rs.getObject(++param_idx));
							// Make it the original
							clearOrig(col_idx+1);
						}
					}
				}
				finally
				{
					rs.close();
				}
			}

		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_refreshRow].methodExit();
		}
	}

	void closeLobObjects() throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_closeLobObjects].methodEntry();
		try
		{
			for (int col_idx=0; col_idx<totalColumns; col_idx++)
			{
				if ((dataType[col_idx]==CLOB) || (dataType[col_idx]==BLOB))
				{
					((SQLMXLob) objectValue[col_idx]).close();
				}
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_closeLobObjects].methodExit();
		}
	}

	String getDataTypeString(int col_num)
	{
		switch (dataType[col_num-1])
		{
			case UNKNOWN:
				return("DataWrapper.UNKNOWN");
			case DataWrapper.BYTE:
				return("DataWrapper.BYTE");
			case DataWrapper.SHORT:
				return("DataWrapper.SHORT");
			case DataWrapper.INTEGER:
				return("DataWrapper.INTEGER");
			case DataWrapper.LONG:
				return("DataWrapper.LONG");
			case DataWrapper.FLOAT:
				return("DataWrapper.FLOAT");
			case DataWrapper.DOUBLE:
				return("DataWrapper.DOUBLE");
			case DataWrapper.BOOLEAN:
				return("DataWrapper.BOOLEAN");
			case DataWrapper.STRING:
				return("DataWrapper.STRING");
			case DataWrapper.BYTES:
				return("DataWrapper.BYTES");
			case DataWrapper.BLOB:
				return("DataWrapper.BLOB");
			case DataWrapper.CLOB:
				return("DataWrapper.CLOB");
			case DataWrapper.BIG_DECIMAL:
				return("DataWrapper.BIG_DECIMAL");
			case DataWrapper.OBJECT:
				return("DataWrapper.OBJECT");
		}
		return("Unknown DataWrapper data type (" + dataType[col_num-1] + ")");
	}

	private static int methodId_DataWrapper					=  0;
	private static int methodId_initDataWrapper				=  1;
	private static int methodId_setOrig						=  2;
	private static int methodId_updateOrig					=  3;
	private static int methodId_clearOrig					=  4;
	private static int methodId_getOrigValue				=  5;
	private static int methodId_getOriginalObject			=  6;
	private static int methodId_updateLong					=  7;
	private static int methodId_setByte						=  8;
	private static int methodId_setShort					=  9;
	private static int methodId_setInt						= 10;
	private static int methodId_setLong						= 11;
	private static int methodId_setFloat					= 12;
	private static int methodId_setDouble					= 13;
	private static int methodId_setBoolean					= 14;
	private static int methodId_setNull						= 15;
	private static int methodId_isNull						= 16;
	private static int methodId_setupOrig					= 17;
	private static int methodId_setupBytes					= 18;
	private static int methodId_setupObjects				= 19;
	private static int methodId_convertStringToNumeric		= 20;
	private static int methodId_convertToNumeric			= 21;
	private static int methodId_getDataType					= 22;
	private static int methodId_getByte						= 23;
	private static int methodId_parseByte					= 24;
	private static int methodId_getShort					= 25;
	private static int methodId_parseShort					= 26;
	private static int methodId_getInt						= 27;
	private static int methodId_parseInt					= 28;
	private static int methodId_getLong						= 29;
	private static int methodId_parseLong					= 30;
	private static int methodId_getFloat					= 31;
	private static int methodId_parseFloat					= 32;
	private static int methodId_getDouble					= 33;
	private static int methodId_parseDouble					= 34;
	private static int methodId_getBoolean					= 35;
	private static int methodId_parseBoolean				= 36;
	private static int methodId_getString					= 37;
	private static int methodId_getObject					= 38;
	private static int methodId_setObject					= 39;
	private static int methodId_outOfRange					= 40;
	private static int methodId_clearUpdated				= 41;
	private static int methodId_getUpdated					= 42;
	private static int methodId_getDeleted					= 43;
	private static int methodId_getInserted					= 44;
	private static int methodId_deleteRow					= 45;
	private static int methodId_insertRow					= 46;
	private static int methodId_copyRows					= 47;
	private static int methodId_setInserted					= 48;
	private static int methodId_refreshRow					= 49;
	private static int methodId_setBlob						= 50;
	private static int methodId_setClob						= 51;
	private static int methodId_setString					= 52;
	private static int methodId_setBytes					= 53;
	private static int methodId_getBytes					= 54;
	private static int methodId_setBigDecimal				= 55;
	private static int methodId_updateRow					= 56;
	private static int methodId_closeLobObjects				= 57;
	private static int methodId_isNumeric					= 58;
	private static int methodId_updateColumn				= 59;
	private static int methodId_clearColumn					= 60;
	private static int methodId_initDataWrapper_I			= 61;
	private static int methodId_setInsertRow				= 62;
	private static int totalMethodIds						= 63;
	private static JdbcDebug[] debug;

	static
	{
		String className = "DataWrapper";

		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_DataWrapper] = new JdbcDebug(className,"DataWrapper");
			debug[methodId_initDataWrapper] = new JdbcDebug(className,"initDataWrapper");
			debug[methodId_setOrig] = new JdbcDebug(className,"setOrig");
			debug[methodId_updateOrig] = new JdbcDebug(className,"updateOrig");
			debug[methodId_clearOrig] = new JdbcDebug(className,"clearOrig");
			debug[methodId_getOrigValue] = new JdbcDebug(className,"getOrigValue");
			debug[methodId_getOriginalObject] = new JdbcDebug(className,"getOriginalObject");
			debug[methodId_updateLong] = new JdbcDebug(className,"updateLong");
			debug[methodId_setByte] = new JdbcDebug(className,"setByte");
			debug[methodId_setShort] = new JdbcDebug(className,"setShort");
			debug[methodId_setInt] = new JdbcDebug(className,"setInt");
			debug[methodId_setLong] = new JdbcDebug(className,"setLong");
			debug[methodId_setFloat] = new JdbcDebug(className,"setFloat");
			debug[methodId_setDouble] = new JdbcDebug(className,"setDouble");
			debug[methodId_setBoolean] = new JdbcDebug(className,"setBoolean]");
			debug[methodId_setNull] = new JdbcDebug(className,"setNull");
			debug[methodId_isNull] = new JdbcDebug(className,"isNull");
			debug[methodId_setupOrig] = new JdbcDebug(className,"setupOrig");
			debug[methodId_setupBytes] = new JdbcDebug(className,"setupBytes");
			debug[methodId_setupObjects] = new JdbcDebug(className,"setupObjects");
			debug[methodId_convertStringToNumeric] = new JdbcDebug(className,"convertStringToNumeric");
			debug[methodId_convertToNumeric] = new JdbcDebug(className,"convertToNumeric");
			debug[methodId_getDataType] = new JdbcDebug(className,"getDataType");
			debug[methodId_getByte] = new JdbcDebug(className,"getByte");
			debug[methodId_parseByte] = new JdbcDebug(className,"parseByte");
			debug[methodId_getShort] = new JdbcDebug(className,"getShort");
			debug[methodId_parseShort] = new JdbcDebug(className,"parseShort");
			debug[methodId_getInt] = new JdbcDebug(className,"getInt");
			debug[methodId_parseInt] = new JdbcDebug(className,"parseInt");
			debug[methodId_getLong] = new JdbcDebug(className,"getLong");
			debug[methodId_parseLong] = new JdbcDebug(className,"parseLong");
			debug[methodId_getFloat] = new JdbcDebug(className,"getFloat");
			debug[methodId_parseFloat] = new JdbcDebug(className,"parseFloat");
			debug[methodId_getDouble] = new JdbcDebug(className,"getDouble");
			debug[methodId_parseDouble] = new JdbcDebug(className,"parseDouble");
			debug[methodId_getBoolean] = new JdbcDebug(className,"getBoolean");
			debug[methodId_parseBoolean] = new JdbcDebug(className,"parseBoolean");
			debug[methodId_getString] = new JdbcDebug(className,"getString");
			debug[methodId_getObject] = new JdbcDebug(className,"getObject");
			debug[methodId_setObject] = new JdbcDebug(className,"setObject");
			debug[methodId_outOfRange] = new JdbcDebug(className,"outOfRange");
			debug[methodId_clearUpdated] = new JdbcDebug(className,"clearUpdated");
			debug[methodId_getUpdated] = new JdbcDebug(className,"getUpdated");
			debug[methodId_getDeleted] = new JdbcDebug(className,"getDeleted");
			debug[methodId_getInserted] = new JdbcDebug(className,"getInserted");
			debug[methodId_deleteRow] = new JdbcDebug(className,"deleteRow");
			debug[methodId_insertRow] = new JdbcDebug(className,"insertRow");
			debug[methodId_copyRows] = new JdbcDebug(className,"copyRows");
			debug[methodId_setInserted] = new JdbcDebug(className,"setInserted");
			debug[methodId_refreshRow] = new JdbcDebug(className,"refreshRow");
			debug[methodId_setBlob] = new JdbcDebug(className,"setBlob");
			debug[methodId_setClob] = new JdbcDebug(className,"setClob");
			debug[methodId_setString] = new JdbcDebug(className,"setString");
			debug[methodId_setBytes] = new JdbcDebug(className,"setBytes");
			debug[methodId_getBytes] = new JdbcDebug(className,"getBytes");
			debug[methodId_setBigDecimal] = new JdbcDebug(className,"setBigDecimal");
			debug[methodId_updateRow] = new JdbcDebug(className,"updateRow");
			debug[methodId_closeLobObjects] = new JdbcDebug(className,"closeLobObjects");
			debug[methodId_isNumeric] = new JdbcDebug(className,"isNumeric");
			debug[methodId_updateColumn] = new JdbcDebug(className,"updateColumn");
			debug[methodId_clearColumn] = new JdbcDebug(className,"clearColumn");
			debug[methodId_initDataWrapper_I] = new JdbcDebug(className,"initDataWrapper_I");
			debug[methodId_setInsertRow] = new JdbcDebug(className,"setInsertRow");
		}
	}

	public boolean isTruncated(int columnIndex) {
		return this.dataTruncation[columnIndex - 1];
	}
    public byte[] getSQLBytes(int col_num) {
        if (SQLbytesValue == null)
            return null;
        return(SQLbytesValue[col_num-1]);
    }
}
