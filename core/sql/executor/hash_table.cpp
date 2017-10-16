// -*-C++-*-
// *********************************************************************
//
// File:         hash_table.cpp
// Description:  
//               
//               
// Created:      7/10/95
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
// *********************************************************************

#include "hash_table.h"

void HashRow::print(ULng32 rowlength) {
  printf("\tHashValue: %6d\n\tData: ", hashValue_);
  unsigned char * data = (unsigned char *)this;
  ULng32 i = 0;
  for (; i < rowlength; i++)
    printf("%02x", data[i]);
  printf("\n\t    : ");
  for (i = 0; i < rowlength; i++)
    printf("%2c", ((data[i] >= ' ' && data[i] < 127) ? data[i] : '?'));
  printf("\n\t      ");
  for (i = 0; i < rowlength; i++)
    if (i % 10) 
      printf ("  ");
    else
      printf ("%1d0", i/10);
  printf ("\n");
}

HashTableCursor::HashTableCursor() {
  init();
}
HashTableCursor::~HashTableCursor() {
}

HashTableHeader::HashTableHeader() {
  init();
}

void HashTableHeader::init() {
  // rowCount_ = 0;
  row_ = NULL;
}


HashTable::HashTable(ULng32 headerCount,
		     NABoolean evenFactor,
		     ULng32 primeFactor,
		     NABoolean noHVDups,
		     NABoolean doResize
		     )
  : headerCount_(headerCount > MAX_HEADER_COUNT ?
                   MAX_HEADER_COUNT : headerCount < MIN_HEADER_COUNT ?
                     MIN_HEADER_COUNT : headerCount),
    rowCount_(0),
    header_(NULL),
    noHashValueDups_(noHVDups), 
    resizeThreshold_(UINT_MAX), // default - don't resize
    originalSize_(TRUE), // this HT was not resized yet
    hashTableOverflow_(FALSE),
    singleChainLastRow_(NULL) {

#ifdef _DEBUG
  if ( getenv("ALLOW_HV_DUPS" ) ) noHashValueDups_ = FALSE; // for testing
#endif
  
  headerCount_ *= 2; // the first iteration below would divide it back

  // allocate the hash table headers. If we can't allocate it,
  // try a smaller table. If we can't even allocate a table with
  // MIN_HEADER_COUNT or somewhat more, we die!
  while (!header_) {

    headerCount_ /= 2;
    if ( headerCount_ < MIN_HEADER_COUNT ) return; // allocation failed

    // Ensure that headerCount_ has no common prime factors with #buckets
    if ( evenFactor && 0 == headerCount_ % 2 ) 
      headerCount_++ ; // the count is even no more
    if ( 0 == headerCount_ % primeFactor )
      headerCount_ += 2 ; // break the common factor, since primeFactor > 2 

    header_ = (HashTableHeader *)collHeap()->allocateMemory((UInt32)
             (headerCount_ * sizeof(HashTableHeader)), FALSE);
  }

  if ( doResize ) 
    resizeThreshold_ = (headerCount_ / 100) * HT_LOAD_FACTOR_PERCENT ;

  init(); // set all entries in the hash table to NULL
}

HashTable::~HashTable() {
  if (header_) {
    collHeap()->deallocateMemory((void*)header_);
    header_ = NULL;
  }
}

void HashTable::init() {
  // memset is about six times faster than this loop
  //     for (unsigned long i = 0; i < headerCount_; i++)
  //        header_[i].init();
  memset( header_ , 0, headerCount_ * sizeof(HashTableHeader) );
}

//////////////////////////////////////////////////////////////////////
// Chains a row into the appropriate hash table chain. It is
// inserted after the first matching row or at the beginning
// of the hash chain, if a matching row is not found.
//////////////////////////////////////////////////////////////////////
void HashTable::insert(atp_struct * workAtp,
		       HashRow * newRow,
                       tupp &workAtpTupp1,
                       tupp &workAtpTupp2,
		       ex_expr * searchExpr) {

  ex_assert( searchExpr, 
	     "This insert() should only be used for searchable entries. ");

  // Calculate the simple hash value for the new row
  SimpleHashValue newRowHV = newRow->hashValue();

  // Determine the hash chain and get the first entry in the chain.
  ULng32 chainIdx = newRowHV % headerCount_;
  ULng32 origChainIdx = chainIdx;
  HashRow *row = header_[chainIdx].row_;

  // Boolean used to control when the data pointer for tupp1 should be set
  NABoolean isTupp1Set = FALSE;


  while (row != NULL) {
    if (newRowHV == row->hashValue()) {

      // The expression does not expect the HashRow to be part of the row.
      // Adjust the datapointer in the work atp to point beyond the HashRow.
      workAtpTupp2.setDataPointer(row->getData());

      // Set the data in the workAtp for the first index if not set already
      if (!isTupp1Set) {

        // The expression does not expect the HashRow to be part of the row.
        // Adjust the datapointer in the work atp to point beyond the HashRow.
        workAtpTupp1.setDataPointer(newRow->getData());
        isTupp1Set = TRUE;
      }

      if ( searchExpr->eval(workAtp, workAtp) == ex_expr::EXPR_TRUE )
         break; // Break from while loop.
      else if ( noHashValueDups_ ) {
	// Different key-value, but with the same HV, try the next HT entry
	chainIdx = ++chainIdx % headerCount_; // next HT entry

	if ( (chainIdx + 1) % headerCount_ == origChainIdx ) {
	  // Oops - an overflow -- more diff key-value sub-chains (with same HV)
	  // than the size of the hash table
	  // (extremely rare - practically impossible -- requires a very
	  // small HT (i.e. memory pressure) and a very bad hash function.)
	  //   After an overflow - revert to the other method -- allow dups
	  // (The "+ 1" above ensures at least one HT entry without this HV
	  //  to prevent an infinite loop in position() )
	  noHashValueDups_ = FALSE; // stop disallowing dups
	  chainIdx = origChainIdx; // return to the original chain
	  hashTableOverflow_ = TRUE; // remember that the HT still contains
	        // "re-placed" sub-chains from before the overflow; they should
	        // be searched for in the position() method
	}

	row = header_[chainIdx].row_; // first row in next chain
	continue; // loop again
      }
    }
    row = row->next();
  }

  // If the row was not found, insert at the beginning of the chain.
  // Otherwise, insert after the first match.
  if (row == NULL) {
    newRow->setNext(header_[chainIdx].row_);
    header_[chainIdx].row_ = newRow;
  }
  else {
    newRow->setNext(row->next());
    row->setNext(newRow);
  }

  // one more row in the chain
  // header_[chainIdx].rowCount_++;
  // one more row in the hash table
  rowCount_++;
}

///////////////////////////////////////////////////////////////////
// chain a row unconditionally into the hash table. Don't look 
// for matches. 
///////////////////////////////////////////////////////////////////
NABoolean HashTable::insert(HashRow * newRow) {
  // determine the hash chain
  ULng32 chainIdx = newRow->hashValue() % headerCount_;

  // Insert at the beginning of the chain
  newRow->setNext(header_[chainIdx].row_);
  header_[chainIdx].row_ = newRow;

  // one more row in the hash table
  return ++rowCount_ > resizeThreshold_ ;
}

////////////////////////////////////////////////////////////////////////
// Resize the hash-table (HGB only; no duplicate entries; don't care order)
// (if we can't allocate a new hash-table then we stop resizing; reason is 
//  that we'd have to overflow anyway soon, and likely flush this cluster.)
///////////////////////////////////////////////////////////////////////
ULng32 HashTable::resize(NABoolean enoughMemory) {
  if ( ! enoughMemory ) {
    resizeThreshold_ = UINT_MAX ; // stop resizing
    return 0;
  }

  ULng32 newSize = resizeTo() ; // the size for the new HT

  HashTableHeader * newHeader = 
    (HashTableHeader *)
    collHeap()->allocateMemory((UInt32)
			       (newSize * sizeof(HashTableHeader)), FALSE);
  
  if ( ! newHeader ) {
    resizeThreshold_ = UINT_MAX ; // stop resizing
    return 0;
  }

  // initialize new HT
  memset( newHeader , 0, newSize * sizeof(HashTableHeader) );

  // insert entries from the old HT into the new HT
  for ( ULng32 oldind = 0; oldind < headerCount_; oldind++ ) {
    HashRow *thisRow = header_[oldind].row_ ;
    // traverse the old chain; reassign each entry
    while ( thisRow ) {
      HashRow * nextRow = thisRow->next_ ;
      
      // determine the new hash chain
      ULng32 chainIdx = thisRow->hashValue() % newSize;
      
      // Insert at the beginning of the chain in the new HT
      thisRow->setNext(newHeader[chainIdx].row_);
      newHeader[chainIdx].row_ = thisRow;

      thisRow = nextRow ;
    }
  }

  ULng32 memAdded = 
    ( newSize - headerCount_ ) * sizeof(HashTableHeader) ;

  // remove old one and replace old fields
  collHeap()->deallocateMemory((void*)header_);
  header_ = newHeader ;
  headerCount_ = newSize ;
  resizeThreshold_ = HT_LOAD_FACTOR_PERCENT * (headerCount_ / 100) ;
  originalSize_ = FALSE; // this HT was resized

  return memAdded ;
}

// HashTable::insertUniq()
// An insert method used by UniqueHashJoin.
// Assumes input is unique and so does not need to check for
// duplicates.
// Entries are inserted in HashValue order.
//
void HashTable::insertUniq(HashRow *newRow)
{
  SimpleHashValue newRowHashValue = newRow->hashValueRaw();

  // Get the first entry for this chain
  //
  ULng32 chainIdx = newRowHashValue % headerCount_;
  HashRow *row = header_[chainIdx].row_;

  // Entries are inserted in HashValue order
  // If this entry belongs at the beginning of the chain
  // insert it there.
  //
  if(!row || newRowHashValue <= row->hashValueRaw()) {
    newRow->setNext(row);
    header_[chainIdx].row_ = newRow;
    return;
  }

  // Cursor so we can insert the new row in the proper position.
  //
  HashRow *prevRow = row;
  row = row->next();

  // Find the position in the chain where this hash value belongs
  //
  while (row && newRowHashValue > row->hashValueRaw()) {
    prevRow = row;
    row = row->next();
  }

  // Insert this entry between this row and the previous row
  //
  newRow->setNext(row);
  prevRow->setNext(newRow);

  return;
  
}

///////////////////////////////////////////////////////////////////
// Add a row to the end of the first hash header.  The purpose
// of this function is to speed up cross product joins that don't
// have a search predicate and to keep the rows in order by
// placing new rows at the tail. This function is only used by
// cross-product.
///////////////////////////////////////////////////////////////////
void HashTable::insertSingleChain(HashRow * newRow) {

  if (header_[0].row_ == NULL)
    header_[0].row_ = newRow;
  else
    singleChainLastRow_->next_ = newRow;

  singleChainLastRow_ = newRow;
  newRow->setNext(NULL);

  // one more row in the hash table
  rowCount_++;

}

///////////////////////////////////////////////////////////////
// convert the rowCounts in the HashTableHeader into offsets
// to allow addressing of the bitmap for right outer joins.
// This feature is disabled for now
///////////////////////////////////////////////////////////////
void HashTable::convertToOffsets() {
  /*
  unsigned long offset = 0;
  unsigned long rowCount = 0;
  for (unsigned long i = 0; i < headerCount_; i++) {
    rowCount += header_[headerCount_].rowCount_;
    header_[headerCount_].rowCount_ = offset;
    offset = rowCount;
  }
  */
}

///////////////////////////////////////////////////////////////
// RETURNS: the next row for the cursor, if it exists.
//          NULL(0), if all rows have been returned.
///////////////////////////////////////////////////////////////
HashRow * HashTable::getNext(HashTableCursor * cursor) {
  
  HashRow * row = cursor->currRow_;

  if (!row)
    // we reached the end of this cursor
    return (HashRow *) NULL;

  if (cursor->currRow_ != cursor->endRow_) {
    cursor->currRow_ = row->next();
    if (cursor->currRow_ == NULL) {
      // end of chain, if there is another chain, switch to it
      cursor->currHeader_++;
      while ((cursor->currHeader_ < headerCount_) &&
	     //skip all empty chains
	     (!(header_[cursor->currHeader_].row_)))
	cursor->currHeader_++;
      if (cursor->currHeader_ < headerCount_)
	cursor->currRow_ = header_[cursor->currHeader_].row_;
    }
  }
  else
    // end of cursor, set currRow_ NULL, such that with the next call
    // we return NULL
    cursor->currRow_ = NULL;

  return row;
}

/////////////////////////////////////////////////////////////////////
// finds the first matching entry and remembers it.
// finds the last matching entry and remembers it.
// They will be used later to return them to the caller by
// calling getNext().
/////////////////////////////////////////////////////////////////////
void HashTable::position(HashTableCursor * cursor,
			 atp_struct * rowAtp,
			 atp_struct * workAtp,
			 short hashTableRowAtpIndex,
			 ex_expr * searchExpr,
			 SimpleHashValue hashValue,
                         NABoolean noDupChaining,
			 NABoolean returnOrdered) {

  ex_assert( searchExpr, 
	     "This position() should only be used for searchable entries. ");

  // Get the tuple used for the search expression and get the chain index.
  tupp &workAtpTupp = workAtp->getTupp(hashTableRowAtpIndex);
  cursor->currHeader_ = hashValue % headerCount_;

  // When no HV dups, and when we need to skip to the next Hash Table entry,
  // the dummyRow is logically positioned one behind the first row in that
  HashRow dummyRow; // next HT entry to fix the for-loop's "row = row->next_" 

  // This loop searches for a first matching row.  If one is found,
  // the last matching row is found within the "if" condition in
  // this loop.
  for (HashRow *row = header_[cursor->currHeader_].row_;
       row != NULL;
       row = row->next()) {

    // If the simple hash doesn't match, then skip to the next row.
    if (hashValue != row->hashValue())
      continue;

    // Set the data pointer in the tuple to point to this row.

    // The expression does not expect the HashRow to be part of the row.
    // Adjust the datapointer in the work atp to point beyond the HashRow.
    workAtpTupp.setDataPointer(row->getData());

    // If this row matches, then set the cursor to the current row,
    // search for the last matching row and return.
    if ( searchExpr->eval(rowAtp, workAtp) == ex_expr::EXPR_TRUE ) {
      ULng32 matchCount = 1; // num matched so far

      // Set the row pointers in the cursor to the first matching row
      cursor->beginRow_ = cursor->endRow_ = cursor->currRow_ = row;

      // Sometimes (e.g. for some (anti)semi-joins) we don't need more than
      // one match and if there are many dups, finding the end of the
      // sub-chain has a severe performance penalty -- see solution
      // 10-080320-1596. 
      if (noDupChaining)
        return;

      // Start searching on the next row for the last matching row
      for (row = row->next(); row != NULL; row = row->next()) {

        if (row->hashValue() != hashValue)
          break; // different HV, search no more (key-value must be different)
       
	if ( ! noHashValueDups_ ) {
	  // the next HV is the same - check if the key-value is different

          // The expression does not expect the HashRow to be part of the row.
          // Adjust the datapointer in the work atp to point beyond the HashRow.
	  workAtpTupp.setDataPointer(row->getData());
	  if ( searchExpr->eval(rowAtp, workAtp) != ex_expr::EXPR_TRUE )
	    break;  // different key-value - end of this sub-chain
	}

	// Found one more row for this sub-chain
        // Set the end row to this matching row.
        cursor->endRow_ = row;
	matchCount++;
      }
      
      // At this point the cursor is pointing to the sub-chain of matched rows

      // Ordering request would be made at most once per sub-chain (since this is
      // only for Ordered HJ with unique left rows); the subchain is ordered now
      //     (first) --> (last) --> (before-last) --> ..... (second)
      // so we reorder the subchain before returning, in a way similar to insert(),
      // by reinserting all the rows following the first one.
      if ( returnOrdered && matchCount > 2 ) {
	HashRow *first = cursor->beginRow_;
	// HashRow *second = cursor->endRow_ ;
	HashRow *beyondLast = cursor->endRow_->next() ;
	HashRow *last = cursor->beginRow_->next();
	HashRow *tmpPtr = last->next(); // start from before-last
	last->setNext( beyondLast ); 
	cursor->endRow_ = last ; // current 2nd is the last
	while ( tmpPtr != beyondLast ) {
	  HashRow *nextOne = tmpPtr->next();
	  tmpPtr->setNext(first->next());
	  first->setNext(tmpPtr);
	  tmpPtr = nextOne;
	}
      }

      return;
    }
    else if ( noHashValueDups_ || hashTableOverflow_ ) {
      // if HT overflow -- keep trying on this chain
      if ( hashTableOverflow_ ) {
	if ( row->next() && row->next()->hashValue() == hashValue ) 
	  continue; // stay on this chain; maybe it is the next row
      }
      // Same HV in this HT entry, but of different key-value, (or after a HT
      // overflow, but at the end of the chain) then try next HT entry
      cursor->currHeader_ = ++cursor->currHeader_ % headerCount_;
      // set the dummyRow logically behind the first row on the next HT entry
      dummyRow.next_ = header_[cursor->currHeader_].row_ ;
      row = &dummyRow ; // so after "row = row->next_" it'll be the 1-st row
      continue; // back to loop to handle the first row of the next HT entry
    }
  }

  // No matching rows were found. Reset the cursor before returning.
  cursor->init();
}

// HashTable::positionUniq()
// A position method used by UniqueHashJoin.
// Assumes entries are unique and so does not need to check for
// duplicates.
// Returns first match found.
//
ex_expr::exp_return_type HashTable::positionUniq(HashRow **currRow,
                                                 atp_struct * leftRowAtp,
                                                 atp_struct * workAtp,
                                                 short rightRowAtpIndex,
                                                 ex_expr * searchExpr,
                                                 SimpleHashValue hashValue)
{

  // Get the first entry for this chain
  //
  ULng32 chainIdx = hashValue % headerCount_;
  HashRow *row = header_[chainIdx].row_;

  // Skip over entries that have a lower hash value.
  //
  while (row && hashValue > row->hashValueRaw()) {
    row = row->next();
  }
  
  // Look for a match among those entries with the same hash value.
  //
  while (row && hashValue == row->hashValueRaw())  {

    // Set the data pointer in the tuple to point to this row.

    // The expression does not expect the HashRow to be part of the row.
    // Adjust the datapointer in the work atp to point beyond the HashRow.
    workAtp->getTupp(rightRowAtpIndex).setDataPointer(row->getData());

    // If this row matches, then return this row
    // No need to search further since we know the entries are unique.
    //
    ex_expr::exp_return_type retCode = searchExpr->eval(leftRowAtp, workAtp);
    if(retCode != ex_expr::EXPR_FALSE) {

      // Set the row pointer to the matching row.
      *currRow = row;
      
      // Found (or error condition).
      return retCode;
    }
    
    // Try the next row.
    row = row->next();
  }

  // If got to the end of the chain or an entry with a greater hash value
  // then no match was found.
  
  // Not found
  return ex_expr::EXPR_FALSE;
}

/////////////////////////////////////////////////////////
// positions to the first entry in the hash table.
// Remember the last entry in the table, if table has
// entries.
// Used to retrieve all rows from the hash table by
// calling getNext().
/////////////////////////////////////////////////////////
void HashTable::position(HashTableCursor * cursor) {
  // find the first non empty chain
  ULng32  i = 0;
  while ((i < headerCount_) && (!(header_[i].row_)))
    i++;
  
  if (i == headerCount_)
    // empty table
    cursor->init();
  else {
    cursor->beginRow_ = header_[i].row_;
    cursor->currHeader_ = i;
      
    // now find the last entry in the hash table
    i = headerCount_ - 1;
    // can never reach i < 0. Atleast one non-null chain must exist
    while (!(header_[i].row_))
      i--;
      
    cursor->endRow_ = header_[i].row_;
    while (cursor->endRow_->next_)
      cursor->endRow_ = cursor->endRow_->next_;
  }
  cursor->currRow_ = cursor->beginRow_;
}

/////////////////////////////////////////////////////////
// positionSingleChain() is used by cross product to
// return a single list of all rows in the inner table
/////////////////////////////////////////////////////////
void HashTable::positionSingleChain(HashTableCursor * cursor) {
  cursor->currHeader_ = 0;
  cursor->currRow_ = cursor->beginRow_ = header_[0].row_;
  cursor->endRow_ = singleChainLastRow_;
}

/////////////////////////////////////////////////////////
// determine the length of hash chain chainIdx
/////////////////////////////////////////////////////////
ULng32 HashTable::getChainSize(ULng32 chainIdx) {
  // sanity check
  ex_assert(chainIdx < headerCount_, "out of bound hash table access");
  ULng32 count = 0;
  HashRow * row = header_[chainIdx].row_;
  while(row) {
    count++;
    row = row->next();
  }
  return count;
}
