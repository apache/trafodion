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

// TO BE DONE (1) fix 3 serialization related methods.

package org.trafodion.sql.udr;
import java.nio.ByteBuffer;


/**
 *  A cardinality constraint
 *
 *  <p> Upper and/or lower bounds for the cardinality of
 *  a table. Note that unlike cardinality estimates, this
 *  is a hard constraint that must be followed by the table,
 *  otherwise incorrect results and errors may occur.
 */
public class CardinalityConstraintInfo extends ConstraintInfo
{

     /**
     *  Default constructor for a cardinality constraint.
     *
     *  <p> Minimum number of rows is 0 and the mamximum is unbounded.
     */
    public CardinalityConstraintInfo() {
        super(ConstraintTypeCode.CARDINALITY, getCurrentVersion());
        minNumRows_ = 0;
        maxNumRows_ = -1;
    }

    /**
     *  Construct a new cardinality constraint.
     *
     *  <p> A cardinality constraint allows to specify a lower and/or an upper
     *  limit for the number of rows in a table.
     *
     *  @param minNumRows The minimum number of rows in the table, 0 or
     *                    a positive number.
     *  @throws UDRException
     */
    public CardinalityConstraintInfo(long minNumRows) throws UDRException {
        super(ConstraintTypeCode.CARDINALITY, getCurrentVersion());
        minNumRows_ = minNumRows;
        maxNumRows_ = -1;
        if (minNumRows < 0)
            throw new UDRException(
                                   38900,
                                   "Invalid lower bound for cardinality constraint: %d",
                                   minNumRows);
    }

    /**
     *  Construct a new cardinality constraint.
     *
     *  <p> A cardinality constraint allows to specify a lower and/or an upper
     *  limit for the number of rows in a table.
     *
     *  @param minNumRows The minimum number of rows in the table, 0 or
     *                    a positive number.
     *  @param maxNumRows The maximum number of rows in the table, or -1
     *                    if there is no upper bound. If it is not -1, maxNumRows
     *                    must be greater or equal minNumRows.
     *  @throws UDRException
     */
    public CardinalityConstraintInfo(long minNumRows,
                                     long maxNumRows) throws UDRException {
        super(ConstraintTypeCode.CARDINALITY, getCurrentVersion());
        minNumRows_ = minNumRows;
        maxNumRows_ = -maxNumRows;
        if (minNumRows < 0 ||
            maxNumRows < -1 ||
            maxNumRows >= 0 && minNumRows > maxNumRows)
            throw new UDRException(
                                   38900,
                                   "Invalid lower/upper bound for cardinality constraint: (%d, %d)",
                                   minNumRows, maxNumRows);
    }

    /** Copy constructor */
    public CardinalityConstraintInfo(CardinalityConstraintInfo constraint)  {
        super(ConstraintTypeCode.CARDINALITY, getCurrentVersion());
        minNumRows_ = constraint.getMinNumRows();
        maxNumRows_ = -constraint.getMaxNumRows();
    }

    /**
     *  Return the minimum number of rows in a table.
     *  @return Minimum number of rows (0 or a positive number).
     */
    long getMinNumRows() {
        return minNumRows_;
    }

    /**
     *  Return the maximum number of rows in a table.
     *  @return Maximum number of rows or -1 if there is no upper bound.
     */
    long getMaxNumRows() {
        return maxNumRows_;
    }  

    // UDR writers can ignore these methods
    @Override
    public String toString(TableInfo ti) {
        return String.format("cardinality constraint(min=%d, max=%d)",
                             minNumRows_, maxNumRows_);
    }

    public static short getCurrentVersion() { return 1; }

    @Override
    public int serializedLength() throws UDRException{
      return super.serializedLength() + 2 * serializedLengthOfLong();
    }

    @Override
    public int serialize(ByteBuffer outputBuffer) throws UDRException{

      int origPos = outputBuffer.position();

      super.serialize(outputBuffer);

      serializeLong(minNumRows_,
                    outputBuffer);

      serializeLong(maxNumRows_,
                    outputBuffer);

      int bytesSerialized = outputBuffer.position() - origPos;

      validateSerializedLength(bytesSerialized);

      return bytesSerialized;
    }

    @Override
    public int deserialize(ByteBuffer inputBuffer) throws UDRException{

      int origPos = inputBuffer.position();

      super.deserialize(inputBuffer);

      validateObjectType(TMUDRObjectType.CARDINALITY_CONSTRAINT_INFO_OBJ);

      minNumRows_ = deserializeLong(inputBuffer);

      maxNumRows_ = deserializeLong(inputBuffer);

      int bytesDeserialized = inputBuffer.position() - origPos;
      validateDeserializedLength(bytesDeserialized);

      return bytesDeserialized;
    }

    private long minNumRows_;
    private long maxNumRows_;
}


