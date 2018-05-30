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

// TO BE DONE (1) constructor needs to throw Exception (2) 
// fix 3 serialization related methods.

package org.trafodion.sql.udr;
import java.nio.ByteBuffer;

/**
 * A constraint on a table-valued input or output table
 *
 * <p> This could be a uniqueness constraint, a cardinality constraint
 *  or some other constraint
 */

public abstract class ConstraintInfo extends TMUDRSerializableObject
{

    /** Type of a constraint */
    public enum ConstraintTypeCode
    {
        /** Cardinality constraint */
        CARDINALITY,  
        /** Uniqueness constraint */
        UNIQUE;

        private static ConstraintTypeCode[] allValues = values();
        public static ConstraintTypeCode fromOrdinal(int n) {return allValues[n];}

      };

    protected ConstraintInfo(ConstraintTypeCode constraintType,
                             short version) {
        super((constraintType == ConstraintTypeCode.CARDINALITY ?
              TMUDRObjectType.CARDINALITY_CONSTRAINT_INFO_OBJ :
           (constraintType == ConstraintTypeCode.UNIQUE ?
              TMUDRObjectType.UNIQUE_CONSTRAINT_INFO_OBJ :
            TMUDRObjectType.UNKNOWN_OBJECT_TYPE)),version);
        constraintType_ = constraintType ;
    }

    /**
     *  Get the type of the constraint.
     *
     *  <p> This allows safe casting to derived classes, based on the type.
     *
     *  @return Type of the constraint.
     */
    public ConstraintTypeCode getType(){
        return constraintType_;
    }   

    // UDR writers can ignore these methods

    abstract String toString(TableInfo ti) throws UDRException;

    static short getCurrentVersion() { return 1; }

    @Override
    int serializedLength() throws UDRException{
      return super.serializedLength() + serializedLengthOfInt();
    }
 
    @Override
    int serialize(ByteBuffer outputBuffer) throws UDRException{
      int origPos = outputBuffer.position();

      super.serialize(outputBuffer);

      serializeInt(constraintType_.ordinal(),
                   outputBuffer);

      int bytesSerialized = outputBuffer.position() - origPos;

      return bytesSerialized;
    }

    @Override
    int deserialize(ByteBuffer inputBuffer) throws UDRException{

      int origPos = inputBuffer.position();

      super.deserialize(inputBuffer);

      int tempInt = deserializeInt(inputBuffer);
      constraintType_ = ConstraintTypeCode.fromOrdinal(tempInt);

      int bytesDeserialized = inputBuffer.position() - origPos;

      return bytesDeserialized;

    }

    private ConstraintTypeCode constraintType_;
}


