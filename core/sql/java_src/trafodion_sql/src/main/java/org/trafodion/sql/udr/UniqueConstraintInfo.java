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

// TO BE DONE (1) fix 3 serialization related methods. (2) uncomment line in toString

package org.trafodion.sql.udr;
import java.util.Vector;
import java.util.ListIterator;
import java.nio.ByteBuffer;


/**
 *  A uniqueness constraint
 *
 *  <p> A list of columns that, together, form a unique key
 */
public class UniqueConstraintInfo extends ConstraintInfo
{

    /**
     *  Default constructor for an empty uniqueness constraint.
     *
     *  <p> Use method addColumn() to add columns.
     */
    public UniqueConstraintInfo() {
        super(ConstraintTypeCode.UNIQUE, getCurrentVersion());
        uniqueColumns_ = new Vector<Integer>();
    }

    public UniqueConstraintInfo(UniqueConstraintInfo constraint) throws UDRException {
        super(ConstraintTypeCode.UNIQUE, getCurrentVersion());
        uniqueColumns_ = new Vector<Integer>();
        for (int i=0; i<constraint.getNumUniqueColumns(); i++) 
                uniqueColumns_.add(Integer.valueOf(constraint.getUniqueColumn(i)));
    }
    /**
     *  Get the number of columns that form the unique key.
     *  @return Number of columns in the uniqueness constraint.
     */
    public int getNumUniqueColumns() {
        return uniqueColumns_.size();
    }

    /**
     *  Get a column of the uniqueness constraint by iterator.
     *
     *  <p> Like in other methods, we use an integer to iterate over the
     *  columns in the set. Note that the columns form a set, so this
     *  number i is merely there to iterate over the set of columns.
     *
     *  @param i A number between 0 and getNumUniqueColumns()-1. 
     *  @return Column number/ordinal of the unique column.
     *  @throws UDRException
     */
    public int getUniqueColumn(int i) throws UDRException {
        if (i < 0 || i >= uniqueColumns_.size())
            throw new UDRException(
                                   38900,
                                   "Invalid index in getUniqueColumn: %d, has %d columns",
                                   i, uniqueColumns_.size());
        return uniqueColumns_.get(i).intValue();
    }

    /**
     *  Add a column to a uniqueness constraint.
     *
     *  @param c Column number/ordinal of one of the unique columns in the
     *           constraint.
     */
    public void addColumn(int c) {
        ListIterator<Integer> it = uniqueColumns_.listIterator();
        int val = -1;
        // insert columns ordered by number and ignore duplicates
        // skip over any elements < c
        while(it.hasNext()) {
            val = it.next().intValue();
            if (val >= c)
                break;
        }

        if (val != c) {
            if (val > c) {
                it.previous(); // go back by 1 if we seen something larger than c
            }
            it.add(Integer.valueOf(c));
        }
    }
               
    // UDR writers can ignore these methods
    @Override
    public String toString(TableInfo ti) throws UDRException {
        String s = "unique(";
        for (int c=0; c<uniqueColumns_.size(); c++)
        {
            if (c>0)
                s +=  ", ";
            
            s += ti.getColumn(uniqueColumns_.get(c)).getColName();
        }
        s += ")";
        return s;
    }
    public static short getCurrentVersion() { return 1; }

    @Override
    public int serializedLength() throws UDRException {
      return super.serializedLength() +
              serializedLengthOfBinary(uniqueColumns_.size() * 4);
    }

    @Override
    public int serialize(ByteBuffer outputBuffer) throws UDRException {

      int origPos = outputBuffer.position();

      super.serialize(outputBuffer);
      int numCols = uniqueColumns_.size();

      // add the binary length of the following array
      serializeInt(numCols * 4,
                   outputBuffer);

      for (int u=0; u<numCols; u++)
         outputBuffer.putInt(uniqueColumns_.get(u).intValue());

      int bytesSerialized = outputBuffer.position() - origPos;

      validateSerializedLength(bytesSerialized);

      return bytesSerialized;
    }

    @Override
    public int deserialize(ByteBuffer inputBuffer) throws UDRException {
      int numCols = 0;

      int origPos = inputBuffer.position();

      super.deserialize(inputBuffer);
      validateObjectType(TMUDRObjectType.UNIQUE_CONSTRAINT_INFO_OBJ);

      numCols = deserializeInt(inputBuffer) /  4;

      for (int u=0; u<numCols; u++) {
        uniqueColumns_.add(deserializeInt(inputBuffer));
      }

      int bytesDeserialized = inputBuffer.position() - origPos;

      validateDeserializedLength(bytesDeserialized);

      return bytesDeserialized;
    }

    private Vector<Integer> uniqueColumns_;
}
