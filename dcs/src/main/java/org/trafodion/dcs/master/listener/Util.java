/**
* @@@ START COPYRIGHT @@@

Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.

* @@@ END COPYRIGHT @@@
 */
package org.trafodion.dcs.master.listener;

import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.net.*;
import java.util.*;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class Util {
	private static  final Log LOG = LogFactory.getLog(Util.class);

	static StringBuilder sb = new StringBuilder();

	Util(){
	}

	static void toHexString(String header, ByteBuffer buf)
	{
		sb.delete(0,sb.length());

		for (int index = 0; index < buf.limit(); index++) {
			String hex = Integer.toHexString(0x0100 + (buf.get(index) & 0x00FF)).substring(1);
			sb.append((hex.length() < 2 ? "0" : "") + hex + " ");
		}
		LOG.debug("hex->" + header + ": position,limit,capacity " + buf.position() + "," + buf.limit() + "," + buf.capacity());
		LOG.debug("hex->"+ sb.toString());
	}

	static String extractString(ByteBuffer buf) throws java.io.UnsupportedEncodingException {
		int len = buf.getInt();
		byte[] str = null;
		if (len > 0) {
			str = new byte[len - 1];;
			buf.get(str, 0, len - 1);
			buf.get(); // trailing null
		}
		else
			str = new byte[0];
		return new String(str, "UTF-8");
	}

	static byte[] extractByteString(ByteBuffer buf) {
		int len = buf.getInt();
		byte[] str = null;
		if (len > 0) {
			str = new byte[len];
			buf.get(str, 0, len);
			buf.get(); // trailing null
		}
		else
			str = new byte[0];
		return str;
	}

	static byte[] extractByteArray(ByteBuffer buf) {
		int len = buf.getInt();
		if (len > 0){
			byte[] a = new byte[len];
			buf.get(a, 0, len);
			return a;
		}
		else
			return new byte[0];
	}

	static void insertString(String str, ByteBuffer buf) throws java.io.UnsupportedEncodingException {
		if (str != null && str.length() > 0) {
			buf.putInt(str.length() + 1);
			buf.put(str.getBytes("UTF-8"), 0, str.length());
			buf.put((byte) 0);
		} else { // buffer is null or length 0
			buf.putInt(0);
		}
	}

	static void insertByteString(byte[] array, ByteBuffer buf) throws java.io.UnsupportedEncodingException {
		if (array != null && array.length > 0) {
			buf.putInt(array.length);
			buf.put(array, 0, array.length);
			buf.put((byte) 0);
		} else { // buffer is null or length 0
			buf.putInt(0);
		}
	}

	static void insertByteArray(byte[] array, ByteBuffer buf) {
		if (array != null && array.length > 0) {
			buf.putInt(array.length);
			buf.put(array, 0, array.length);
		} else
			buf.putInt(0);
	}


	static void printBBInfo(ByteBuffer buf) {
		LOG.debug("Info : position,limit,capacity " + buf.position() + "," + buf.limit() + "," + buf.capacity());
	}
}
