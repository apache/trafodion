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
import java.nio.ByteBuffer;


// TO BE DONE (1) fix 3 serialization related methods.

/**
   *  Describes a column in an input or output table or a parameter
   *
   *  <p> This describes a column or parameter value that is passed through
   *  the UDR interface, either as a value read from an input table, a
   *  value produced in an output table or a parameter.
  */

  public class ColumnInfo extends TMUDRSerializableObject
  {

      /** Info on whether a table-valued input or output column is used */
      public enum ColumnUseCode
      {
          /** Column usage is not yet determined */
          UNKNOWN, 
          /** For an input, it’s needed by the UDF, for an output it’s needed by the SQL Engine */
          USED, 
          /** Input or output is not needed. Input will be removed after the describeDataflowAndPredicates() call. Output will be retained to avoid errors at runtime when the UDF tries to set this column value. */
          NOT_USED, 
          /** Output is not needed and will be removed after the describeDataflowAndPredicates() call. */ 
          NOT_PRODUCED
      };

      /**
       *  Default constructor
       */
      public ColumnInfo() {
          super(TMUDRObjectType.COLUMN_INFO_OBJ, getCurrentVersion());
          usage_ = ColumnUseCode.UNKNOWN;
          estimatedUniqueEntries_ = -1;
      }

      /**
       *  Constructor, specifying a name and a type
       *
       *  @param name       Name of the column to add. Use UPPER CASE letters,
       *                    digits and underscore, otherwise you will need to
       *                    use delimited column names with matching case in
       *                    Trafodion.
       *  @param type       Type of the column to add.
       */
      public ColumnInfo(String name, TypeInfo type){
          super(TMUDRObjectType.COLUMN_INFO_OBJ, getCurrentVersion());
          name_ = name;
          type_ = type;
          usage_ = ColumnUseCode.UNKNOWN;
          estimatedUniqueEntries_ = -1;
      }

      /**
       *  Copy constructor
       */
      public ColumnInfo(ColumnInfo c){
          super(TMUDRObjectType.COLUMN_INFO_OBJ, getCurrentVersion());
          name_ = new String(c.name_);
          type_ = new TypeInfo(c.type_);
          usage_ = c.usage_;
          estimatedUniqueEntries_ = c.estimatedUniqueEntries_;
          if (c.provenance_ != null)
              provenance_ = new ProvenanceInfo(c.provenance_);
      }

      /**
       *  Get the name of the column.
       *  @return Name of the column in UTF-8.
       */
      public String getColName() {
          return name_;
      }

      /**
       *  Get the type of the column.
       *  @return Type of the column.
       */
      public TypeInfo getType() {
          return type_ ;
      }

      /**
       *  Get the estimated number of unique entries.
       *
       *  <p> This returns an estimate for the number of unique values
       *  for this column in the table. For example, a column containing
       *  the names of US states would have approximately 50 distinct
       *  values, assuming that most or all states are represented.
       *  This estimate can be provided by the UDR writer, through the
       *  setUniqueEntries() method, or in some cases it can also be
       *  provided by the Trafodion compiler.
       *
       *  @see ColumnInfo#setEstimatedUniqueEntries(long)
       *
       *  @return Estimated number of unique entries.
       */
      public long getEstimatedUniqueEntries() {
          return estimatedUniqueEntries_ ;
      }

      /**
       *  Get the usage of an input or output column.
       *
       *  <p> This usage may be set in the
       *  UDR::describeDataflowAndPredicates() method,
       *  set automatically by Trafodion for certain situations
       *  with passthru columns, or left at the default of USED.
       *
       *  @return Usage enum value for the column.
       */
      public ColumnUseCode getUsage() {
          return usage_ ;
      }

      /**
       *  Get provenance info for an output column.
       *  @return Provenance of the column.
       */
      public ProvenanceInfo getProvenance() {
          if (provenance_ == null)
              provenance_ = new ProvenanceInfo();
          return provenance_;
      }

      // for use during compilation
      /**
       *  Set the name of the column.
       *  @param colName Name of the column (in UTF-8). There is a length
       *         limit of 256 bytes for the column name.
       */
      public void setColName(String colName) {
          name_ = colName;
      }

      /**
       *  Set the type of the column.
       *
       *  <p> This is done by constructing a TypeInfo object and passing it to this method.
       *
       *  @param type Type of the column.
       */
      public void setType(TypeInfo type) {
          type_ = type ;
      }

      /**
       *  Provide an estimate for the number of unique values of a column.
       *
       *  <p> Only use this method from within the following methods:
       *  <ul>
       *  <li> @link UDR#describeParamsAndColumns
       *  <li> @link UDR#describeDataflowAndPredicates
       *  <li> @link UDR#describeConstraints
       *  <li> @link UDR#describeStatistics
       *  </ul>
       *  @see ColumnInfo#getEstimatedUniqueEntries
       *
       *  @param estimatedUniqueEntries Estimate of the number of unique entries.
       */
      public void setEstimatedUniqueEntries(long estimatedUniqueEntries) {
          estimatedUniqueEntries_ = estimatedUniqueEntries ;
      }

      /**
       *  Set the usage of the column.
       *
       *  <p> See the ColumnInfo::COLUMN_USE enum for different options.
       *
       *  <p> Only use this method from within the following method:
       *  <ul>
       *  <li> UDR#describeParamsAndColumns()
       *  </ul>
       *  @param usage Usage enum value of the column.
       */
      public void setUsage(ColumnUseCode usage) {
          usage_ = usage;
      }

      /**
       *  Set the provenance of an output column.
       *
       *  <p> This defines a relationship between an output column and
       *  a column of a table-valued input from which the output value
       *  is copied. Such columns are called pass-thru columns. See
       *  class ProvenanceInfo for more information.
       *
       *  <p> Only use this method from within the following method:
       *  <ul>
       *  <li> UDR#describeParamsAndColumns()
       *  </ul>
       *  @param provenance The provenance information.
       */
      public void setProvenance(ProvenanceInfo provenance) {
          provenance_ = new ProvenanceInfo(provenance);
      }

      // Functions for debugging
      public String toString(boolean longForm) {
          String s = name_;
          if (longForm)
          {
              s += " ";
              s += type_.toString(longForm);
              if (provenance_ != null &&
                  provenance_.isFromInputTable())
              {
                  s += String.format(" passthru(%d,%d)",
                                     provenance_.getInputTableNum(),
                                     provenance_.getInputColumnNum());
              }
              switch (usage_)
              {
              case UNKNOWN:
              case USED:
                  // don't show anything for these "normal" cases
                  break;
              case NOT_USED:
                  s+= " (not used)";
                  break;
              case NOT_PRODUCED:
                  s+= " (not produced)";
                  break;
              default:
                  s+= " (invalid usage code)";
                  break;
              }
              if (estimatedUniqueEntries_ >= 0)
              {
                  s += String.format(" uec=%d", estimatedUniqueEntries_);
              }
          }
          return s;
      }

      
      public String toString() {
          return toString(false);
      }
      
      // UDR writers can ignore these methods
      static short getCurrentVersion() { return 1; }

      @Override
      int serializedLength() throws UDRException{
        return (super.serializedLength() + 
                serializedLengthOfString(name_) + 
                type_.serializedLength() + 
                (3 * serializedLengthOfInt()) + 
                serializedLengthOfLong());
      }

      @Override
      int serialize(ByteBuffer outputBuffer) throws UDRException{
        int origPos = outputBuffer.position();

        super.serialize(outputBuffer);

        serializeString(name_, 
                        outputBuffer);

        type_.serialize(outputBuffer);

        serializeInt(usage_.ordinal(), 
                     outputBuffer);

        serializeLong(estimatedUniqueEntries_,
                      outputBuffer);

        serializeInt(getProvenance().getInputTableNum(),
                     outputBuffer);

        serializeInt(getProvenance().getInputColumnNum(),
                     outputBuffer);

        int bytesSerialized = outputBuffer.position() - origPos;

        validateSerializedLength(bytesSerialized);

        return bytesSerialized;
      }

      @Override
      int deserialize(ByteBuffer inputBuffer) throws UDRException {

       int origPos = inputBuffer.position();
       
       super.deserialize(inputBuffer);
      
       validateObjectType(TMUDRObjectType.COLUMN_INFO_OBJ);

       name_ = deserializeString(inputBuffer);

       if (type_ == null)
         type_ = new TypeInfo();

       type_.deserialize(inputBuffer);

       int tempInt1 =  deserializeInt( inputBuffer);

       usage_ = ColumnUseCode.values()[tempInt1];

       estimatedUniqueEntries_ = deserializeLong(inputBuffer);

        tempInt1 = deserializeInt(inputBuffer);
        int tempInt2 = deserializeInt(inputBuffer);

        provenance_ = new ProvenanceInfo(tempInt1, tempInt2); 

        int bytesDeserialized = inputBuffer.position() - origPos;
        validateDeserializedLength(bytesDeserialized);

        return bytesDeserialized;
      }

      private String name_;
      private TypeInfo type_;
      private ColumnUseCode usage_;
      private long estimatedUniqueEntries_;
      private ProvenanceInfo provenance_;
  }


