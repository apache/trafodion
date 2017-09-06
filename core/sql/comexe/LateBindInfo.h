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
/* -*-C++-*-
****************************************************************************
*
* File:         LateBindInfo.h
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef LATEBINDINFO_H
#define LATEBINDINFO_H

#include "NAStdlib.h"			// memset()
#include "NABoolean.h"
#include "ComSizeDefs.h"
#include "ComSmallDefs.h"
#include "NAVersionedObject.h"
#include "exp_tuple_desc.h"
#include "ComQueue.h"
#include "ComAnsiNamePart.h"
#include "Collections.h"

class Queue;
class NAMemory;
class ExpTupleDesc;
class AnsiName;
class ResolvedNameListPre1800;

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for ArkFsKeyClass
// ---------------------------------------------------------------------
//////////////////////////////////////////////////////////////////
//    Class: UninitializedMvName
//    Description:
//      The purpose of this class is to keep track of all 
//      uninitialized mv names found during compile time. 
//                               
//////////////////////////////////////////////////////////////////

#define MAX_PHYSICAL_NAME_LENGTH     ComMAX_EXTERNAL_GUARDIAN_FNAME_LEN+3
#define MAX_ANSI_NAME_LENGTH         ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+1
class UninitializedMvName : public NABasicObject
{
public:       
    UninitializedMvName();    

    char * getPhysicalName() { return physicalName_; }
    char * getAnsiName() { return ansiName_; }


    void setPhysicalName( const char *physicalName );
    void setAnsiName( const char *ansiName );
    
private:
    char physicalName_[MAX_PHYSICAL_NAME_LENGTH];      
    char ansiName_[MAX_ANSI_NAME_LENGTH];         
};
typedef LIST(UninitializedMvName*) UninitializedMvNameList;
typedef NABasicPtrTempl<UninitializedMvName> UninitializedMvNamePtr;
//////////////////////////////////////////////////////////////////
// The purpose of this class is to keep track of
// late name resolution information generated at compile-time,
// and used that at runtime to compute the physical gua name.
// Late name resolution is done at fixup time of root master
// executor node. At that time, field resolvedGuaName_ is
// filled with the physical gua name.
// Field resolvedGuaName_ is allocated in this class
// so we can ship this class to ESPs without packing/unpacking.
// ESPs need this so the PA nodes there
// can get to this field at runtime.
// All other pointer fields are only used in the
// master executor and need not be shipped anywhere and could
// be allocated from heap.
////////////////////////////////////////////////////////////////
class LateNameInfo : public NAVersionedObject
{
public:
  LateNameInfo()
    : NAVersionedObject(-1)
    {
      flags_ = 0;
      runtimeFlags_ = 0;
      varName_[0] = 0;
      compileTimeAnsiName_[0] = 0;
      lastUsedAnsiName_[0] = 0;
      resolvedPhyName_[0] = 0;
    };

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(LateNameInfo); }
  
  // ComAnsiNameSpace &nameSpace() { return nameSpace_; };
  void setNameSpace(ComAnsiNameSpace ns) { nameSpace_ = ns; };
  ComAnsiNameSpace getNameSpace() { return (ComAnsiNameSpace)nameSpace_; };

  // short &inputListIndex() { return inputListIndex_; };
  short getInputListIndex() { return inputListIndex_; };
  void setInputListIndex(short ilix) { inputListIndex_ = ilix; };

  Int32 getCachedParamOffset() { return cachedParamOffset_; }
  void setCachedParamOffset(Int32 o) { cachedParamOffset_ = o; }

  char * variableName()         { return varName_; };
  char * compileTimeAnsiName();  
  char * lastUsedAnsiName();  
  char * resolvedPhyName()      { return resolvedPhyName_; };
  void   zeroLastUsedAnsiName(); 
  char * lastUsedExtAnsiName();

  void setVariable(short v) {(v ? flags_ |= VARIABLE : flags_ &= ~VARIABLE); };
  NABoolean isVariable() { return (flags_ & VARIABLE) != 0; };

  void setEnvVar(short v) { (v ? flags_ |= ENV_VAR : flags_ &= ~ENV_VAR); };
  NABoolean isEnvVar() { return (flags_ & ENV_VAR) != 0; };

  void setCachedParam(short v) { (v ? flags_ |= CACHED_PARAM : flags_ &= ~CACHED_PARAM); };
  NABoolean isCachedParam() { return (flags_ & CACHED_PARAM) != 0; };

  void setAnsiPhySame(short v) { (v ? flags_ |= ANSI_PHY_SAME : flags_ &= ~ANSI_PHY_SAME); };
  NABoolean isAnsiPhySame() { return (flags_ & ANSI_PHY_SAME) != 0; };

  void setIndex(short v) {(v ? flags_ |= IS_INDEX : flags_ &= ~IS_INDEX); };
  NABoolean isIndex() { return (flags_ & IS_INDEX) != 0; };

  void setView(short v) {(v ? flags_ |= IS_VIEW : flags_ &= ~IS_VIEW); };
  NABoolean isView() { return (flags_ & IS_VIEW) != 0; };

  void setAnsiNameChange(short v) { (v ? runtimeFlags_ |= ANSI_NAME_CHANGE : runtimeFlags_ &= ~ANSI_NAME_CHANGE); };
  NABoolean isAnsiNameChange() { return (runtimeFlags_ & ANSI_NAME_CHANGE) != 0; };

  void setViewNameChange(short v) { (v ? runtimeFlags_ |= VIEW_NAME_CHANGE : runtimeFlags_ &= ~VIEW_NAME_CHANGE); };
  NABoolean isViewNameChange() { return (runtimeFlags_ & VIEW_NAME_CHANGE) != 0; };

  void setIgnoreTS(short v) { (v ? runtimeFlags_ |= IGNORE_TS : runtimeFlags_ &= ~IGNORE_TS); };
  NABoolean ignoreTS() { return (runtimeFlags_ & IGNORE_TS) != 0; };

  void setReservedName(short v) { (v ? runtimeFlags_ |= RESERVED_NAME : runtimeFlags_ &= ~RESERVED_NAME); };
  NABoolean reservedName() { return (runtimeFlags_ & RESERVED_NAME) != 0; };

  void resetRuntimeFlags();

  NABoolean isLastUsedNameCompEmbedded() { return (runtimeFlags_ & LASTUSED_NAME_STR_PTR) == 0; };

  static NABoolean makeSQLIdentifier(char * invalue, char * outvalue);
  
  AnsiName *getLastUsedName(NAMemory *heap);
  void setCompileTimeName(char *name, NAMemory *heap);
  void setLastUsedName(char *name, NAMemory *heap);
  void setLastUsedName(AnsiName *name);
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  
  static NABoolean extractParts
  (const char * inName,  // IN: inName separated by "."s
   char * outBuffer,  // IN/OUT: space where parts will be moved.
                      // Must be allocated by caller
   Lng32 &numParts,    // OUT: number of parts extracted
   char * parts[],    // IN/OUT: array entries initialized to parts on return
   NABoolean dQuote); // IN: if TRUE, parts are double quoted.

private:
  void setCompileNameCompMode(short v) 
  { (v ? flags_ |= COMP_NAME_STR_PTR :flags_ &= ~COMP_NAME_STR_PTR); };
  
  NABoolean isCompileNameCompEmbedded() { return (flags_ & COMP_NAME_STR_PTR) == 0; };

  void setLastUsedNameMode(short v) 
  {
    if (v)
    {
      runtimeFlags_ |= LASTUSED_NAME_CLASS_PTR;
      runtimeFlags_ &= ~LASTUSED_NAME_STR_PTR;
    }
    else
	runtimeFlags_ &= ~LASTUSED_NAME_CLASS_PTR; 
  };
  NABoolean isLastUsedNameEmbedded() { return (runtimeFlags_ & LASTUSED_NAME_CLASS_PTR) == 0; };

  void setLastUsedNameCompMode(short v) 
  {
    if (v)
    {
      runtimeFlags_ |= LASTUSED_NAME_STR_PTR;
      runtimeFlags_ &= ~LASTUSED_NAME_CLASS_PTR;
    }
    else
      runtimeFlags_ &= ~LASTUSED_NAME_STR_PTR; 
  };

  enum { MAX_ANSI_IDENTIFIER_LEN = 258 }; // See common/ComAnsiNamePart.h
  enum CompileTimeFlags
  { 
    VARIABLE = 0x0001,        // name is a hvar/param/envVar
    ENV_VAR = 0x0002,         // name is an envVar
    ANSI_PHY_SAME = 0x0004,   // name is a variable but the runtime value of
                              // variable is the resolved physical name.
                              // Used when the variable name is the
                              // name of a resource fork.
    IS_INDEX = 0x0008,        // this is an index.
    IS_VIEW = 0x0020,         // this is a view
    COMP_NAME_STR_PTR = 0x0100, //  If not set, compileTimeAnsiName is the embedded string
    CACHED_PARAM = 0x0200 // prototyped hvar for cached tablenames.
  };
  
  enum RunTimeFlags
  {
    ANSI_NAME_CHANGE = 0x0001, IGNORE_TS = 0x0002, VIEW_NAME_CHANGE = 0x0004,
    RESERVED_NAME = 0x0008,
    LASTUSED_NAME_STR_PTR = 0x0010,
    LASTUSED_NAME_CLASS_PTR =0x0020, // If set, LastUsedName points to AnsiName, else is the
				// character string embedded in LateBindInfo
  };

  UInt32 flags_;                                                  //  00- 03
  UInt32 runtimeFlags_;                                           //  04- 07

  // Is this a base table or index being resolved? ComAnsiNameSpace (see
  // ComSmallDefs.h)
  //
  Int32 nameSpace_;                                               //  08- 11
  
  // index into the input hvar/param descriptors
  // where the table name host variable or param is.
  // Valid if this is a variable name and not an env var.
  Int16 inputListIndex_;                                          //  12- 13

  // name of hvar/param/envVar used to input the table name.
  char varName_[MAX_ANSI_IDENTIFIER_LEN+1/*null terminator*/];    //  14-272
 
  // the ANSI name as known at compile time. Could be:
  //  -- the specified table identifier, if name is not a variable.
  //  -- the value of env var, if name is an env var.
  //  -- the prototype value, if name is a hostvar/param.
  char compileTimeAnsiName_[MAX_ANSI_IDENTIFIER_LEN+1/*null terminator*/];
                                                                  // 273-531

  // the corresponding guardian (physical) name for
  // runtimeAnsiValue_(next field). This field is filled in at
  // compile time initially. It could change at runtime if the
  // physical name corresponding to the lastUsedAnsiName changes.
  // Could happen: if the variable ANSI name changes, or the physical
  // name changes as a result of drop and recreate of table/view with
  // the same ansi name.
  char resolvedPhyName_[MAX_PHYSICAL_NAME_LENGTH];                // 532-581

  // Ansi name we used last. Intially set at compile time to 
  // compileTimeAnsiName_. Changed at runtime for variable name
  // depending on the 'current' ansi name.
  char lastUsedAnsiName_[MAX_ANSI_IDENTIFIER_LEN+1];              // 582-840

  char filler1_[3];                                               // 841-843
  Int32  cachedParamOffset_;                                      // 844-847
  char fillersLateNameInfo_[102];                                 // 848-949

};


// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for LateNameInfo
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<LateNameInfo> LateNameInfoPtr;

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for LateNameInfoPtr
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrArrayTempl<LateNameInfoPtr> LateNameInfoPtrArray;

// ---------------------------------------------------------------------
// An array of LateNameInfo_ generated at compiled time.
// ---------------------------------------------------------------------
class LateNameInfoList : public NAVersionedObject
{
public:
  LateNameInfoList()
    : NAVersionedObject(-1)
  { flags_ = 0; numEntries_ = 0; };

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()  { return (short)sizeof(LateNameInfoList); }

  virtual Long pack(void *);
  virtual Lng32 unpack(void *, void * reallocator);

  void allocateList(Space *space, Lng32 numEntries)
  {
    numEntries_ = numEntries;
    lateNameInfo_.allocatePtrArray(space,numEntries);
  }

  LateNameInfo &getLateNameInfo(Int32 i)
  {
    return *(lateNameInfo_[i]);
  };

  void setLateNameInfo(Int32 i, LateNameInfo *lni)
  {
    lateNameInfo_[i] = lni;
  }

  // unsigned long &numEntries() { return numEntries_; };
  ULng32 getNumEntries() { return numEntries_; };
  void setNumEntries(ULng32 num) { numEntries_ = num; };

  // returns the length of total info that needs to be sent to compiler
  // at recomp time. This info is used to get to the actual tablename
  // (and not the prototype name) that was specified thru a hvar/param/env
  // var.
  ULng32 getRecompLateNameInfoListLen();
  // puts recomp info into 'buffer'. Space is to be allocated by caller.
  void getRecompLateNameInfoList(char * buffer);
  ULng32 getRecompLateNameInfoListLenPre1800();

  // puts recomp info into 'buffer'. Space is to be allocated by caller.
  void getRecompLateNameInfoListPre1800(char * buffer);
  void resetRuntimeFlags();

  NABoolean viewPresent() { return (flags_ & VIEW_PRESENT) != 0; };
  void setViewPresent(short v) 
  { (v ? flags_ |= VIEW_PRESENT : flags_ &= ~VIEW_PRESENT); };

  NABoolean envvarsPresent() { return (flags_ & ENVVARS_PRESENT) != 0; };
  void setEnvvarsPresent(short v) 
  { (v ? flags_ |= ENVVARS_PRESENT : flags_ &= ~ENVVARS_PRESENT); };

 NABoolean variablePresent() { return (flags_ & VARIABLE_PRESENT) != 0; };
  void setVariablePresent(short v) 
  { (v ? flags_ |= VARIABLE_PRESENT : flags_ &= ~VARIABLE_PRESENT); };


private:
  enum Flags
  { 
    VIEW_PRESENT = 0x0002,   // one or more views used in query
    ENVVARS_PRESENT = 0x0004, // one or more envvars as tablenames used
                              // in the query.
    VARIABLE_PRESENT = 0x0008 // one of more tablenames used in the query
                              // are passed in as variables.
                              // (hostvar, envvars)
  };
 
  UInt32 flags_;                                                //   00-  03
  UInt32 numEntries_;                                           //   04-  07
  LateNameInfoPtrArray lateNameInfo_;                           //   08-  15
  char fillersLateNameInfoList_[16];                            //   16-  31
};

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for LateNameInfoList
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<LateNameInfoList> LateNameInfoListPtr;

class AnsiName : public NABasicObject
{
public:
  AnsiName(char *inName);
  
  char *getInternalName() ;
  char *getExternalName() ;
  Int16 extractParts(Lng32 &numParts,
		char *parts[]);
  Int16 equals(AnsiName *name);
  Int16 convertAnsiName(bool doCheck = TRUE);
  Int16 fillInMissingParts(char *schemaName);
private:

  char  extName_[MAX_ANSI_NAME_LENGTH]; // Given (external) 3-part name
  // The ansi name with quotes stripped out and checked for ANSI name conventions.
  // The maximum length (in bytes) of a UTF8 internal name is almost as long as
  // that of the corresponding external name therefore we make the size of the array
  // for the 3-part interal name the same as that for the 3-part external name.
  char	intName_[MAX_ANSI_NAME_LENGTH];
  Int16 noOfParts_;
  char  parts_[4][ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+1]; // in UTF8
  bool  isValid_;  // The flag that denotes if the name is checked and extracted into parts
  bool  isError_;  
};

///////////////////////////////////////////////////////////
// class TrafSimilarityTableInfo
///////////////////////////////////////////////////////////
class TrafSimilarityTableInfo : public NAVersionedObject
{
public:
  TrafSimilarityTableInfo(char * tableName,
                          NABoolean isHive,
                          char * hdfsRootDir, 
                          Int64 modTS, Int32 numPartnLevels,
                          Queue * hdfsDirsToCheck,
                          char * hdfsHostName,
                          Int32 hdfsPort);

  TrafSimilarityTableInfo();
  ~TrafSimilarityTableInfo();

  NABoolean operator==(TrafSimilarityTableInfo &o);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize() { return (short)sizeof(TrafSimilarityTableInfo); }

  Long pack(void * space);
  Lng32 unpack(void *, void * reallocator);

  Int64 modTS() { return modTS_; }
  Int32 numPartnLevels() { return numPartnLevels_; }

  char * tableName() { return tableName_; }
  char * hdfsRootDir() { return hdfsRootDir_; }
  Queue * hdfsDirsToCheck() { return hdfsDirsToCheck_; }

  char * hdfsHostName() { return hdfsHostName_; }
  Int32 hdfsPort() { return hdfsPort_; }

  NABoolean isHive() {return ((flags_ & HIVE) != 0);};
  void setIsHive(NABoolean v) 
  { (v ? flags_ |= HIVE : flags_ &= ~HIVE); };

private:
  enum Flags
  {
    HIVE                    = 0x0001
  };

  Int64 modTS_;
  Int32 numPartnLevels_;
  UInt32 flags_;                                                    

  NABasicPtr tableName_;
  NABasicPtr hdfsRootDir_;
  QueuePtr hdfsDirsToCheck_;

  NABasicPtr hdfsHostName_;
  Int32 hdfsPort_;

  char fillers_[12];                            

};
typedef NAVersionedObjectPtrTempl<TrafSimilarityTableInfo> TrafSimilarityTableInfoPtr;

///////////////////////////////////////////////////////////////////
// class TrafQuerySimilarityInfo
///////////////////////////////////////////////////////////////////
class TrafQuerySimilarityInfo : public NAVersionedObject
{
public:
  TrafQuerySimilarityInfo(Queue * siList);
  TrafQuerySimilarityInfo();
  ~TrafQuerySimilarityInfo();

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize() { return (short)sizeof(TrafQuerySimilarityInfo); }

  Queue * siList() { return siList_; };

  Long pack(void * space);
  Lng32 unpack(void *, void * reallocator);
  
  NABoolean disableSimCheck() 
  {return ((flags_ & DISABLE_SIM_CHECK) != 0);};
  void setDisableSimCheck(NABoolean v) 
  {(v ? flags_ |= DISABLE_SIM_CHECK : flags_ &= ~DISABLE_SIM_CHECK);};
  
  NABoolean disableAutoRecomp() 
  {return ((flags_ & DISABLE_AUTO_RECOMP) != 0);};
  void setDisableAutoRecomp(NABoolean v) 
  {(v ? flags_ |= DISABLE_AUTO_RECOMP : flags_ &= ~DISABLE_AUTO_RECOMP);};
  
private:
  enum Flags
    {
      DISABLE_SIM_CHECK       = 0x0002,
      DISABLE_AUTO_RECOMP     = 0x0004
    };

  // Queue of class TrafSimilarityTableInfo
  QueuePtr siList_;                                                 // 00-07

  Int16 option_;                                                    // 08-09

  Int16 flags_;                                                     // 10-11

  char fillersQuerySimilarityInfo_[36];                             // 12-47
};

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for TrafQuerySimilarityInfo
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<TrafQuerySimilarityInfo> TrafQuerySimilarityInfoPtr;


#endif // EX_LATEBIND_H
