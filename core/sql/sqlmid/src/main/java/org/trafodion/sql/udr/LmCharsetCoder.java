/**********************************************************************
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
**********************************************************************/
/* -*-Java-*-
 ******************************************************************************
 *
 * File:         LmCharsetCoder.java
 * Description:  Wrapper class for Java CharsetDecoder, CharsetEncoder
 *               classes
 *
 * Created:      10/17/2003
 * Language:     Java
 *
 *
 ******************************************************************************
 */
package org.trafodion.sql.udr;

import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.*;
import java.util.HashMap;

/**
 *  Java class to Character encoding and decoding between different 
 *  character sets and Unicode
 *
 **/
public class LmCharsetCoder
{
   static CharsetDecoder iso88591Decoder_;
   static CharsetEncoder iso88591Encoder_;
   static CharsetDecoder ucs2Decoder_;
   static CharsetEncoder ucs2Encoder_;
   static boolean        reset_;
   static CharsetDecoder sjisDecoder_;
   static CharsetEncoder sjisEncoder_;
   static CharsetDecoder utf8Decoder_;
   static CharsetEncoder utf8Encoder_;

   static HashMap<String, Integer> charsetHashMap_;

   static
   {
      // LOAD ISO88591 character set
      Charset iso88591Charset = Charset.forName("ISO-8859-1");

      iso88591Decoder_ = iso88591Charset.newDecoder();
      iso88591Decoder_.onMalformedInput(CodingErrorAction.REPORT);
      iso88591Decoder_.onUnmappableCharacter(CodingErrorAction.REPORT);

      iso88591Encoder_ = iso88591Charset.newEncoder();
      iso88591Encoder_.onMalformedInput(CodingErrorAction.REPORT);
      iso88591Encoder_.onUnmappableCharacter(CodingErrorAction.REPORT);

      // LOAD UCS2 character set
      Charset ucs2Charset;
      if (System.getProperty("os.name").compareTo("NONSTOP_KERNEL") == 0)
      {
         ucs2Charset = Charset.forName("UTF-16BE");
      }
      else
      {
         ucs2Charset = Charset.forName("UTF-16LE");
      }

      ucs2Decoder_ = ucs2Charset.newDecoder();
      ucs2Decoder_.onMalformedInput(CodingErrorAction.REPORT);
      ucs2Decoder_.onUnmappableCharacter(CodingErrorAction.REPORT);

      ucs2Encoder_ = ucs2Charset.newEncoder();
      ucs2Encoder_.onMalformedInput(CodingErrorAction.REPORT);
      ucs2Encoder_.onUnmappableCharacter(CodingErrorAction.REPORT);

      // LOAD SJIS character set. MS932 is used for SJIS
      // instead of SJIS or Shift_JIS.
      Charset sjisCharset = Charset.forName("MS932");

      sjisDecoder_ = sjisCharset.newDecoder();
      sjisDecoder_.onMalformedInput(CodingErrorAction.REPORT);
      sjisDecoder_.onUnmappableCharacter(CodingErrorAction.REPORT);

      sjisEncoder_ = sjisCharset.newEncoder();
      sjisEncoder_.onMalformedInput(CodingErrorAction.REPORT);
      sjisEncoder_.onUnmappableCharacter(CodingErrorAction.REPORT);

      // LOAD UTF-8 character set
      Charset utf8Charset = Charset.forName("UTF-8");

      utf8Decoder_ = utf8Charset.newDecoder();
      utf8Decoder_.onMalformedInput(CodingErrorAction.REPORT);
      utf8Decoder_.onUnmappableCharacter(CodingErrorAction.REPORT);

      utf8Encoder_ = utf8Charset.newEncoder();
      utf8Encoder_.onMalformedInput(CodingErrorAction.REPORT);
      utf8Encoder_.onUnmappableCharacter(CodingErrorAction.REPORT);

      // A map for charset name to an integer
      charsetHashMap_ = new HashMap<String, Integer>(4);
      charsetHashMap_.put("ISO88591", 1);
      charsetHashMap_.put("UCS2", 2);
      charsetHashMap_.put("SJIS", 3);
      charsetHashMap_.put("UTF8", 4);

      reset_ = System.getProperty("os.name").equals("Linux");
   }

  /**
   * Converts given character set bytes to Unicode String.
   *
   * @param bytes   Array of input character bytes
   * @param fromCharset input character set
   * @param offset  offset into the array
   * @param length  length of the array
   *
   **/
  public static String getUnicodeStringFromBytes(byte[] inputBytes,
                                                 String fromCharset,
                                                 int offset,
                                                 int length)
  throws java.nio.charset.CharacterCodingException
  {
    ByteBuffer bBuffer = ByteBuffer.allocate(length);
    bBuffer.put(inputBytes, 0, length);
    bBuffer.flip();

    CharBuffer cBuffer = null;

    // call proper decoder to decode the bytes
    switch (charsetHashMap_.get(fromCharset))
    {
      case 1:
        cBuffer = iso88591Decoder_.decode(bBuffer);
        break;

      case 2:
        cBuffer = ucs2Decoder_.decode(bBuffer);
        break;

      case 3:
        cBuffer = sjisDecoder_.decode(bBuffer);
        break;

      case 4:
        cBuffer = utf8Decoder_.decode(bBuffer);
        break;
    }

    return cBuffer.toString();
  }

  /**
   * Converts Unicode String to bytes of a given character set.
   *
   * @param inputString   Unicode String
   * @param toCharset     Charset to convert unicode string to
   *
   **/
  public static byte[] getBytesFromUnicodeString(String inputString,
                                                 String toCharset)
  throws java.nio.charset.CharacterCodingException
  {
    ByteBuffer bBuffer = null;

    // call proper encoder to convert the string to bytes
    switch (((Integer)charsetHashMap_.get(toCharset)).intValue())
    {
      case 1:
        bBuffer = iso88591Encoder_.encode(CharBuffer.wrap(inputString));
        break;

      case 2:
        bBuffer = ucs2Encoder_.encode(CharBuffer.wrap(inputString));
        break;

      case 3:
        bBuffer = sjisEncoder_.encode(CharBuffer.wrap(inputString));
        break;

      case 4:
        bBuffer = utf8Encoder_.encode(CharBuffer.wrap(inputString));
        break;
    }

    byte[] outputBytes = bBuffer.array();

// If we don't reset on Linux, Linux throws IllegalStateException
      if (reset_)
        iso88591Decoder_.reset();
    return outputBytes;
  }
}
