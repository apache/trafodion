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
//
**********************************************************************/
#ifndef SQLCIPARSEGLOBALS_H
#define SQLCIPARSEGLOBALS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciParseGlobals.h		 05/16/97
 * Description:	 Sqlci parser and lexer globals
 * RCS:          $Id: SqlciParseGlobals.h,v 1.1.18.1 1998/07/08 21:48:16  Exp $
 *****************************************************************************
 */

// Globals for the Sqlci Parser, in sqlci/
// -- as distinct from the SqlParser, in parser/SqlParserGlobals.h

#undef GLOB_
#undef INIT_
#ifdef SQLCIPARSEGLOBALS__INITIALIZE
  #define GLOB_
  #define INIT_(val)	= val
#else
  #define GLOB_		extern
  #define INIT_(val)
#endif

  void  SqlciLexReinit();
  Int32   sqlciparse();

  class SqlciNode;	// Forward refs to keep #include dependencies minimal
  class SqlciEnv; // "

  GLOB_ UInt32		 SqlciParse_InputPos		INIT_(0);
  GLOB_ char			*SqlciParse_InputStr		INIT_(NULL);
  GLOB_ char			*SqlciParse_OriginalStr		INIT_(NULL);
  GLOB_ Int32			 SqlciParse_HelpCmd		INIT_(0);
  GLOB_ Int32			 SqlciParse_IdentifierExpected	INIT_(0);
  GLOB_ Int32			 SqlciParse_SyntaxErrorCleanup	INIT_(0);

  // this global variable returns the final parse tree
  GLOB_ SqlciNode		*SqlciParseTree			INIT_(NULL);
  GLOB_ SqlciEnv        *SqlciEnvGlobal         INIT_(NULL);


#undef GLOB_
#undef INIT_

#endif /* SQLCIPARSEGLOBALS_H */
