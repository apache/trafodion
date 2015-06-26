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
#include <stdlib.h>
#include <string.h>
#include "Pseudo.h"
#include "GenColumn.h"
#include "GenTable.h"

#if !defined(__TANDEM) || (__CPLUSPLUS_VERSION >= 3)
using namespace std;
#endif

//--------------------------------------------------------
//  Column values interpreted as null for certain columns.
//--------------------------------------------------------
long intNullVal_50p       = 299;
char charNullVal_100[]    = "DY";

//------------------------------------------------------------
//  Seed values for priming random generators for each column.
//------------------------------------------------------------
const long seedSInt16_10    =   373;
const long seedSInt32_100   =   769;
const long seedSInt32_50p   =  1531;
const long seedSInt32_uniq  =  2309;
const long seedUInt16_10    =  3167;
const long seedUInt32_100   =  3851;
const long seedUInt32_50p   =  4547;
const long seedUInt32_uniq  =  5351;
const long seedInt64_100    =  6029;
const long seedInt64_uniq   =  6833;
const long seedChar_10      =  7529;
const long seedChar_100     =  8317;
const long seedChar_50p     =  9001;
const long seedChar_uniq    =  9749;
const long seedUNum_10      = 10513;
const long seedSNum_100     = 11003;
const long seedUNum_50p     = 11527;
const long seedSNum_uniq    = 12007;
const long seedDate_12      = 12413;
const long seedDate_200     = 13291;
const long seedDate_uniq    = 13597;
const long seedVarchar_100  = 14303;
const long seedVarchar_uniq = 14931;

//--------------------------------------------------------
//  Columns available for use in generating table scripts.
//--------------------------------------------------------
const long    kColumnCount = 23;
AbstColumnPtr columns[kColumnCount];
const long colSInt16_10    =  0;
const long colSInt32_100   =  1;
const long colSInt32_50p   =  2;
const long colSInt32_uniq  =  3;
const long colUInt16_10    =  4;
const long colUInt32_100   =  5;
const long colUInt32_50p   =  6;
const long colUInt32_uniq  =  7;
const long colInt64_100    =  8;
const long colInt64_uniq   =  9;
const long colChar_10      = 10;
const long colChar_100     = 11;
const long colChar_50p     = 12;
const long colChar_uniq    = 13;
const long colUNum_10      = 14;
const long colSNum_100     = 15;
const long colUNum_50p     = 16;
const long colSNum_uniq    = 17;
const long colDate_12      = 18;
const long colDate_200     = 19;
const long colDate_uniq    = 20;
const long colVarchar_100  = 21;
const long colVarchar_uniq = 22;

void
initKeyColumns(long* pKeyColumns, long pNumColumns, long pTotalRowCount)
{

  for (long colIdx = 0; colIdx < pNumColumns; colIdx++)
    {
      switch (pKeyColumns[colIdx])
        {
        case colSInt16_10:
          columns[colSInt16_10]    = new IntColumn(10);
          break;

        case colSInt32_100:
          columns[colSInt32_100]   = new IntColumn(100);
          break;

        case colSInt32_50p:
          columns[colSInt32_50p]   = new IntColumn(pTotalRowCount/2);
          break;

        case colSInt32_uniq:
          columns[colSInt32_uniq]  = new IntColumn(pTotalRowCount);
          break;

        case colUInt16_10:
          columns[colUInt16_10]    = new IntColumn(10);
          break;

        case colUInt32_100:
          columns[colUInt32_100]   = new IntColumn(100);
          break;

        case colUInt32_50p:
          columns[colUInt32_50p]   = new IntColumn(pTotalRowCount/2);
          break;

        case colUInt32_uniq:
          columns[colUInt32_uniq]  = new IntColumn(pTotalRowCount);
          break;

        case colInt64_100:
          columns[colInt64_100]    = new IntColumn(100);
          break;

        case colInt64_uniq:
          columns[colInt64_uniq]   = new IntColumn(pTotalRowCount);
          break;

        case colChar_10:
          columns[colChar_10]      = new CharColumn(2,5,8);
          break;

        case colChar_100:
          columns[colChar_100]     = new CharColumn(4,25,8);
          break;

        case colChar_50p:
          columns[colChar_50p]     = new CharColumn(12,25,8);
          break;

        case colChar_uniq:
          columns[colChar_uniq]    = new CharColumn(24,25,8);
          break;

        case colUNum_10:
        case colSNum_100:
        case colUNum_50p:
        case colSNum_uniq:
          cout << "Fixed Point Numerics not allowed as keys!!!" 
               << " initKeyColumns()"
               << endl;
          exit(0);
          break;

        case colDate_12:
          columns[colDate_12]      = new DateColumn(12);
          break;

        case colDate_200:
          columns[colDate_200]     = new DateColumn(200);
          break;

        case colDate_uniq:
          columns[colDate_uniq]    = new DateColumn(pTotalRowCount);

          break;

        case colVarchar_100:
          columns[colVarchar_100]  = new VarcharColumn(4,25,16,8);
          break;

        case colVarchar_uniq:
          columns[colVarchar_uniq] = new VarcharColumn(24,25,50,8);
          break;

        default:
          cout << "Invalid character index in initKeyColumns()!!!" << endl;
          exit(0);
        }
    }
       
} // initKeyColumns()

void
initNonKeyColumns(long pTotalRowCount)
{

  //------------------------
  //  Loop over all columns.
  //------------------------
  for (long colIdx = 0; colIdx < kColumnCount; colIdx++)
    {

      //----------------------------------------
      //  Not already allocated as a key column.
      //----------------------------------------
      if (columns[colIdx] == 0)
        {
          switch (colIdx)
            {
            case colSInt16_10:
              columns[colSInt16_10]    = new IntColumn(seedSInt16_10,
                                                       pTotalRowCount,
                                                       10);
              break;

            case colSInt32_100:
              columns[colSInt32_100]   = new IntColumn(seedSInt32_100,
                                                       pTotalRowCount,
                                                       100);

              break;

            case colSInt32_50p:
              columns[colSInt32_50p]   = new IntColumn(seedSInt32_50p,
                                                       pTotalRowCount,
                                                       pTotalRowCount/2,
                                                       &intNullVal_50p);

              break;

            case colSInt32_uniq:
              columns[colSInt32_uniq] = new IntColumn(seedSInt32_uniq,
                                                       pTotalRowCount,
                                                       pTotalRowCount);
              break;

            case colUInt16_10:
              columns[colUInt16_10]    = new IntColumn(seedUInt16_10,
                                                       pTotalRowCount,
                                                       10);
              break;

            case colUInt32_100:
              columns[colUInt32_100]   = new IntColumn(seedUInt32_100,
                                                       pTotalRowCount,
                                                       100);

              break;

            case colUInt32_50p:
              columns[colUInt32_50p]   = new IntColumn(seedUInt32_50p,
                                                       pTotalRowCount,
                                                       pTotalRowCount/2,
                                                       &intNullVal_50p);
              break;

            case colUInt32_uniq:
              columns[colUInt32_uniq]  = new IntColumn(seedUInt32_uniq,
                                                       pTotalRowCount,
                                                       pTotalRowCount);
              break;

            case colInt64_100:
              columns[colInt64_100]    = new IntColumn(seedInt64_100,
                                                       pTotalRowCount,
                                                       100);
              break;

            case colInt64_uniq:
              columns[colInt64_uniq]   = new IntColumn(seedInt64_uniq,
                                                       pTotalRowCount,
                                                       pTotalRowCount);
              break;

            case colChar_10:
              columns[colChar_10]      = new CharColumn(seedChar_10,
                                                        pTotalRowCount,
                                                        2,
                                                        5,
                                                        8);
              break;

            case colChar_100:
              columns[colChar_100]     = new CharColumn(seedChar_100,
                                                        pTotalRowCount,
                                                        4,
                                                        25,
                                                        8,
                                                        charNullVal_100);
              break;

            case colChar_50p:
              columns[colChar_50p]     = new CharColumn(seedUNum_50p,
                                                        pTotalRowCount,
                                                        12,
                                                        25,
                                                        8);
              break;

            case colChar_uniq:
              columns[colChar_uniq]    = new CharColumn(seedChar_uniq,
                                                        pTotalRowCount,
                                                        24,
                                                        25,
                                                        8);
              break;

            case colUNum_10:
              columns[colUNum_10]      = new FixedPointColumn(seedUNum_10,
                                                              pTotalRowCount,
                                                              10,
                                                              6);
              break;

            case colSNum_100:
              columns[colSNum_100]     = new FixedPointColumn(seedSNum_100,
                                                              pTotalRowCount,
                                                              100,
                                                              6);
              break;

            case colUNum_50p:
              columns[colUNum_50p]     = new FixedPointColumn(seedUNum_50p,
                                                              pTotalRowCount,
                                                              pTotalRowCount/2,
                                                              6);
              break;

            case colSNum_uniq:
              columns[colSNum_uniq]    = new FixedPointColumn(seedSNum_uniq,
                                                              pTotalRowCount,
                                                              pTotalRowCount,
                                                              6);
              break;

            case colDate_12:
              columns[colDate_12]      = new DateColumn(seedDate_12,
                                                        pTotalRowCount,
                                                        12);
              break;

            case colDate_200:
              columns[colDate_200]     = new DateColumn(seedDate_200,
                                                        pTotalRowCount,
                                                        200);
              break;

            case colDate_uniq:
              columns[colDate_uniq]    = new DateColumn(seedDate_uniq,
                                                        pTotalRowCount,
                                                        pTotalRowCount);
              break;

            case colVarchar_100:
              columns[colVarchar_100]  = new VarcharColumn(seedVarchar_100,
                                                           pTotalRowCount,
                                                           4,
                                                           25,
                                                           16,
                                                           8);
              break;

            case colVarchar_uniq:
              columns[colVarchar_uniq] = new VarcharColumn(seedVarchar_uniq,
                                                           pTotalRowCount,
                                                           24,
                                                           25,
                                                           50,
                                                           8);
              break;

            default:
              cout << "Invalid character index in initNonKeyColumns()!!!" 
                   << endl;
              exit(0);
            }
        }

    }
       
} // initNonKeyColumns()
void
resetColumns()
{
  for (long idx = 0; idx < kColumnCount; idx++)
    {
      delete columns[idx];
      columns[idx] = 0;
    }

} // resetColumns()

void
genUTAB00(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "UTAB00";
  
  long       keyColumns[]         = {colSInt32_uniq};
  long       keyColumnNumValues[] = {totalRowCount};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt16_10,
                                colSInt32_100, 
                                colSInt32_50p,
                                colUInt16_10,
                                colUInt32_100,
                                colUInt32_50p,
                                colUInt32_uniq,
                                colInt64_100,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colDate_uniq,
                                colVarchar_100,
                                colVarchar_uniq};
  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colSInt16_10,
                              colSInt32_100, 
                              colSInt32_50p,
                              colSInt32_uniq,
                              colUInt16_10,
                              colUInt32_100,
                              colUInt32_50p,
                              colUInt32_uniq,
                              colInt64_100,
                              colInt64_uniq,
                              colChar_10,
                              colChar_100,
                              colChar_50p,
                              colChar_uniq,
                              colUNum_10,
                              colSNum_100,
                              colUNum_50p,
                              colSNum_uniq,
                              colDate_12,
                              colDate_200,
                              colDate_uniq,
                              colVarchar_100,
                              colVarchar_uniq};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genUTAB00()

void
genPTAB00(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB00";

  long       keyColumns[]         = {colSInt32_uniq};
  long       keyColumnNumValues[] = {totalRowCount};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt16_10,
                                colSInt32_100, 
                                colSInt32_50p,
                                colUInt16_10,
                                colUInt32_100,
                                colUInt32_50p,
                                colUInt32_uniq,
                                colInt64_100,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colDate_uniq,
                                colVarchar_100,
                                colVarchar_uniq};
  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colSInt16_10,
                              colUInt16_10,
                              colSInt32_100, 
                              colUInt32_100,
                              colUInt32_50p,
                              colSInt32_50p,
                              colUInt32_uniq,
                              colSInt32_uniq,
                              colInt64_100,
                              colInt64_uniq,
                              colChar_10,
                              colChar_100,
                              colChar_50p,
                              colChar_uniq,
                              colUNum_10,
                              colSNum_100,
                              colUNum_50p,
                              colSNum_uniq,
                              colDate_12,
                              colDate_200,
                              colDate_uniq,
                              colVarchar_100,
                              colVarchar_uniq};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB00()

void
genPTAB01(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB01";

  long       keyColumns[]         = {colUInt32_uniq};
  long       keyColumnNumValues[] = {totalRowCount};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt16_10,
                                colSInt32_100, 
                                colSInt32_50p,
                                colSInt32_uniq,
                                colUInt16_10,
                                colUInt32_100,
                                colUInt32_50p,
                                colInt64_100,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colDate_uniq,
                                colVarchar_100,
                                colVarchar_uniq};
  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colSInt16_10,
                              colUInt16_10,
                              colChar_10,
                              colSInt32_100, 
                              colChar_100,
                              colUInt32_100,
                              colChar_50p,
                              colUInt32_50p,
                              colSInt32_50p,
                              colChar_uniq,
                              colUInt32_uniq,
                              colSInt32_uniq,
                              colInt64_100,
                              colInt64_uniq,
                              colUNum_10,
                              colSNum_100,
                              colUNum_50p,
                              colSNum_uniq,
                              colDate_12,
                              colDate_200,
                              colDate_uniq,
                              colVarchar_100,
                              colVarchar_uniq};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB01()

void
genPTAB02(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB02";

  long       keyColumns[]         = {colInt64_uniq};
  long       keyColumnNumValues[] = {totalRowCount};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt16_10,
                                colSInt32_100, 
                                colSInt32_50p,
                                colSInt32_uniq,
                                colUInt16_10,
                                colUInt32_100,
                                colUInt32_50p,
                                colUInt32_uniq,
                                colInt64_100,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colDate_uniq,
                                colVarchar_100,
                                colVarchar_uniq};
  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colSInt16_10,
                              colUInt16_10,
                              colChar_10,
                              colUNum_10,
                              colSInt32_100, 
                              colChar_100,
                              colUInt32_100,
                              colSNum_100,
                              colChar_50p,
                              colUInt32_50p,
                              colSInt32_50p,
                              colUNum_50p,
                              colChar_uniq,
                              colUInt32_uniq,
                              colSInt32_uniq,
                              colSNum_uniq,
                              colInt64_100,
                              colInt64_uniq,
                              colDate_12,
                              colDate_200,
                              colDate_uniq,
                              colVarchar_100,
                              colVarchar_uniq};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB02()

void
genPTAB03(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB03";

  long       keyColumns[]         = {colSInt32_uniq};
  long       keyColumnNumValues[] = {totalRowCount};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt16_10,
                                colSInt32_100, 
                                colSInt32_50p,
                                colUInt16_10,
                                colUInt32_100,
                                colUInt32_50p,
                                colUInt32_uniq,
                                colInt64_100,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colDate_uniq,
                                colVarchar_100,
                                colVarchar_uniq};
  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colSInt32_uniq,
                              colUInt32_uniq,
                              colSNum_uniq,
                              colInt64_uniq,
                              colChar_uniq,
                              colDate_uniq,
                              colSInt16_10,
                              colUInt16_10,
                              colChar_10,
                              colUNum_10,
                              colSInt32_100, 
                              colChar_100,
                              colUInt32_100,
                              colSNum_100,
                              colChar_50p,
                              colUInt32_50p,
                              colSInt32_50p,
                              colUNum_50p,
                              colInt64_100,
                              colDate_12,
                              colDate_200,
                              colVarchar_100,
                              colVarchar_uniq};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB03()

void
genPTAB04(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB04";

  long       keyColumns[]         = {colUInt32_uniq};
  long       keyColumnNumValues[] = {totalRowCount};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt16_10,
                                colSInt32_100, 
                                colSInt32_50p,
                                colSInt32_uniq,
                                colUInt16_10,
                                colUInt32_100,
                                colUInt32_50p,
                                colInt64_100,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colDate_uniq,
                                colVarchar_100,
                                colVarchar_uniq};
  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colSInt32_uniq,
                              colUInt32_uniq,
                              colSNum_uniq,
                              colInt64_uniq,
                              colChar_uniq,
                              colDate_uniq,
                              colSInt16_10,
                              colUInt16_10,
                              colChar_10,
                              colUNum_10,
                              colSInt32_100, 
                              colChar_100,
                              colUInt32_100,
                              colSNum_100,
                              colInt64_100,
                              colUInt32_50p,
                              colSInt32_50p,
                              colUNum_50p,
                              colChar_50p,
                              colDate_12,
                              colDate_200,
                              colVarchar_uniq,
                              colVarchar_100};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB04()

void
genPTAB05(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB05";

  long       keyColumns[]         = {colSInt32_100,colUInt16_10};
  long       keyColumnNumValues[] = {100,10};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt16_10,
                                colSInt32_50p,
                                colSInt32_uniq,
                                colUInt32_100,
                                colUInt32_50p,
                                colUInt32_uniq,
                                colInt64_100,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colDate_uniq,
                                colVarchar_100,
                                colVarchar_uniq};
  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colSInt16_10,
                              colUInt16_10,
                              colChar_10,
                              colUNum_10,
                              colSInt32_uniq,
                              colUInt32_uniq,
                              colSNum_uniq,
                              colInt64_uniq,
                              colChar_uniq,
                              colDate_uniq,
                              colSInt32_100, 
                              colChar_100,
                              colUInt32_100,
                              colSNum_100,
                              colInt64_100,
                              colUInt32_50p,
                              colSInt32_50p,
                              colUNum_50p,
                              colChar_50p,
                              colDate_12,
                              colDate_200,
                              colVarchar_uniq,
                              colVarchar_100};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB05()

void
genPTAB06(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB06";

  long       keyColumns[]         = {colUInt32_100,colSInt16_10};
  long       keyColumnNumValues[] = {100,10};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt32_100,
                                colSInt32_50p,
                                colSInt32_uniq,
                                colUInt16_10,
                                colUInt32_50p,
                                colUInt32_uniq,
                                colInt64_100,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colDate_uniq,
                                colVarchar_100,
                                colVarchar_uniq};
  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colSInt32_100,
                              colChar_100,
                              colUInt32_100,
                              colSNum_100,
                              colInt64_100,
                              colSInt16_10,
                              colUInt16_10,
                              colChar_10,
                              colUNum_10,
                              colSInt32_uniq,
                              colUInt32_uniq,
                              colSNum_uniq,
                              colInt64_uniq,
                              colChar_uniq,
                              colDate_uniq,
                              colUInt32_50p,
                              colSInt32_50p,
                              colUNum_50p,
                              colChar_50p,
                              colDate_12,
                              colDate_200,
                              colVarchar_uniq,
                              colVarchar_100};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB06()

void
genPTAB07(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB07";

  long       keyColumns[]         = {colInt64_100,colSInt16_10};
  long       keyColumnNumValues[] = {100,10};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt32_100,
                                colSInt32_50p,
                                colSInt32_uniq,
                                colUInt16_10,
                                colUInt32_100,
                                colUInt32_50p,
                                colUInt32_uniq,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colDate_uniq,
                                colVarchar_100,
                                colVarchar_uniq};
  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colSInt32_100,
                              colChar_100,
                              colUInt32_100,
                              colSNum_100,
                              colInt64_100,
                              colSInt16_10,
                              colUInt16_10,
                              colChar_10,
                              colUNum_10,
                              colSInt32_uniq,
                              colUInt32_uniq,
                              colSNum_uniq,
                              colInt64_uniq,
                              colChar_uniq,
                              colDate_uniq,
                              colUInt32_50p,
                              colSInt32_50p,
                              colUNum_50p,
                              colChar_50p,
                              colDate_12,
                              colDate_200,
                              colVarchar_uniq,
                              colVarchar_100};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB07()

void
genPTAB08(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB08";

  long       keyColumns[]         = {colInt64_100,colSInt16_10,colUInt16_10};
  long       keyColumnNumValues[] = {100,10,10};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt32_100,
                                colSInt32_50p,
                                colSInt32_uniq,
                                colUInt32_100,                                
                                colUInt32_50p,
                                colUInt32_uniq,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colDate_uniq,
                                colVarchar_100,
                                colVarchar_uniq};

  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colUInt32_50p,
                              colSInt32_50p,
                              colUNum_50p,
                              colChar_50p,
                              colSInt32_100,
                              colChar_100,
                              colUInt32_100,
                              colSNum_100,
                              colInt64_100,
                              colSInt16_10,
                              colUInt16_10,
                              colChar_10,
                              colUNum_10,
                              colSInt32_uniq,
                              colUInt32_uniq,
                              colSNum_uniq,
                              colInt64_uniq,
                              colChar_uniq,
                              colDate_uniq,
                              colDate_12,
                              colDate_200,
                              colVarchar_uniq,
                              colVarchar_100};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB08()

void
genPTAB09(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB09";

  long       keyColumns[]         = {colInt64_100,colSInt16_10,colUInt16_10};
  long       keyColumnNumValues[] = {100,10,10};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt32_100,
                                colSInt32_50p,
                                colSInt32_uniq,
                                colUInt32_100,                                
                                colUInt32_50p,
                                colUInt32_uniq,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colDate_uniq,
                                colVarchar_100,
                                colVarchar_uniq};

  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colUInt32_50p,
                              colSInt32_50p,
                              colUNum_50p,
                              colChar_50p,
                              colSInt32_100,
                              colChar_100,
                              colUInt32_100,
                              colSNum_100,
                              colInt64_100,
                              colSInt16_10,
                              colUInt16_10,
                              colChar_10,
                              colUNum_10,
                              colSInt32_uniq,
                              colUInt32_uniq,
                              colSNum_uniq,
                              colInt64_uniq,
                              colChar_uniq,
                              colDate_uniq,
                              colDate_12,
                              colDate_200,
                              colVarchar_uniq,
                              colVarchar_100};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB09()

void
genPTAB10(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB10";

  long       keyColumns[]         = {colChar_uniq};
  long       keyColumnNumValues[] = {totalRowCount};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt16_10,
                                colSInt32_100,
                                colSInt32_50p,
                                colSInt32_uniq,
                                colUInt16_10,                       
                                colUInt32_100,                                
                                colUInt32_50p,
                                colUInt32_uniq,
                                colInt64_100,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colDate_uniq,
                                colVarchar_100,
                                colVarchar_uniq};

  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colChar_10,
                              colChar_100,
                              colChar_50p,
                              colChar_uniq,
                              colSInt16_10,
                              colUInt16_10,
                              colSInt32_100,
                              colUInt32_100,
                              colUInt32_50p,
                              colSInt32_50p,
                              colUInt32_uniq,
                              colSInt32_uniq,
                              colInt64_100,
                              colInt64_uniq,
                              colUNum_10,
                              colSNum_100,
                              colUNum_50p,
                              colSNum_uniq,
                              colDate_12,
                              colDate_200,
                              colDate_uniq,
                              colVarchar_100,
                              colVarchar_uniq};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB10()

void
genPTAB11(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB11";

  long       keyColumns[]         = {colVarchar_uniq};
  long       keyColumnNumValues[] = {totalRowCount};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt16_10,
                                colSInt32_100,
                                colSInt32_50p,
                                colSInt32_uniq,
                                colUInt16_10,                       
                                colUInt32_100,                                
                                colUInt32_50p,
                                colUInt32_uniq,
                                colInt64_100,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colDate_uniq,
                                colVarchar_100};

  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colUNum_10,
                              colSNum_100,
                              colUNum_50p,
                              colSNum_uniq,
                              colChar_10,
                              colChar_100,
                              colChar_50p,
                              colChar_uniq,
                              colSInt16_10,
                              colUInt16_10,
                              colSInt32_100,
                              colUInt32_100,
                              colUInt32_50p,
                              colSInt32_50p,
                              colUInt32_uniq,
                              colSInt32_uniq,
                              colInt64_100,
                              colInt64_uniq,
                              colDate_12,
                              colDate_200,
                              colDate_uniq,
                              colVarchar_100,
                              colVarchar_uniq};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB11()

void
genPTAB12(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB12";

  long       keyColumns[]         = {colDate_uniq};
  long       keyColumnNumValues[] = {totalRowCount};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt16_10,
                                colSInt32_100,
                                colSInt32_50p,
                                colSInt32_uniq,
                                colUInt16_10,                       
                                colUInt32_100,                                
                                colUInt32_50p,
                                colUInt32_uniq,
                                colInt64_100,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colVarchar_100,
                                colVarchar_uniq};

  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colDate_12,
                              colDate_200,
                              colDate_uniq,
                              colInt64_100,
                              colInt64_uniq,
                              colChar_10,
                              colChar_100,
                              colChar_50p,
                              colChar_uniq,
                              colSInt16_10,
                              colUInt16_10,
                              colSInt32_100,
                              colUInt32_100,
                              colUInt32_50p,
                              colSInt32_50p,
                              colUInt32_uniq,
                              colSInt32_uniq,
                              colUNum_10,
                              colSNum_100,
                              colUNum_50p,
                              colSNum_uniq,
                              colVarchar_uniq,
                              colVarchar_100};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB12()

void
genPTAB13(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB13";

  long       keyColumns[]         = {colDate_uniq};
  long       keyColumnNumValues[] = {totalRowCount};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt16_10,
                                colSInt32_100,
                                colSInt32_50p,
                                colSInt32_uniq,
                                colUInt16_10,                       
                                colUInt32_100,                                
                                colUInt32_50p,
                                colUInt32_uniq,
                                colInt64_100,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colVarchar_100,
                                colVarchar_uniq};

  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colSInt16_10,
                              colUInt16_10,
                              colSInt32_100,
                              colUInt32_100,
                              colUInt32_50p,
                              colSInt32_50p,
                              colUInt32_uniq,
                              colSInt32_uniq,
                              colUNum_10,
                              colSNum_100,
                              colUNum_50p,
                              colSNum_uniq,
                              colDate_12,
                              colDate_200,
                              colDate_uniq,
                              colInt64_100,
                              colInt64_uniq,
                              colChar_10,
                              colChar_100,
                              colChar_50p,
                              colChar_uniq,
                              colVarchar_uniq,
                              colVarchar_100};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB13()

void
genPTAB14(long pNumOfInserts, long pRowsPerInsert)
{

  long totalRowCount = pNumOfInserts * pRowsPerInsert;

  char tableName[] = "PTAB14";

  long       keyColumns[]         = {colSInt32_100,colUInt16_10};
  long       keyColumnNumValues[] = {100,10};
  const long keyColumnCount = sizeof(keyColumns) / sizeof(long);
  initKeyColumns(keyColumns,keyColumnCount,totalRowCount);

  long       nonKeyColumns[] = {colSInt16_10,
                                colSInt32_50p,
                                colSInt32_uniq,
                                colUInt32_100,
                                colUInt32_50p,
                                colUInt32_uniq,
                                colInt64_100,
                                colInt64_uniq,
                                colChar_10,
                                colChar_100,
                                colChar_50p,
                                colChar_uniq,
                                colUNum_10,
                                colSNum_100,
                                colUNum_50p,
                                colSNum_uniq,
                                colDate_12,
                                colDate_200,
                                colDate_uniq,
                                colVarchar_100,
                                colVarchar_uniq};
  const long nonKeyColumnCount = sizeof (nonKeyColumns) / sizeof(long);
  initNonKeyColumns(totalRowCount);

  long       columnOrder[] = {colSInt16_10,
                              colUInt16_10,
                              colChar_10,
                              colUNum_10,
                              colSInt32_uniq,
                              colUInt32_uniq,
                              colSNum_uniq,
                              colInt64_uniq,
                              colChar_uniq,
                              colDate_uniq,
                              colSInt32_100, 
                              colChar_100,
                              colUInt32_100,
                              colSNum_100,
                              colInt64_100,
                              colUInt32_50p,
                              colSInt32_50p,
                              colUNum_50p,
                              colChar_50p,
                              colDate_12,
                              colDate_200,
                              colVarchar_uniq,
                              colVarchar_100};

  TableScriptGenerator tsg(tableName,
                           pNumOfInserts,
                           pRowsPerInsert,
                           kColumnCount,
                           columns,
                           columnOrder,
                           keyColumnCount,
                           keyColumns,
                           keyColumnNumValues,
                           nonKeyColumnCount,
                           nonKeyColumns);

  tsg.generateTableScript();

  resetColumns();

} // genPTAB14()

void main () {

  genUTAB00(6,100);
  genPTAB00(6,100);
  genPTAB01(6,100);
  genPTAB02(6,100);
  genPTAB03(6,100);
  genPTAB04(6,100);
  genPTAB05(6,100);
  genPTAB06(6,100);
  genPTAB07(6,100);
  genPTAB08(6,100);
  genPTAB09(6,100);
  genPTAB10(6,100);
  genPTAB11(6,100);
  genPTAB12(6,100);
  genPTAB13(6,100);
  genPTAB14(6,100);

}
