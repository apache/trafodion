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
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//  Define a symbol to prevent multiple inclusions of this header file.
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
#ifndef GenTable_H
#define GenTable_H

#include "GenColumn.h"

class TableScriptGenerator {

  private:

    const char*    tableName_;

    ofstream*      stream_;

    const long     insertCount_;

    const long     rowsPerInsert_;

    const long     columnCount_;

    AbstColumnPtr* columns_;

    long*          columnOrder_;

    const long     keyColumnCount_;

    const long*    keyColumns_;

    const long*    keyColumnNumValues_;

    long*          keyValuesGenerated_;

    const long     nonKeyColumnCount_;

    long*          nonKeyColumns_;

  //---------------------------
  //  Private member functions.
  //---------------------------
  void generateKeyColumnValues();
  void generateNonKeyColumnValues();
  void generateColumnValues();
  void generateInsertPreamble();
  void generateComma();
  void generateRow();
  void generateInsertEpilogue();

  public:

  //--------------
  //  Constructor.
  //--------------
  TableScriptGenerator(char*          pTableName,
                       long           pInsertCount,
                       long           pRowsPerInsert,
                       long           pColumnCount,
                       AbstColumnPtr* pColumns,
                       long*          pColumnOrder,
                       long           pKeyColumnCount,
                       long*          pKeyColumns,
                       long*          pKeyColumnNumValues,
                       long           pNonKeyColumnCount,
                       long*          pNonKeyColumns);

  virtual ~TableScriptGenerator();

  virtual void generateTableScript();

};

#endif   // ifndef GenTable_H
