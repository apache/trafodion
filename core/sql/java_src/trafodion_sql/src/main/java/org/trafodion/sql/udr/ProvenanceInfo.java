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

/**
   *  Describes where an output column is coming from.
   *
   *  <p> Points to input table and input column number that is the
   *  source of a column. This must only be used if the result column
   *  always has the exact same value as the current value of the
   *  corresponding input column.
   *
   *  @see ColumnInfo#getProvenance()
   *  @see ColumnInfo#setProvenance(ProvenanceInfo)
   *  @see UDRInvocationInfo#addPassThruColumns()
  */

  public class ProvenanceInfo
  {
      /**
       * Default constructor, generates unspecified provenance.
       */
      public ProvenanceInfo(){
          inputTableNum_ = -1;
          inputColNum_ = -1;
      }

      /**
       * Copy constructor.
       */
      public ProvenanceInfo(ProvenanceInfo p){
          inputTableNum_ = p.inputTableNum_;
          inputColNum_ = p.inputColNum_;
      }

      /**
       *  Constructor to link an output column to a specific input column
       *
       *  <p> This constructor can be used to produce a "passthru column". An easier
       *  way to do this is the UDRInvocationInfo::addPassThruColumns() method.
       *
       *  @param inputTableNum Input table number (0 for a TMUDF with a single
       *                       table-valued input, the most common case).
       *  @param inputColNum   Column number in intput table "inputTableNum"
       *                       that is the source of the output column to be
       *                       produced.
       */
      public ProvenanceInfo(int inputTableNum,
                            int inputColNum){
          inputTableNum_ = inputTableNum;
          inputColNum_ = inputColNum;
      }

      /**
       *  Get the input table number.
       *  @return Input table number.
       */
      public int getInputTableNum(){
          return inputTableNum_;
      }

      /**
       *  Get the input column number.
       *  @return Input column number.
       */
      public int getInputColumnNum(){
          return inputColNum_;
      }

      /**
       *  Test whether the column comes from any or from a specific table-valued input.
       *  @param  inputTableNum -1 to test for any table-valued input, or a specific
       *          input table number.
       *  @return true if the provenance indicates a column that comes from the
       *          specified input table(s), false otherwise
       */
      public boolean isFromInputTable(int inputTableNum){
          return (inputTableNum_ >= 0 &&
                  inputColNum_ >= 0 &&
                  (inputTableNum > 0 ? inputTableNum == inputTableNum_ : true));
      }

      /**
       *  Test whether the column comes from any table-valued input.
       *  @return true if the provenance indicates a column that comes from
       *          some input table
       */
      public boolean isFromInputTable(){
          return (inputTableNum_ >= 0 &&
                  inputColNum_ >= 0);
      }

      private int inputTableNum_;
      private int inputColNum_;
  }


