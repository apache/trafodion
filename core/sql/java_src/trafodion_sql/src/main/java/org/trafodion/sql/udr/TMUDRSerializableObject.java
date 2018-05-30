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



package org.trafodion.sql.udr;
import org.trafodion.sql.udr.UDRException;
import java.nio.ByteBuffer;

  // Class to help with serialization, note that it is not required
  // to inherit from this class in order to participate in serialization.
  // UDR writers can ignore this class.
public class TMUDRSerializableObject
{

  enum Endianness
   {
     UNKNOWN_ENDIANNESS,
     IS_LITTLE_ENDIAN,
     IS_BIG_ENDIAN

/*
     private final int endianness_;
  
    Endianness(int val)
    {
      endianness_ = val;
    }
     
    public int Endianness()
    {
      return endianness_;
    }
*/
  
   };

  enum TMUDRObjectType
    {
      UNKNOWN_OBJECT_TYPE (0),
      TYPE_INFO_OBJ (100),
      PARAMETER_INFO_OBJ (200),
      COLUMN_INFO_OBJ (400),
      CARDINALITY_CONSTRAINT_INFO_OBJ (510),
      UNIQUE_CONSTRAINT_INFO_OBJ (520),
      COMP_PREDICATE_INFO_OBJ (710),
      PARTITION_INFO_OBJ (800),
      ORDER_INFO_OBJ (900),
      TUPLE_INFO_OBJ (1000),
      TABLE_INFO_OBJ (1100),
      PARAMETER_LIST_INFO_OBJ (1200),
      UDR_INVOCATION_INFO_OBJ (1300),
      UDR_PLAN_INFO_OBJ (1400);

      private final int value_;
  
      private TMUDRObjectType(int val)
      {
        value_ = val;
      }
     
/*
      private static final Map<Integer, TMUDRObjectType> intToObjectType = new HashMap<Integer, TMUDRObjectType>();

      static {
        for (TMUDRObjectType objType : TMUDRObjectType.values()) {
            intToObjectType.put(objType.value_, objType);
        }
      }

      public static TMUDRObjectType fromInt(int value) {
        return intToObjectType.get(value);
      }
*/


      public int getValue()
      {
        return value_;
      }

    };

  TMUDRSerializableObject(
       TMUDRObjectType objectType,
       short version) { 
  objectType_  = objectType.value_;
  totalLength_ = -1; // will be set when we serialize the object
  version_     = version;
  // this class is serialized as little-endian
  endianness_  = (short) Endianness.IS_LITTLE_ENDIAN.ordinal();
  flags_       = 0;
  filler_      = 0;
  }

  int sizeOf()
  {
    return  SIZEOF_INT + SIZEOF_INT + SIZEOF_SHORT +
            SIZEOF_SHORT + SIZEOF_INT + SIZEOF_INT;
  }

  TMUDRObjectType getObjectType()
  {
    switch (objectType_)
    {
      case 100:
        return TMUDRObjectType.TYPE_INFO_OBJ;
      case 200:
        return TMUDRObjectType.PARAMETER_INFO_OBJ;
      case 400:
        return TMUDRObjectType.COLUMN_INFO_OBJ;
      case 510:
        return TMUDRObjectType.CARDINALITY_CONSTRAINT_INFO_OBJ;
      case 520:
        return TMUDRObjectType.UNIQUE_CONSTRAINT_INFO_OBJ;
      case 710:
        return TMUDRObjectType.COMP_PREDICATE_INFO_OBJ;
      case 800:
        return TMUDRObjectType.PARTITION_INFO_OBJ;
      case 900:
        return TMUDRObjectType.ORDER_INFO_OBJ;
      case 1000:
        return TMUDRObjectType.TUPLE_INFO_OBJ;
      case 1100:
        return TMUDRObjectType.TABLE_INFO_OBJ;
      case 1200:
        return TMUDRObjectType.PARAMETER_LIST_INFO_OBJ;
      case 1300:
        return TMUDRObjectType.UDR_INVOCATION_INFO_OBJ;
      case 1400:
        return TMUDRObjectType.UDR_PLAN_INFO_OBJ;
      case 0:
      default:
        return TMUDRObjectType.UNKNOWN_OBJECT_TYPE;
    }
  }


  TMUDRObjectType getObjectType(int objectType)
  {
    switch (objectType)
    {
      case 100:
        return TMUDRObjectType.TYPE_INFO_OBJ;
      case 200:
        return TMUDRObjectType.PARAMETER_INFO_OBJ;
      case 400:
        return TMUDRObjectType.COLUMN_INFO_OBJ;
      case 510:
        return TMUDRObjectType.CARDINALITY_CONSTRAINT_INFO_OBJ;
      case 520:
        return TMUDRObjectType.UNIQUE_CONSTRAINT_INFO_OBJ;
      case 710:
        return TMUDRObjectType.COMP_PREDICATE_INFO_OBJ;
      case 800:
        return TMUDRObjectType.PARTITION_INFO_OBJ;
      case 900:
        return TMUDRObjectType.ORDER_INFO_OBJ;
      case 1000:
        return TMUDRObjectType.TUPLE_INFO_OBJ;
      case 1100:
        return TMUDRObjectType.TABLE_INFO_OBJ;
      case 1200:
        return TMUDRObjectType.PARAMETER_LIST_INFO_OBJ;
      case 1300:
        return TMUDRObjectType.UDR_INVOCATION_INFO_OBJ;
      case 1400:
        return TMUDRObjectType.UDR_PLAN_INFO_OBJ;
      case 0:
      default:
        return TMUDRObjectType.UNKNOWN_OBJECT_TYPE;
    }
  }

  short getVersion() 
  {
    return version_;
  }

  int serializedLength() throws UDRException
  {
    return sizeOf();
  }

  int serialize(ByteBuffer outputBuffer) throws UDRException
  {
    totalLength_ = serializedLength();

    int bufSize = outputBuffer.limit() - outputBuffer.position();
    if (bufSize < totalLength_)
      throw new UDRException(38900,"need %d bytes to serialize object of type %d, have %d bytes",
                             totalLength_,
                             objectType_,
                             bufSize);

    outputBuffer.putInt(objectType_);
    outputBuffer.putInt(totalLength_);
    outputBuffer.putShort(version_);
    outputBuffer.putShort(endianness_);
    outputBuffer.putInt(flags_);
    outputBuffer.putInt(filler_);
    int allocatedBytes = bufSize - (outputBuffer.limit() - outputBuffer.position());
    return allocatedBytes;
  }

  int deserialize(ByteBuffer inputBuffer) throws UDRException
  {
    int bufSize = inputBuffer.limit() - inputBuffer.position();
    if (bufSize < sizeOf())
      throw new UDRException(38900,
                             "not enough data to deserialize object header, need %d, got %d bytes",
                             sizeOf(),
                             bufSize);

    objectType_ = inputBuffer.getInt();
    totalLength_ = inputBuffer.getInt();
    version_ = inputBuffer.getShort();
    endianness_ = inputBuffer.getShort();
    flags_ = inputBuffer.getInt();
    filler_ = inputBuffer.getInt();

    if (bufSize < totalLength_)
        throw new UDRException(
           38900,
           "not enough data to deserialize object of type %d, need %d, got %d bytes",
           objectType_,
           totalLength_,
           bufSize);

    int retrievedBytes = bufSize - (inputBuffer.limit() - inputBuffer.position());

    return retrievedBytes;
  }


  void validateObjectType(TMUDRObjectType o) throws UDRException
  {
    if (objectType_ != o.getValue())
      throw new UDRException(38900,
                             "Object type of expected object (%d) does not match the type (%d) in the serialized buffer",
                       	     o,
                             objectType_);

  }

  void validateSerializedLength(int l) throws UDRException
  {
      if (l != totalLength_)
         throw new UDRException(38900,
                                "Expected %d bytes to serialize object of type %d, actually produced %d bytes",
                                totalLength_,
                                objectType_,
                                l);

  }

  void validateDeserializedLength(int l) throws UDRException
  {
    if (l != totalLength_)
      throw new UDRException(38900,
                             "Expected %d bytes to deserialize object of type %d, actually consumed %d bytes",
                             totalLength_,
                             objectType_,
                             l);
  }


  // helper methods to serialize ints and strings, they
  // return the length of the serialized information
  int serializedLengthOfInt()
  {
    return SIZEOF_INT;
  }

  int serializedLengthOfLong()
  {
    return SIZEOF_LONG;
  }

  int serializedLengthOfString(byte[] s)
  {
    return (SIZEOF_INT + s.length);
  }

  int serializedLengthOfString(int stringLength)
  {
    return SIZEOF_INT + stringLength;
  }

  int serializedLengthOfString(String s) throws UDRException
  {
    byte[] temp;
    try {
      temp = s.getBytes("UTF8");
    }
    catch (Exception e) {
      throw new UDRException(38900,
                             "could not convert string to UTF8");
    }

    return serializedLengthOfString(temp.length);
  }

  int serializedLengthOfBinary(int binaryLength)
  {
    return serializedLengthOfString(binaryLength);
  }

  int serializeInt(int i,
                   ByteBuffer outputBuffer) throws UDRException
  {
    int bufSize = outputBuffer.limit() - outputBuffer.position();
    if (bufSize < SIZEOF_INT)
      throw new UDRException(38900,"insufficient space to serialize an int");

    outputBuffer.putInt(i);
    int allocatedBytes = bufSize - (outputBuffer.limit() - outputBuffer.position());

    return allocatedBytes;
  }

  int serializeLong(long i,
                    ByteBuffer outputBuffer) throws UDRException
  {
    int bufSize = outputBuffer.limit() - outputBuffer.position();
    if (bufSize < SIZEOF_LONG)
      throw new UDRException(38900,"insufficient space to serialize an long");

    outputBuffer.putLong(i);
    int allocatedBytes = bufSize - (outputBuffer.limit() - outputBuffer.position());

    return allocatedBytes;
  }

  int serializeBinary(byte[] s,
                      ByteBuffer outputBuffer) throws UDRException
  {
    int bufSize = outputBuffer.limit() - outputBuffer.position();
    if (bufSize < SIZEOF_INT + s.length)
      throw new UDRException(38900,
                          "buffer to serialize string has %d bytes, needs %d",
                           bufSize, SIZEOF_INT + s.length);


    outputBuffer.putInt(s.length);
    outputBuffer.put(s);
    int allocatedBytes = bufSize - (outputBuffer.limit() - outputBuffer.position());

    return allocatedBytes;
  }

  int serializeBinary(byte[] s,
                      int len,
                      ByteBuffer outputBuffer) throws UDRException
  {
    int bufSize = outputBuffer.limit() - outputBuffer.position();
    if (bufSize < SIZEOF_INT + len)
      throw new UDRException(38900,
                          "buffer to serialize string has %d bytes, needs %d",
                           bufSize, SIZEOF_INT + len);

    outputBuffer.putInt(len);
    outputBuffer.put(s,0,len);
    int allocatedBytes = bufSize - (outputBuffer.limit() - outputBuffer.position());

    return allocatedBytes;
  }

  int serializeString(String s,
                      ByteBuffer outputBuffer) throws UDRException
  {
    byte[] b;
    try {
      b = s.getBytes("UTF8");
    }
    catch (Exception e) {
      throw new UDRException(38900,
                             "could not convert string to UTF8");
    }
    return serializeBinary(b, outputBuffer);
  }

  int deserializeInt(ByteBuffer inputBuffer) throws UDRException
  {
    int bufSize = inputBuffer.limit() - inputBuffer.position();
    if (bufSize < SIZEOF_INT)
      throw new UDRException(38900,"insufficient space to deserialize an int");

    return inputBuffer.getInt();
  }

  long deserializeLong(ByteBuffer inputBuffer) throws UDRException
  {
    int bufSize = inputBuffer.limit() - inputBuffer.position();
    if (bufSize < SIZEOF_LONG)
      throw new UDRException(38900,"insufficient space to deserialize a long");

    return inputBuffer.getLong();
  }

/*
  byte[] deserializeString(ByteBuffer inputBuffer) throws UDRException
  {
    int bufSize = inputBuffer.limit() - inputBuffer.position();
    if (bufSize <  SIZEOF_INT)
          throw new UDRException(38900,
                                 "insufficient space to deserialize length field of a string");

    int len = inputBuffer.getInt();

    Charset utf8Charset = Charset.forName("UTF-8");
    byte[] temp = utf8Charset.decode(inputBuffer).toString().getBytes();
    if (temp.length < len)
         throw new UDRException(38900,
                                "string length indicator value %d exceeds size %d of serialized buffer",
                                len, result.length);
    byte result[] = new byte[temp.length];
    System.arrayCopy(temp, 0, result, 0, temp.length);

    if (len < 0)
      return null;
    else 
      return result;
  }
*/

  byte[] deserializeBinary(ByteBuffer inputBuffer) throws UDRException {
    int bufSize = inputBuffer.limit() - inputBuffer.position();
    if (bufSize <  SIZEOF_INT)
          throw new UDRException(38900,
                                 "insufficient space to deserialize length field of a string");

    int len = inputBuffer.getInt();

    if ((bufSize - SIZEOF_INT) < len)
         throw new UDRException(38900,
                                "string length indicator value %d exceeds size %d of serialized buffer",
                                len, bufSize - SIZEOF_INT);
    byte[] b = new byte[len];
    inputBuffer.get(b, 0, len);

    return b;
  }
  
  String deserializeString(ByteBuffer inputBuffer) throws UDRException
  {
    int bufSize = inputBuffer.limit() - inputBuffer.position();
    if (bufSize <  SIZEOF_INT)
          throw new UDRException(38900,
                                 "insufficient space to deserialize length field of a string");

    int len = inputBuffer.getInt();

    if ((bufSize - SIZEOF_INT) < len)
         throw new UDRException(38900,
                                "string length indicator value %d exceeds size %d of serialized buffer",
                                len, bufSize - SIZEOF_INT);
    byte[] b = new byte[len];
    inputBuffer.get(b, 0, len);
    String result;
    try {
      result = new String(b, "UTF8");
    }
    catch (Exception e) {
          throw new UDRException(38900,
                                 "insufficient space to deserialize length field of a string");
    }
    return result;
  }

  TMUDRObjectType getNextObjectType(ByteBuffer inputBuffer) throws UDRException
  {
    int bufSize = inputBuffer.limit() - inputBuffer.position();
    int origPos = inputBuffer.position();

    if (bufSize < sizeOf())
      throw new UDRException(38900,
                             "not enough data to look at next object header, need %d, got %d bytes",
                             sizeOf(),
                             bufSize);
    TMUDRObjectType objType =  getObjectType(inputBuffer.getInt());
    inputBuffer.position(origPos);

    return objType;
  }


  private static final int SIZEOF_INT=4;
  private static final int SIZEOF_SHORT=2;
  private static final int SIZEOF_LONG=8;
  private int objectType_;
  private int totalLength_;
  private short version_;
  private short endianness_;
  private int flags_;
  private int filler_;
}

