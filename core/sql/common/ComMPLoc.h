/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComMPLoc.h
 * Description:  This class parses a string representing an MP file,
 *		 subvol (a/k/a MPLOC), volume, or system, using NSK syntax,
 *		 with certain defaults allowed.
 *
 *		 An ODBC SHORTANSI string can also be parsed, resulting
 *		 in an MPLOC.
 *
 *		 See ComMPLoc::parse() in the .cpp file for details.
 *               
 * Created:      7/1/99
 * Language:     C++
 *
 *
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
 *
 *****************************************************************************
 */


#ifndef _COM_MP_LOC_H_
#define _COM_MP_LOC_H_

#include "ComSmallDefs.h"
#include "Platform.h"

inline ComBoolean  IsMPLocPrefix(const char c)
  { return c == '$' || c == '\\'; }

inline ComBoolean  IsMPLocPrefix(const unsigned char c)
  { return c == '$' || c == '\\'; }

inline ComBoolean  HasMPLocPrefix(const char *s)
  { return IsMPLocPrefix(*s); }

inline const char *ChopMPLocPrefix(const char *s)
  { return HasMPLocPrefix(s) ? &s[1] : s; }

// Max length of an externally formatted name
// of "numparts" parts and "numparts-1" dots.
#define ComMPLoc_NAMELEN(numparts)	\
	(size_t(numparts * ComMPLoc::MAX_NAMEPART_LEN + numparts-1))

class ComMPLoc
{
  // ---------------------------------------------------------------------
  // Friend Functions:
  // ---------------------------------------------------------------------
   friend ostream &operator<< (ostream &s, const ComMPLoc &name);

  // ---------------------------------------------------------------------
  // Public Methods:
  // ---------------------------------------------------------------------
   public:

   enum			{ MAX_NAMEPART_LEN = 8 };

   enum Format		{ INVALID = -99,
   			  UNKNOWN = 0,
   			  SYS = 1,	// "\SYS" has 1 name part,
			  VOL = 2,	// "$VOL" is the 2nd part,
			  SUBVOL = 3,	// and so forth
			  FILE = 4,
			  FULLFILE = -FILE
			};

   ComMPLoc     ();
   ComMPLoc     (const ComString &nam);
   ComMPLoc     (const ComString &nam, Format fmt);

   void parse   (const ComString &nam, Format fmt,
   		 ComBoolean shortAnsi = FALSE);
   void reparse (Format fmt, ComBoolean shortAnsi = FALSE)
   					  {parse(getMPName(), fmt, shortAnsi);}

   const ComString &getSysDotVol () const {return sysdotvol_;}
   const ComString &getSystemName() const {return system_;}
   const ComString &getVolumeName() const {return volume_;}
   const ComString &getSubvolName() const {return subvol_;}
   const ComString &getFileName  () const {return file_;}

   ComBoolean hasSystemName() const	  {return !system_.isNull();}
   ComBoolean hasVolumeName() const	  {return !volume_.isNull();}
   ComBoolean hasSubvolName() const	  {return !subvol_.isNull();}
   ComBoolean hasFileName  () const	  {return !file_.isNull();}

   ComBoolean isValid(Format fmt);
   Format     getFormat() const		  {return format_;}
   void	      setUnknown()		  {format_ = UNKNOWN;}
   ComBoolean getAsSubvol();

   ComString getMPName() const;
   ComString getMPName(size_t *lenArray /* array[5] */ ) const;
   ComString getOSSName() const;

   Int32  applyDefaults(const ComMPLoc &defaults,
		      ComBoolean emptySystemNameMatchCountsAsAMatch=FALSE);

   void initializeSystemName(ComBoolean ignoreNADefaults = FALSE);

  // ---------------------------------------------------------------------
  // Private Members:
  // ---------------------------------------------------------------------
   private:

   Format    format_;
   ComString sysdotvol_;	//  \<sys>.$<vol>  (e.g.  \AZTEC.$SQL)
   ComString system_;
   ComString volume_;
   ComString subvol_;
   ComString file_;
};

#endif // _COM_MP_LOC_H_
