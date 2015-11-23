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

package org.trafodion.jdbc.t2;

import java.io.Serializable;
import java.util.BitSet;


// ****************************************************************************
//  Class: ObjectArray
//
//  This class describes an array of Objects, a corresponding array of
//  updated objects, and a bitmap containing the updated element positions
//  It is used by TrafT4ResultSets to handle result set rows.
// ****************************************************************************

class ObjectArray implements Serializable, Cloneable {

  // Class members
  private BitSet arrayElements_;
  private int    arraySize_;
  private Object objectArray_[];
  private Object updatedArray_[];

  // Constructors
  ObjectArray(int arraySize) {
    arraySize_    = arraySize;
    objectArray_  = new Object[arraySize];
    updatedArray_ = new Object[arraySize];
    
    // Initializes bit map of elements to false
    arrayElements_ = new BitSet(arraySize_);
  }

  ObjectArray(int arraySize, Object objectArray[]) {
    arraySize_    = arraySize;
    objectArray_  = new Object[arraySize];
    updatedArray_ = new Object[arraySize];

    // Initializes bit map of elements to false
    arrayElements_ = new BitSet(arraySize_);

    // populate object array with passed in object array
    for (int idx = 0; idx < arraySize_; idx++) {
      objectArray_[idx] = objectArray[idx];
    }
  }

  // Helpers
  protected int     getSize() { return arraySize_; }

  protected Object getUpdatedArrayElement(int idx) {
    if (arrayElements_ .get(idx - 1)) {
      return updatedArray_[idx - 1];
    } 
    else {
      return objectArray_[idx - 1];
    }
  }

  protected void updateArrayElement(int idx, Object obj) { 
    updatedArray_[idx - 1] = obj; 
    arrayElements_.set(idx - 1);
  }

}

