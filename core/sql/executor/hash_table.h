#ifndef HASH_TABLE_H
#define HASH_TABLE_H

// -*-C++-*-
// ***************************************************************************
//
// File:         hash_table.h
// Description:  
//               
//               
// Created:      4/15/95
// Language:     C++
//
//
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
//
//
// ***************************************************************************
//


#include "ex_stdh.h"
#include "ex_expr.h"
#include "BaseTypes.h"
#include "NABasicObject.h"

#include "HashRow.h"

// Reasonable constraints on hash table size
#define ONE_MEG (1024 * 1024)
#define MAX_HEADER_COUNT 4 * ONE_MEG
#define MIN_HEADER_COUNT 512

// how loaded need the HT be before it is resized ( 75 % )
#define HT_LOAD_FACTOR_PERCENT 75
// multiplier for resizing the HT
#define HT_RESIZE_FACTOR  2

///////////////////////////////////////////////////////////
// class HashTableCursor
// used to retireve data from the hash table. Multiple
// cursors can be started on the hash table to retrieve the
// data
///////////////////////////////////////////////////////////
class HashTableCursor : public NABasicObject {
  friend class HashTable;
  
  HashRow * beginRow_;
  HashRow * endRow_;
  HashRow * currRow_; 
  ULng32 currHeader_;

public:
  HashTableCursor();
  ~HashTableCursor();
  void init() {
    beginRow_ =
      endRow_ =
      currRow_ = NULL;
    currHeader_ = 0;
  };
  inline HashRow * getBeginRow() {return beginRow_;};
};

/////////////////////////////////////////////////////////
// the header of a hash table chain
/////////////////////////////////////////////////////////
class HashTableHeader {
  friend class HashTable;
public:
  HashTableHeader();
  inline ~HashTableHeader() {};
  void init();
private:
  // for now we disable the rowCount_. This cuts the size of the hash table
  // header in half. The rowCount_ is only required for 1) statistics and 2)
  // right outer joins. 1) can be done with an extra loop over all the rows
  // (probably acceptable when collecting detailed stats) and 2) is not
  // supported anyway right now.
  // unsigned long rowCount_;
  HashRow * row_;
};

//////////////////////////////////////////////////////////////////////////////
// The actual hash table. Created at runtime.
//
// The implementation uses row chaining to resolve hash conflicts (i.e., when
// two rows with different key-values compute the same hash-value modolu the
// the size of the table -- the two rows are chained at that table entry)
//
// For hash-join, a variant of "in place hashing" is used to prevent sub-chains
// (i.e., several contiguous rows with the same key-value) with the same hash
// values from using the same HT entry (i.e. chain). That is, the next sub-chain
// of the same hash-value would be inserted into the _next_ HT entry chain.
// With this variant, search for rows is more efficient as the search expr.
// needs to be evaluated only once, on the first row in the sub-chain; otherwise
// every row in the sub-chain would need to be searched.
//
// (Extremely unlikely) The above method may overflow (more same HV sub-chains
// than the size of the table). A special flag indicates this condition, after
// which we revert to the original chaining method.  
///////////////////////////////////////////////////////////////////////////////
class HashTable : public NABasicObject {
public:
  // The ctor takes a header count for the number of entries, plus
  // two parameters (to avoid the count having common prime factors with the
  // number of clusters, in which case part of the hash table would be unused):
  //   evenFactor: can the count be even ?
  //   primeFactor: the count should not be divisible by this factor !
  // plus
  //   noHVDups: Disallow duplicate hash-values in the same chain (TRUE only 
  //             for HJ, sans cross-product; may become FALSE later)
  //   doResize: If TRUE (HGB) then this table is resizable - when the table
  //             becomes %75 full, then allocate a new one of twice the
  //             previous size, move the entries and deallocate the old one.
  HashTable(ULng32 headerCount,
	    NABoolean evenFactor,
	    ULng32 primeFactor,
	    NABoolean noHVDups = TRUE,
	    NABoolean doResize = FALSE
	    ); 
  ~HashTable();

  void init();

  void insert(atp_struct * workAtp,
	      HashRow * newRow,
              tupp& workAtpTupp1,
              tupp& workAtpTupp2,
	      ex_expr * searchExpr);

  // Used only by Hash-Groupby; return TRUE is HT need be resized
  NABoolean insert(HashRow * newRow);

  // Used only by Hash-Groupby; resize the HT (only if enough memory)
  ULng32 resize(NABoolean enoughMemory);

  // An insert method used by UniqueHashJoin.
  // Assumes input is unique and so does not need to check for
  // duplicates.
  // Entries are inserted in HashValue order.
  //
  void insertUniq(HashRow *newRow);

  void insertSingleChain(HashRow * newRow);

  void convertToOffsets();

  HashRow * getNext(HashTableCursor * cursor);

  void position(HashTableCursor * cursor);

  void position(HashTableCursor * cursor,
		atp_struct * rowAtp,
		atp_struct * workAtp,
		short hashTableRowAtpIndex,
		ex_expr * searchExpr,
		SimpleHashValue hashValue,
                NABoolean noDupChaining = FALSE,  // True for some 
                                                  // (anti)semi-joins.
		NABoolean returnOrdered = FALSE );

  // A position method used by UniqueHashJoin.
  // Assumes entries are unique and so does not need to check for
  // duplicates.
  // Returns first match found.
  //
  ex_expr::exp_return_type positionUniq(HashRow **currRow,
                                        atp_struct * leftRowAtp,
                                        atp_struct * workAtp,
                                        short rightRowAtpIndex,
                                        ex_expr * searchExpr,
                                        SimpleHashValue hashValue);

  void positionSingleChain(HashTableCursor * cursor);

  inline ULng32 getHeaderCount() const {
  return headerCount_;
};

  ULng32 getChainSize(ULng32 i);

  // In case the object was created, but not enough memory for the actual HT
  inline NABoolean noTableAllocated() { return header_ == NULL ; };

  // Return the size the HT would be resized-to (used to check mem availability)
  ULng32 resizeTo() { return headerCount_ * HT_RESIZE_FACTOR ; };

  inline NABoolean originalSize() { return originalSize_ ; };

private:
  HashTableHeader * header_;
  ULng32 headerCount_;

  ULng32 resizeThreshold_; // resize if rowCount_ exceeds this
  NABoolean originalSize_; // FALSE if this HT was resized

  ULng32 rowCount_;

  HashRow *singleChainLastRow_; // Only used for cross-product

  NABoolean noHashValueDups_;   // If TRUE: Do not allow same hash-values (of 
                  // different key-values) in the same chain in the hash table.

  NABoolean hashTableOverflow_; // If TRUE (extremely rare!): The hash-table
          // overflew - more different key-values with the same hash-value than 
          // the size of the hash table 
};

#endif

