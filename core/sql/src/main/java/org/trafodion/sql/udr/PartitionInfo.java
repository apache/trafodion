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
import org.trafodion.sql.udr.UDRException;
import java.util.Vector;

 /**
   *  Partitioning key of an input table or result table
   *
   *  <p> Describes the partitioning key of a table-valued input or
   *  result.  When executing a UDR in parallel, if a table is
   *  partitioned on some columns, e.g. (a,b), this means that a rows
   *  with particular values for (a,b), e.g. (10, 20) will all go to
   *  the same parallel instance and will be seen as a contiguous
   *  group. This is similar to the key of a reducer in MapReduce,
   *  except that in this case we process a group of rows with the
   *  same key, not a single key and a list of values.
  */
public class PartitionInfo
{

    /** 
     * Type of partitioning 
     */
    public enum PartitionTypeCode
    {
        /** 
         * Partitioning type not yet determined 
         */
        UNKNOWN, 
        /** 
         * No limitations on parallel execution, typical for
         * mappers, any row can be evaluated by any parallel
         * instance of the UDF
         */
        ANY,
        /**
         * No partitioning is allowed, execute serially in a
         * single instance
         */
        SERIAL,  
         /**
         * Allow parallelism with partitioning key, if specified,
         * serial execution otherwise
         */
        PARTITION,
        /**
         * Replicate the data to each parallel instance
         */
        REPLICATE;

        private static PartitionTypeCode[] allValues = values();
        public static PartitionTypeCode fromOrdinal(int n) {return allValues[n];}
    };

    // const Functions for use by UDR writer, both at compile and at run time

    /**
     *  Default constructor
     *
     *  <p> Use this constructor to generate an object to be passed
     *  to UDRInvocationInfo::setChildPartitioning()
     */
    public  PartitionInfo() {
        type_ = PartitionTypeCode.UNKNOWN;
    }

    /**
     *  Get the partitioning type
     *  @return Partition type enum
     */
    public PartitionTypeCode getType() {
        return type_ ;
    }

    /**
     *  Get the number of columns that form the partitioning key
     *
     *  <p> Returns the number of columns in the list of partitioning keys
     *  or zero if there are no such columns.
     *
     *  @return Number of partitioning key columns (could be zero)
     */
    public int getNumEntries() {
        return partCols_.size();
    }

    /**
     *  Get the number/ordinal of the ith partitioning column.
     *
     *  @return Number/ordinal (0-based) of the ith partitioning column in
     *          the list of partitioning columns.
     *  @throws UDRException
     */
    public int getColumnNum(int i) throws UDRException {
        if (i < 0 || i >= partCols_.size())
            throw new UDRException(
                                   38900,
                                   "Trying to access colnum entry %d of a PartitionInfo object with %d entries",
                                   i, partCols_.size());

        return partCols_.get(i).intValue();
    }

    /**
     *  Set the partitioning type.
     *  @param type Partition type enum.
     */
    public void setType(PartitionTypeCode type){
        type_ = type;
    }

    /**
     *  Add a new column to the list of partitioning columns
     *
     *  <p> Add a new column to the list of column numbers that form the
     *  partitioning key. Use this only if the type of the partitioning
     *  is set to PARTITION.
     *
     *  @param colNum Number of the column (ordinal, 0-based) of the
     *                associated table.
     *  @throws UDRException
     */
    public void addEntry(int colNum) throws UDRException{
        // don't allow duplicates
        for (int i=0; i<partCols_.size(); i++)
            if (partCols_.get(i).intValue() == colNum)
                throw new UDRException(
                                   38900,
                                   "Trying to add column number %d more than once to a PartitionInfo object",
                                   colNum);
        
        partCols_.add(Integer.valueOf(colNum));
    }

    void clear()
    {
      type_ = PartitionTypeCode.UNKNOWN;
      partCols_.clear();
    }

    public void mapColumnNumbers(Vector<Integer> map) throws UDRException {
        for (int i=0; i<partCols_.size(); i++)
        {
            int colNum = partCols_.get(i).intValue();
            
            if (map.get(colNum).intValue() < 0)
                throw new UDRException(
                                       38900,
                                       "Invalid mapping for PARTITION BY column %d",
                                       colNum);
            partCols_.add(i, map.get(colNum));
        }
    }

    public Vector<Integer> getPartCols() {
        return partCols_;
    }

    private PartitionTypeCode type_;
    private Vector<Integer> partCols_;
}
