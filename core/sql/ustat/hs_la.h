/**********************************************************************
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
**********************************************************************/
#ifndef HSLA_H
#define HSLA_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_la.h
 * Description:  Function to retrieve some lable info.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include "hs_const.h"
#include "hs_util.h"
#include "hiveHook.h"
#include "HDFSHook.h"

struct HSColumnStruct;

class HSTableDef : public NABasicObject
  {
  public:
    enum formatType  {INTERNAL_FORMAT, EXTERNAL_FORMAT};
    enum labelDetail {MIN_INFO, MAX_INFO};
    enum tblOrigin   {SQ_TBL, HIVE_TBL, HBASE_TBL};

    // Factory function to create an instance of the appropriate subclass.
    static HSTableDef* create(CollHeap* heap,
                              const ComObjectName &tableName,
                              const hs_table_type tableType,
                              const ComAnsiNameSpace nameSpace = COM_TABLE_NAME);

    HSTableDef(const ComObjectName &tableName,
               const hs_table_type tableType,
               const ComAnsiNameSpace nameSpace);

    ~HSTableDef();
    virtual NABoolean objExists(NABoolean createExternalTable = FALSE) = 0;
    virtual NABoolean publicSchemaExists() = 0;
    Lng32 getColNum(const char *colName, NABoolean errIfNotFound = TRUE) const;
    char* getColName(Lng32 colNum) const;
    HSColumnStruct& getColInfo(Lng32 colNum) const;
    virtual NAString getNodeName() const = 0;
    NAString getObjectFullName() const;
    NATable* getNATable() const {return naTbl_;}
    void setNATable();

    virtual NAString getCatalogLoc(formatType format = INTERNAL_FORMAT) const = 0;
    NAString getPrimaryLoc(formatType format = INTERNAL_FORMAT) const;
    virtual NAString getHistLoc(formatType format = INTERNAL_FORMAT) const = 0;
    NAString getCatName(formatType format = INTERNAL_FORMAT) const;
    NAString getSchemaName(formatType format = INTERNAL_FORMAT) const;
    NAString getObjectName(formatType format = INTERNAL_FORMAT) const;
    virtual Lng32  getColumnNames() = 0;
    ComObjectType getObjectType() const {return objectType_;}
    virtual Lng32 getFileType() const = 0;
    Lng32  getErrorCode() const {return retcode_;}
    Lng32 getNumCols()   const {return numCols_;}
    virtual Lng32 getNumPartitions() const = 0;
    virtual Lng32 getRecordLength() const = 0;
    Int64 getObjectUID() const {return objectUID_;}
    virtual Int64 getModTime() const = 0;
    virtual Lng32 getIsFormat2Table() const = 0;
    ComAnsiNameSpace getNameSpace() const {return nameSpace_;}
    NABoolean isVolatile() const { return isVolatile_; }

    virtual void getRowChangeCounts(Int64 &inserts, Int64 &deletes, Int64 &updates) = 0;
    virtual void resetRowCounts() = 0;
    virtual Int64 getRowCount(NABoolean &isEstimate,
                              Int32 &errorCode,
                              Int32 &breadCrumb,
                              NABoolean estimateIfNecessary = TRUE) = 0;
    virtual Int64 getRowCount(NABoolean &isEstimate,
                      Int64 &numInserts,
                      Int64 &numDeletes,
                      Int64 &numUpdates,
                      Int64 &numPartitions,
                      Int64 &minRowCtPerPartition,
                      Int32 &errorCode,
                      Int32 &breadCrumb,
                      NABoolean estimateIfNecessary
                     ) = 0;
    Int64 getRowCountUsingSelect();
    ComDiskFileFormat getObjectFormat() const {return objActualFormat_;}
    virtual Lng32 collectFileStatistics() const = 0;
            NABoolean isMetadataObject() const {return isMetadataObject_;}
    virtual NABoolean isInMemoryObjectDefn() const = 0;
            NABoolean hasSyskey() const {return hasSyskey_;}
    virtual Lng32 getTotalVarcharLength() const = 0;
    virtual Lng32 getBlockSize() const = 0;
    
    // This should be called only once - it sets the hasSyskey_ flag.
    virtual Lng32 setHasSyskeyFlag() = 0;

    // SQ, Hive, or Hbase table?
    virtual tblOrigin getTblOrigin() const = 0;

    void addTruncatedSelectList(NAString & qry);

    NABoolean allUserColumnsAreLOBs();

  protected:
    NABoolean setObjectUID(NABoolean createExternalObject);

  protected:
    NAString           *tableName_;
    NAString           *ansiName_;
    NAString           *guardianName_;
    NATable            *naTbl_;
    ComAnsiNameSpace    nameSpace_;
    NAString           *catalog_;
    NAString           *schema_;
    NAString           *object_;
    Int64               objectUID_;
    ComDiskFileFormat   objActualFormat_;
    hs_table_type       objFormat_;
    Lng32               numCols_;
    NABoolean           labelAccessed_;
    HSColumnStruct     *colInfo_;
    Lng32               retcode_;
    NABoolean           isVolatile_;
    ComObjectType       objectType_;
    NABoolean           isMetadataObject_;
    NABoolean           hasSyskey_;

    virtual void GetLabelInfo(labelDetail detail = MIN_INFO) = 0;
    virtual Lng32 DescribeColumnNames() = 0;
  };


class HSSqTableDef : public HSTableDef
  {
  public:
    HSSqTableDef(const ComObjectName& tableName,
                 const hs_table_type tableType,
                 const ComAnsiNameSpace nameSpace = COM_TABLE_NAME)
      : HSTableDef(tableName, tableType, nameSpace),
        inMemoryObjectDefn_(FALSE)
      {}
    ~HSSqTableDef()
      {}
    NABoolean objExists(NABoolean createExternalTable = FALSE);
    NABoolean publicSchemaExists();
    NAString getNodeName() const;
    NAString getCatalogLoc(formatType format = INTERNAL_FORMAT) const;
    NAString getHistLoc(formatType format = INTERNAL_FORMAT) const {return getPrimaryLoc(format);}
    Lng32  getFileType()  const
    {
    	return -1;
    }

    Lng32  getNumPartitions() const
    {
    	return -1;
    }
    Lng32  getRecordLength() const
    {
    return -1;
    }

    Int64 getModTime() const;

    Lng32 getIsFormat2Table() const
    {
    	return -1;
    }

    virtual Lng32 getColumnNames();

    void getRowChangeCounts(Int64 &inserts, Int64 &deletes, Int64 &updates);
    void resetRowCounts();
    Int64 getRowCount(NABoolean &isEstimate,
                      Int32 &errorCode,
                      Int32 &breadCrumb,
                      NABoolean estimateIfNecessary = TRUE);
    Int64 getRowCount(NABoolean &isEstimate,
                      Int64 &numInserts,
                      Int64 &numDeletes,
                      Int64 &numUpdates,
                      Int64 &numPartitions,
                      Int64 &minRowCtPerPartition,
                      Int32 &errorCode,
                      Int32 &breadCrumb,
                      NABoolean estimateIfNecessary
                     );
    Lng32 collectFileStatistics() const;
    NABoolean isInMemoryObjectDefn() const {return inMemoryObjectDefn_;}
    Lng32 getTotalVarcharLength() const ;

    Lng32 getBlockSize() const
    {
    	return -1;
    }

    tblOrigin getTblOrigin() const
    {
      return SQ_TBL;
    }
    
    // This should be called only once - it sets the hasSyskey_ flag.
    Lng32 setHasSyskeyFlag();

  private:
    void GetLabelInfo(labelDetail detail = MIN_INFO);
    Lng32 DescribeColumnNames();
    NABoolean inMemoryObjectDefn_;
  };



class HSHiveTableDef : public HSTableDef
{
  public:
    HSHiveTableDef(const ComObjectName &tableName,
                   const hs_table_type tableType,
                   const ComAnsiNameSpace nameSpace = COM_TABLE_NAME)
      : minPartitionRows_(-1),
        HSTableDef(tableName, tableType, nameSpace)
      {}

    ~HSHiveTableDef()
      {}
    NABoolean objExists(NABoolean createExternalTable = FALSE);
    NABoolean publicSchemaExists()
      {
        return FALSE;
      };
    NAString getNodeName() const;
    NAString getCatalogLoc(formatType format = INTERNAL_FORMAT) const;
    NAString getHistLoc(formatType format = INTERNAL_FORMAT) const;
    Lng32 getFileType()  const
      {
        return -1;
      }
    Lng32 getNumPartitions() const
      {
        return tableStats_->entries();
      }
    Lng32 getRecordLength() const
      {
        return tableStats_->getEstimatedRecordLength();
      }
    Int64 getModTime() const 
      {
        return tableStats_->getValidationTimestamp();
      }
    Lng32 getIsFormat2Table() const
      {
        return 0;  // nsk only
      }

    virtual Lng32 getColumnNames();

    void getRowChangeCounts(Int64 &inserts, Int64 &deletes, Int64 &updates)
      {
        //@ZXhive -- need to issue a warning if NECESSARY is used on a hive
        //           table, indicating that the only case it works for is if
        //           opt has requested stats where there are none (see header
        //           comment for AddNecessaryColumns()).
        inserts = deletes = updates = 0;
      }
    void resetRowCounts()
      {}
    Int64 getRowCount(NABoolean &isEstimate,
                      Int32 &errorCode,
                      Int32 &breadCrumb, 
                      NABoolean estimateIfNecessary = TRUE)
      {
        isEstimate = TRUE;
        errorCode = 0;
        breadCrumb = -3;
        return (estimateIfNecessary ? tableStats_->getEstimatedRowCount() : 0);
      }
    Int64 getRowCount(NABoolean &isEstimate,
                      Int64 &numInserts,
                      Int64 &numDeletes,
                      Int64 &numUpdates,
                      Int64 &numPartitions,
                      Int64 &minRowCtPerPartition,
                      Int32 &errorCode,
                      Int32 &breadCrumb,
                      NABoolean estimateIfNecessary);
    Lng32 collectFileStatistics() const
      {
        return 0;
      }
    NABoolean isInMemoryObjectDefn() const
      {
        return FALSE;
      }
    Lng32 getTotalVarcharLength() const
      {
        // Used for row count estimation for sq tables. Not needed for hive.
        return 0;
      }
    Lng32 getBlockSize() const
      {
        return tableStats_->getEstimatedBlockSize();
      }
    Lng32 setHasSyskeyFlag()
      {
        return 0;
      }

    tblOrigin getTblOrigin() const
    {
      return HIVE_TBL;
    }

  private:
    void GetLabelInfo(labelDetail detail = MIN_INFO)
      {}
    Lng32 DescribeColumnNames();

    const HHDFSTableStats* tableStats_;
    hive_tbl_desc* hiveTblDesc_;
    Int64 minPartitionRows_;
};


class HSHbaseTableDef : public HSTableDef
{
  public:
    HSHbaseTableDef(const ComObjectName &tableName,
                    const hs_table_type tableType,
                    const ComAnsiNameSpace nameSpace = COM_TABLE_NAME)
      : minPartitionRows_(-1),
        HSTableDef(tableName, tableType, nameSpace)
      {}

    ~HSHbaseTableDef()
      {}
    NABoolean objExists(NABoolean createExternalTable = FALSE);
    NABoolean publicSchemaExists()
      {
        return FALSE;
      };
    NAString getNodeName() const;
    NAString getCatalogLoc(formatType format = INTERNAL_FORMAT) const;
    NAString getHistLoc(formatType format = INTERNAL_FORMAT) const;

    Lng32 getFileType()  const
      {
        return -1;
      }
    Lng32 getNumPartitions() const;
    Lng32 getRecordLength() const
      {
        //return tableStats_->getEstimatedRecordLength();
        return 100;
      }
    Int64 getModTime() const 
      {
        //return tableStats_->getValidationTimestamp();
        return 0;
      }
    Lng32 getIsFormat2Table() const
      {
        return 0;  // nsk only
      }

    virtual Lng32  getColumnNames();

    void getRowChangeCounts(Int64 &inserts, Int64 &deletes, Int64 &updates)
      {
        //@ZXhbase -- need to issue a warning if NECESSARY is used on an HBase
        //            table, indicating that the only case it works for is if
        //            opt has requested stats where there are none (see header
        //            comment for AddNecessaryColumns()).
        //            USAS will be part of Enterprise version of TOPL.
        inserts = deletes = updates = 0;
      }
    void resetRowCounts()
      {}
    Int64 getRowCount(NABoolean &isEstimate,
                      Int32 &errorCode,
                      Int32 &breadCrumb,
                      NABoolean estimateIfNecessary = TRUE);
    Int64 getRowCount(NABoolean &isEstimate,
                      Int64 &numInserts,
                      Int64 &numDeletes,
                      Int64 &numUpdates,
                      Int64 &numPartitions,
                      Int64 &minRowCtPerPartition,
                      Int32 &errorCode,
                      Int32 &breadCrumb,
                      NABoolean estimateIfNecessary);
    Lng32 collectFileStatistics() const
      {
        return 0;
      }
    NABoolean isInMemoryObjectDefn() const
      {
        return FALSE;
      }
    Lng32 getTotalVarcharLength() const
      {
        // Used for row count estimation for sq tables. Not needed for HBase.
        return 0;
      }
    Lng32 getBlockSize() const
      {
        //return tableStats_->getEstimatedBlockSize();
        return 100;
      }
    Lng32 setHasSyskeyFlag()
      {
        return 0;
      }

    tblOrigin getTblOrigin() const
    {
      return HBASE_TBL;
    }

  private:
    void GetLabelInfo(labelDetail detail = MIN_INFO)
      {}
    Lng32 DescribeColumnNames();

    Int64 minPartitionRows_;
};


#endif /* HSLA_H */
