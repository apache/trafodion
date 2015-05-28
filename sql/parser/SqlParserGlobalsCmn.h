/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef SQLPARSERGLOBALSCMN_H
#define SQLPARSERGLOBALSCMN_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlParserGlobalsCmn.h
 *
 * Description:  Sql Parser and Lexer globals that must reside in common.lib
 *               rather than parser.lib 
 *               (so that DLLs which don't use parser.lib will link)
 *
 *****************************************************************************
 */

#undef GLOB_
#undef INIT_
#ifdef SQLPARSERGLOBALSCMN__INITIALIZE          // common/NAString.cpp does this
  #define GLOB_
  #define INIT_(val)    = val
#else
  #define GLOB_         extern
  #define INIT_(val)
#endif

#if defined(SQLPARSERGLOBALS_FLAGS) || defined(SQLPARSERGLOBALSCMN__INITIALIZE)
  #include "BaseTypes.h"

  // Changes to these values must be reflected in regression test
  // settings of env var "SQLPARSER_FLAGS", used by parser.cpp.
enum SqlParser_Flags_Enum {

    // IPC-copiable flags

    // ## We could define bits here to help implement a SQL Flagger
    // ## (Ansi 4.34 and 23.3/23.4), e.g.
    //      DISALLOW_FULL_LEVEL_SQL, DISALLOW_INTERMEDIATE_LEVEL_SQL

    ALLOW_SPECIALTABLETYPE        =        0x1,   // used by SqlParser.y
    ALLOW_FUNNY_IDENTIFIER        =        0x2,   // used by NAString.C
    ALLOW_FUNNY_TABLE_CREATE      =        0x4,   // CatExecCreateTable.C
    ALLOW_ADD_CONSTRAINT_NOT_DROPPABLE =   0x8,   // BindStmtDDL.C

    // QSTUFF
    // this allows old and new values to be specified for
    // update with return values
    ALLOW_OLD_AND_NEW_KEYWORD     =       0x10,
    // QSTUFF

    // Allows users to be registered even when they don't exist
    DISABLE_EXTERNAL_USERNAME_CHECK = 0x40,

    // set in sqlcomp/parser.cpp Parser::parseSQL and used in
    // parser/SqlParserAux.cpp literalOfNumericPassingScale to
    // allow generator & binder to specify arbitrary precision
    // for exact numeric literals 
    ALLOW_ARB_PRECISION_LITERALS  =      0x100,
    ALLOW_UNKNOWN_CHARSET         = 0x200,  // used by SqlParser.y for charsets


    // Indicate that an error should not be returned
    // if volatile schema name has been explicitely specified
    // as part of the table name.
    // Used when internal queries prepared since they may
    // already be fullyqualified.
    ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME = 0x8000,

    // set if volatile schema usage is to be disabled.
    // For example, when operations (alter, create view)
    // are not supported on volatile objects and name lookup
    // should not look for them in the volatile schema
    DISABLE_VOLATILE_SCHEMA       = 0x10000,

    // set, if a query is internally issued from master executor as
    // part of an ExeUtil operator.
    INTERNAL_QUERY_FROM_EXEUTIL   = 0x20000,


    DISABLE_RUNTIME_STATS         = 0x80000,


    // allows cqds that are set in ndcs DSN to be processed. These
    // are also called setOnce cqds (see sqlcomp/nadefaults.cpp).
    // These cqds cannot be set by users during a regular session.
    // If this flag is not set, these cqds cannot be set.
    // (exception is when defaults are internally sent during mxcmp
    //  recreation after an abend, or to the secondary mxcmp).
    ALLOW_SET_ONCE_DEFAULTS       = 0x400000,


    // causes errors to be given for any constructs not allowed in the context
    // of the Where clause of an incremental update stats (IUS) statement.
    PARSING_IUS_WHERE_CLAUSE      = 0x1000000,

    // special mode_special_4 mode. Used to enable specialized and non-ansi
    // functionality.
    // See cqd mode_special_4.
    IN_MODE_SPECIAL_4                           = 0x2000000,

    // The bits of the flag word are divided into copiable and uncopiable portions.
    // See the comments for function receiveAndSetUp() in arkcmp/cmpconnection.cpp.
    // Enum values above are copiable, those below are uncopiable.
    IPC_COPIABLE_MASK             = 0x0FFFFFFF,   // 28 lowest flag bits
    IPC_UNCOPIABLE_MASK           = 0xF0000000,   // 4 highest flag bits

    // IPC-uncopiable flags
    DELAYED_RESET                 = 0x10000000    // used by cmpmain.cpp


};

  GLOB_ THREAD_P ULng32            SqlParser_Flags                INIT_(0);

  inline static ULng32 Get_SqlParser_Flags(ULng32 flagbits)
  { return SqlParser_Flags & flagbits; }

  inline static void Set_SqlParser_Flags(ULng32 flagbits)
  {
    if (flagbits)
      SqlParser_Flags |= flagbits;
    else
      SqlParser_Flags = 0;
  }

  inline static void Assign_SqlParser_Flags(ULng32 flagbits)
  {
    SqlParser_Flags = flagbits;
  }

 inline static void Reset_SqlParser_Flags(ULng32 flagbits)
  {
    if (flagbits)
      SqlParser_Flags &= ~flagbits;
    else
      SqlParser_Flags = 0;
  }
#endif

#if defined(SQLPARSERGLOBALS_NADEFAULTS) || defined(SQLPARSERGLOBALSCMN__INITIALIZE)
  #include "charinfo.h"
  #include "ComMPLoc.h"
  #include "NADefaults.h"
  #include "ObjectNames.h"

  // This set of globals provides:
  // a) global access to compiler defaults in yyparse() and in ../common code;
  // b) faster access -- because these are in optimal formats, rather than
  //    pure NADefaults strings or floats -- to the defaults
  //    in other parts of the compiler and catman.
  //
  // See SchemaDB.h for why the default Ansi cat+schema do not appear here!
  //
  class SqlParser_NADefaults {
    public:
      SqlParser_NADefaults()
        : MPLOC_()
        , MPLOC_as_SchemaName_()
	, NAMETYPE_(DF_ANSI)
	, NATIONAL_CHARSET_(CharInfo::UNICODE)
	, DEFAULT_CHARSET_(CharInfo::ISO88591)
	, ORIG_DEFAULT_CHARSET_(CharInfo::ISO88591)
	, ISO_MAPPING_(CharInfo::ISO88591)
	{}

    ComMPLoc		MPLOC_;
    SchemaName	MPLOC_as_SchemaName_;
    DefaultToken	NAMETYPE_;
    CharInfo::CharSet	NATIONAL_CHARSET_;
    CharInfo::CharSet DEFAULT_CHARSET_;
    CharInfo::CharSet ORIG_DEFAULT_CHARSET_;
    CharInfo::CharSet ISO_MAPPING_;
  };

  #define SqlParser_MPLOC					\
  	  SqlParser_NADefaults_Glob->MPLOC_
  #define SqlParser_MPLOC_as_SchemaName				\
  	  SqlParser_NADefaults_Glob->MPLOC_as_SchemaName_
  #define SqlParser_NAMETYPE					\
  	  SqlParser_NADefaults_Glob->NAMETYPE_
  #define SqlParser_NATIONAL_CHARSET				\
  	  SqlParser_NADefaults_Glob->NATIONAL_CHARSET_
  #define SqlParser_DEFAULT_CHARSET				\
          SqlParser_NADefaults_Glob->DEFAULT_CHARSET_
  #define SqlParser_ORIG_DEFAULT_CHARSET			\
          SqlParser_NADefaults_Glob->ORIG_DEFAULT_CHARSET_
  #define SqlParser_ISO_MAPPING				\
          SqlParser_NADefaults_Glob->ISO_MAPPING_

  GLOB_ THREAD_P const SqlParser_NADefaults *SqlParser_NADefaults_Glob	INIT_(NULL);

  inline NABoolean SqlParser_Initialized()
  { return SqlParser_NADefaults_Glob != NULL; }


  #if defined(SQLPARSERGLOBALS_NADEFAULTS_SET)
    // Dangerous -- casting away constness and modifying the global --
    // caller must be sure to save and restore!

    inline DefaultToken SetSqlParser_NAMETYPE(DefaultToken t)
    { DefaultToken r = SqlParser_NAMETYPE;
      ((SqlParser_NADefaults *)SqlParser_NADefaults_Glob)->NAMETYPE_ = t;
      return r;
    }

    inline void SetSqlParser_DEFAULT_CHARSET(CharInfo::CharSet cs)
    { 
      ((SqlParser_NADefaults *)SqlParser_NADefaults_Glob)->DEFAULT_CHARSET_ = cs;
    }


  #endif // SQLPARSERGLOBALS_NADEFAULTS_SET

#endif // SQLPARSERGLOBALS_NADEFAULTS


#undef GLOB_
#undef INIT_

#endif // SQLPARSERGLOBALSCMN_H
