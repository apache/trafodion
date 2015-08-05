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
class AnsiOrNskName;
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
    NA_EIDPROC UninitializedMvName();    

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

  void setDefine(short v) { (v ? flags_ |= DEFINE : flags_ &= ~DEFINE); };
  NABoolean isDefine() { return (flags_ & DEFINE) != 0; };

  void setCachedParam(short v) { (v ? flags_ |= CACHED_PARAM : flags_ &= ~CACHED_PARAM); };
  NABoolean isCachedParam() { return (flags_ & CACHED_PARAM) != 0; };

  void setAnsiPhySame(short v) { (v ? flags_ |= ANSI_PHY_SAME : flags_ &= ~ANSI_PHY_SAME); };
  NABoolean isAnsiPhySame() { return (flags_ & ANSI_PHY_SAME) != 0; };

  void setIndex(short v) {(v ? flags_ |= IS_INDEX : flags_ &= ~IS_INDEX); };
  NABoolean isIndex() { return (flags_ & IS_INDEX) != 0; };

  void setView(short v) {(v ? flags_ |= IS_VIEW : flags_ &= ~IS_VIEW); };
  NABoolean isView() { return (flags_ & IS_VIEW) != 0; };

  void setMPalias(short v) {(v ? flags_ |= IS_MPALIAS : flags_ &= ~IS_MPALIAS); };
  NABoolean isMPalias() { return (flags_ & IS_MPALIAS) != 0; };

// Set to avoid similarity check and recompilation
  void setAvoidSimCheck(short v)
  {
    if (v) flags_ |= NAME_ONLY; else flags_ &= ~NAME_ONLY;
  }
  
  // Force skipping the similarity check (while still resolving the
  // physical name.) Used in special statements (e.g., SET TABLE TIMEOUT)
  // when only the table name is needed, not its meta data 
  NABoolean isAvoidSimCheck() { return((flags_ & NAME_ONLY) ? TRUE : FALSE); };

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
  static NABoolean applyMPAliasDefaults(char * invalue, 
					char * outvalue,
					char * defValString);
  
  AnsiOrNskName *getLastUsedName(NAMemory *heap);
  void setCompileTimeName(char *name, NAMemory *heap);
  void setLastUsedName(char *name, NAMemory *heap);
  void setLastUsedName(AnsiOrNskName *name);
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
    DEFINE = 0x0010,          // name is a guardian Define
    IS_VIEW = 0x0020,         // this is a view
    IS_MPALIAS = 0x0040,      // the variable contains an mpalias name.
    NAME_ONLY = 0x0080,        // variable, but only the table-name is needed
                              // (not the table's meta-data.) So avoid simila
                              // check and recompilation. Used by SET TIMEOUT
    COMP_NAME_STR_PTR = 0x0100, //  If not set, compileTimeAnsiName is the embedded string
    CACHED_PARAM = 0x0200 // prototyped hvar for cached tablenames.
  };
  
  enum RunTimeFlags
  {
    ANSI_NAME_CHANGE = 0x0001, IGNORE_TS = 0x0002, VIEW_NAME_CHANGE = 0x0004,
    RESERVED_NAME = 0x0008,
    LASTUSED_NAME_STR_PTR = 0x0010,
    LASTUSED_NAME_CLASS_PTR =0x0020, // If set, LastUsedName points to AnsiOrNskName, else is the
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

  // name of hvar/param/envVar/define used to input the table name.
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

  NABoolean definePresent() { return (flags_ & DEFINE_PRESENT) != 0; };
  void setDefinePresent(short v) 
  { (v ? flags_ |= DEFINE_PRESENT : flags_ &= ~DEFINE_PRESENT); };

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
    DEFINE_PRESENT = 0x0001, // one or more DEFINEs used in query
    VIEW_PRESENT = 0x0002,   // one or more views used in query
    ENVVARS_PRESENT = 0x0004, // one or more envvars as tablenames used
                              // in the query.
    VARIABLE_PRESENT = 0x0008 // one of more tablenames used in the query
                              // are passed in as variables.
                              // (hostvar, defines or envvars)
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


class ResolvedName
{
public:
  ResolvedName() 
  : flags_(0),
    numIndexes_(-1)
  {str_pad(filler_, sizeof(filler_), '\0');};

  char * resolvedGuardianName() { return resolvedGuardianName_; };
  char * resolvedAnsiName() { return resolvedAnsiName_; };
  void setResolvedAnsiName(char *name) { resolvedAnsiName_ = name; };

  Int16 numIndexes() { return numIndexes_; }
  void setNumIndexes(Int16 i) { numIndexes_ = i; }

  void resetFlags() { flags_ = 0; };
  void resetFiller() {str_pad(filler_, sizeof(filler_), '\0'); };
  NABoolean ignoreTS() { return (flags_ & IGNORE_TS) != 0; };
  void setIgnoreTS(short v) 
  { (v ? flags_ |= IGNORE_TS : flags_ &= ~IGNORE_TS); }

  NABoolean validateNumIndexes() { return (flags_ & VALIDATE_NUM_INDEXES) != 0; };
  void setValidateNumIndexes(short v) 
  { (v ? flags_ |= VALIDATE_NUM_INDEXES : flags_ &= ~VALIDATE_NUM_INDEXES); }

private:
  enum 
  { 
    IGNORE_TS = 0x0001,
    VALIDATE_NUM_INDEXES = 0x0002
  };

  ULng32 flags_;
  char resolvedGuardianName_[MAX_PHYSICAL_NAME_LENGTH];
  char *resolvedAnsiName_;

  Int16 numIndexes_;
  char filler_[50];
};

// An list of LateNameInfo_ generated at compiled time.
// All list entries are allocated in a contiguous space
// so they could be accessed quickly.
class ResolvedNameList
{
public:
  ResolvedNameList()
  {str_pad(filler_, sizeof(filler_), '\0');version_=0; };

  ResolvedName &getResolvedName(Int32 i)
  {
    return resolvedName_[i];
  };

  ULng32 &numEntries() { return numEntries_; };

  // returns the length of total info that needs to be sent to compiler
  // at recomp time. This info is used to get to the actual tablename
  // (and not the prototype name) that was specified thru a hvar/param/env
  // var.
  ULng32 getRecompResolvedNameListLen();
  // puts recomp info into 'buffer'. Space is to be allocated by caller.
  void getRecompResolvedNameList(char * buffer);

  void resetFlags()
  { 
    flags_ = 0;
    for (UInt32 i = 0; i < numEntries_; i++)
      {
	resolvedName_[i].resetFlags();
      }
  };
  void resetFiller()
    {
       str_pad(filler_, sizeof(filler_), '\0');
    
    for (UInt32 i = 0; i < numEntries_; i++)
      {
	resolvedName_[i].resetFiller();
      }  
    }
  void translateFromOldVersion(ResolvedNameListPre1800 *newrnl);
  void setVersion(short version) { version_ = version; }
  short getVersion() {return version_;}

private:
  ULng32 flags_;

  ULng32 numEntries_;

  ResolvedName resolvedName_[1];
  short version_;
  char filler_[54]; 
};

// name info needed at automatic recomp time. Sent
// by executor to arkcmp. It uses this to replace the original
// compile time ansi name with the actual runtime ansi name
// when the query is recompiled.
class RecompLateNameInfo
{
public:
  RecompLateNameInfo(){flags_ = 0; str_pad(filler_, sizeof(filler_), '\0');};

  char * varName() { return varName_; };
  char * compileTimeAnsiName() { return compileTimeAnsiName_; };
  char * actualAnsiName() { return actualAnsiName_; };

  void setMPalias(short v) {(v ? flags_ |= IS_MPALIAS : flags_ &= ~IS_MPALIAS); };
  NABoolean isMPalias() { return (flags_ & IS_MPALIAS) != 0; };

private:
  enum RecompLateNameInfoFlags
    { 
    IS_MPALIAS = 0x0001       // the variable contains an mpalias name.
    };

char varName_[50];

// the compile-time ANSI name prototype 
// value for the table name at original compile time.
char compileTimeAnsiName_[ComAnsiNamePart::MAX_ANSI_NAME_EXT_LEN+1];

// actual value of varName_ that was input at runtime.
char actualAnsiName_[ComAnsiNamePart::MAX_ANSI_NAME_EXT_LEN+1];

ULng32 flags_;
char filler_[50];
};

class RecompLateNameInfoList
{
public:
  RecompLateNameInfoList()
    {
       str_pad(filler_, sizeof(filler_), '\0');
       version_=0;
    };

  ULng32 &numEntries() { return numEntries_; };

  RecompLateNameInfo &getRecompLateNameInfo(Int32 i)
  {
    return lateNameInfo_[i];
  };

private:                       
  ULng32 numEntries_;            
  RecompLateNameInfo lateNameInfo_[1];  
  short version_;
  char filler_[50];
};


//------------------------------------------------------------------------------

class RecompLateNameInfoPre1800
{
public:
  RecompLateNameInfoPre1800(){};
  char * varName() { return varName_; };
  char * compileTimeAnsiName() { return compileTimeAnsiName_; };
  char * actualAnsiName() { return actualAnsiName_; };

  void setMPalias(short v) {(v ? flags_ |= IS_MPALIAS : flags_ &= ~IS_MPALIAS); };
  NABoolean isMPalias() { return (flags_ & IS_MPALIAS) != 0; };

private:
  enum RecompLateNameInfoFlags
  { 
    IS_MPALIAS = 0x0001       // the variable contains an mpalias name.
  };

  char varName_[50];

  // the compile-time ANSI name prototype 
  // value for the table name at original compile time.
  char compileTimeAnsiName_[ComAnsiNamePart::MAX_ANSI_NAME_EXT_LEN+1];

  // actual value of varName_ that was input at runtime.
  char actualAnsiName_[ComAnsiNamePart::MAX_ANSI_NAME_EXT_LEN+1];

  ULng32 flags_;
 
};

class RecompLateNameInfoListPre1800
{
public:
  RecompLateNameInfoListPre1800(){};
  ULng32 &numEntries() { return numEntries_; };

  RecompLateNameInfoPre1800 &getRecompLateNameInfo(Int32 i)
  {
    return lateNameInfo_[i];
  };
  void translateFromNewVersion();
private:                   
  ULng32 numEntries_;            
  RecompLateNameInfoPre1800 lateNameInfo_[1];  
};


class ResolvedNamePre1800
{
public:
  ResolvedNamePre1800() 
  : flags_(0),
    numIndexes_(-1)
  {};
  char * resolvedGuardianName() { return resolvedGuardianName_; };
  char * resolvedAnsiName() { return resolvedAnsiName_; };
  void setResolvedAnsiName(char *name) { resolvedAnsiName_ = name; };

  Int16 numIndexes() { return numIndexes_; }
  void setNumIndexes(Int16 i) { numIndexes_ = i; }

  void resetFlags() { flags_ = 0; };
  NABoolean ignoreTS() { return (flags_ & IGNORE_TS) != 0; };
  void setIgnoreTS(short v) 
  { (v ? flags_ |= IGNORE_TS : flags_ &= ~IGNORE_TS); }

  NABoolean validateNumIndexes() { return (flags_ & VALIDATE_NUM_INDEXES) != 0; };
  void setValidateNumIndexes(short v) 
  { (v ? flags_ |= VALIDATE_NUM_INDEXES : flags_ &= ~VALIDATE_NUM_INDEXES); }
private:
  enum 
  { 
    IGNORE_TS = 0x0001,
    VALIDATE_NUM_INDEXES = 0x0002
  };

  ULng32 flags_;
  char resolvedGuardianName_[MAX_PHYSICAL_NAME_LENGTH];
  char *resolvedAnsiName_;

  Int16 numIndexes_;

};


class ResolvedNameListPre1800
{
public:
  ResolvedNameListPre1800()
  {};
  ResolvedNameListPre1800(ResolvedNameList *newrnl, CollHeap *heap);
  ResolvedNamePre1800 &getResolvedName(Int32 i)
  {
    return resolvedName_[i];
  };

  ULng32 &numEntries() { return numEntries_; };

  // returns the length of total info that needs to be sent to compiler
  // at recomp time. This info is used to get to the actual tablename
  // (and not the prototype name) that was specified thru a hvar/param/env
  // var.
  ULng32 getRecompResolvedNameListLen();
  // puts recomp info into 'buffer'. Space is to be allocated by caller.
  void getRecompResolvedNameList(char * buffer);

  void resetFlags()
  { 
    flags_ = 0;
    for (UInt32 i = 0; i < numEntries_; i++)
      {
	resolvedName_[i].resetFlags();
      }
  };

  void translateFromNewVersion(ResolvedNameList *newrnl);
private:
  ULng32 flags_;

  ULng32 numEntries_;

  ResolvedNamePre1800 resolvedName_[1];
 
};
//------------------------------------------------------------------------------



class SimilarityTableInfo : public NAVersionedObject
{
public:
  SimilarityTableInfo();
  ~SimilarityTableInfo();

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

  virtual short getClassSize() { return (short)sizeof(SimilarityTableInfo); }

  Long pack(void * space);
  Lng32 unpack(void *, void * reallocator);

  NABoolean entrySeq() {return ((flags_ & ENTRY_SEQ) != 0);};
  void setEntrySeq(NABoolean v) 
  { (v ? flags_ |= ENTRY_SEQ : flags_ &= ~ENTRY_SEQ); };

  NABoolean audited() {return ((flags_ & AUDITED) != 0);};
  void setAudited(NABoolean v) 
  { (v ? flags_ |= AUDITED : flags_ &= ~AUDITED); };

  NABoolean isPartitioned() {return ((flags_ & PARTITIONED) != 0);};
  void setIsPartitioned(NABoolean v) 
  { (v ? flags_ |= PARTITIONED : flags_ &= ~PARTITIONED); };

  NABoolean noPartitionSimCheck() {return ((flags_ & NO_PARTITION_SIM_CHECK) != 0);};
  void setNoPartitionSimCheck(NABoolean v) 
  { (v ? flags_ |= NO_PARTITION_SIM_CHECK : flags_ &= ~NO_PARTITION_SIM_CHECK); };

  unsigned short &numPartitions() { return numPartitions_; };

  void setPartitioningScheme(const char * scheme)
  {
    str_cpy_all(partitioningScheme_, scheme, 2);
  }
  char * getPartitioningScheme(){return partitioningScheme_;};

private:
  enum Flags
  {
    ENTRY_SEQ = 0x0001,
    AUDITED   = 0x0002,
    PARTITIONED = 0x0004,

    // if set, indicates that partition sim check always passes even if
    // the num of partitions are different. Used in cases where we know
    // that the change in num of partns will not change the plan, for ex,
    // if OLT opt is being used in which case only one partn will be 
    // accessed at runtime.
    NO_PARTITION_SIM_CHECK = 0x0008
  };

  UInt32 flags_;                                                    // 00-03
  UInt16 numPartitions_;                                            // 04-05

  // see common/ComSmallDefs.h for values for this field. 
  // (COM_RANGE_PARTITIONING_LIT...etc).
  char partitioningScheme_[2];                                      // 06-07

  char fillersSimilarityTableInfo_[16];                             // 08-23

};

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for SimilarityTableInfo
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<SimilarityTableInfo> SimilarityTableInfoPtr;

class SimilarityInfo : public NAVersionedObject
{
public:
  SimilarityInfo(NAMemory * heap = NULL);
  ~SimilarityInfo();

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

  virtual short getClassSize()     { return (short)sizeof(SimilarityInfo); }

  Long pack(void * space);
  Lng32 unpack(void *, void * reallocator);
  
  ExpTupleDesc* getTupleDesc() { return tupleDesc_; };
  void setTupleDesc(ExpTupleDesc *td) { tupleDesc_ = td; };

  Queue* getColNameList() { return colNameList_; };
  void setColNameList(Queue *cnl) { colNameList_ = cnl; };
  SimilarityTableInfo* getTableInfo() { return sti_;};
  void setTableInfo(SimilarityTableInfo* sti) { sti_ = sti;};
  //  ArkFsIndexMapArray* getIndexMapArray() { return indexMapArray_; };
  //  void setIndexMapArray(ArkFsIndexMapArray* ima) { indexMapArray_ = ima; };
  //++ MV
  void setMvAttributesBitmap(UInt32 bitmap) {mvAttributesBitmap_ = bitmap;};
  UInt32 getMvAttributesBitmap() const{return mvAttributesBitmap_;};

  void setSimCheck(short s)
  {
    if (s)
      runtimeFlags_ |= DO_SIM_CHK;
    else
      runtimeFlags_ &= ~DO_SIM_CHK;
  }
  
  NABoolean doSimCheck() 
  { 
    return ((runtimeFlags_ & DO_SIM_CHK) ? TRUE : FALSE); 
  };

  void setReResolveName(short s)
  {
    if (s)
      runtimeFlags_ |= RE_RESOLVE_NAME;
    else
      runtimeFlags_ &= ~RE_RESOLVE_NAME;
  }
  
  NABoolean reResolveName() 
  { 
    return ((runtimeFlags_ & RE_RESOLVE_NAME) ? TRUE : FALSE); 
  };

  void resetRuntimeFlags() { runtimeFlags_ = 0;};

  void disableSimCheck() { compiletimeFlags_ |= SIM_CHECK_DISABLE; };
  void enableSimCheck()  { compiletimeFlags_ &= ~SIM_CHECK_DISABLE; };
  NABoolean simCheckDisable() { return ((compiletimeFlags_ & SIM_CHECK_DISABLE) != 0);};

  void setInternalSimCheck() { compiletimeFlags_ |= INTERNAL_SIM_CHECK; };
  NABoolean internalSimCheck() { return ((compiletimeFlags_ & INTERNAL_SIM_CHECK) != 0);};

  void setGetMatchingIndex() { compiletimeFlags_ |= GET_MATCHING_INDEX; };
  NABoolean getMatchingIndex() { return ((compiletimeFlags_ & GET_MATCHING_INDEX) != 0);};

private:
  enum CompiletimeFlags
  {
    SIM_CHECK_DISABLE = 0x0001, INTERNAL_SIM_CHECK = 0x0002,
    GET_MATCHING_INDEX = 0x0004 // skip sim check, get resolved name from
                                // index info list(see QuerySimilarityList)
 
  };

  enum RuntimeFlags
  {
    DO_SIM_CHK = 0x0001,
    RE_RESOLVE_NAME = 0x0002
  };

  UInt32 compiletimeFlags_;                                         // 00-03
  UInt32 runtimeFlags_;                                             // 04-07
  ExpTupleDescPtr tupleDesc_;                                       // 08-15
  // list of column names. Each name is a null-terminated "char *".
  QueuePtr colNameList_;                                            // 24-31

  SimilarityTableInfoPtr sti_;                                      // 32-39
  UInt32 mvAttributesBitmap_;					    // 56-59

  char fillersSimilarityInfo_[36];                                  // 60-95
};

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for SimilarityInfo
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<SimilarityInfo> SimilarityInfoPtr;

class IndexInfo : public NAVersionedObject
{
public:
  IndexInfo()
    {
      flags_ = 0;
    };

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
NA_EIDPROC
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }
  
NA_EIDPROC
  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }
  
NA_EIDPROC
  virtual short getClassSize() { return (short)sizeof(IndexInfo); }

  // Fix for CR 10-010614-3437: Redefined ime_ to be a versioned
  // object pointer, in order to pack and unpack it correctly.
  // Also defined the get and set methods.
  char* indexAnsiName() { return indexAnsiName_;}
  char* indexPhyName() { return indexPhyName_; }

  virtual Long pack(void * space);
  virtual Lng32 unpack(void *, void * reallocator);

  void setSimPartInfo(NABoolean v)
  {
    (v ? flags_ |= SIM_PART_INFO : flags_ &= ~SIM_PART_INFO);
  };
  NABoolean simPartInfo() { return (flags_ & SIM_PART_INFO) != 0; };

  void setPartitioningScheme(const char * scheme)
  {
    str_cpy_all(partitioningScheme_, scheme, 2);
  }
  char * getPartitioningScheme() {return partitioningScheme_;};

  void setNumPartitions(ULng32 numPartitions = 0)
  {
    numPartitions_ = (unsigned short) numPartitions;
  }
  ULng32 getNumPartitions() {return (ULng32) numPartitions_;};

private:
  enum Flags
  {
    // set by genSimilarityInfo if index partition info present
    SIM_PART_INFO = 0x0001
  };

  // See common/ComAnsiNamePart.h. 6 bytes added here for null terminator and
  // filler to make length multiple of 8.
  enum { MAX_ANSI_IDENTIFIER_LEN = 258 + 6 }; 

  char indexAnsiName_[MAX_ANSI_IDENTIFIER_LEN];         // 00-263

  // resolved index name                                  
  char indexPhyName_[56];                               // 264-319
#ifdef NA_64BIT
  // dg64 - 32-bits on disk
  UInt32  flags_;                                 // 328-331
#else
  ULng32 flags_;                                 // 328-331
#endif

  // number of partitions
  unsigned short numPartitions_;                        // 332-333

  // partitioning scheme, see common/comSmallDefs.h
  char partitioningScheme_[2];                          // 334-335

  char filler_[32];                                     // 336-367
};

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for SimilarityInfo
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<IndexInfo> IndexInfoPtr;

class QuerySimilarityInfo : public NAVersionedObject
{
public:
  enum Options { RECOMP_ON_TS_MISMATCH, ERROR_ON_TS_MISMATCH,
		 SIM_CHECK_ON_TS_MISMATCH, INTERNAL_SIM_CHECK,
                 SIM_CHECK_AND_RECOMP_ON_FAILURE, 
		 SIM_CHECK_AND_ERROR_ON_FAILURE
               };
  QuerySimilarityInfo(NAMemory * heap);
  QuerySimilarityInfo();
  ~QuerySimilarityInfo();

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

  virtual short getClassSize() { return (short)sizeof(QuerySimilarityInfo); }

  Queue * siList() { return siList_; };

  Queue * indexInfoList() { return indexInfoList_; };

  void setIndexInfoList(Queue *iil)
  {
    indexInfoList_ = iil;
  }

  // Options &similarityCheckOption() { return option_; };
  Options getSimilarityCheckOption() { return (Options)option_; };
  void setSimilarityCheckOption(Options op) { option_ = op; };

  Int16 &namePosition() { return namePosition_; };

  Long pack(void * space);
  Lng32 unpack(void *, void * reallocator);
  
private:

  NAMemory *heap_;
#ifndef NA_64BIT
  char fillersQuerySimilarityInfo1_[4];
#endif                                                              // 00-07

  // Queue of class SimilarityInfo
  QueuePtr siList_;                                                 // 08-15

  // List of indices that have to be checked for similarity.
  // The table whose indices are to be
  // checked is at position namePosition_ of siList_(member of this
  // class) and LateNameInfoList (member of Root Tdb).
  // Queue of class IndexInfo. 
  QueuePtr indexInfoList_;                                          // 16-23
  Int16 namePosition_;                                              // 24-25

  Int16 option_;                                                    // 26-27

  char fillersQuerySimilarityInfo_[36];                             // 28-55
};

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for QuerySimilarityInfo
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<QuerySimilarityInfo> QuerySimilarityInfoPtr;


class AnsiOrNskName : public NABasicObject
{
public:
  AnsiOrNskName(char *inName);
  
  char *getInternalName() ;
  char *getExternalName() ;
  Int16 extractParts(Lng32 &numParts,
		char *parts[]);
  Int16 equals(AnsiOrNskName *name);
  Int16 convertAnsiOrNskName(bool doCheck = TRUE);
  Int16 fillInMissingParts(char *schemaName);
  bool  isNskName();
  Int16 updateNSKInternalName(char *inName);
  Int16 quoteNSKExtName();
private:

  char  extName_[MAX_ANSI_NAME_LENGTH]; // Given (external) 3-part name
  // The ansi name with quotes stripped out and checked for ANSI name conventions.
  // The maximum length (in bytes) of a UTF8 internal name is almost as long as
  // that of the corresponding external name therefore we make the size of the array
  // for the 3-part interal name the same as that for the 3-part external name.
  char	intName_[MAX_ANSI_NAME_LENGTH];
  Int16 noOfParts_;
  bool  isNskName_; // TRUE if NSK name, FALSE if ansi name
  char  parts_[4][ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+1]; // in UTF8
  bool  isValid_;  // The flag that denotes if the name is checked and extracted into parts
  bool  isError_;  
};


#endif // EX_LATEBIND_H
