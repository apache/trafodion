/**********************************************************************
 *
 * File:         sqludr.h
 * Description:  Interface between the SQL engine and routine bodies
 * Language:     C
 *
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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
 *
 *********************************************************************/

#include "sqludr.h"
#include <stdio.h>
#include <cstdarg>

using namespace tmudr;

// ------------------------------------------------------------------------
// This file includes implementations of methods defined in sqludr.h that
// are of interest to UDR writers.
// 
// For example, the default action for the C++ compiler interface for
// TMUDFs are shown here. These can be called by TMUDFs, to provide
// additional features and this source code can be used to decide whether
// the default actions are sufficient for a given TMUDF.UDF developer
// can inspect this code, copy and modify it for their own use or
// call it from derived classes.
//
// This file gets compiled in Trafodion as part of 
// trafodion/core/sql/optimizer/UdfDllInteraction.cpp.
// It does not need to be included into the DLL of the UDF.
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
// Member functions for class UDRException
// ------------------------------------------------------------------------

UDRException::UDRException(unsigned int sqlState, const char *printf_format, ...)
{
  va_list args;
  const int maxMsgLen = 250;
  char msg[maxMsgLen];

  va_start(args, printf_format);
  vsnprintf(msg, maxMsgLen, printf_format, args);
  va_end(args);

  sqlState_ = sqlState;
  text_ = msg;
}

// ------------------------------------------------------------------------
// Member functions for class TypeInfo
// ------------------------------------------------------------------------

TypeInfo::TypeInfo(CTYPE_CODE cType,
                   int length,
                   bool nullable) :
       cType_(cType),
       sqlType_(UNDEFINED_SQL_TYPE),
       nullable_(nullable),
       scale_(0),
       charset_(UNDEFINED_CHARSET),
       intervalCode_(UNDEFINED_INTERVAL_CODE),
       precision_(0),
       collation_(SYSTEM_COLLATION),
       length_(0)
  {

    // derive the SQL type + length from the C type
    switch (cType)
      {
      case INT16:
        sqlType_ = SMALLINT;
        length_  = 2;
        break;

      case UINT16:
        sqlType_ = SMALLINT_UNSIGNED;
        length_  = 2;
        break;

      case INT32:
        sqlType_ = INT;
        length_  = 4;
        break;

      case UINT32:
        sqlType_ = INT_UNSIGNED;
        length_  = 4;
        break;

      case INT64:
        sqlType_ = LARGEINT;
        length_  = 8;
        break;

      case FLOAT:
        sqlType_ = REAL;
        length_  = 4;
        break;

      case DOUBLE:
        sqlType_ = DOUBLE_PRECISION;
        length_  = 8;
        break;

      case CHAR_ARRAY:
        sqlType_ = CHAR;
        length_ = length;
        charset_ = CHARSET_UTF8;
        break;

      case VARCHAR_ARRAY:
        sqlType_ = VARCHAR;
        length_ = length;
        charset_ = CHARSET_UTF8;
        break;

      case STRING:
        sqlType_ = VARCHAR;
        length_  = length;
        // assume this is a
        // [VAR]CHAR (<length> BYTES) CHARACTER SET UTF8
        charset_ = CHARSET_UTF8;
        break;

        //case BYTES:
        //sqlType_ = VARCHAR;
        //length_  = length;
        //break;

        //case TIME:
        //sqlType_ = SQL_TIME;
        // Todo: length_  = ?;
        //break;

      case UNDEFINED_C_TYPE:
        sqlType_ = UNDEFINED_SQL_TYPE;
        break;

      default:
        throw UDRException(38900,"Invalid C Type code for the short TypeInfo constructor: %d", cType);
        break;

      }
  }

TypeInfo::TypeInfo(SQLTYPE_CODE sqlType,
                   int length,
                   bool nullable,
                   int scale,
                   SQLCHARSET_CODE charset,
                   SQLINTERVAL_CODE intervalCode,
                   int precision,
                   SQLCOLLATION_CODE collation) :
       cType_(UNDEFINED_C_TYPE),
       sqlType_(sqlType),
       nullable_(nullable),
       scale_(scale),
       charset_(charset),
       intervalCode_(intervalCode),
       precision_(precision),
       collation_(collation),
       length_(length)
{
  switch (sqlType)
    {
    case SMALLINT:
      cType_ = INT16;
      length_ = 2;
      break;

    case INT:
      cType_ = INT32;
      length_ = 4;
      break;

    case LARGEINT:
      cType_ = INT64;
      length_ = 8;
      break;

    case NUMERIC:
      cType_ = INT32;
      length_ = 4;
      if (scale_ < 0 || scale > 18)
        throw UDRException(38900,"Scale %d of a numeric in TypeInfo::TypeInfo is out of the allowed range of 0-18", scale_);
      if (precision_ < 0 || precision_ > 18)
        throw UDRException(38900,"Precision %d of a numeric in TypeInfo::TypeInfo is out of the allowed range of 0-18", precision_);
      if (scale > precision)
        throw UDRException(38900,"Scale %d of a numeric in TypeInfo::TypeInfo is greater than precision %d", scale_, precision_);
      break;

    case DECIMAL_LSE:
      cType_ = CHAR_ARRAY;
      if (scale_ < 0 || scale > 18)
        throw UDRException(38900,"Scale %d of a decimal in TypeInfo::TypeInfo is out of the allowed range of 0-18", scale_);
      if (precision_ < 0 || precision_ > 18)
        throw UDRException(38900,"Precision %d of a decimal in TypeInfo::TypeInfo is out of the allowed range of 0-18", precision_);
      if (scale > precision)
        throw UDRException(38900,"Scale %d of a decimal in TypeInfo::TypeInfo is greater than precision %d", scale_, precision_);
      break;

    case SMALLINT_UNSIGNED:
      cType_ = UINT16;
      length_ = 2;
      break;

    case INT_UNSIGNED:
      cType_ = UINT32;
      length_ = 4;
      break;

    case NUMERIC_UNSIGNED:
      cType_ = INT32;
      length_ = 4;
      if (scale_ < 0 || scale > 18)
        throw UDRException(38900,"Scale %d of a numeric unsigned in TypeInfo::TypeInfo is out of the allowed range of 0-18", scale_);
      if (precision_ < 0 || precision_ > 18)
        throw UDRException(38900,"Precision %d of a numeric unsigned in TypeInfo::TypeInfo is out of the allowed range of 0-18", precision_);
      if (scale > precision)
        throw UDRException(38900,"Scale %d of a numeric unsigned in TypeInfo::TypeInfo is greater than precision %d", scale_, precision_);
      break;

    case DECIMAL_UNSIGNED:
      cType_ = CHAR_ARRAY;
      if (scale_ < 0 || scale > 18)
        throw UDRException(38900,"Scale %d of a decimal unsigned in TypeInfo::TypeInfo is out of the allowed range of 0-18", scale_);
      if (precision_ < 0 || precision_ > 18)
        throw UDRException(38900,"Precision %d of a decimal unsigned in TypeInfo::TypeInfo is out of the allowed range of 0-18", precision_);
      if (scale > precision)
        throw UDRException(38900,"Scale %d of a decimal unsigned in TypeInfo::TypeInfo is greater than precision %d", scale_, precision_);
      break;

    case REAL:
      cType_ = FLOAT;
      length_ = 4;
      break;

    case DOUBLE_PRECISION:
      cType_ = DOUBLE;
      length_ = 8;
      break;

    case CHAR:
      cType_ = CHAR_ARRAY;
      if (charset_ == UNDEFINED_CHARSET)
        throw UDRException(38900,"Charset must be specified for CHAR type in TypeInfo::TypeInfo");
      break;

    case VARCHAR:
      cType_ = VARCHAR_ARRAY;
      if (charset_ == UNDEFINED_CHARSET)
        throw UDRException(38900,"Charset must be specified for CHAR type in TypeInfo::TypeInfo");
      if (collation_ == UNDEFINED_COLLATION)
        throw UDRException(38900,"Collation must be specified for CHAR type in TypeInfo::TypeInfo");
      break;

    case DATE:
      // string yyyy-mm-dd
      cType_ = CHAR_ARRAY;
      length_ = 10;
      scale_ = 0;
      break;

    case TIME:
      // string hh:mm:ss
      cType_ = CHAR_ARRAY;
      length_ = 8;
      scale_ = 0;
      break;

    case TIMESTAMP:
      // string yyyy-mm-dd hh:mm:ss.ffffff
      //        12345678901234567890123456
      cType_ = CHAR_ARRAY;
      length_ = 19;
      if (scale > 0)
        length_ += scale+1;
      if (scale < 0 || scale_ > 6)
        throw UDRException(38900,"Scale %d of timestamp in TypeInfo::TypeInfo is outside the allowed range of 0-6", sqlType);
      break;

    case INTERVAL:
      // all intervals are treated like numbers
      cType_ = INT32;
      length_ = 4;
      if (intervalCode_ == UNDEFINED_INTERVAL_CODE)
        throw UDRException(38900,"Interval code in TypeInfo::TypeInfo is undefined");
      if (scale < 0 || scale_ > 6)
        throw UDRException(38900,"Scale %d of interval in TypeInfo::TypeInfo is outside the allowed range of 0-6", sqlType);
      // todo: Check scale and precision in more detail
      break;

    default:
      throw UDRException(38900,"Invalid SQL Type code for the short TypeInfo constructor with an SQL code: %d", sqlType);
      break;
    }
}

TypeInfo::SQLTYPE_CLASS_CODE TypeInfo::getSQLTypeClass() const
{
  switch (sqlType_)
    {
    case SMALLINT:
    case INT:
    case LARGEINT:
    case NUMERIC:
    case DECIMAL_LSE:
    case SMALLINT_UNSIGNED:
    case INT_UNSIGNED:
    case NUMERIC_UNSIGNED:
    case DECIMAL_UNSIGNED:
    case REAL:
    case DOUBLE_PRECISION:
      return NUMERIC_TYPE;

    case CHAR:
    case VARCHAR:
      return CHARACTER_TYPE;

    case DATE:
    case TIME:
    case TIMESTAMP:
      return DATETIME_TYPE;

    case INTERVAL:
      return INTERVAL_TYPE;

    default:
      break;
    }

  return UNDEFINED_TYPE_CLASS;
}

long TypeInfo::getLongValue(const Bytes data,
                            int dataLen) const
{
  long result = 999999999;
  int tempSQLType = sqlType_;

  if (nullable_)
    throw UDRException(38901,
                       "TypeInfo::getNumericValue() not supported for nullable types yet");

  // convert NUMERIC to the corresponding type with binary precision
  if (sqlType_ == NUMERIC || sqlType_ == NUMERIC_UNSIGNED)
    {
      if (precision_ <= 4)
        if (sqlType_ == NUMERIC)
          tempSQLType = SMALLINT;
        else
          tempSQLType = SMALLINT_UNSIGNED;
      else if (precision_ <= 9)
        if (sqlType_ == NUMERIC)
          tempSQLType = INT;
        else
          tempSQLType = INT_UNSIGNED;
      else if (precision_ <= 18)
        if (sqlType_ == NUMERIC)
          tempSQLType = LARGEINT;
        // else unsigned 8 byte integer is not supported
    }

  switch (tempSQLType)
    {
    case SMALLINT:
      result = *((short *) data);
      break;

    case INT:
      result = *((int *) data);
      break;

    case LARGEINT:
      result = *((long *) data);
      break;

    case SMALLINT_UNSIGNED:
      result = *((unsigned short *) data);
      break;

    case INT_UNSIGNED:
      result = *((unsigned int *) data);
      break;

    default:
      throw UDRException(38902,
                         "TypeInfo::getNumericValue() not supported for SQL type %d",
                         sqlType_);
      break;
    }

  return result;
}
  
double TypeInfo::getDoubleValue(const Bytes data,
                                int dataLen) const
{
  double result = 999999999;

  if (nullable_)
    throw UDRException(38903,
                       "TypeInfo::getNumericValue() not supported for nullable types yet");

  switch (sqlType_)
    {
    case REAL:
      result = *((float *) data);
      break;

    case DOUBLE_PRECISION:
      result = *((double *) data);
      break;

    default:
      result = getLongValue(data, dataLen);
      break;
    }

  return result;
}

const char *TypeInfo::getStringValue(const Bytes data,
                                     int dataLen) const
{
  // Todo - handle different types of char/varchar
  return (const char *) data;
}

// ------------------------------------------------------------------------
// Member functions for class ParameterInfo
// ------------------------------------------------------------------------


// ------------------------------------------------------------------------
// Member functions for class ColumnInfo
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
// Member functions for class ConstraintInfo
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
// Member functions for class PredicateInfo
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
// Member functions for class PartitionInfo
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
// Member functions for class TableInfo
// ------------------------------------------------------------------------

TableInfo::~TableInfo()
{
  // delete all columns and constraints
  for (std::vector<ColumnInfo *>::iterator it1 = columns_.begin();
       it1 != columns_.end();
       it1++)
    delete *it1;

  for (std::vector<ConstraintInfo *>::iterator it2 = constraints_.begin();
       it2 != constraints_.end();
       it2++)
    delete *it2;
}

const ColumnInfo &TableInfo::getColumn(const std::string &name) const
{
  for (std::vector<ColumnInfo *>::const_iterator it = columns_.begin();
       it != columns_.end();
       it++)
    if ((*it)->getColName() == name)
      return **it;

  throw UDRException(38904, "Column %s not found", name.data());
}

ColumnInfo &TableInfo::getColumn(const std::string &name)
{
  for (std::vector<ColumnInfo *>::iterator it = columns_.begin();
       it != columns_.end();
       it++)
    if ((*it)->getColName() == name)
      return **it;

  throw UDRException(38905, "Column %s not found", name.data());
}

void TableInfo::addColumn(const ColumnInfo &column)
{
  ColumnInfo *newCol = new ColumnInfo(column);

  columns_.push_back(newCol);
}

void TableInfo::addIntegerColumn(const char *colName, bool isNullable)
{
  addColumn(ColumnInfo(colName, TypeInfo(TypeInfo::INT32,0,isNullable)));
}

void TableInfo::addCharColumn(const char *colName,
                              int length,
                              bool isNullable,
                              TypeInfo::SQLCHARSET_CODE charset,
                              TypeInfo::SQLCOLLATION_CODE collation)
{
  addColumn(ColumnInfo(colName, TypeInfo(TypeInfo::CHAR,
                                         length,
                                         isNullable,
                                         0,
                                         charset,
                                         TypeInfo::UNDEFINED_INTERVAL_CODE,
                                         0,
                                         collation)));
}

void TableInfo::addVarCharColumn(const char *colName,
                                 int length,
                                 bool isNullable,
                                 TypeInfo::SQLCHARSET_CODE charset,
                                 TypeInfo::SQLCOLLATION_CODE collation)
{
  addColumn(ColumnInfo(colName, TypeInfo(TypeInfo::VARCHAR,
                                         length,
                                         isNullable,
                                         0,
                                         charset,
                                         TypeInfo::UNDEFINED_INTERVAL_CODE,
                                         0,
                                         collation)));
}

void TableInfo::addColumns(const std::vector<ColumnInfo *> &columns)
{
  for (std::vector<ColumnInfo *>::const_iterator it = columns.begin();
       it != columns.end();
       it++)
    addColumn(**it);
}

void TableInfo::addColumnAt(const ColumnInfo &column, int position)
{
  ColumnInfo *newCol = new ColumnInfo(column);

  columns_.insert(columns_.begin() + position, newCol);
}

void TableInfo::deleteColumn(unsigned int i)
{
  std::vector<ColumnInfo *>::iterator it = columns_.begin() + i;

  if (it != columns_.end())
    {
      delete *it;
      columns_.erase(it);
    }
  else
    throw UDRException(38906, "Column number %d not found", i);
}

void TableInfo::deleteColumn(const std::string &name)
{
  for (std::vector<ColumnInfo *>::iterator it = columns_.begin();
       it != columns_.end();
       it++)
    if ((*it)->getColName() == name)
      {
        // delete the object the pointer is pointing to
        // and remove the pointer from the list
        delete *it;
        columns_.erase(it);
      }

  throw UDRException(38907, "Column %s not found", name.data());
}

void TableInfo::addConstraint(ConstraintInfo &constraint)
{
  ConstraintInfo *newConstr = new ConstraintInfo(constraint);

  constraints_.push_back(newConstr);
}

void TableInfo::setIsStream(bool stream)
{
  if (stream)
    throw UDRException(38908, "Stream tables not yet supported");
}

// ------------------------------------------------------------------------
// Member functions for class UDFWriterPrivateData
// ------------------------------------------------------------------------

UDFWriterPrivateData::UDFWriterPrivateData()
{}

UDFWriterPrivateData::~UDFWriterPrivateData()
{}

// ------------------------------------------------------------------------
// Member functions for class UDRInvocationInfo
// ------------------------------------------------------------------------

UDRInvocationInfo::UDRInvocationInfo() :
     numTableInputs_(0),
     funcType_(SETFUNC),
     version_(1),
     udfWriterPrivateData_(NULL)
{}

UDRInvocationInfo::~UDRInvocationInfo()
{
  // delete all the content of collections of pointers
  for (std::vector<ParameterInfo *>::iterator f = formalParameterInfo_.begin();
       f != formalParameterInfo_.end();
       f++)
    delete *f;
  for (std::vector<ParameterInfo *>::iterator a = actualParameterInfo_.begin();
       a != actualParameterInfo_.end();
       a++)
    delete *a;
  for (std::vector<PredicateInfo *>::iterator p = predicates_.begin();
       p != predicates_.end();
       p++)
    delete *p;

  // delete UDF writer's data
  if (udfWriterPrivateData_)
    delete udfWriterPrivateData_;
}

const TableInfo &UDRInvocationInfo::getInputTableInfo(int childNum) const
{
  if (childNum >= numTableInputs_)
    throw UDRException(38909, "Invalid child table number %d", childNum);

  return inputTableInfo_[childNum];
}

const ParameterInfo &UDRInvocationInfo::getFormalParameterInfo(
     const std::string &name) const
{
  for (std::vector<ParameterInfo *>::const_iterator it = formalParameterInfo_.begin();
       it != formalParameterInfo_.end();
       it++)
    if ((*it)->getParameterName() == name)
      return **it;

  throw UDRException(38910, "Parameter %s not found", name.data());
}

const ParameterInfo &UDRInvocationInfo::getActualParameterInfo(
     const std::string &name) const
{
  std::vector<ParameterInfo *>::const_iterator it;
  int i;

  for (it = formalParameterInfo_.begin(), i=0;
       it != formalParameterInfo_.end();
       it++, i++)
    if ((*it)->getParameterName() == name)
      return getActualParameterInfo(i);

  throw UDRException(38911, "Actual parameter %s not found", name.data());
}

void UDRInvocationInfo::addPassThruColumns(int inputTableNum,
                                           int startInputColumnNum,
                                           int endInputColumnNum)
{
  // Adding one or more columns from an input (child) table as output columns
  // The advantage of doing this is that the query optimizer can automatically
  // apply some optimizations:
  //
  // - Push predicates on output columns down to input tables. This reduces the
  //   number of rows that have to be processed by the TMUDF.
  // - If a table-valued input is ordered, the TMUDF output is assumed to be
  //   also ordered on the corresponding columns.
  // - Similar for partitioning.
  // - If there are histogram statistics on an input column, these statistics
  //   will be used for the output columns as well, even though the TMUDF may
  //   eliminate some input rows and duplicate others, so the total row count
  //   and frequencies of values may or may not be usable.

  if (endInputColumnNum == -1)
    endInputColumnNum = getInputTableInfo(inputTableNum).getNumColumns() - 1;

  for (int c=startInputColumnNum; c<=endInputColumnNum; c++)
    {
      // make a copy of the input column
      ColumnInfo newCol(
             getInputTableInfo(inputTableNum).getColumn(c));

      // change the provenance info of the column
      newCol.setProvenance(ProvenanceInfo(inputTableNum, c));

      outputTableInfo_.addColumn(newCol);
    }
}

void UDRInvocationInfo::addPredicate(const PredicateInfo &pred)
{
  PredicateInfo *newPred = new PredicateInfo(pred);

  predicates_.push_back(newPred);
}

void UDRInvocationInfo::setPrivateData(UDFWriterPrivateData *privateData)
{
  // for now we can't allow this, since we would call the destructor of
  // this object after we unloaded the DLL containing the code
  // Todo: Cache DLL opens, at least until after the
  // UDRInvocationInfo objects get deleted.
  throw UDRException(
       38912,
       "UDRInvocationInfo::setPrivateData() not yet supported");
  /*
  if (udfWriterPrivateData_)
    delete udfWriterPrivateData_;

  udfWriterPrivateData_ = privateData;
  */
}

// ------------------------------------------------------------------------
// Member functions for class UDRPlanInfo
// ------------------------------------------------------------------------

UDRPlanInfo::UDRPlanInfo() :
     costPerRow_(0),
     degreeOfParallelism_(ANY_DEGREE_OF_PARALLELISM),
     udfWriterPrivateData_(NULL)
{}

UDRPlanInfo::~UDRPlanInfo()
{
  if (udfWriterPrivateData_)
    delete udfWriterPrivateData_;
}

void UDRPlanInfo::setPrivateData(UDFWriterPrivateData *privateData)
{
  // for now we can't allow this, since we would call the destructor of
  // this object after we unloaded the DLL containing the code
  // Todo: Cache DLL opens, at least until after the UDRPlanInfo objects get
  // deleted.
  throw UDRException(38913,"UDRPlanInfo::setPrivateData() not yet supported");

  /*
  if (udfWriterPrivateData_)
    delete udfWriterPrivateData_;

  udfWriterPrivateData_ = privateData;
  */
}

// ------------------------------------------------------------------------
// Member functions for class TMUDRInterface
// ------------------------------------------------------------------------

TMUDRInterface::TMUDRInterface() {}

TMUDRInterface::~TMUDRInterface() {}

// Describe the output columns of a TMUDF, based on a description of
// its parameters (including parameter values that are specified as a
// constant) and the description of the table-valued input columns

// This default implementation does nothing
void TMUDRInterface::describeParamsAndColumns(UDRInvocationInfo &info)
{
  // First method called during compilation of a TMUDF invocation
  // ------------------------------------------------------------

  // When the optimizer calls this, it will have set up the formal and
  // actual parameter descriptions as well as an output column
  // description containing all the output parameters defined in the
  // CREATE FUNCTION DDL (if any).

  // This method should do a general check of things it expects
  // that can be validated at this time.

  // This method can add, alter, or remove input and output
  // parameters:
  // - input parameters that are constants can be altered
  //   to a different constant value
  // - Todo: additional constant input parameters can be added
  // - non-constant input parameters: ???
  // - output parameters (columns of the table-valued result)
  //   can be input columns that are passed through or
  //   they can be columns generated by the TMUDF

  // Example 1: check for a constant parameter that is expected:
  /*
     if (info.getNumParameters() != 3)
       throw UDRException(38088, 
                          "Expecting three input parameters");
     if (!info.getParameterInfo(0).isAvailable())
       throw UDRException(38099, 
                          "First argument of TMUDF %s needs to be a compile time constant",
                          info.getName().data());
  */

  // Example 2: To add a column produced by the TMUDF, use the
  // TableInfo::addColumn() method. Example: Add a char(10) output
  // column created by the UDF as the first column:

  /*
     info.getOutputTableInfo().addColumnAt(
       ColumnInfo("ColProducedByTMUDF",      // name of output column
                  TypeInfo(TypeInfo::STRING, // C type of column
                           10)),             // length of string
       0);                                   // output column position
  */

  // Example 3: Add all columns provided by the table-valued
  //            inputs as output columns

  /*
     for (int t=0; t<info.getNumTableInputs(); t++)
       info.addPassThruColumns(t);
  */

}

void TMUDRInterface::describeDataflow(UDRInvocationInfo &info)
{
  // TBD
}

void TMUDRInterface::describeConstraints(UDRInvocationInfo &info)
{
  // TBD
}

void TMUDRInterface::describeStatistics(UDRInvocationInfo &info)
{
  // TBD
}

void TMUDRInterface::describeDesiredDegreeOfParallelism(UDRInvocationInfo &info,
                                                        UDRPlanInfo &plan)
{
  // The default behavior is to allow any degree of parallelism for
  // TMUDFs with one table-valued input, and to force serial execution
  // in all other cases. The reason is that for a single table-valued
  // input, there is a natural way to parallelize the function by
  // parallelizing its input a la MapReduce. In all other cases,
  // parallel execution requires active participation by the UDF,
  // which is why the UDF needs to signal explicitly that it can
  // handle such flavors of parallelism.

  // Note that this is NOT foolproof, and that the TMUDF might still
  // need to validate the PARTITION BY and ORDER BY syntax used in its
  // invocation.

  if (info.getNumTableInputs() == 1)
    plan.setDesiredDegreeOfParallelism(UDRPlanInfo::ANY_DEGREE_OF_PARALLELISM);
  else
    plan.setDesiredDegreeOfParallelism(1); // serial execution

  // Note about using the pre-defined constants:
  // 
  // ANY_DEGREE_OF_PARALLELISM:
  //        This will allow the optimizer to choose any degree
  //        of parallelism, including 1 (serial execution)
  // DEFAULT_DEGREE_OF_PARALLELISM:
  //        Currently the same as ANY_DEGREE_OF_PARALLELISM.
  //        The optimizer will use a heuristic based on
  //        the estimated cardinality.
  // MAX_DEGREE_OF_PARALLELISM:
  //        Choose the highest possible degree of parallelism.
  // ONE_INSTANCE_PER_NODE:
  //        Start one parallel instance on every Trafodion node.
  //        This is mostly meant for internal TMUDFs, e.g. a
  //        TMUDF to read the log files on every node.
}

void TMUDRInterface::describePlanProperties(UDRInvocationInfo &info,
                                                    UDRPlanInfo &plan)
{
  // TBD
}

void TMUDRInterface::completeDescription(UDRInvocationInfo &info,
                                                 UDRPlanInfo &plan)
{
  // TBD
}

