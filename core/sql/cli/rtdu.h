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
#ifndef RTDU_H
#define RTDU_H
        
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         rtdu.h
 * Description:  Contains description of the object module.
 *               
 *               
 * Created:      10/15/94
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "NAVersionedObject.h"
#include "Int64.h"
#include "ExpError.h"

//-----------------------------------------------------------------------------
//
// An SQL module consists of a header and several areas. The header
// locates tha areas. An area is a contiguous string of bytes and it can
// appear anywhere.
//
// The SQL module is created or replaced by the SQL compiler and contains:
//
//        (1) Header.
//        (2) PLT (Procedure Location Table) area
//        (3) SQL source area.
//        (4) CONTROL area.
//        (5) Name Map area.
//        (6) DLT (Descriptor Location Table) area
//        (7) Descriptor area.
//        (8) SQL object area.
//
//  The layout of a SQL module and the contents of the areas are discussed
//  below.
//
//  The term "offset" is often used. Offsets in the HEADER refer
//  to the offset in the module of other areas relative to the start
//  of the module header.
//  Offsets in the different areas refer to offset WITHIN that area.
//
//  (1) A HEADER area, describing global properties of the SQL module
//      and anchoring the areas described below.
//
//  (2) A PLT (Procedure Location Table) which is an area containing
//      entries for all procedures in the module.
//
//  (3) A SOURCE area which is an area containing the source SQL statements
//      for all the procedures.
//
//  (4) A CONTROL area which is an area containing the source SQL compiler
//      CONTROL options for all the procedures. Used during auto recomp.
//
//  (5) A NAME-MAP area which is an area containing name mapping information
//      (e.g. default volume, DEFINEs, etc.) for all the procedures.
//
//  (6) A DLT (Descriptor Location Table) which is an area containing
//      entries for all static descriptor allocated in the module.
//
//  (7) A DESCRIPTOR area which is an area containing the actual input and
//      output descriptors.
//
//  (8) An OBJECT area which is an area containing generated SQL object
//      code for all the procedures.
//
//
//      Following is a picture of a SQL module having 3 Procedures
//       and 2 descriptors:
//
//         Procedures 1 and 3 are CURSOR statements
//         Procedure 1 and 2 have statement names.
//
//                       +============================================+
//   MODULE HEADER       |                                            |
//                       +============================================+
//   PLT AREA            | PLT (Procedure Location Table)             |
//                       |  PLT Header                                |
//                       |  Entry for Procedure 1                     |
//                       |   Offset to source 1                       |
//                       |   Offset to object 1                       |
//                       |   Offset of CURSOR name.                   |
//                       |   Offset of STATEMENT name.                |
//                       |  Entry for Procedure 2                     |
//                       |   Offset to source 2                       |
//                       |   Offset to object 2                       |
//                       |   Offset of CURSOR name = -1 (no cursor)   |
//                       |   Offset of STATEMENT name.                |
//                       |  Entry for Procedure 3                     |
//                       |   Offset to source 3                       |
//                       |   Offset to object 3                       |
//                       |   Offset of CURSOR name.                   |
//                       |   Offset of STATEMENT name = -1 (none)     |
//                       |  CURSOR Name for Procedure 1               |
//                       |  STATEMENT Name for Procedure 1            |
//                       |  STATEMENT Name for Procedure 2            |
//                       |  CURSOR Name for Procedure 3               |
//                       +============================================+
//   SQL SOURCE AREA     | SQL STATEMENTS                             |
//                       |  source statement 1                        |
//                       |  source statement 2                        |
//                       |  source statement 3                        |
//                       +============================================+
//   CONTROL AREA        |                                            |
//                       +============================================+
//   NAME-MAP AREA       |                                            |
//                       +============================================+
//   DLT AREA            | DLT (Descriptor Location Table)            |
//                       |  DLT Header                                |
//                       |  Entry for Descriptor 1                    |
//                       |   Offset to descriptor 1                   |
//                       |   Offset of descriptor name.               |
//                       |  Entry for Descriptor 2                    |
//                       |   Offset to descriptor 2                   |
//                       |   Offset of descriptor name.               |
//                       |  Descriptor Name for descriptor 1          |
//                       |  Descriptor Name for descriptor 2          |
//                       +============================================+
//   DESCRIPTOR AREA     |                                            |
//                       +============================================+
//   SQL OBJECT AREA     | OBJECTS FOR SQL STATEMENT                  |
//                       |  object 1                                  |
//                       |  object 2                                  |
//                       |  object 3                                  |
//                       +============================================+
//-----------------------------------------------------------------------------

//=============================================================================
//      The SQL module header MODULE_HEADER_STRUCT is stored at the beginning
//  of the module. It contains global information and anchors the other areas
//  of the SQL module. It is loaded by the EXECUTOR when first referenced. All
//  offsets stored in the header of a module are relative to the beginning of
//  the module itself.
//=============================================================================

class module_header_struct;
class plt_entry_struct;
class plt_header_struct;
class dlt_entry_struct;
class dlt_header_struct;
class desc_entry_struct;
class desc_header_struct;

//--------------------------------------------
// SQL module header.
//--------------------------------------------
const Int32 module_header_name_length = 512;  // 3-part ANSI name @128 bytes each
                                            // plus some dots and some slack

// of original 127
const Int32 module_header_filler_at_end = 125; // update this as you add fields
const Int32 plt_entry_filler_at_end = 14; 

#define EYE_CATCHER_LEN         4
#define MODULE_EYE_CATCHER      "MOD "
#define PLT_EYE_CATCHER         "PLT "
#define DLT_EYE_CATCHER         "DLT "

// Assuming that the module header version doesn't change between R1.8 and R2.0 ...
#define R18_MODULE_HEADER_VERSION       3
#define R15_MODULE_HEADER_VERSION	2
#define R1_MODULE_HEADER_VERSION	1

enum { NULL_RTDU_OFFSET = -1, MAX_RTDU_NAME_LEN = 34 };

// SourceArea should be made Unicode (when we have time)
typedef char        SourceChar;
typedef SourceChar* SourceBuf;

class module_header_struct : public NAVersionedObject {
public:
  Int64 prep_timestamp;         // julian time stamp of preprocessing  00-07
  Int64 compile_timestamp;      // julian time stamp of SQL compile    08-15

  char  eye_catcher[EYE_CATCHER_LEN];  // set to "MOD "                16-19
  Int32 module_length;          // length of the module                20-23
  Int32 flags;                  // various status flags                24-27

  Int32 num_areas;              // number of areas (excluding header)  28-31
                                //  i.e. number of following offsets
                                //  to such areas. presently = 8

  Int32 plt_area_length;        // length of plt area                  32-35
  Int32 plt_area_offset;        // offset to plt area                  36-39
  Int32 plt_hdr_length;         // length of plt header                40-43
  Int32 plt_entry_length;       // length of one plt entry             44-47

  Int32 source_area_length;     // length of sql source area           48-51
  Int32 source_area_offset;     // offset to sql source area           52-55

  Int32 recomp_control_area_length; // length of control info area     56-59
  Int32 recomp_control_area_offset; // offset to control info area     60-63

  Int32 schema_names_area_length; // length of cat/sch name area       64-67
  Int32 schema_names_area_offset; // offset to cat/sch name area       68-71

  Int32 dlt_area_length;        // length of dlt area                  72-75
  Int32 dlt_area_offset;        // offset to dlt area                  76-79
  Int32 dlt_hdr_length;         // length of dlt header                80-83
  Int32 dlt_entry_length;       // length of one dlt entry             84-87

  Int32 descriptor_area_length; // length of descriptor area           88-91
  Int32 descriptor_area_offset; // offset to descriptor area           92-95
  Int32 desc_hdr_length;        // length of descriptor header         96-99
  Int32 desc_entry_length;      // length of one descriptor entry     100-103

  Int32 object_area_length;     // length of object area              104-107
  Int32 object_area_offset;     // offset to object area.             108-111
                                                                      
  char  module_name[module_header_name_length]; //                    112-623
                                // fully qualified name of object file
                                //  at time of sql static compilation.
                                // blanks before sql compilation.
                                // Right now only 34 bytes are used!!

  UInt32 version_;              // module version, as defined in 
                                // ComVersionPublic.h                 624-627
                                // room for future growth

  Int32 vproc_area_length;      // length of vproc area               628-631
  Int32 vproc_area_offset;      // offset of vproc area               632-635

  Int32 filler_[module_header_filler_at_end]; //                      636-879

  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // the module_header_struct's size and data member layout (except 
  // filler_ and its replacements) must not change across compatible
  // versions. otherwise, ContextCli::addmodule() would not be able to
  // correctly read the binary modules written by StaticCompiler.cpp
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                                                                       
  module_header_struct() : NAVersionedObject(-1)
  {
    initialize();
  }

  void initialize();

  virtual unsigned char getClassVersionID() { return R18_MODULE_HEADER_VERSION;}
  virtual short getClassSize()      { return (short)sizeof(*this); }
  virtual void populateImageVersionIDArray()
  {
  setImageVersionID(0,getClassVersionID());
  }

  unsigned char getVersion()
  {
     return getImageVersionID(0);
  }

  ULng32 getModuleVersion() const
  {
     return version_;
  }
  
  void setModuleVersion(const ULng32 value)
  {
     version_ = value;
  }


  Lng32 RtduStructIsCorrupt()
  {
    if (str_cmp(eye_catcher, MODULE_EYE_CATCHER, EYE_CATCHER_LEN) != 0 ||
	module_length <= 0 ||
	num_areas <= 0 ||
	plt_area_length < 0)
      return -CLI_MODULEFILE_CORRUPTED;		// diags code, it is corrupt
    return 0;					// no error, not corrupt
  }

  // drivePack and write this structure out to file fd
  Lng32 write(Int32 fd);
};


//----------------------------------------------------------------------------
// The PLT (Procedure location table) of a module is stored within the plt area
//  of a module file.
// It consists of:
//  (1) a fixed length header described by plt_header_struct.
//  (2) a variable length array of procedure entries, an entry being described
//      by plt_entry_struct.
//  (3) a variable length array containing cursor names and statement names.
//      Each name is a varchar string of maximum length 34( cm^sim^name^len ).
//      Names are stored packed, i.e. trailing blanks have been stripped.
//      hence a name may be on an odd byte boundary.
//
// Each entry describes and locates a "procedure", where a procedure logically
//  consists of 4 parts:
//   (a) a sql source statement.
//   (b) a cursor name and/or a statement name.
//   (c) a corresponding sql object.
//   (d) optional input and output descriptor.
// Note that some of the parts of a procedure may be missing.
//
// All offsets are set to a negative value (= -1) when they are invalid.
//
// Offsets to statements and objects of procedures are computed relative
//  to the beginning of the source area and object area resp.
//
// Offsets to cursor/statement names are computed relative to the end
//  of the PLT itself, i.e. the offset of the first name is = 0.
//
//-----------------------------------------------------------------------------


//------------------------------------------------------------
// body of template for a plt entry
//------------------------------------------------------------
class plt_entry_struct : public NAVersionedObject {
public:
  plt_entry_struct()
  : NAVersionedObject(-1)
  { }

  //-------------------------------------------------------------------------
  // Byte offset of statement name for a procedure.
  // Offsets are measured from the end of the PLT.
  // Names are stored in varchar format, and they are packed, i.e. their
  //  offsets may be odd numbers.
  // Note that it could hold NAString * sometime, see arkcmp/cmp_templ.cpp
  //-------------------------------------------------------------------------
  Long stmtname_offset;   //                                      00-03
  
  //-------------------------------------------------------------------------
  // Byte offset of cursor name for a procedure.
  // Set to -1 if no cursor name is associated with the statement.
  // Offsets are measured from the end of the PLT.
  // Names are stored in varchar format, and they are packed, i.e. their
  //  offsets may be odd numbers.
  // Note that it could hold NAString * sometime, see arkcmp/cmp_templ.cpp
  //-------------------------------------------------------------------------
  Long curname_offset;    //                                      04-07

  //--------------------------------------------------
  // description of sql source statement for section
  //--------------------------------------------------
  Int32 source_offset;     // offset of sql statement from         08-11
                           //  beginning of source area.
  Int32 source_length;     // length of item pointed by source_off 12-15

  //----------------------------------------------------------------
  // description of the cat.sch name used when compiling this stmt.
  // Used at recomp time.
  //----------------------------------------------------------------
  Int32 schema_name_offset;  // offset of schema name from           16-19
                             //  beginning of schema_names area.
  Int32 schema_name_length;  // length of item pointed by sch_nm_off 20-23

  //--------------------------------------------------------
  // description of sql object for procedure.
  // a section object is:
  //  generated code for a dml statement.
  //  the unmodified source statement for a ddl statement.
  //--------------------------------------------------------
  Int32 gen_object_offset; // offset of sql object from beginning  24-27
                           //  of object area.
  Int32 gen_object_length; // byte length of sql object produced   28-31
                           //  by sql compiler.

  Int32 input_desc_entry;  // entry into DLT describing the        32-35
                           // input descriptor. 0 based.

  Int32 output_desc_entry; // entry into DLT describing the        36-39
                           // output descriptor. 0 based.
 
  //---------------------------------------------------------------------------
  // Offset to name-map
  //---------------------------------------------------------------------------
  Int32 name_map_offset;   //                                      40-43

  //------------------------------------
  // status flags.
  //------------------------------------
  Int32 status;            //                                      44-47

  // if the statement entry points to a cursor this indicates whether
  // the cursor is holdable..or not
  Int32 holdable;         //                                       48-51

  Int32 statementIndex;   //                                       52-55

  //--------------------------------------------------------------------------
  // Offset and length of control information(CQD, CT, CQS) for this stmt.
  // Offset relative to start of recomp_control_area_offset pointed to
  // by module header.
  // Offset points to class RecompControlInfo created at static compile time.
  // Used at auto recomp time to ship back to mxcmp.
  //--------------------------------------------------------------------------
  Int32 recomp_control_info_offset;                            // 56-59
  Int32 recomp_control_info_length;                            // 60-63

  Int32 filler_[plt_entry_filler_at_end]; //                       64-91

  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // the plt_entry_struct's size and data member layout (except 
  // filler_ and its replacements) must not change across compatible
  // versions. otherwise, ContextCli::addmodule() would not be able to
  // correctly read the binary modules written by StaticCompiler.cpp
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  // get this entry's statement or cursor name and its length
  inline Int32 getNameAndLen
  (module_header_struct &mHdr, plt_header_struct &pHdr, char *&name);

  virtual unsigned char getClassVersionID() { return 1; }
  virtual short getClassSize()      { return (short)sizeof(*this); }
  virtual void populateImageVersionIDArray()
  {
  setImageVersionID(0,getClassVersionID());
  }

  NABoolean autoRecomp()      { return (status & AUTO_RECOMP)    != 0; }
  void setAutoRecomp(NABoolean v)      
           { (v ? status |= AUTO_RECOMP : status &= ~AUTO_RECOMP); }

  NABoolean recompWarn()      { return (status & RECOMP_WARN)    != 0; }
  void setRecompWarn(NABoolean v)      
           { (v ? status |= RECOMP_WARN : status &= ~RECOMP_WARN); }
  
  NABoolean nametypeNsk()      { return (status & NAMETYPE_NSK)    != 0; }
  void setNametypeNsk(NABoolean v)      
           { (v ? status |= NAMETYPE_NSK : status &= ~NAMETYPE_NSK); }

  NABoolean odbcProcess()      { return (status & ODBC_PROCESS)    != 0; }
  void setOdbcProcess(NABoolean v)      
           { (v ? status |= ODBC_PROCESS : status &= ~ODBC_PROCESS); }


private:
  enum StatusFlags {
    AUTO_RECOMP = 0x0001,
    RECOMP_WARN = 0x0002,
    NAMETYPE_NSK = 0x0004,
    ODBC_PROCESS = 0x0008
  };
};
 
//----------------------------------------------------------------------------
// Structure describing the header of a PLT (Procedure Location Table).
//----------------------------------------------------------------------------
class plt_header_struct : public NAVersionedObject {
public:
  plt_header_struct()
  :  NAVersionedObject(-1)
  { }

  char             eye_catcher[EYE_CATCHER_LEN]; // set to "PLT "   00-03
  Int32            plt_len;        // Total length of the PLT area. 04-07
  Int32            num_procedures; // Number of procedure entries   08-11
  Int32            name_len;       // total byte length of          12-15
                                   // cursor/statement name area

  char filler_[16];                                             //  16-23

  //plt_entry_struct plte[1];      // (num_procedures - 1) entries following

  plt_entry_struct *getPLTEntry(Int32 zbi, Int32 entryLen) {
    // this address arithmetic must reflect the way StaticCompiler.cpp
    // writes and Context.cpp's addModule() reads the plt_area
    assert(0 <= zbi && zbi < num_procedures); 
    return (plt_entry_struct*)((char*)this + sizeof(*this) + zbi*entryLen);
  }

  char *getStmtNameStart(module_header_struct &hdr) {
    // this address arithmetic must reflect the way StaticCompiler.cpp
    // writes and Context.cpp's addModule() reads the plt_area
    return (((char*)(this)) + hdr.plt_hdr_length 
            + num_procedures * hdr.plt_entry_length);
  }

  virtual unsigned char getClassVersionID() { return 1; }
  virtual short getClassSize()      { return (short)sizeof(*this); }
  virtual void populateImageVersionIDArray()
  {
  setImageVersionID(0,getClassVersionID());
  }

  Lng32 RtduStructIsCorrupt()
  {
  if (str_cmp(eye_catcher, PLT_EYE_CATCHER, EYE_CATCHER_LEN) != 0 ||
      plt_len <= 0)
    return -CLI_MODULEFILE_CORRUPTED;		// diags code, it is corrupt
  return 0;					// no error, not corrupt
  }

  // drivePack and write this structure out to file fd
  Lng32 write(Int32 fd);
};

//----------------------------------------------------------------------------
// The DLT (Descriptor location table) of a module is stored within the dlt
//  area of a module file.
// It consists of:
//  (1) a fixed length header described by dlt_header_struct.
//  (2) a variable length array of dlt entries, an entry being described
//      by dlt_entry_struct.
//  (3) a variable length array containing descriptor names.
//      Each name is a varchar string of maximum length 34( ?? ).
//      Names are stored packed, i.e. trailing blanks have been stripped.
//      hence a name may be on an odd byte boundary.
//
// Each entry describes a "descriptor", where a descriptor logically
//  consists of 2 parts:
//
//   (a) a descriptor name
//   (b) a corresponding descriptor
//
// Offsets to descriptor is computed relative to the beginning of the
//  descriptor area.
//
// Offsets to descriptor names are computed relative to the end
//  of the DLT itself, i.e. the offset of the first name is = 0.
//
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------
// struct describing an input or output descriptor
// All *_offset fields in this struct are relative to the end of all
// entries for this desc_header_struct.
//---------------------------------------------------------------------------
class desc_entry_struct : public NAVersionedObject {
public:
  Int32 datatype;              // 00-03
  Int32 length;                // 04-07
  Int32 charset_offset;        // 08-11
  Int32 coll_seq_offset;       // 12-15
  Int32 scale;                 // 16-19
  Int32 precision;             // 20-23
  Int32 fractional_precision;  // 24-27
  Int32 output_name_offset;    // 28-31
  Int32 heading_offset;        // 32-35
  Int32 var_ptr;               // 36-39
  Int32 var_data;              // 40-43
  Int32 ind_ptr;               // 44-47
  Int32 ind_data;              // 48-51
  Int32 datatype_ind;          // 52-55
  Int32 rowsetSize;            // 56-59

  Int16 nullable;              // 60-61
  Int16 generated_output_name; // 62-63
  Int16 string_format;         // 64-65



  char  reservedForSPJ_[6];    // 66-71

  char  filler_[30];            // 72-101

  desc_entry_struct()
  :  NAVersionedObject(-1)
  {
    Lng32 lenDescEntryStruct = ((char *)filler_ - (char *)&datatype);
    memset (&datatype, 0, lenDescEntryStruct);
    memset (filler_, 0, 30);
  }

  virtual unsigned char getClassVersionID() { return 1; }
  virtual short getClassSize()      { return (short)sizeof(*this); }
  virtual void populateImageVersionIDArray()
  {
  setImageVersionID(0,getClassVersionID());
  }
};

class desc_header_struct : public NAVersionedObject {
public:
  desc_header_struct()
  :  NAVersionedObject(-1)
  { }

  Int32             num_entries;   // 00-03
  char              filler_[4];    // 04-07
  //desc_entry_struct desc_entry[1];// followed by (num_entries - 1) entries
                                   // of desc_entry_struct

  desc_entry_struct *getDescEntry(Int32 zbi, Int32 entryLen) {
    // this address arithmetic must reflect the way StaticCompiler.cpp
    // writes and Context.cpp's addModule() reads DescSeq/SimpleDesc area
    assert(0 <= zbi && zbi < num_entries); 
    return (desc_entry_struct*)((char*)this + sizeof(*this) + zbi*entryLen);
  }

  virtual unsigned char getClassVersionID() { return 1; }
  virtual short getClassSize()      { return (short)sizeof(*this); }
  virtual void populateImageVersionIDArray()
  {
  setImageVersionID(0,getClassVersionID());
  }
};

//------------------------------------------------------------
// body of template for a dlt entry
//------------------------------------------------------------
class dlt_entry_struct : public NAVersionedObject {
public:
  dlt_entry_struct()
  : NAVersionedObject(-1)
  { }

  //-------------------------------------------------------------------------
  // Byte offset of descriptor name.
  // Offsets are measured from the end of the DLT.
  // Names are stored in varchar format, and they are packed, i.e. their
  //  offsets may be odd numbers.
  //-------------------------------------------------------------------------
#ifdef NA_64BIT
  // dg64 - 32-bits on disk
  Int32  descname_offset;   //                                00-03
#else
  Lng32 descname_offset;   //                                00-03
#endif

  //-------------------------------------------------------------------------
  // Byte offset of statement name for which this descriptor is defined.
  // Offsets are measured from the end of the DLT.
  // Names are stored in varchar format, and they are packed, i.e. their
  //  offsets may be odd numbers.
  //-------------------------------------------------------------------------
#ifdef NA_64BIT
  // dg64 - 32-bits on disk
  Int32  stmtname_offset;   //                                04-07
#else
  Lng32 stmtname_offset;   //                                04-07
#endif

  //--------------------------------------------------
  // description of descriptor
  //--------------------------------------------------
#ifdef NA_64BIT
  // dg64 - 32-bits on disk
  Int32  descriptor_offset; // offset of descriptor from the  08-11
#else
  Lng32 descriptor_offset; // offset of descriptor from the  08-11
#endif
                          //  beginning of descriptor area.
                          // Points to desc_header_struct.

  enum DESC_TYPE {
    INPUT_DESC_TYPE, OUTPUT_DESC_TYPE
  };
  Int16 /*DESC_TYPE*/ desc_type;  //                        12-13
  char                filler_[26]; //                       14-39
  
  virtual unsigned char getClassVersionID() { return 1; }
  virtual short getClassSize()      { return (short)sizeof(*this); }
  virtual void populateImageVersionIDArray()
  {
  setImageVersionID(0,getClassVersionID());
  }
};
 
//----------------------------------------------------------------------------
// Structure describing the header of a DLT (Descriptor Location Table).
//----------------------------------------------------------------------------
class dlt_header_struct : public NAVersionedObject {
public:
  char             eye_catcher[EYE_CATCHER_LEN]; // set to "DLT "   00-03
  Int32            dlt_len;         //                              04-07
  Int32            num_descriptors; // Number of descriptor entries 08-11
  Int32            name_len;        // Total byte length of         12-15
                                    // descriptor name area.

  char filler_[16];                                             //  16-23

  // dlt_entry_struct dlte[1];      // (num_descriptors - 1) entries following
                                    // this struct.

  dlt_entry_struct *getDLTEntry(Int32 zbi, Int32 entryLen) {
    assert(0 <= zbi && zbi < num_descriptors); 
    // this address arithmetic must reflect the way StaticCompiler.cpp
    // writes and Context.cpp's addModule() reads the dlt_area
    return (dlt_entry_struct*)((char*)this + sizeof(*this) + zbi*entryLen);
  }

  virtual unsigned char getClassVersionID() { return 1; }
  virtual short getClassSize()      { return (short)sizeof(*this); }

  Lng32 RtduStructIsCorrupt()
  {
  if (str_cmp(eye_catcher, DLT_EYE_CATCHER, EYE_CATCHER_LEN) != 0 ||
      num_descriptors <= 0 ||
      dlt_len <= 0)
    return -CLI_MODULEFILE_CORRUPTED;		// diags code, it is corrupt
  return 0;					// no error, not corrupt
  }

  virtual void populateImageVersionIDArray()
  {
  setImageVersionID(0,getClassVersionID());
  }
};

//--------------------------------------------------------------------------
// class RtduRecompControlInfo
//
//   This class contains all information related to control options.
//   It is created at static compile time and is shipped back as is
//   during auto recompilation by cli to mxcmp.
//
// cqdInfo_:  contains packed currentDefaults_ 
//            (see sqlcomp/nadefaults.cpp)
// ctoInfo_:  contains packed control table options 
//            (see optimizer/ControlDB.cpp)
// cqsInfo_:  contains the shape in effect for this stmt.
//
//--------------------------------------------------------------------------
class RtduRecompControlInfo : public NAVersionedObject
{
public:
  RtduRecompControlInfo()
       :  NAVersionedObject(-1),
	  numCqdInfoEntries_(0),
	  cqdInfo_(NULL), ctoInfo_(NULL), cqsInfo_(NULL),
	  cqdInfoLength_(0), ctoInfoLength_(0), cqsInfoLength_(0),
	  flags_(0)
  {
  };

  virtual unsigned char getClassVersionID()       { return 1; }
  virtual void populateImageVersionIDArray()
  { setImageVersionID(0,getClassVersionID()); }

  void initialize(Lng32 numCqdInfoEntries,
		  char * cqdInfo, Lng32 cqdInfoLength,
		  char * ctoInfo, Lng32 ctoInfoLength,
		  char * cqsInfo, Lng32 cqsInfoLength)
  {
    flags_ = 0;
    numCqdInfoEntries_ = numCqdInfoEntries;
    cqdInfo_ = cqdInfo;
    cqdInfoLength_ = cqdInfoLength;
    ctoInfo_ = ctoInfo;
    ctoInfoLength_ = ctoInfoLength;
    cqsInfo_ = cqsInfo;
    cqsInfoLength_ = cqsInfoLength;

    memset(filler_, 0, 48);
  };

  Lng32 numCqdInfoEntries() { return numCqdInfoEntries_;};
  char * cqdInfo() { return cqdInfo_;}
  Lng32   cqdInfoLength() { return cqdInfoLength_;};
  char * ctoInfo() { return ctoInfo_;}
  Lng32   ctoInfoLength() { return ctoInfoLength_;};
  char * cqsInfo() { return cqsInfo_;}
  Lng32   cqsInfoLength() { return cqsInfoLength_;};

  Lng32 packedLength()
  {
    Lng32 size = sizeof(RtduRecompControlInfo);
    size += cqdInfoLength_ + ctoInfoLength_ + cqsInfoLength_;

    return size;
  };

  void packIt(char * basePtr)
  {
    if (cqdInfoLength_ > 0)
      {
	cqdInfo_ = (char *)((char *)cqdInfo_ - (char *)basePtr);
      }

    if (ctoInfoLength_ > 0)
      {
	ctoInfo_ = (char *)((char *)ctoInfo_ - (char *)basePtr);
      }

    if (cqsInfoLength_ > 0)
      {
	cqsInfo_ = (char *)((char *)cqsInfo_ - (char *)basePtr);
      }
  };

  void unpackIt(char * basePtr)
  {
    if (cqdInfoLength_ > 0)
      {
	cqdInfo_ = (Long)cqdInfo_ + (char *)basePtr;
      }

    if (ctoInfoLength_ > 0)
      {
	ctoInfo_ = (Long)ctoInfo_ + (char *)basePtr;
      }

    if (cqsInfoLength_ > 0)
      {
	cqsInfo_ = (Long)cqsInfo_ + (char *)basePtr;
      }
  };

private:
#ifdef NA_64BIT
  // dg64 - 32-bits on disk
  Int32    numCqdInfoEntries_;
#else
  Lng32   numCqdInfoEntries_;
#endif
  char * cqdInfo_;    // information related to CQD
#ifdef NA_64BIT
  // dg64 - 32-bits on disk
  Int32    cqdInfoLength_;
#else
  Lng32   cqdInfoLength_;
#endif

  char * ctoInfo_;    // information related to Control Table Options
#ifdef NA_64BIT
  // dg64 - 32-bits on disk
  Int32    ctoInfoLength_;
#else
  Lng32   ctoInfoLength_;
#endif

  char * cqsInfo_;    // the Control Query Shape in use for this stmt.
#ifdef NA_64BIT
  // dg64 - 32-bits on disk
  Int32    cqsInfoLength_;
#else
  Lng32   cqsInfoLength_;
#endif

  UInt32 flags_;

  char filler_[48];   // of original 48 filler bytes
};


// get this entry's statement or cursor name and its length
inline Int32 plt_entry_struct::getNameAndLen
(module_header_struct &mHdr, plt_header_struct &pHdr, char *&name)
{
  const Int32 LEN = 4; Int32 length=0;
  char *start = pHdr.getStmtNameStart(mHdr);
  
  if (stmtname_offset >= 0) {
    str_cpy_all((char *)&length, (start + stmtname_offset), LEN);
    name = start + stmtname_offset + LEN;
  }
  else if (curname_offset >= 0) {
    str_cpy_all((char *)&length, (start + curname_offset), LEN);
    name = start + curname_offset + LEN;
  }
  return length;
}

inline static void initializeVersionFields(plt_header_struct *dest)
{
  plt_header_struct src;
  if (dest) memcpy(dest, &src, sizeof(NAVersionedObject));
}

inline static void initializeVersionFields(plt_entry_struct *dest)
{
  plt_entry_struct src;
  if (dest) memcpy(dest, &src, sizeof(NAVersionedObject));
}

inline static void initializeVersionFields(dlt_header_struct *dest)
{
  dlt_header_struct src;
  if (dest) memcpy(dest, &src, sizeof(NAVersionedObject));
}

inline static void initializeVersionFields(dlt_entry_struct *dest)
{
  dlt_entry_struct src;
  if (dest) memcpy(dest, &src, sizeof(NAVersionedObject));
}


inline static void initializeVersionFields(desc_header_struct *dest) 
{
  desc_header_struct src;
  if (dest) memcpy(dest, &src, sizeof(NAVersionedObject));
}


inline static void initializeVersionFields(desc_entry_struct *dest)
{
  desc_entry_struct src;
  if (dest) 
  {
    memcpy(dest, &src, sizeof(NAVersionedObject));
  }
}


#endif
