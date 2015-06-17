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
import java.util.Vector;
import java.nio.ByteBuffer;


// TO BE DONE (1) 3 serialize methods and (2) uncomment line in toString

/**
   *  A comparison predicate to be evaluated on a table
   *
   *  A predicate that compares a column value to a constant or
   *  another value that evaluates to a constant at runtime,
   *  like an SQL query parameter.
  */
public class ComparisonPredicateInfo extends PredicateInfo
{
    /**
     *  Get the column number of the column in this comparison predicate.
     *
     *  @return Column number.
     */
    public int getColumnNumber() {
        return columnNumber_;
    }

    /**
     *  Return whether this comparison value involves a constant.
     *
     *  <p> The method returns whether the comparison predicate is of the form
     *  "column" "op" "constant". If it returns false, the predicate
     *  compares the column with a parameter or some other value not
     *  available to the UDR. Predicates that do not involve a constant
     *  cannot be evaluated in the UDR itself, since the comparison value
     *  is not available to the UDR. They can be evaluated on a table-valued
     *  input, however.
     *
     *  @return true if the comparison is with a constant, false otherwise
     */
    public boolean hasAConstantValue() {
        return value_.length() > 0 ;
    }

    /**
     *  Return the value, as a string, of the constant in this predicate.
     *
     *  <p> This returns the value, using SQL syntax, of the constant involved
     *  in the comparison predicate. It throws an exception if method
     *  hasAConstantValue() would return false.
     *
     *  @see ComparisonPredicateInfo#hasAConstantValue()
     *
     *  @return Value of the constant in this comparison predicate.
     */
    public String getConstValue() {
        return value_;
    }

    // UDR writers can ignore these methods
    public ComparisonPredicateInfo() {
        super(TMUDRObjectType.COMP_PREDICATE_INFO_OBJ);
        columnNumber_ = -1;
    }

    public void setColumnNumber(int columnNumber) {
        columnNumber_ = columnNumber;
    }

    public void setValue(String value) {
        value_ = value;
    }
       
    public void mapColumnNumbers(Vector<Integer> map) throws UDRException {
        if (map.get(columnNumber_).intValue() < 0)
            throw new UDRException(
                                   38900,
                                   "Invalid column mapping for column %d in a predicate",
                                   columnNumber_);
        columnNumber_ = map.get(columnNumber_).intValue();
    }

    public String toString(TableInfo ti) {
        
        //String s = ti.getColumn(columnNumber_).getColName();
        String s = "hi";
        switch (getOperator())
        {
        case UNKNOWN_OP:
            s += " unknown operator ";
            break;
        case EQUAL:
                s += " = ";
                break;
        case NOT_EQUAL:
            s += " <> ";
            break;
        case LESS:
            s += " < ";
            break;
        case LESS_EQUAL:
            s += " <= ";
            break;
        case GREATER:
            s += " > ";
            break;
        case GREATER_EQUAL:
            s += " >= ";
            break;
        case IN:
            s += " in ";
            break;
        case NOT_IN:
            s += " not in ";
            break;
        default:
            s += " invalid operator ";
            break;
        }

        s += value_;
        return s;
    }
    
    public static short getCurrentVersion() { return 1; }
    public int serializedLength() throws UDRException{
      return super.serializedLength() +
        serializedLengthOfInt() +
        serializedLengthOfString(value_);
    }
    public int serialize(ByteBuffer outputBuffer) throws UDRException{

      int origPos = outputBuffer.position();

      super.serialize(outputBuffer);

      serializeInt(columnNumber_,
                   outputBuffer);

      serializeString(value_,
                      outputBuffer);

      int bytesSerialized = outputBuffer.position() - origPos;

      validateSerializedLength(bytesSerialized);

      return bytesSerialized;
    }

    public int deserialize(ByteBuffer inputBuffer) throws UDRException{

      int origPos = inputBuffer.position();

      super.deserialize(inputBuffer);

      columnNumber_ = deserializeInt(inputBuffer);
      value_ = deserializeString(inputBuffer);

      int bytesDeserialized = inputBuffer.position() - origPos;

      validateObjectType(TMUDRObjectType.COMP_PREDICATE_INFO_OBJ);
      validateDeserializedLength(bytesDeserialized);

      return bytesDeserialized;
    }
    
    private int columnNumber_;
    private String value_;
}

