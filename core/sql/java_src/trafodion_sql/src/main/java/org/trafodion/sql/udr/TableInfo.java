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
import java.util.Vector;
import java.nio.ByteBuffer;

/**
 *  Describes a table-valued input or a table-valued output
 */
public class TableInfo extends TupleInfo {

    public TableInfo() {
        super(TMUDRObjectType.TABLE_INFO_OBJ, getCurrentVersion());
        estimatedNumRows_ = -1;
        estimatedNumPartitions_ = -1;
        queryPartitioning_ = new PartitionInfo();
        queryOrdering_ = new OrderInfo();
        constraints_ = new Vector<ConstraintInfo>();
    }
    
    // Functions for use by UDR writer, both at compile and at run time
    
    /**
     *  Get the estimated number of rows of this table.
     *
     *  @see TableInfo#setEstimatedNumRows(long)
     *  @see TableInfo#getEstimatedNumPartitions()
     *
     *  @return Estimated number of rows.
     */
    public long getEstimatedNumRows() {
        return estimatedNumRows_;
    }

    /**
     *  Get the estimated number of rows of this table.
     *
     *  @see TableInfo#getEstimatedNumRows()
     *
     *  @return Estimated number of partitions, if the table
     *          has a PARTITION BY clause, -1 otherwise.
     */
    public long getEstimatedNumPartitions() {
        return estimatedNumPartitions_;
    }

    /**
     * Get the PARTITION BY clause for this input table.
     *
     *  <p> This returns either the PARTITION BY clause specified in the
     *  SQL query, or the updated partitioning information, set by
     *  UDRInvocationInfo#setChildPartitioning(), called during
     *  UDR#describeParamsAndColumns().
     *
     *  @return Partitioning clause for this input table.
     */
    public PartitionInfo getQueryPartitioning() {
        return queryPartitioning_;
    }

    /**
     *  Get the ORDER BY clause for this input table.
     *
     *  <p> This returns either the ORDER BY clause specified in the
     *  SQL query, or the updated ordering information, set by
     *  UDRInvocationInfo#setChildOrdering(), called during
     *  UDR#describeParamsAndColumns().
     *
     *  @return Ordering clause for this input table.
     */
    public OrderInfo getQueryOrdering() {
        return queryOrdering_ ;
    }

    /**
     *  Returns whether the UDF result is treated as a continuous stream.
     *
     *  <p> Note: This is currently not supported. The method always returns false
     *  for now.
     *
     *  @return true if the UDF result is a stream, false otherwise.
     */
    public boolean isStream() {
        return false ;
    }

    /**
     *  Get the number of constraints defined on this table.
     *
     *  @return Number of constraints defined on this table.
     */
    public int getNumConstraints() {
        return constraints_.size();
    }

    /**
     *  Get a constraint by index/ordinal number.
     *
     *  @param i index/ordinal (0-based) of the constraint.
     *  @return Constraint for a given index/ordinal.
     *  @throws UDRException
     */
    public ConstraintInfo getConstraint(int i) throws UDRException {
        if (i < 0 || i >= constraints_.size())
            throw new UDRException(
                                   38900,
                                   "Trying to access constraint %d of a ConstraintInfo object with %d constraints",
                                   i, constraints_.size());
        
        return constraints_.get(i);
    }

    // non-const methods, used during compile time only
    /**
 *  Set the estimated number of rows for a UDF table-valued result.
 *
 *  <p> Setting this value can help the Trafodion optimizer generate a better
 *  plan for queries containing table-valued UDFs. Note that this is only
 *  an estimate, a strict correspondence to the actual number of rows
 *  returned at runtime is not required.
 *
 *  <p> Only use this method from within the following methods:
 *  <ul>
 *  <li> UDR#describeParamsAndColumns()
 *  <li> UDR#describeDataflowAndPredicates()
 *  <li> UDR#describeConstraints()
 *  <li> UDR#describeStatistics()
 *  </ul>
 *  @param rows Estimated number of rows for this table.
 */
    public void setEstimatedNumRows(long rows) {
        estimatedNumRows_ = rows ;
    }

    /**
     *  Add a cardinality constraint to the UDF table-valued output.
     *
     *  <p> Only use this method from within the following methods:
     * <ul>
     *  <li> UDR#describeParamsAndColumns()
     *  <li> UDR#describeDataflowAndPredicates()
     *  <li> UDR#describeConstraints()
     * </ul>
     *  @param constraint New constraint to add. 
     */
    public void addCardinalityConstraint(CardinalityConstraintInfo constraint) {
        constraints_.add(new CardinalityConstraintInfo(constraint));
    }

    /**
     *  Add a uniqueness constraint to the UDF table-valued output.
     *
     *  <p> Only use this method from within the following methods:
     * <ul>
     * <li> UDR#describeParamsAndColumns()
     * <li> UDR#describeDataflowAndPredicates()
     * <li> UDR#describeConstraints()
     * </ul>
     *  @param constraint New uniqueness constraint to add. 
     * @throws UDRException
     */
    public void addUniquenessConstraint(UniqueConstraintInfo constraint) throws UDRException {
        constraints_.add(new UniqueConstraintInfo(constraint));
    }

    /**
     *  Set whether a table should be treated as a stream.
     *
     *  <p> This method is not yet supported.
     *
     *  @param stream true if the table is a stream, false otherwise.
     *  @throws UDRException
     */
    public void setIsStream(boolean stream) throws UDRException {
        if (stream)
            throw new UDRException(38908, "Stream tables not yet supported");
    }

    // Functions for debugging
    /**
     *  Print the object, for use in debugging.
     *
     *  @see UDR#debugLoop()
     *  @see UDRInvocationInfo.DebugFlags#PRINT_INVOCATION_INFO_AT_RUN_TIME
     */
    @Override
    public void print() throws UDRException {
        super.print();
        StringBuilder sb = new StringBuilder();
        sb.append(String.format("    Estimated number of rows : %d\n", getEstimatedNumRows()));
        sb.append("    Partitioning             : ");
        switch (getQueryPartitioning().getType())
        {
        case UNKNOWN:
            sb.append("unknown\n");
            break;
        case ANY:
            sb.append("any\n");
            break;
        case SERIAL:
            sb.append("serial\n");
            break;
        case PARTITION:
            boolean needsComma = false;
            sb.append("(");
            for (int p=0; p<getQueryPartitioning().getNumEntries(); p++)
            {
                if (needsComma)
                    sb.append(", ");
                try {sb.append(getColumn(getQueryPartitioning().getColumnNum(p)).getColName()); }
                catch (Exception e1) 
                    {sb.append(String.format("error while getting partition info for column %d\n", p));}
                needsComma = true;
            }
            sb.append(")\n");
            sb.append(String.format("    Estimated # of partitions: %d\n", getEstimatedNumPartitions()));
            break;
        case REPLICATE:
            sb.append("replicate\n");
            break;
        default:
            sb.append("invalid partitioning specification!\n");
            break;
        }
        sb.append("    Ordering                 : ");
        if (getQueryOrdering() != null &&
            getQueryOrdering().getNumEntries() > 0)
        {
            sb.append("(");
            for (int o=0; o<getQueryOrdering().getNumEntries(); o++)
            {
                if (o>0)
                    sb.append(", ");
                try {sb.append(getColumn(getQueryOrdering().getColumnNum(o)).getColName());}
                 catch (Exception e1) 
                    {sb.append(String.format("error while getting order info for column %d\n", o));}
                OrderInfo.OrderTypeCode ot ;
                try {ot = getQueryOrdering().getOrderType(o);}
                catch (Exception e1) {ot = OrderInfo.OrderTypeCode.NO_ORDER;}
                if (ot == OrderInfo.OrderTypeCode.DESCENDING)
                    sb.append(" DESC");
                else if (ot != OrderInfo.OrderTypeCode.ASCENDING)
                    sb.append(" - invalid order type!");
            }
            sb.append(")\n");
        }
        else
            sb.append("none\n");
        if (constraints_.size() > 0)
        {
            sb.append("    Constraints              :\n");
            
            for (int c=0; c<constraints_.size(); c++)
                sb.append("        " + constraints_.elementAt(c).toString(this));
        }
        System.out.print(sb.toString());
    }

    public String toString(boolean longForm) {
        return "TableInfo" ;
    }
    

    // UDR writers can ignore these methods
    void setQueryPartitioning(PartitionInfo partInfo) {
        queryPartitioning_ = partInfo;
    }

    void setQueryOrdering(OrderInfo orderInfo) {
        queryOrdering_ = orderInfo;
    }
    
    static short getCurrentVersion() { return 1; }

    @Override
    int serializedLength() throws UDRException{
      int result = super.serializedLength() +
                   2 * serializedLengthOfLong() +
                   4 * serializedLengthOfInt() +
                   serializedLengthOfBinary((getQueryPartitioning().getNumEntries() +
                   2 * getQueryOrdering().getNumEntries()) * 4);

      for (int c=0; c<constraints_.size(); c++)
        result += constraints_.get(c).serializedLength();

      return result;
    }

    @Override
    int serialize(ByteBuffer outputBuffer) throws UDRException {

      int origPos = outputBuffer.position();

      super.serialize(outputBuffer);
      int numPartCols = queryPartitioning_.getNumEntries();
      int numOrderCols = queryOrdering_.getNumEntries();
      int numConstraints = constraints_.size();
      int c;

      serializeLong(estimatedNumRows_,
                    outputBuffer);

      serializeLong(estimatedNumPartitions_,
                    outputBuffer);

      serializeInt(numPartCols,
                   outputBuffer);

      serializeInt(numOrderCols,
                   outputBuffer);

      serializeInt(queryPartitioning_.getType().ordinal(),
                   outputBuffer);

      // the length, in bytes, of the integer array that follows
      serializeInt((numPartCols + 2*numOrderCols) * 4,
                   outputBuffer);

      for (c=0; c<numPartCols; c++)
        outputBuffer.putInt(queryPartitioning_.getColumnNum(c));

      for (c=0; c<numOrderCols; c++)
      {
        outputBuffer.putInt(queryOrdering_.getColumnNum(c));
        outputBuffer.putInt(queryOrdering_.getOrderType(c).ordinal());
      }

      serializeInt(numConstraints,
                             outputBuffer);
      for (c=0; c<numConstraints; c++)
        constraints_.get(c).serialize(outputBuffer);

      int bytesSerialized = outputBuffer.position() - origPos;

      validateSerializedLength(bytesSerialized);

      return bytesSerialized;
    }

    @Override
    int deserialize(ByteBuffer inputBuffer) throws UDRException{

      int origPos = inputBuffer.position();

      super.deserialize(inputBuffer);

      validateObjectType(TMUDRObjectType.TABLE_INFO_OBJ);
      int numPartCols = 0;
      int numOrderCols = 0;
      int numConstraints = 0;
      int partType = 0;
      int c;
      int tempInt;

      estimatedNumRows_ = deserializeLong(inputBuffer);

      estimatedNumPartitions_ = deserializeLong(inputBuffer);

      numPartCols = deserializeInt(inputBuffer);

      numOrderCols = deserializeInt(inputBuffer);

      partType = deserializeInt(inputBuffer);

      tempInt = deserializeInt(inputBuffer);

      if (tempInt != (numPartCols + 2*numOrderCols) * 4 ||
          numPartCols < 0 || numOrderCols < 0)
          throw new UDRException(38900,
                                 "Invalid int array size in TableInfo, got %d, expected %d",
                                 tempInt,
                                 (numPartCols + 2*numOrderCols) * 4);

      queryPartitioning_.clear();
      queryPartitioning_.setType(PartitionInfo.PartitionTypeCode.fromOrdinal(partType));
      for (c=0; c<numPartCols; c++)
        queryPartitioning_.addEntry(deserializeInt(inputBuffer));

      queryOrdering_.clear();
      for (c=0; c<numOrderCols; c++)
        queryOrdering_.addEntry(deserializeInt(inputBuffer),
                                OrderInfo.OrderTypeCode.fromOrdinal(deserializeInt(inputBuffer)));

      numConstraints = deserializeInt(inputBuffer);
      constraints_.clear();
      for (c=0; c<numConstraints; c++)
      {
        ConstraintInfo constr = null;
        switch (getNextObjectType(inputBuffer))
        {
        case CARDINALITY_CONSTRAINT_INFO_OBJ:
          constr = new CardinalityConstraintInfo();
          break;
        case UNIQUE_CONSTRAINT_INFO_OBJ:
          constr = new UniqueConstraintInfo();
          break;
        default:
          throw new UDRException(
               38900,
               "Invalid object type during constraint deserialization: %d",
               getNextObjectType(inputBuffer).getValue());
        }
        constr.deserialize(inputBuffer);
        constraints_.add(constr);
      }

      int bytesDeserialized = inputBuffer.position() - origPos;
      validateDeserializedLength(bytesDeserialized);
      return bytesDeserialized;
    }

    private long                          estimatedNumRows_;
    private long                          estimatedNumPartitions_;
    private PartitionInfo                 queryPartitioning_;
    private OrderInfo                     queryOrdering_;
    private Vector<ConstraintInfo>        constraints_;
}
