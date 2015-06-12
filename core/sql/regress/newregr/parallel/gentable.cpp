/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1999-2015 Hewlett-Packard Development Company, L.P.
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
**********************************************************************/
#include <iostream>
#include <string.h>
#include "GenTable.h"

#if !defined(__TANDEM) || (__CPLUSPLUS_VERSION >= 3)
using namespace std;
#endif

void
TableScriptGenerator::generateKeyColumnValues()
{

  for (long keyIdx = (keyColumnCount_ - 1); keyIdx >= 0; keyIdx--)
    {
      if (keyValuesGenerated_[keyIdx] < keyColumnNumValues_[keyIdx])
        {
          columns_[keyColumns_[keyIdx]]->getNextColumnValue();
          ++keyValuesGenerated_[keyIdx];
          return;
        }

      columns_[keyColumns_[keyIdx]]->getNextColumnValue();
      keyValuesGenerated_[keyIdx] = 1;
    }
   
} // TableScriptGenerator::generateKeyColumnValues()

void
TableScriptGenerator::generateNonKeyColumnValues()
{

  for (long idx = 0; idx < nonKeyColumnCount_; idx++)
    {
      columns_[nonKeyColumns_[idx]]->getNextColumnValue();
    }

} // TableScriptGenerator::generateNonKeyColumnValues()

void
TableScriptGenerator::generateColumnValues()
{

  generateKeyColumnValues();
  generateNonKeyColumnValues();

} // TableScriptGenerator::generateColumnValues()

void
TableScriptGenerator::generateInsertPreamble()
{

  *stream_ << "insert into " << tableName_ << " values " << endl;

} // TableScriptGenerator::generateInsertPreamble()

void
TableScriptGenerator::generateComma()
{

  *stream_ << ", ";

} // TableScriptGenerator::generateComma()

void
TableScriptGenerator::generateRow()
{

  *stream_ << "( ";
  for (long colIdx = 0; colIdx < (columnCount_ - 1); colIdx++)
    {
      columns_[columnOrder_[colIdx]]->printColumn(*stream_);
      generateComma();
    }
  columns_[columnOrder_[columnCount_ - 1]]->printColumn(*stream_);
  *stream_ << ")";

} // TableScriptGenerator::generateRow()

void
TableScriptGenerator::generateInsertEpilogue()
{

  *stream_ << ";" << endl;

}  // TableScriptGenerator::generateInsertEpilogue()

TableScriptGenerator::TableScriptGenerator(char*          pTableName,
                                           long           pInsertCount,
                                           long           pRowsPerInsert,
                                           long           pColumnCount,
                                           AbstColumnPtr* pColumns,
                                           long*          pColumnOrder,
                                           long           pKeyColumnCount,
                                           long*          pKeyColumns,
                                           long*          pKeyColumnNumValues,
                                           long           pNonKeyColumnCount,
                                           long*          pNonKeyColumns)
: tableName_          (pTableName),
  insertCount_        (pInsertCount),
  rowsPerInsert_      (pRowsPerInsert),
  columnCount_        (pColumnCount),
  columns_            (pColumns),
  columnOrder_        (pColumnOrder),
  keyColumnCount_     (pKeyColumnCount),
  keyColumns_         (pKeyColumns),
  keyColumnNumValues_ (pKeyColumnNumValues),
  keyValuesGenerated_ (new long[pKeyColumnCount]),
  nonKeyColumnCount_  (pNonKeyColumnCount),
  nonKeyColumns_      (pNonKeyColumns)
{

  char outFileName[50];
  strcpy(outFileName,"load");
  strcat(outFileName,pTableName);
  stream_ = new ofstream(outFileName);

  for (long keyIdx = 0; keyIdx < (pKeyColumnCount - 1); keyIdx++)
    {
      columns_[keyColumns_[keyIdx]]->getNextColumnValue();
      keyValuesGenerated_[keyIdx] = 1;
    }

  keyValuesGenerated_[pKeyColumnCount - 1] = 0;

} // TableScriptGenerator constructor

TableScriptGenerator::~TableScriptGenerator()
{

  delete [] keyValuesGenerated_;

} // TableScriptGenerator destructor

void
TableScriptGenerator::generateTableScript()
{

  cout << "Generating table " << tableName_ << " ..." << endl;

  for (long insIdx = 0; insIdx < insertCount_; insIdx++)
    {
      generateInsertPreamble();
      for (long rowsIdx = 0; rowsIdx < (rowsPerInsert_ - 1); rowsIdx++)
        {
          generateColumnValues();
          generateRow();
          generateComma();
          *stream_ << endl;

        }
      generateColumnValues();
      generateRow();
      generateInsertEpilogue();
    }

  cout << "Table " << tableName_ << " generated." << endl;

} // TableScriptGenerator::generateTableScript()
