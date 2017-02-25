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
import java.util.Vector;

/**
   *  Ordering of a table by some ascending or descending columns
   *
   *  <p> A list of columns, represented by column numbers, with an
   *  ascending/descending indicator for each column.
  */
public class OrderInfo
{

    /**
     *  Ascending/descending order of columns
     *
     *  <p> For outputs, the ordering of values from the first row out to the
     *  last. Note that this ordering applies within a parallel instance
     *  of the UDF at runtime, but it does not guarantee a total
     *  order. For example, two parallel instances may get these ordered
     *  values: instance 0 gets 1,3,5,7 instance 1 gets 2,4,6,8
     */
    public enum OrderTypeCode
      {
          /**
           *  Unspecified order
           */
          NO_ORDER,  ///< Unspecified order
          /**
           *  Ascending order
           */
          ASCENDING, ///< Ascending order
          /**
           *  Descending order
           */
          DESCENDING; ///< Descending order

        private static OrderTypeCode[] allValues = values();
        public static OrderTypeCode fromOrdinal(int n) {return allValues[n];}

      };

    // const Functions for use by UDR writer, both at compile and at run time

/**
 *  Default constructor.
 */
    public OrderInfo() {
        columnNumbers_ = new Vector<Integer>();
        orderTypes_ = new Vector<OrderTypeCode>();
    }

/**
 *  Copy constructor.
 */
    public OrderInfo(OrderInfo o) {
        columnNumbers_ = new Vector<Integer>(o.columnNumbers_);
        orderTypes_ = new Vector<OrderTypeCode>(o.orderTypes_);
    }

/**
 *  Get the number of entries (columns) in the ordering.
 *  @return Number of entries/columns that make up the ordering.
 */
    public int getNumEntries() {
        return columnNumbers_.size();
    }

/**
 *  Get the column number of an entry of the ordering.
 *  @param i the position (0-based) of the ordering, 0 meaning the leading position.
 *  @return The column number of the n-th entry of the ordering (both are 0-based).
 *  @throws UDRException
 */
    public int getColumnNum(int i) throws UDRException {
        if (i < 0 || i >= columnNumbers_.size())
            throw new UDRException(
                                   38900,
                                   "Trying to access colnum entry %d of an OrderInfo object with %d entries",
                                   i, columnNumbers_.size());

        return columnNumbers_.get(i).intValue();
    }

/**
 *  Get the order type of an entry of the ordering.
 *  @param i the position (0-based) of the ordering, 0 meaning the leading position.
 *  @return The order type of the n-th entry of the ordering (0-based).
 *  @throws UDRException
 */
    public OrderTypeCode getOrderType(int i) throws UDRException {
        if (i < 0 || i >= orderTypes_.size())
            throw new UDRException(
                                   38900,
                                   "Trying to access order type entry %d of an OrderInfo object with %d entries",
                                   i, orderTypes_.size());

        return orderTypes_.get(i);
    }

    // Functions available at compile time only

/**
 *  Append an entry to the ordering.
 *  @param colNum Column number to append to the ordering.
 *  @param orderType Order type (ascending or descending) to use.
 */
    public void addEntry(int colNum, OrderTypeCode orderType){
        columnNumbers_.add(Integer.valueOf(colNum));
        orderTypes_.add(orderType);
    }

/**
 *  Append an entry to the ordering with ASCENDING orderType.
 *  @param colNum Column number to append to the ordering.
 */
    public void addEntry(int colNum){
        addEntry(colNum, OrderTypeCode.ASCENDING);
    }

/**
 *  Insert an entry at any position of the ordering.
 *
 *  <p> A quick example to illustrate this: Let's say we have a table
 *  with columns (a,b,c). Their column numbers are 0, 1, and 2.
 *  We produce an ordering (C ASCENDING):
 *
 *  @code OrderInfo myorder;
 *  
 *  myorder.addEntryAt(0, 2); @endcode
 *
 *  Next, we want to make this into (B DESCENDING, C ASCENDING):
 *
 *  @code myorder.addEntryAt(0, 1, DESCENDING); @endcode
 *
 *  @param pos Position (0-based) at which we want to insert. The new
 *             entry will be position "pos" after the insertion, any
 *             existing entries will be moved up.
 *  @param colNum Number of the column by which we want to order
 *  @param orderType Order type (ascending or descending) to use
 *  @throws UDRException
 */
    public void addEntryAt(int pos,
                           int colNum,
                           OrderTypeCode orderType) throws UDRException {
        if (pos > columnNumbers_.size())
            throw new UDRException(
                                   38900,
                                   "OrderInfo::addEntryAt at position %d with a list of %d entries",
                                   pos, columnNumbers_.size());
        columnNumbers_.add(pos, Integer.valueOf(colNum));
        orderTypes_.add(pos, orderType);
    }

/**
 *  Insert an entry at any position of the ordering with orderType ASCENDING.
 *
 *  <p> A quick example to illustrate this: Let's say we have a table
 *  with columns (a,b,c). Their column numbers are 0, 1, and 2.
 *  We produce an ordering (C ASCENDING):
 *
 *  @code OrderInfo myorder;
 *  
 *  myorder.addEntryAt(0, 2); @endcode
 *
 *  @param pos Position (0-based) at which we want to insert. The new
 *             entry will be position "pos" after the insertion, any
 *             existing entries will be moved up.
 *  @param colNum Number of the column by which we want to order
 *  @throws UDRException
 */
    public void addEntryAt(int pos,
                           int colNum) throws UDRException {
        addEntryAt(pos, colNum, OrderTypeCode.ASCENDING);
    }

    // UDR writers can ignore these methods

    void clear()
    {
      columnNumbers_.clear();
      orderTypes_.clear();
    }

    private Vector<Integer> columnNumbers_;
    private Vector<OrderTypeCode> orderTypes_;
}
