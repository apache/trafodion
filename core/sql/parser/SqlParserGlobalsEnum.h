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

#ifndef SQLPARSERGLOBALSENUM_H
#define SQLPARSERGLOBALSENUM_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlParserGlobalsEnum.h
 *
 * Description:  Contains the enum SqlParser_Flags_Enum
 *
 *****************************************************************************
 */


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

#endif // SQLPARSERGLOBALSENUM_H
