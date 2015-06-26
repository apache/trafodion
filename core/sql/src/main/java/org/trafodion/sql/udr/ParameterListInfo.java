// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

package org.trafodion.sql.udr;
import java.nio.ByteBuffer;

/**
   *  Describes the parameters of a UDR.
   *
   *  <p> This method currently has no methods relevant to the UDR
   *  writer, but the base class, TupleInfo, has a variety of methods.
   *
   *  @see TupleInfo
  */

public class ParameterListInfo extends TupleInfo
{
    public ParameterListInfo() {
        super(TMUDRObjectType.PARAMETER_LIST_INFO_OBJ, getCurrentVersion());
    }
    
    // UDR writers can ignore these methods    
    public static short getCurrentVersion() { return 1; }
    public int serializedLength() throws UDRException {
      int result = super.serializedLength() + serializedLengthOfInt();

      result += serializedLengthOfBinary(constBuffer_.limit());

      return result;
    }

    public int serialize(ByteBuffer outputBuffer) throws UDRException {
      int origPos = outputBuffer.position();
      super.serialize(outputBuffer);

      serializeBinary(constBuffer_.array(),
                      outputBuffer);

      int bytesSerialized = outputBuffer.position() - origPos;

      validateSerializedLength(bytesSerialized);

      return bytesSerialized;
    }

    public int deserialize(ByteBuffer inputBuffer) throws UDRException {

      int origPos = inputBuffer.position();

      super.deserialize(inputBuffer);

      validateObjectType(TMUDRObjectType.PARAMETER_LIST_INFO_OBJ);

      int size = deserializeInt(inputBuffer);

      byte[] tmp = new byte[size];
      inputBuffer.get(tmp,0,size);

      setConstBuffer(tmp);
      setRow(tmp);

      int bytesDeserialized = inputBuffer.position() - origPos;
      validateSerializedLength(bytesDeserialized);

      return bytesDeserialized;
    }
    
    
    private void setConstBuffer(byte[] constBuffer) {
        constBuffer_ = ByteBuffer.wrap(constBuffer);
    }
    private ByteBuffer constBuffer_;
    
}
