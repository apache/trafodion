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
#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         Descriptor.h
 * Description:  SQL Descriptors, used to interface between host programs
 *               and SQL. A descriptor describes one or more host variables
 *               (their data type, precision, character set, etc).
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "Platform.h"
//#include <sys/types.h>
//#include <stdarg.h>

#include "CliDefs.h"
#include "ComSysUtils.h"
#include "ex_god.h"
#include "ComTdbRoot.h"
// -----------------------------------------------------------------------
// Forward references
// -----------------------------------------------------------------------

class ContextCli;
class Attributes;
class CliStatement;

// -----------------------------------------------------------------------
// BulkMoveInfo
// -----------------------------------------------------------------------

class BulkMoveInfo 
{
public:
  friend class Descriptor;

  ULng32 maxEntries() { return maxEntries_; };
  ULng32 usedEntries() { return usedEntries_; };

  // returns info about the i'th entry. Entry num is 0 based.
  NABoolean isExeDataPtr(Int32 i) { return bmiArray_[i].isExePtr(); };
  ULng32 getLength(Int32 i) {return bmiArray_[i].length_;};
  char * getDescDataPtr(Int32 i) {return bmiArray_[i].descDataPtr_;};
  short getExeAtpIndex(Int32 i) {return bmiArray_[i].exeAtpIndex_;};
  Long  getExeOffset(Int32 i) {return bmiArray_[i].exeOffset_;};
  char * getExeDataPtr(Int32 i) {return bmiArray_[i].exeDataPtr_;};
  short getFirstEntryNum(Int32 i) {return bmiArray_[i].firstEntryNum_;};
  short getLastEntryNum(Int32 i) {return bmiArray_[i].lastEntryNum_;};
  NABoolean isVarchar(Int32 i) { return bmiArray_[i].isVarchar();};
  NABoolean isNullable(Int32 i) { return bmiArray_[i].isNullable();};

  void addEntry(ULng32 length, char * descDataPtr,
		short exeAtpIndex, NABoolean exeIsPtr,
		Long exeOffset,short firstEntryNum,short lastEntryNum,
                NABoolean isVarchar, NABoolean isNullable);

private:
  struct BMInfoStruct
  {
    enum {
      IS_EXE_PTR  = 0x0001,
      IS_VARCHAR  = 0x0002,
      IS_NULLABLE = 0x0004
    };

    NABoolean isExePtr() { return (bmiFlags_ & IS_EXE_PTR) != 0; }
    void setIsExePtr(NABoolean v)
    { (v ? bmiFlags_ |= IS_EXE_PTR : bmiFlags_ &= ~IS_EXE_PTR); }

    NABoolean isVarchar() { return (bmiFlags_ & IS_VARCHAR) != 0; }
    void setIsVarchar(NABoolean v)
    { (v ? bmiFlags_ |= IS_VARCHAR : bmiFlags_ &= ~IS_VARCHAR); }

    NABoolean isNullable() { return (bmiFlags_ & IS_NULLABLE) != 0; }
    void setIsNullable(NABoolean v)
    { (v ? bmiFlags_ |= IS_NULLABLE : bmiFlags_ &= ~IS_NULLABLE); }

    ULng32 length_;
    char * descDataPtr_;
    short exeAtpIndex_;
    short bmiFlags_;
    union
    {
      Long   exeOffset_;
      char * exeDataPtr_;
    };
    short firstEntryNum_;
    short lastEntryNum_;
  };

  ULng32 flags_;
  ULng32 maxEntries_;
  ULng32 usedEntries_;

  BMInfoStruct bmiArray_[1];
};

// -----------------------------------------------------------------------
// Descriptor
// -----------------------------------------------------------------------
#pragma warning (disable : 4275)   //warning elimination
class Descriptor : public ExGod {
#pragma warning (default : 4275)   //warning elimination
  enum Flags
  {
  /*************************************************************

     Rowwise Rowset Versions

       There two versions of rowsets, version 1 and version 2. The 
       original version (version 1) of rowwise rowsets does not 
       support all SQL/MX data types and does not support nullable 
       columns. If the input/output params/columns contained any of 
       the following attributes:

	 Varchar type
	 Intervals type
	 Datetime type
	 Nullable
	 Character set was not single byte

       Rowwise rowsets and/or bulk move was disallowed.

       Version 2 of rowwise rowsets removes most of the restrictions 
       imposed by version 1. All data types are supported and nullable 
       columns are allowed. In certain very rare cases bulk move may 
       be disallowed (e.g. interval types) for a given param/column, 
       but this will not disallow bulk move for all other 
       params/columns. Rowwise rowsets will be returned in all cases,
       except if the column is special. A column is special if it is 
       an "added column" (note: I'm not entirely sure what a 
       special column is. As best I can tell from the code, it is as 
       stated, a column that was added after the table was created).

     Rowset Types

       A Descriptor contains information about values used as input or
       output. Each descriptor has a rowset type associated with it. 
       Typically, the rowset type is set by the user. The executor will 
       format data based on the rowset type.

       There are 4 rowset types as defined in SQLCLIdev.h.

         ROWSET_NOT_SPECIFIED 	
         ROWSET_COLUMNWISE    	
         ROWSET_ROWWISE       	
         ROWSET_ROWWISE_V1    
         ROWSET_ROWWISE_V2     

       ROWSET_NOT_SPECIFIED is the default rowset type, and indicates that 
       rowsets are not being used.

       ROWSET_COLUMNWISE instructs the executor to use columnwise format 
       for processing data.

       ROWSET_ROWWISE and ROWSET_ROWWISE_V1 are the same value. If this 
       value is set, then the executor will use the version 1 rowwise rowset 
       logic for processing.

       ROWSET_ROWWISE_V2 instructs the executor to use version 2 rowwise 
       rowset logic for processing.

     Internal Flags

       A "Descriptor" object contains information about values used as input
       or output. The Descriptor has contains an array of "desc_struct" 
       structures, where each element of the array contains information about
       one of the values. The desc_struct data member is named "desc".

       Both the Descriptor object and desc_struct structure contain a data 
       member than is a flag bit map. The Descriptor object flag bit map is 
       named "flags_". The desc_struct structure bit map data member is named
       "desc_flag". Possible values for both the Descriptor flags_ and 
       desc_struct desc_flags bit maps are:

         BULK_MOVE_DISABLED 	      
         BULK_MOVE_SETUP 		      
         DESC_TYPE_WIDE 		     
         DO_SLOW_MOVE 			     
         ROWWISE_ROWSET    		     
         ROWWISE_ROWSET_V1 		      
         ROWWISE_ROWSET_DISABLED 	
         ROWWISE_ROWSET_V2 		      

       All flags are mutually exclusive. That is, setting one flag will not 
       implicitly change the value of another flag.

       The BULK_MOVE_DISABLED flag is used to disable bulk move for all of
       the Descriptor's entries. If this flag is set, then none of the 
       descriptor's entries are bulk moved.

       The BULK_MOVE_SETUP flag is used to indicate that the appropriate 
       actions have been taken to set up for bulk move of some or all of
       the entries in a Descriptor (i.e. InputOutputExpr::setupBulkMoveInfo
       has been called).

       The DESC_TYPE_WIDE flag is used to indicate that the Descriptor is a
       wide descriptor.

       The bulk move logic will disable bulk move (i.e. set the BULK_MOVE_DISABLED flag)
       if the DESC_TYPE_WIDE flag is set.

       The DO_SLOW_MOVE flag is used by the desc_struct to indicate that the value 
       represented by that desc_struct is not bulk moved (i.e. it is slow moved by
       convDoIt). It is also used by the Descriptor to indicate there is a slow
       move value in its list of values. That is, the DO_SLOW_MOVE flag is set in 
       the Descriptor if any of the Descriptor's values has the DO_SLOW_MOVE flag
       set.

       The ROWWISE_ROWSET and ROWWISE_ROWSET_V1 flags are the same value. This flag
       is used by the Descriptor to indicate that the rowset type is version 1. It 
       is set by the user, via the SQL_EXEC_SetDescItem method. It indicates that 
       the user wants the executor to use the original rowwise rowset and bulk move
       logic for processing rowwise rowsets and bulk move. 

       The ROWWISE_ROWSET_V2 flag is used by the Descriptor to indicate that the
       rowset type is version 2. It is set by the user, via the SQL_EXEC_SetDescItem
       method. It indicates that the user wants the executor to use the new rowwise
       rowset and bulk move logic for processing rowwise rowsets and bulk move.

  **************************************************************/

    BULK_MOVE_DISABLED      = 0x0001,
    BULK_MOVE_SETUP         = 0x0002,
    DESC_TYPE_WIDE          = 0x0004, 
    DO_SLOW_MOVE            = 0x0008,
    ROWWISE_ROWSET          = 0x0010,
    ROWWISE_ROWSET_V1       = ROWWISE_ROWSET,
    ROWWISE_ROWSET_DISABLED = 0x0020,
    ROWWISE_ROWSET_V2       = 0x0040

  };
  
  SQLDESC_ID descriptor_id;
  SQLMODULE_ID module;
  short        dyn_alloc_flag;

  Lng32         rowsetSize;      // Number of rows in rowset array.
  void        *rowsetStatusPtr; // Optional. Specifies the status array. It
                                // contains the row status values for each row
                                // after a RowSet fetch. 
                                // Must be size of RowsetSize
                                // Application is responsable for this ptr.
  Lng32         rowsetNumProcessed; // Number of rows processed in the rowset
  Lng32         rowsetHandle;    // A handle/cursor to current row in rowset
                                // A value of "-1" indicates that a rowset
                                // has not been defined.
  Lng32         max_entries;     // Max entries of desc_struct
  Lng32         used_entries;    // Num active entries of desc_struct
  
  ULng32 flags_;

  ULng32 compoundStmtsInfo_; // contains information needed for compound statements

  Lng32         rowwiseRowsetSize;   // Number of rows in rowset array they
                                    // are being passed in stacked rowwise.
  Lng32         rowwiseRowsetRowLen; // length of each row.
  Long          rowwiseRowsetPtr;    // ptr to the start of rowset buffer.
  Lng32         rowwiseRowsetPartnNum; // partition number where this rwrs
                                      // buffer need to go to.
  char filler_[16]; 

  struct desc_struct {
    enum 
    {
      IS_NUMERIC = 0x0001,
      IS_NUMERIC_UNSIGNED = 0x0002,
      DO_ITEM_SLOW_MOVE = 0x0004,
      IS_CASE_INSENSITIVE = 0x0008
    };

    Lng32  rowsetVarLayoutSize;  /* Three cases:
                                 * ZERO: when entry is not participating
                                 *       in a rowset or the same value is
                                 *       used in the rowset (i.e., without
                                 *       an array)
                                 * COLUMN_WISE: Same value as length. There
                                 *       is an array of values of that length
                                 * Row-WISE: value is the size of the structure
                                 *       where this item is contained. There
                                 *       are several item in the structure, 
                                 *       even items outside the rowset.
                                 */    
    Lng32  rowsetIndLayoutSize;  /* Three cases:
                                 * ZERO: when entry is not participating
                                 *       in a rowset or the same value is
                                 *       used in the rowset (i.e., without
                                 *       an array)
                                 * COLUMN_WISE: Same value as ind_length. There
                                 *       is an array of values of that length
                                 * Row-WISE: value is the size of the structure
                                 *       where this item is contained. There
                                 *       are several item in the structure, 
                                 *       even items outside the rowset.
                                 */
    Lng32  datatype;
    Lng32  datetime_code;
    Lng32  length;
    short nullable;
    Lng32 charset;	       // 
    char *charset_schema;      // Internally charset and collation 
    char *charset_catalog;     // is stored as a long. 
    char *charset_name;        // All other charset and collation related 

    Lng32 coll_seq;             // desc items are stored as character strings.
    char *coll_schema;
    char *coll_catalog;
    char *coll_name;

    Lng32  scale;
    Lng32  precision;
    Lng32  int_leading_precision;
    char *output_name;
    short generated_output_name;
    char *heading;
    short string_format;
    char *table_name;
    char *schema_name;
    char *catalog_name;
    Long   var_ptr;             // For Bound. Application allocate memory
                               // If Rowset is equal to 1,
                               // then var_ptr is a pointer to single var.
                               // else var_ptr is a pointer to ptr array of
                               // variables.
    char *var_data;            // For Unbound. Rowsets are not used
    Long   ind_ptr;             // Same convention as var_ptr
    Lng32  ind_data;            // For Unbound. Rowsets are not used
    Lng32  ind_datatype;
    Lng32  ind_length;
    Lng32  vc_ind_length;
    Lng32  aligned_length;      //length + ind_length + vc_ind_length + fillers

    // offsets to values in the actual sql row.
    // Used by caller to align their data so bulkmove could be done.
    Lng32  data_offset;
    Lng32  null_ind_offset;

    ULng32 desc_flags;

    // Vicz: three new descriptor items to support call stmt description
    short parameterMode;
    short ordinalPosition;
    short parameterIndex;

    char *text_format;

#ifdef _DEBUG
    Lng32  rowwise_var_offset;   // testing logic
    Lng32  rowwise_ind_offset;   // testing logic
#endif

  };
  
  desc_struct *desc;

  BulkMoveInfo *bmInfo_;

  // the statement for which bulk move was done from/to this descriptor.
  CliStatement * bulkMoveStmt_;

  ContextCli *context_;        // the context that contains this descriptor

  NABoolean lockedForNoWaitOp_; // true if this descriptor is in use
                                // by a pending no-wait operation

  static void DescItemDefaultsForDatetimeCode(desc_struct & descItem,
                                              Lng32 datetime_interval);
  static void DescItemDefaultsForType(desc_struct & descItem,
                                      Lng32 datatype,
				      NAHeap & heap);
  static Lng32 DefaultPrecision(Lng32 datatype);
  static Lng32 DefaultScale(Lng32 datatype);

  static Lng32 setIndLength(desc_struct &descItem);
  static Lng32 setVarLength(desc_struct &descItem);

  char *getVarItem(desc_struct &descItem, Lng32 idxrow);
  char *getIndItem(desc_struct &descItem, Lng32 idxrow);
  
  RETCODE processNumericDatatype(desc_struct &descItem,
				 Lng32 numeric_value);

  RETCODE processNumericDatatypeWithPrecision(desc_struct &descItem,
					      ComDiagsArea &diags);

  void set_text_format(desc_struct &descItem);

public:
  Descriptor(SQLDESC_ID * descriptor_id_,
	     Lng32 max_entries_,
	     ContextCli *context);
  ~Descriptor();

  NABoolean operator ==(Descriptor &other);

  RETCODE alloc(Lng32 used_entries_);
  
  RETCODE dealloc();

  RETCODE addEntry(Lng32 entry);

  char * getVarData(Lng32 entry);
  char * getVarData(Lng32 entry, Lng32 idxrow);
  Int32 getVarDataLength(Lng32 entry);
  Int32 getVarIndicatorLength(Lng32 entry);
  const char * getVarDataCharSet(Lng32 entry);
  char * getIndData(Lng32 entry);
  char * getIndData(Lng32 entry, Lng32 idxrow);
  Int32 getIndLength(Lng32 entry);
  RETCODE getDescItemMainVarInfo(Lng32 entry,
                                 short &var_isnullable,
                                 Lng32  &var_datatype,
                                 Lng32  &var_length,
                                 void   ** var_ptr,
                                 Lng32  &ind_datatype,
                                 Lng32  &ind_length,
                                 void   ** ind_ptr);

  RETCODE getDescItem(Lng32 entry, Lng32 what_to_get, 
		      void * numeric_value, char * string_value,
		      Lng32 max_string_len, Lng32 * returned_len,
		      Lng32 start_from_offset, 
		      Descriptor* desc_to_get_more_info = 0, 
		      Int32 entry_in_desc_to_get_more_info = 0
                     ) ;
  
  RETCODE getDescItemPtr(Lng32 entry, Lng32 what_to_get,
                         char **string_ptr, Lng32 *returned_len);

  RETCODE setDescItem(Lng32 entry, Lng32 what_to_set, Long numeric_value,
		      char * string_value,
		      Descriptor* desc_to_get_more_info = 0, 
		      Int32 entry_in_desc_to_get_more_info = 0
		      );
 
  RETCODE setDescItemInternal(Lng32 entry, Lng32 what_to_set, 
			      Lng32 numeric_value,
			      char * string_value
			      );
  
  //
  // GetNameViaDesc - retrieve the name of a statement,cursor, or descriptor
  //   from a given descriptor id.  The name is returned in a OBJ_ID 
  // dynamically
  //   allocated from the executor heap.
  //
  static SQLCLI_OBJ_ID* GetNameViaDesc(SQLDESC_ID *desc_id, ContextCli *context, NAHeap &heap);

  inline void setMaxEntryCount(Lng32 max_entries_);
  inline Lng32 getMaxEntryCount();

  Lng32 getUsedEntryCount();

  void setUsedEntryCount(Lng32 used_entries_);

  Lng32 getRowsetSize() 
  { 
    return rowsetSize;
  }

  Lng32 getRowwiseRowsetSize() { return rowwiseRowsetSize; }
  Lng32 getRowwiseRowsetRowLen() { return rowwiseRowsetRowLen; }
  Int64 getRowwiseRowsetPtr() { return rowwiseRowsetPtr; }

  Lng32 getRowsetNumProcessed()
  {
    return rowsetNumProcessed;
  }

  Lng32 getCurrRowOffsetInRowwiseRowset()
  {
    return (rowwiseRowsetRowLen * rowsetNumProcessed);
  }

  Long getCurrRowPtrInRowwiseRowset()
  {
    return (rowwiseRowsetPtr + rowwiseRowsetRowLen * rowsetNumProcessed);
  }

  ULng32 getCompoundStmtsInfo() const;
  
  void setCompoundStmtsInfo(ULng32 info);

  inline NABoolean thereIsACompoundStatement() const 
  {return ((compoundStmtsInfo_ & ComTdbRoot::COMPOUND_STATEMENT_IN_QUERY) != 0);}

  inline void setCompoundStatement() 
  { compoundStmtsInfo_ |= ComTdbRoot::COMPOUND_STATEMENT_IN_QUERY; }

  inline void * getDescHandle();

  inline SQLDESC_ID* getDescName();

  const SQLMODULE_ID *getModuleId() { return descriptor_id.module; };
/*
  inline char *getModuleName();
  inline long getModuleNameLen() 
    { return GET_SQL_MODULE_NAME_LEN_PTR(descriptor_id.module); };
*/
  inline ULng32 getDescFlags();
  inline void setDescFlags(ULng32 f);
  // static helper functions to deal with FS types
  static Lng32 ansiTypeFromFSType(Lng32 datatype);
  static const char * ansiTypeStrFromFSType(Lng32 datatype);
  static Lng32 datetimeIntCodeFromTypePrec(Lng32 datatype, Lng32 precision);
  static short isIntervalFSType(Lng32 datatype);
  static short isCharacterFSType(Lng32 datatype);
  static short isIntegralFSType(Lng32 datatype);
  static short isFloatFSType(Lng32 datatype);
  static short isNumericFSType(Lng32 datatype);
  static short isDatetimeFSType(Lng32 datatype);
  static short isBitFSType(Lng32 datatype);

  inline short dynAllocated();
  ContextCli * getContext()                  { return context_; }

// This function gets char string content from an ASCII CHAR host variable.
  static char *
  getCharDataFromCharHostVar(ComDiagsArea & diags, 
                             NAHeap& heap,
		   	     char * host_var_string_value, 
			     Lng32 host_var_string_value_length,
			     const char* the_SQLDESC_option,
			     Descriptor* info_desc = 0,
                             Int32 info_desc_index = 0,
                             short target_type = -1);

  //////////////////////////////////////////////////
  // Methods to do Bulk Move
  //////////////////////////////////////////////////
  RETCODE allocBulkMoveInfo();
  RETCODE deallocBulkMoveInfo();

  BulkMoveInfo * bulkMoveInfo() { return bmInfo_; };

  NABoolean bulkMoveDisabled();
  void setBulkMoveDisabled(NABoolean v)
    { (v ? flags_ |= BULK_MOVE_DISABLED : flags_ &= ~BULK_MOVE_DISABLED); }
  NABoolean bulkMoveSetup();
  void setBulkMoveSetup(NABoolean v)
    { (v ? flags_ |= BULK_MOVE_SETUP : flags_ &= ~BULK_MOVE_SETUP); }

  void reComputeBulkMoveInfo()
    {
      // set the flags so bulk move info could be recomputed.
      setBulkMoveDisabled(FALSE); // bulk move is no longer disabled
      setBulkMoveSetup(FALSE);    // but needs to be set up.
    }

  enum BulkMoveStatus
    {
      // bulk  move is disallowed for this case. Use convDoIt
      // to move data(slow move) for this entry.
      BULK_MOVE_DISALLOWED,
      
      // bulk move is turned off for all entries.
      // Use convDoIt for everything.
      BULK_MOVE_OFF,

      // bulk move is allowed.
      BULK_MOVE_OK
    };

  //
  // This method will return the proper location in the 
  // descriptor's data buffer.
  //
  Long  getRowsetVarPtr(short entry);


  //
  // This method will call the appropriate checkBulkMoveStatus method
  // depending on the rowwise rowset version currently in use.
  //
  // The methods called from this method also have the side affect of 
  // setting the varPtr parameter to the proper location in the 
  //  descriptor's data buffer for this items bulk move start address.
  //
  // isInputDesc: TRUE, if bulk move is being done for input.
  //              FALSE, if bulkd move is being done for output.
  //
  inline  BulkMoveStatus checkBulkMoveStatus( short       entry
                                            , Attributes *op
                                            , Long     &varPtr
                                            , NABoolean   isInputDesc
                                            , NABoolean   isOdbc
                                            , NABoolean isRWRS
                                            , NABoolean isInternalIntervalFormat
                                            )
  {
  if (rowwiseRowsetV2() == TRUE)
    return checkBulkMoveStatusV2(entry, op, varPtr, isInputDesc, isOdbc, 
				 isRWRS, isInternalIntervalFormat);
  else
    return checkBulkMoveStatusV1(entry, op, varPtr, isInputDesc, 
				 isRWRS, isInternalIntervalFormat);
  };  // end Descriptor::checkBulkMoveStatus

  BulkMoveStatus checkBulkMoveStatusV1(short entry, Attributes * op,
				       Long   &varPtr,
				       NABoolean isInputDesc,
				       NABoolean isRWRS,
				       NABoolean isInternalIntervalFormat);


  BulkMoveStatus checkBulkMoveStatusV2(short entry, Attributes * op,
				       Long   &varPtr,
				       NABoolean isInputDesc,
				       NABoolean isOdbc,
				       NABoolean isRWRS,
				       NABoolean isInternalIntervalFormat);


  // this flag indicates that the entry number 'entry' is to be
  // be moved using 'slow' move method (convDoIt).
  NABoolean doSlowMove(short entry) 
  { 
    desc_struct  &descItem =  desc[entry - 1]; // Zero base
    
    return (descItem.desc_flags & descItem.DO_ITEM_SLOW_MOVE) != 0; 
  }
  void setDoSlowMove(short entry, NABoolean v)
  { 
    desc_struct  &descItem =  desc[entry - 1]; // Zero base
    
    (v ? descItem.desc_flags |= descItem.DO_ITEM_SLOW_MOVE : descItem.desc_flags &= ~descItem.DO_ITEM_SLOW_MOVE); 
  }
  
  NABoolean doSlowMove();

  void setDoSlowMove(NABoolean v)
  { 
    (v ? flags_ |= DO_SLOW_MOVE : flags_ &= ~DO_SLOW_MOVE); 
  }

  
  //
  // Common Rowwise rowset methods.
  //

  // Set rowwise rowset disabled flag
  void setRowwiseRowsetDisabled(NABoolean v)
  { 
    (v ? flags_ |= ROWWISE_ROWSET_DISABLED : flags_ &= ~ROWWISE_ROWSET_DISABLED); 
  }
  
  // True if rowwise rowset disabled flag is on. 
  NABoolean rowwiseRowsetDisabled() 
  { 
    return (flags_ & ROWWISE_ROWSET_DISABLED) != 0; 
  }

  // True if any type of rowwise rowset is on.
  NABoolean rowwiseRowset() 
  { 
    return ((flags_ & ROWWISE_ROWSET_V1) || (flags_ & ROWWISE_ROWSET_V2)) != 0; 
  }

  // True if any type of rowwise rowset is on and rowwise rowsets are not disabled.
  NABoolean rowwiseRowsetEnabled() 
  { 
    return ((rowwiseRowset()) && (NOT rowwiseRowsetDisabled()));
  }

  //
  // Rowwise rowset version 1 methods.
  //

  NABoolean rowwiseRowsetV1() 
  { 
    return (flags_ & ROWWISE_ROWSET_V1) != 0; 
  }
  void setRowwiseRowsetV1(NABoolean v)
  { 
    (v ? flags_ |= ROWWISE_ROWSET_V1 : flags_ &= ~ROWWISE_ROWSET_V1); 
  }

  NABoolean rowwiseRowsetV1Enabled() 
  { 
    return ((rowwiseRowsetV1()) && (NOT rowwiseRowsetDisabled()));
  }

  //
  // Rowwise rowset version 2 methods.
  //

  NABoolean rowwiseRowsetV2() 
  { 
    return (flags_ & ROWWISE_ROWSET_V2) != 0; 
  }
  void setRowwiseRowsetV2(NABoolean v)
  { 
    (v ? flags_ |= ROWWISE_ROWSET_V2 : flags_ &= ~ROWWISE_ROWSET_V2); 
  }

  NABoolean rowwiseRowsetV2Enabled() 
  { 
    return ((rowwiseRowsetV2()) && (NOT rowwiseRowsetDisabled()));
  }

  CliStatement* &bulkMoveStmt() { return bulkMoveStmt_; }

  NABoolean isDescTypeWide();
  void setDescTypeWide(NABoolean v)
    { (v ? flags_ |= DESC_TYPE_WIDE : flags_ &= ~DESC_TYPE_WIDE); }

  // Methods to mark and unmark Descriptor as in use for a pending
  // no-wait operation

  // returns SUCCESS if Descriptor is not already locked; ERROR
  // otherwise (caller should generate diagnostic if appropriate)
  RETCODE lockForNoWaitOp(void);

  // returns SUCCESS if Descriptor is locked; ERROR otherwise
  // (caller should generate diagnostic if appropriate)
  RETCODE unlockForNoWaitOp(void);

  // returns true if Descriptor is locked, false if unlocked;
  // does not change lock state
  NABoolean lockedForNoWaitOp(void)
    { return  lockedForNoWaitOp_; };

};

inline void * Descriptor::getDescHandle()
{
  return descriptor_id.handle;
}

inline SQLDESC_ID* Descriptor::getDescName()
{
  return &descriptor_id;
}

/*
inline char *Descriptor::getModuleName()
{
  return descriptor_id.module->module_name;
}
*/

/* returns -1 if this descriptor was allocated by a call to AllocDesc */
inline short Descriptor::dynAllocated(){
  return dyn_alloc_flag;
}


inline Lng32 Descriptor::getMaxEntryCount(){
  return max_entries;
}

inline void Descriptor::setMaxEntryCount(Lng32 max_entries_){
  max_entries = max_entries_;
}
inline ULng32 Descriptor::getDescFlags(){return flags_;}
inline void Descriptor::setDescFlags(ULng32 f){flags_ = f;}

void stripBlanks(char * buf, Lng32& len);

void upperCase(char * buf);

#endif
