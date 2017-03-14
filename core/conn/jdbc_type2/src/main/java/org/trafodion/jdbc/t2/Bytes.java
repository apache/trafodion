/*******************************************************************************
 * // @@@ START COPYRIGHT @@@
 * //
 * // Licensed to the Apache Software Foundation (ASF) under one
 * // or more contributor license agreements.  See the NOTICE file
 * // distributed with this work for additional information
 * // regarding copyright ownership.  The ASF licenses this file
 * // to you under the Apache License, Version 2.0 (the
 * // "License"); you may not use this file except in compliance
 * // with the License.  You may obtain a copy of the License at
 * //
 * //   http://www.apache.org/licenses/LICENSE-2.0
 * //
 * // Unless required by applicable law or agreed to in writing,
 * // software distributed under the License is distributed on an
 * // "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * // KIND, either express or implied.  See the License for the
 * // specific language governing permissions and limitations
 * // under the License.
 * //
 * // @@@ END COPYRIGHT @@@
 *******************************************************************************/

package org.trafodion.jdbc.t2;

/**
 * <code>Bytes</code> contains a set of static methods used for byte
 * manipulation. There are three basic types of methods:
 * <ul>
 * <li>extract</li>
 * <li>insert</li>
 * <li>create</li>
 * </ul>
 * <p>
 * Extract methods will copy from a raw byte array into a basic Java type.
 * Insert methods will copy from a basic Java type into a raw byte array. Create
 * methods will copy from a basic Java type into a new raw byte array which is
 * returned to the user.
 * 
 * As Java is always BigEndian, set the swap parameter to <code>true</code> to
 * convert to and from LittleEndian.
 * 
 * There is no error checking in order to improve performance. Length checking
 * should be done before calling these methods or the resulting exceptions
 * should be handled by the user.
 * 
 */
class Bytes {

    static short extractShort(byte[] array, int offset, boolean swap) {
        short value;

        if (swap) {
            value = (short) (((array[offset]) & 0x00ff) | ((array[offset + 1] << 8) & 0xff00));
        } else {
            value = (short) (((array[offset + 1]) & 0x00ff) | ((array[offset] << 8) & 0xff00));
        }

        return value;
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

    static int extractInt(byte[] array, int offset, boolean swap) {
        int value;

        if (swap) {
            value = ((array[offset]) & 0x000000ff) | ((array[offset + 1] << 8) & 0x0000ff00)
                | ((array[offset + 2] << 16) & 0x00ff0000) | ((array[offset + 3] << 24) & 0xff000000);
        } else {
            value = ((array[offset + 3]) & 0x000000ff) | ((array[offset + 2] << 8) & 0x0000ff00)
                | ((array[offset + 1] << 16) & 0x00ff0000) | ((array[offset] << 24) & 0xff000000);
        }

        return value;
    }

    static long extractUInt(byte[] array, int offset, boolean swap) {
        long value;

        if (swap) {
            value = ((array[offset]) & 0x000000ff) | ((array[offset + 1] << 8) & 0x0000ff00)
                | ((array[offset + 2] << 16) & 0x00ff0000) | ((array[offset + 3] << 24) & 0xff000000);
        } else {
            value = ((array[offset + 3]) & 0x000000ff) | ((array[offset + 2] << 8) & 0x0000ff00)
                | ((array[offset + 1] << 16) & 0x00ff0000) | ((array[offset] << 24) & 0xff000000);
        }

        return value & 0xffffffffL;
    }

    static long extractLong(byte[] array, int offset, boolean swap) {
        long value = 0;
        int i=offset;

        if(swap) {
            for (int shift = 0; shift < 64; shift += 8) {
                value |= ( (long)( array[i] & 0xff ) ) << shift;
                i++;
            }

        }else {
            for (int shift = 56; shift >= 0; shift -= 8) {
                value |= ( (long)( array[i] & 0xff ) ) << shift;
                i++;
            }
        }

        return value;
    }

    static int insertShort(byte[] array, int offset, short value, boolean swap) {
        if (swap) {
            array[offset + 1] = (byte) ((value >>> 8) & 0xff);
            array[offset] = (byte) ((value) & 0xff);
        } else {
            array[offset] = (byte) ((value >>> 8) & 0xff);
            array[offset + 1] = (byte) ((value) & 0xff);
        }

        return offset + 2;
    }

    static int insertInt(byte[] array, int offset, int value, boolean swap) {
        if (swap) {
            array[offset + 3] = (byte) ((value >>> 24) & 0xff);
            array[offset + 2] = (byte) ((value >>> 16) & 0xff);
            array[offset + 1] = (byte) ((value >>> 8) & 0xff);
            array[offset] = (byte) ((value) & 0xff);
        } else {
            array[offset] = (byte) ((value >>> 24) & 0xff);
            array[offset + 1] = (byte) ((value >>> 16) & 0xff);
            array[offset + 2] = (byte) ((value >>> 8) & 0xff);
            array[offset + 3] = (byte) ((value) & 0xff);
        }

        return offset + 4;
    }

    static int insertLong(byte[] array, int offset, long value, boolean swap) {
        if (swap) {
            array[offset + 7] = (byte) ((value >>> 56) & 0xff);
            array[offset + 6] = (byte) ((value >>> 48) & 0xff);
            array[offset + 5] = (byte) ((value >>> 40) & 0xff);
            array[offset + 4] = (byte) ((value >>> 32) & 0xff);
            array[offset + 3] = (byte) ((value >>> 24) & 0xff);
            array[offset + 2] = (byte) ((value >>> 16) & 0xff);
            array[offset + 1] = (byte) ((value >>> 8) & 0xff);
            array[offset] = (byte) ((value) & 0xff);
        } else {
            array[offset] = (byte) ((value >>> 56) & 0xff);
            array[offset + 1] = (byte) ((value >>> 48) & 0xff);
            array[offset + 2] = (byte) ((value >>> 40) & 0xff);
            array[offset + 3] = (byte) ((value >>> 32) & 0xff);
            array[offset + 4] = (byte) ((value >>> 24) & 0xff);
            array[offset + 5] = (byte) ((value >>> 16) & 0xff);
            array[offset + 6] = (byte) ((value >>> 8) & 0xff);
            array[offset + 7] = (byte) ((value) & 0xff);
        }

        return offset + 8;
    }

    static byte[] createShortBytes(short value, boolean swap) {
        byte[] b = new byte[2];
        Bytes.insertShort(b, 0, value, swap);

        return b;
    }

    static byte[] createIntBytes(int value, boolean swap) {
        byte[] b = new byte[4];
        Bytes.insertInt(b, 0, value, swap);


        return b;
    }

    static byte[] createLongBytes(long value, boolean swap) {
        byte[] b = new byte[8];
        Bytes.insertLong(b, 0, value, swap);

        return b;
    }

    // -------------------------------------------------------------
    // -------------------------------------------------------------
    // ---------------TODO: get rid of these methods!---------------
    // -------------------------------------------------------------
    // -------------------------------------------------------------

    /**
     * @deprecated
     */
    static char[] read_chars(byte[] buffer, int index) {
        int len = 0;

        // find the null terminator
        while (buffer[index + len] != 0) {
            len = len + 1;
        }

        char[] temp1 = read_chars(buffer, index, len);

        return temp1;
    } // end read_chars

    /**
     * @deprecated
     */
    static char[] read_chars(byte[] buffer, int index, int tLen) {
        char[] la_chars;
        int len = tLen;

        if (len == -1) // must find null to get length
        {
            int ii = index;
            while (buffer[ii] != (byte) 0) {
                ii = ii + 1;
            }
            len = ii - index;
        }

        la_chars = new char[len];

        int i = 0;
        while (i < len) {
            la_chars[i] = (char) (buffer[index] & 0xff);
            i = i + 1;
            index = index + 1;
        }

        return la_chars;
    } // end read_chars
}
