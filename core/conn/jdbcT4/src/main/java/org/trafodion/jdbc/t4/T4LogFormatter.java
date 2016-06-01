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

package org.trafodion.jdbc.t4;

import java.text.DateFormat;
import java.text.DecimalFormat;
import java.util.logging.LogRecord;

public class T4LogFormatter extends java.util.logging.Formatter {

	static DecimalFormat df = new DecimalFormat("########################################################00000000");

	// ----------------------------------------------------------
	public T4LogFormatter() {
	}

	// ----------------------------------------------------------
	public String format(LogRecord lr) {
		String m1;
		String separator = " ~ ";
		Object params[] = lr.getParameters();
		Object tempParam = null;

		try {
			long sequence_number = lr.getSequenceNumber();
			String time_stamp = null;
			long thread_id = lr.getThreadID();
			String connection_id = "";
			String server_id = "";
			String dialogue_id = "";
			String class_name = lr.getSourceClassName();
			String method = lr.getSourceMethodName();
			String parameters = ""; // need to fix
			String message = lr.getMessage();

			long time_mills = lr.getMillis();
			java.util.Date d1 = new java.util.Date(time_mills);
			DateFormat df1 = java.text.DateFormat.getDateTimeInstance(DateFormat.MEDIUM, DateFormat.FULL);

			time_stamp = df1.format(d1);

			//
			// By convension, the first parameter is a TrafT4Connection object or
			// a T4Properties object
			//
			TrafT4Connection sc = null;
			T4Properties tp = null;

			if (params != null && params.length > 0) {
				if (params[0] instanceof TrafT4Connection)
					tp = ((TrafT4Connection) params[0]).props_;
				else
					tp = (T4Properties) params[0];
			}

			if (tp != null) {
				connection_id = tp.getConnectionID();
				server_id = tp.getServerID();
				dialogue_id = tp.getDialogueID();
			}

			//
			// Format for message:
			//
			// sequence-number ~ time-stamp ~ thread-id ~ [connection-id] ~
			// [server-id]
			// ~ [dialogue-id] ~ [class] ~ [method] ~ [parameters] ~ [text]
			//

			Long l1 = new Long(sequence_number);
			String t1 = df.format(l1);
			String p1 = "";

			m1 = t1 + separator + time_stamp + separator + thread_id + separator + connection_id + separator
					+ server_id + separator + dialogue_id + separator + class_name + "." + method + "(";

			if (params != null) {
				String paramText = null;

				//
				// Skip the first parameter, which is a T4Connection, and is
				// handled above.
				//
				for (int i = 1; i < params.length; i++) {
					tempParam = params[i];
					if (tempParam != null) {
						//
						// If the parameter is an array, try to print each
						// element of the array.
						//
						tempParam = makeObjectArray(tempParam);

						if (tempParam instanceof Object[]) {
							Object[] tempOa = (Object[]) tempParam;
							String tempOas = "";
							String tempStr2 = null;

							for (int j = 0; j < tempOa.length; j++) {
								if (tempOa[j] != null) {
									tempStr2 = tempOa[j].toString();
								} else {
									tempStr2 = "null";
								}
								tempOas = tempOas + " [" + j + "]" + tempStr2;
							}
							paramText = tempOas;
						} else {
							paramText = tempParam.toString();
						}
					} else {
						paramText = "null";

					}
					p1 = p1 + "\"" + paramText + "\"";
					if (i + 1 < params.length) {
						p1 = p1 + ", ";
					}
				}
			}

			m1 = m1 + p1 + ")" + separator + message + "\n";

		} catch (Exception e) {
			//
			// Tracing should never cause an internal error, but if it does, we
			// do want to
			// capture it here. An internal error here has no effect on the user
			// program,
			// so we don't want to throw an exception. We'll put the error in
			// the trace log
			// instead, and instruct the user to report it
			//
			m1 = "An internal error has occurred in the tracing logic. Please report this to your representative. \n"
					+ "  exception = "
					+ e.toString()
					+ "\n"
					+ "  message   = "
					+ e.getMessage()
					+ "\n"
					+ "  Stack trace = \n";

			StackTraceElement st[] = e.getStackTrace();

			for (int i = 0; i < st.length; i++) {
				m1 = m1 + "    " + st[i].toString() + "\n";
			}
			m1 = m1 + "\n";
		} // end catch

		//
		// The params array is reused, so we must null it out before returning.
		//
		if (params != null) {
			for (int i = 0; i < params.length; i++) {
				params[i] = null;
			}
		}

		return m1;

	} // end formatMessage

	// ---------------------------------------------------------------------
	Object makeObjectArray(Object obj) {
		Object retVal = obj;
		Object[] newVal = null;
		int i;

		if (obj instanceof boolean[]) {
			boolean[] temp = (boolean[]) obj;
			newVal = new Boolean[temp.length];
			for (i = 0; i < temp.length; i++)
				newVal[i] = new Boolean(temp[i]);
		} else if (obj instanceof char[]) {
			char[] temp = (char[]) obj;
			newVal = new Character[temp.length];
			for (i = 0; i < temp.length; i++)
				newVal[i] = new Character(temp[i]);
		} else if (obj instanceof byte[]) {
			byte[] temp = (byte[]) obj;
			newVal = new Byte[temp.length];
			for (i = 0; i < temp.length; i++)
				newVal[i] = new Byte(temp[i]);
		} else if (obj instanceof short[]) {
			short[] temp = (short[]) obj;
			newVal = new Short[temp.length];
			for (i = 0; i < temp.length; i++)
				newVal[i] = new Short(temp[i]);
		} else if (obj instanceof int[]) {
			int[] temp = (int[]) obj;
			newVal = new Integer[temp.length];
			for (i = 0; i < temp.length; i++)
				newVal[i] = new Integer(temp[i]);
		} else if (obj instanceof long[]) {
			long[] temp = (long[]) obj;
			newVal = new Long[temp.length];
			for (i = 0; i < temp.length; i++)
				newVal[i] = new Long(temp[i]);
		} else if (obj instanceof float[]) {
			float[] temp = (float[]) obj;
			newVal = new Float[temp.length];
			for (i = 0; i < temp.length; i++)
				newVal[i] = new Float(temp[i]);
		} else if (obj instanceof double[]) {
			double[] temp = (double[]) obj;
			newVal = new Double[temp.length];
			for (i = 0; i < temp.length; i++)
				newVal[i] = new Double(temp[i]);
		}

		if (newVal != null)
			retVal = newVal;

		return retVal;
	} // end makeObjectArray

} // end class T4LogFormatter
