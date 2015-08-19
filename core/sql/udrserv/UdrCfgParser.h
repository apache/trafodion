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
********************************************************************/
#ifndef __UdrCfgParser_H
#define __UdrCfgParser_H

#include "udrdefs.h"

class NAString;

#define BUFFMAX 1024

class UdrCfgParser{
public:

/* cfgFileIsOpen: open config file, if already open just return TRUE 
   First look for envvar MXUDRCFG, if none use default of 
   /usr/tandem/sqlmx/udr/mxudrcfg
   (NT default is c:/tdm_sql/udr/mxudrcfg) */
   static NABoolean cfgFileIsOpen(NAString &errorText);

/* closeCfgFile: close config file.  */
   static void closeCfgFile();
                                                                
/* readSection: position to BOF for a new section, do continuous reads until no
   more attr/value pairs in that section */
   static Int32 readSection( const char *section, char *buf, Int32 bufLen
                           , NAString &errorText);

/* textPos: get values position of an attribute  */
   static char *textPos( char *buf, const char *entry );

/* getPrivateProfileString: read value of string attribute */
   static Int32 getPrivateProfileString( const char *section, const char *entry,
                         const char *defaultString, char *buffer,
                         Int32   bufLen, NAString &errorText );


private:
   static FILE *cfgFile;
   static char *cfgFileName;
      
   static Int32 containTitle( char *buf, const char *section );
   static Int32 gotoSection( FILE *is, const char *section, NAString &errorText);
   static Int32 isTitleLine( char *bufPtr );
   static Int32 readEntry( FILE *is, const char *entry, char *buf, Int32 bufSize
                        , NAString &errorText );
   static void remEOL(char *buffer);
   static void rtrim(char *buf);
   static void stripComment( char *buf );
   static Int32 strnicmp(const char *s1, const char *s2, Int32 n);
   static char *titlePos( char *buf, Int32 *len );
   static Int32 readPair( FILE *is, char *buf, Int32 bufSize, NAString &errorText);

};

#endif
