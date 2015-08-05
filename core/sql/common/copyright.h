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

#ifndef COPYRIGHT_H

  #define COPYRIGHT_H \
  "(c) Copyright 2014 Hewlett-Packard Development Company, LP."

  // The COPYRIGHT_VERSION_H macro is no longer defined here.
  // Instead, the version is defined in sqenvcom.sh and then passed
  // via -D compiler directives in a few makefiles: sqlcilib, arkcmplib, sqlc


  #define COPYRIGHT_TOP_PRODNAME_H	"Trafodion"
  #define COPYRIGHT_XTOP_PRODNAME_H	"Trafodion"

  #define COPYRIGHT_ARKCMP_PRODNAME_H	"Trafodion Compiler"
  #define COPYRIGHT_CATMAN_PRODNAME_H	"Trafodion Catalog Manager"
  #define COPYRIGHT_SQLCI_PRODNAME_H	"Trafodion Conversational Interface"
  #define COPYRIGHT_SQLC_PRODNAME_H	"Trafodion C/C++ Preprocessor"
  #define COPYRIGHT_SQLCO_PRODNAME_H	"Trafodion COBOL Preprocessor"
  #define COPYRIGHT_UDRSERV_PRODNAME_H	"Trafodion UDR Server"

  #define COPYRIGHT_BANNER_1(ostrm,prod) \
  ostrm << prod " " COPYRIGHT_VERSION_H << endl << COPYRIGHT_H << endl

  #define COPYRIGHT_BANNER_2(ostrm,prod) \
  COPYRIGHT_BANNER_1(ostrm,prod) << endl

  #define SQLCO_CPYRT COPYRIGHT_SQLCO_PRODNAME_H " " COPYRIGHT_VERSION_H
  #define SQLC_CPYRT  COPYRIGHT_SQLC_PRODNAME_H  " " COPYRIGHT_VERSION_H

#endif
