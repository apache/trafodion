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

import java.math.BigDecimal;
import java.math.BigInteger;
import java.nio.charset.Charset;
import java.util.Hashtable;

public class InterfaceUtilities {
	static private Hashtable<Integer, String> valueToCharset;
	static {
		valueToCharset = new Hashtable<Integer, String>(11);
		valueToCharset.put(new Integer(1), "ISO-8859-1"); // ISO
		valueToCharset.put(new Integer(10), "Shift_JIS"); // SJIS
		valueToCharset.put(new Integer(11), "UTF-16"); // UNICODE
		valueToCharset.put(new Integer(12), "EUC-JP"); // EUCJP
		valueToCharset.put(new Integer(13), "Big5"); // BIG5
		valueToCharset.put(new Integer(14), "GB18030"); // GB18030
		valueToCharset.put(new Integer(15), "UTF-8"); // UTF8
		valueToCharset.put(new Integer(16), "KSC5601"); // MB_KSC5601
		valueToCharset.put(new Integer(17), "GB2312"); // GB2312
		valueToCharset.put(new Integer(18), "GBK"); // GBK
	}
	static private Hashtable<String, Integer> charsetToValue;
	static {
		charsetToValue = new Hashtable<String, Integer>(11);
		charsetToValue.put("ISO-8859-1", new Integer(1)); // ISO
		charsetToValue.put("Shift_JIS", new Integer(10)); // SJIS
		charsetToValue.put("UTF-16", new Integer(11)); // UNICODE
		charsetToValue.put("EUC-JP", new Integer(12)); // EUCJP
		charsetToValue.put("Big5", new Integer(13)); // BIG5
		charsetToValue.put("GB18030", new Integer(14)); // GB18030
		charsetToValue.put("UTF-8", new Integer(15)); // UTF8
		charsetToValue.put("KSC5601", new Integer(16)); // MB_KSC5601
		charsetToValue.put("GB2312", new Integer(17)); // GB2312
		charsetToValue.put("GBK", new Integer(18)); // GBK
	}

	static final int SQLCHARSETCODE_UNKNOWN = 0;
	static final String SQLCHARSET_UNKNOWN = "UNKNOWN";

	// these are the only real column types
	static final int SQLCHARSETCODE_ISO88591 = 1;
	static final String SQLCHARSET_ISO88591 = "ISO-8859-1";
	static final int SQLCHARSETCODE_UNICODE = 11;
	static final String SQLCHARSET_UNICODE = "UTF-16";

	// ISO_MAPPING values
	static final int SQLCHARSETCODE_SJIS = 10;
	static final int SQLCHARSETCODE_UTF8 = 15;

    static String getCharsetName(int charset) {
        String ret = valueToCharset.get(charset);

        if (ret == null)
            return SQLCHARSET_UNKNOWN;

        return ret;
    }

    static int getCharsetValue(String charset) {
        Integer i = charsetToValue.get(charset);
        if (i == null) {
            i = charsetToValue.get(convertToCanonicalCharsetName(charset));
            if (i == null) {
                return SQLCHARSETCODE_UNKNOWN;
            } else {
                return i.intValue();
            }
        } else {
            return i.intValue();
        }
    }

    private static String convertToCanonicalCharsetName (String aliasName) {
        return Charset.forName(aliasName).name();
    }

	static private final int[] powersOfTen = { 10, 100, 1000, 10000 };

	public static byte[] convertBigDecimalToSQLBigNum(BigDecimal bd, int targetLength, int targetScale) {
		byte[] sourceData = bd.setScale(targetScale, BigDecimal.ROUND_DOWN).unscaledValue().toString().getBytes(); // add
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
		while (zeros < sourceData.length && (sourceData[zeros] == '0' || sourceData[zeros] == '-'))
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

		// convert the data back to bytes
		for (int i = 0; i < targetInShorts.length; i++) {
//			targetData[i * 2] = (byte) ((targetInShorts[i] & 0xFF00) >> 8);
//			targetData[i * 2 + 1] = (byte) (targetInShorts[i] & 0xFF);
			targetData[i * 2 ] = (byte) (targetInShorts[i] & 0xFF);
			targetData[i * 2 + 1] = (byte) ((targetInShorts[i] & 0xFF00) >> 8);
		}

		// add sign
		if ((bd.signum() < 0))
                    //server side is little-endian so here should be length-1
                    targetData[targetData.length - 1] |= 0x80;

		return targetData;
	}

    public static BigDecimal convertSQLBigNumToBigDecimal(byte[] sourceData, int scale, boolean swap, boolean isUnsigned) {
		String strVal = ""; // our final String

		// we need the data in an array which can hold UNSIGNED 16 bit values
		// in java we dont have unsigned datatypes so 32-bit signed is the best
		// we can do
		int[] dataInShorts = new int[sourceData.length / 2];
		for (int i = 0; i < dataInShorts.length; i++)
			dataInShorts[i] = Bytes.extractUShort(sourceData, i * 2, swap); // copy
		// the
		// data
		
                boolean negative = false;
                if (!isUnsigned) {
                    negative = ((dataInShorts[dataInShorts.length - 1] & 0x8000) > 0);
                    dataInShorts[dataInShorts.length - 1] &= 0x7FFF; // force sign to 0, continue
                                                                     // normally
                }

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
}
