/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.trafodion.dcs.util;

import org.trafodion.dcs.servermt.ServerConstants;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.math.*;
import java.util.Arrays;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class ByteBufferUtils {

    private static  final Log LOG = LogFactory.getLog(ByteBufferUtils.class);

    private ByteBufferUtils() {
    }

    /**
     * Allocate ByteBuffer.
     * 
     * @param capacity
     *         ByteBuffer capacity
     * @param direct
     *         allocate DirectByteBuffer
     * @return
     *         allocated ByteBuffer
     * @throws IllegalArgumentException
     *         if capacity is negative
     */
    public static ByteBuffer allocate(int capacity, boolean direct)
            throws IllegalArgumentException {
        if (capacity < 0)
            throw new IllegalArgumentException("capacity can't be negative");
        return direct ? ByteBuffer.allocateDirect(capacity) : ByteBuffer.allocate(capacity);
    }

    /**
     * Increase ByteBuffer's capacity.
     * 
     * @param buffer
     *         the ByteBuffer want to increase capacity
     * @param size
     *         increased size
     * @return
     *        increased capacity ByteBuffer
     * @throws IllegalArgumentException
     *         if size less than 0 or buffer is null
     */
    public static ByteBuffer increaseCapacity(ByteBuffer buffer, int size)
            throws IllegalArgumentException {
        if (buffer == null)
            throw new IllegalArgumentException("buffer is null");
        if (size < 0)
            throw new IllegalArgumentException("size less than 0");

        ByteOrder buffOrder = buffer.order();
        int capacity = buffer.capacity() + size;
        ByteBuffer result = allocate(capacity, buffer.isDirect());
        result.order(buffOrder);
        buffer.flip();
        result.put(buffer);
        return result;
    }

    /**
     * Gather ByteBuffers to one ByteBuffer.
     * 
     * @param buffers
     *         ByteBuffers
     * @return
     *         the gather ByteBuffer
     */
    public static ByteBuffer gather(ByteBuffer[] buffers) {
        int remaining = 0;
        for (int i = 0; i < buffers.length; i++) {
            if (buffers[i] != null)
                remaining += buffers[i].remaining();
        }
        ByteBuffer result = ByteBuffer.allocate(remaining);
        for (int i = 0; i < buffers.length; i++) {
            if (buffers[i] != null)
                result.put(buffers[i]);
        }
        result.flip();
        return result;
    }

    /**
     * Judge ByteBuffers have remaining bytes.
     * 
     * @param buffers
     *         ByteBuffers
     * @return
     *         have remaining
     */
    public static boolean hasRemaining(ByteBuffer[] buffers) {
        if (buffers == null)
            return false;
        for (int i = 0; i < buffers.length; i++) {
            if (buffers[i] != null && buffers[i].hasRemaining())
                return true;
        }
        return false;
    }

    /**
     * Returns the index within this buffer of the first occurrence of the
     * specified pattern buffer.
     * 
     * @param buffer
     *         the buffer
     * @param pattern
     *         the pattern buffer
     * @return
     *         the position within the buffer of the first occurrence of the 
     * pattern buffer
     */
    public static int indexOf(ByteBuffer buffer, ByteBuffer pattern) {
        int patternPos = pattern.position();
        int patternLen = pattern.remaining();
        int lastIndex = buffer.limit() - patternLen + 1;

        Label: for (int i = buffer.position(); i < lastIndex; i++) {
            for (int j = 0; j < patternLen; j++) {
                if (buffer.get(i + j) != pattern.get(patternPos + j))
                    continue Label;
            }
            return i;
        }
        return -1;
    }
//
//============================================================
//
    public static void toHexString(String header, ByteBuffer buf)
    {
        StringBuilder sb = new StringBuilder();
        int bufPosition = buf.position();
        int bufLimit = buf.limit();
        
//        sb.delete(0,sb.length());

        LOG.debug("hex->" + header + ": position,limit,capacity " + buf.position() + "," + buf.limit() + "," + buf.capacity());
        for (int index = buf.position(); index < buf.limit(); index++) {
            String hex = Integer.toHexString(0x0100 + (buf.get(index) & 0x00FF)).substring(1);
            sb.append((hex.length() < 2 ? "0" : "") + hex + " ");
        }
        LOG.debug("hex->"+ sb.toString());
        buf.position(bufPosition);
        buf.limit(bufLimit);
    }

    public static void toHexString(String header, ByteBuffer buf, int length)
    {
        StringBuilder sb = new StringBuilder();
        int bufPosition = buf.position();
        int bufLimit = buf.limit();
        int len = bufPosition + length;
        len = len > bufLimit ? bufLimit : len;
        
//        sb.delete(0,sb.length());

        LOG.debug("hex->" + header + ": position,limit,capacity " + buf.position() + "," + buf.limit() + "," + buf.capacity());
        for (int index = buf.position(); index < len; index++) {
            String hex = Integer.toHexString(0x0100 + (buf.get(index) & 0x00FF)).substring(1);
            sb.append((hex.length() < 2 ? "0" : "") + hex + " ");
        }
        LOG.debug("hex->"+ sb.toString());
        buf.position(bufPosition);
        buf.limit(bufLimit);
    }

//========================== extract ==================================
    
    public static String extractString(ByteBuffer buf) throws java.io.UnsupportedEncodingException {
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

    public static String extractStringWithCharset(ByteBuffer buf) throws java.io.UnsupportedEncodingException {
        int len;
        int charset = 15;        //UTF-8
        byte[] str = null;
        
        len = buf.getInt();
        if (len > 0) {
            str = new byte[len - 1];;
            buf.get(str, 0, len - 1);
            buf.get(); // trailing null
            charset = buf.getInt();
        }
        else
            str = new byte[0];
        return new String(str, charsetToString(charset));
    }

    public static byte[] extractByteString(ByteBuffer buf) {
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
    
    public static byte[] extractByteArray(ByteBuffer buf) {
        int len = buf.getInt();
        if (len > 0){
            byte[] a = new byte[len];
            buf.get(a, 0, len);
            return a;
        }
        else
            return new byte[0];
    }
    public static int extractUShort(ByteBuffer buf) {
        int value;

        byte byte1 = buf.get(), byte2 = buf.get();
        
        if (buf.order() == ByteOrder.LITTLE_ENDIAN) {
            value = ((byte1 & 0x00ff) | 
                    ((byte2 << 8) & 0xff00));
        } else {
            value = ((byte2 & 0x00ff) | 
                    ((byte1 << 8) & 0xff00));
        }
        return value & 0xffff;
    }
    
    public static long extractUInt(ByteBuffer buf) {
        long value;

        byte byte1 = buf.get(), byte2 = buf.get(), byte3 = buf.get(), byte4 = buf.get();

        if (buf.order() == ByteOrder.LITTLE_ENDIAN) {
            value = ((byte1 & 0x000000ff) | 
                    ((byte2 << 8) & 0x0000ff00) | 
                    ((byte3 << 16) & 0x00ff0000) | 
                    ((byte4 << 24) & 0xff000000));
        } else {
            value = ((byte4 & 0x000000ff) | 
                    ((byte3 << 8) & 0x0000ff00) | 
                    ((byte2 << 16) & 0x00ff0000) | 
                    ((byte1 << 24) & 0xff000000));
        }
        return value & 0xffffffffL;
    }

    public static byte[] extractByteArrayLen(ByteBuffer buf, int len) {
        byte[] a = new byte[len];
        for (int i = 0; i < len; i++) a[i] = buf.get();
        return a;
    }
    
    public static String extractShortString(ByteBuffer buf) throws java.io.UnsupportedEncodingException {
        short len = buf.getShort();
        byte[] str = null;
        if (len > 0) {
            str = new byte[len];
            buf.get(str, 0, len);
//          buf.get(); // trailing null
        }
        else
            str = new byte[0];
        return new String(str, "UTF-8");
    }
//============================= insert =========================================
    
    public static void insertString(String str, ByteBuffer buf) throws java.io.UnsupportedEncodingException {
        if (str != null && str.length() > 0) {
            buf.putInt(str.length() + 1);
            buf.put(str.getBytes("UTF-8"), 0, str.length());
            buf.put((byte) 0);
        } else { // buffer is null or length 0
            buf.putInt(0);
        }
    }

    public static void insertByteString(byte[] array, ByteBuffer buf) throws java.io.UnsupportedEncodingException {
        if (array != null && array.length > 0) {
            buf.putInt(array.length);
            buf.put(array, 0, array.length);
            buf.put((byte) 0);
        } else { // buffer is null or length 0
            buf.putInt(0);
        }
    }

    public static void insertStringWithCharset(byte[] str, int charset, ByteBuffer buf) throws java.io.UnsupportedEncodingException {
        if (str != null && str.length > 0) {
            buf.putInt(str.length + 1);
            buf.put(str, 0, str.length);
            buf.put((byte) 0);
            buf.putInt(charset);
        } else {
            buf.putInt(0);
        }
    }

    public static void insertByteArray(byte[] array, ByteBuffer buf) {
        if (array != null && array.length > 0) {
            buf.putInt(array.length);
            buf.put(array, 0, array.length);
        } else
            buf.putInt(0);
    }

    public static void insertByteArray(byte[] value, int len, ByteBuffer buf) {
        buf.put(value, 0, len);
    }

    public static void insertUInt(long value, ByteBuffer buf) {
        if (buf.order() == ByteOrder.LITTLE_ENDIAN ){
            buf.put((byte)((value) & 0xff));
            buf.put((byte)((value >>> 8) & 0xff));
            buf.put((byte)((value >>> 16) & 0xff));
            buf.put((byte)((value >>> 24) & 0xff));
        }
        else {
            buf.put((byte)((value >>> 24) & 0xff));
            buf.put((byte)((value >>> 16) & 0xff));
            buf.put((byte)((value >>> 8) & 0xff));
            buf.put((byte)((value) & 0xff));
        }
    }

//========================= dataLength ================================================
    
    public static int lengthOfString(String str) {
        int dataLength = 0;

        if (str != null && str.length() > 0) {
            dataLength += ServerConstants.INT_FIELD_SIZE;
            dataLength += str.length();
            dataLength += ServerConstants.BYTE_FIELD_SIZE;
        } else
            dataLength += ServerConstants.INT_FIELD_SIZE;

        return dataLength;
    }
    
    public static int lengthOfByteString(byte[] array) {
        int dataLength = 0;

        if (array != null && array.length > 0) {
            dataLength += ServerConstants.INT_FIELD_SIZE;
            dataLength += array.length;
            dataLength += ServerConstants.BYTE_FIELD_SIZE;
        } else
            dataLength += ServerConstants.INT_FIELD_SIZE;
        
        return dataLength;
    }

    public static int lengthOfStringWithCharset(byte[] array) {
        int dataLength = 0;

        if (array != null && array.length > 0) {
            dataLength += ServerConstants.INT_FIELD_SIZE;
            dataLength += array.length;
            dataLength += ServerConstants.BYTE_FIELD_SIZE;
            dataLength += ServerConstants.INT_FIELD_SIZE;
        } else
            dataLength += ServerConstants.INT_FIELD_SIZE;
        
        return dataLength;
    }

    public static int lengthOfByteArray(byte[] array) {
        int dataLength = 0;

        if (array != null && array.length > 0) {
            dataLength += ServerConstants.INT_FIELD_SIZE;
            dataLength += array.length;
        } else
            dataLength += ServerConstants.INT_FIELD_SIZE;
        
        return dataLength;
    }

//===================================================================
    
    public static void printBBInfo(ByteBuffer buf) {
        LOG.debug("Info : position,limit,capacity " + buf.position() + "," + buf.limit() + "," + buf.capacity());
    }

    public static String charsetToString(int charset){
        String strCharset = "UTF-8";
        switch (charset)
        {
        case 1: strCharset = "ISO8859_1"; break;
        case 10: strCharset = "MS932"; break;
        case 11: strCharset = "UTF-16BE"; break;
        case 12: strCharset = "EUCJP"; break;
        case 13: strCharset = "MS950"; break;
        case 14: strCharset = "GB18030"; break;
        case 15: strCharset = "UTF-8"; break;
        case 16: strCharset = "MS949"; break;
        case 17: strCharset = "GB2312"; break;
        }
        return strCharset;
    }
}

