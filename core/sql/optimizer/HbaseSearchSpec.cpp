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

#include "HbaseSearchSpec.h"
#include "ItemColRef.h"

void HbaseSearchSpec::addColumnNames(const ValueIdSet& vs)
{
  // TEMP TEMP. Not all needed column names are being set up.
  // for now, return without populating result.
  // that will cause all columns to be retrieved.
  //return;

   for (ValueId vid = vs.init(); vs.next(vid); vs.advance(vid)) {
      ItemExpr* ie = vid.getItemExpr();

      NAString colName;
      if ( ie->getOperatorType() == ITM_BASECOLUMN ) {
	colName = ((BaseColumn*)ie)->getColName();
      } else
	if ( ie->getOperatorType() == ITM_INDEXCOLUMN ) {
	  colName = ((IndexColumn*)ie)->getNAColumn()->getIndexColName();
	}
      
      if (NOT colNames_.contains(colName))
	colNames_.insert(colName);
   }
}

const NAString HbaseSearchSpec::getText() const
{
   if ( colNames_.entries() == 0 )
     return "columns: all ";

   NAString result("columns: ");
   for ( CollIndex i=0; i<colNames_.entries(); i++ ) {
      result.append(colNames_[i]);

      if (i < colNames_.entries() - 1)
         result.append(",");
   }

   result.append(" ");
   return result;
}

static NABoolean extractKeyValuePairs(const NAString& source, NAString& result);

const NAString HbaseUniqueRows::getText() const
{
   NAString result = HbaseSearchSpec::getText();

   for ( CollIndex i=0; i<rowIds_.entries(); i++ ) {

      result.append("unique_rows: ");
      extractKeyValuePairs(rowIds_[i], result);
      result.append(" ");
   }

   return result;
}

//
// data (begin or end) is in this format: 
//  <length><data> ... <length><data>
//
// a). length is 2-bytes long
// b). data is <length>-bytes long
// c). there are <n> such pairs, where <n> is determined by
//     the total length of the source and the length of each pair.
//
static NABoolean extractKeyValuePairs(const NAString& source, NAString& result) 
  {
    UInt16 typeLen = sizeof(UInt16);
    size_t header = 0;
    size_t tail = source.length();
    NABoolean hasData = ( tail > 0 );
  
    const char* data = source.data();
  
    while ( header < tail ){
      // get the length of the string
      size_t begin = header;
      size_t end = begin + typeLen;
      // error, do not have enough buffer to read
      if ( end > tail )
        break;
      NAString len;
      source.extract(begin, end-1, len);
  
      // get the string
      begin = end;
      end = begin + (*(UInt16 *)(len.data()));
      // if the string lengh is 0, continue
      if ( end == begin) {
        header = end;
        if ( header < tail )
          result.append(",");
        continue;
      } 
      // error, do not have enough buffer to read
      else if ( end > tail )
        break;
      source.extract(begin, end-1, result);
      header = end;
      if ( header < tail )
        result.append(",");
    }
  
    return hasData;
  }

const NAString HbaseRangeRows::getText() const
{
   NAString result = HbaseSearchSpec::getText();

   result.append("begin_keys");
   result.append(beginKeyExclusive_ ? "(excl)" : "(incl)");
   result.append(": ");

   // rowId_ (begin or end) is in this format: 
   //  <length><data> ... <length><data>
   //
   // a). length is 2-bytes long
   // b). data is <length> bytes long
   // c). there are <n> such pairs, where <n> is determined by
   //     the total length of the data and the length of each pair.

   extractKeyValuePairs(beginRowId_, result); 
         
   result.append(" ");

   result.append("end_keys");
   result.append(endKeyExclusive_ ? "(excl)" : "(incl)");
   result.append(": ");

   extractKeyValuePairs(endRowId_, result);

   result.append(" ");

   return result;
}

const NAString ListOfUniqueRows::getText() const
{
  NAString result;
  for (CollIndex i=0; i<entries(); i++) {
     HbaseUniqueRows getRow = (*this)[i];
     result.append(getRow.getText());
  }
  return result;
}

const NAString ListOfRangeRows::getText() const
{
  NAString result;
  for (CollIndex i=0; i<entries(); i++) {
     HbaseRangeRows rangeRow = (*this)[i];
     result.append(rangeRow.getText());
  }
  return result;
}

