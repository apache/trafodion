/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComMPLoc.cpp
 * Description:  See ComMPLoc.h
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
 *
 *****************************************************************************
 */


#define   SQLPARSERGLOBALS_NADEFAULTS		// first

#include <ctype.h>
#include "Platform.h"
#include "ComASSERT.h"
#include "ComMPLoc.h"
#include "ComOperators.h"
#include "ComRtUtils.h"
#include "NAString.h"
#include "SqlParserGlobals.h"			// last


#define PREALLOCATE_STRING_MEMBERS		\
  system_(ComMPLoc_NAMELEN(1)),			\
  volume_(ComMPLoc_NAMELEN(1)),			\
  subvol_(ComMPLoc_NAMELEN(1)), 		\
  sysdotvol_(ComMPLoc_NAMELEN(2))
  // file_(ComMPLoc_NAMELEN(1))		// not always used so don't preallocate


ComMPLoc::ComMPLoc()			// lightweight ctor
: format_(UNKNOWN), PREALLOCATE_STRING_MEMBERS
{}

// This ctor parses left-first:
//   "X" is interpreted as "\X", "X.Y" as "\X.$Y", "X.Y.Z" as "\X.$Y.Z", ...
//
ComMPLoc::ComMPLoc(const ComString &fileName)
: PREALLOCATE_STRING_MEMBERS
{
  ComString nam(fileName);
  TrimNAStringSpace(nam);
  const char *s = nam.data();
  Int32 namepartnum = (*s == '\\') ? 1 : (*s == '$') ? 2 : 3;

  while (*s)
    if (*s++ == '.')
      namepartnum++;

  parse(nam, (Format)namepartnum);
}

// This ctor parses right-first, more in the style of Guardian name parsing:
//   "X" as "X",  ...,  "X.Y.Z" as "$X.Y.Z",  "X.Y.Z.f" as "\X.$Y.Z.F"
//
// If fmt is FILE,
// no namepart defaulting (filling in default \system, $volume, subvol) is done:
// any of these is accepted:
//	file
//	subvol.file
//	$volume.subvol.file
//	\system.$volume.subvol.file
//
// If fmt is FULLFILE, only the last two forms are accepted
// (and the third form gets defaulted to \currentSystem.$v.sv.f).
//
// If fmt is SUBVOL, as in
//	"SET MPLOC $volume.subvol;"  or  "SET MPLOC \system.$volume.subvol;",
// - \system name is optional (gets defaulted to current \currentSystem name),
// - $volume and subvolume are required,
// - no filename part is allowed:
// any of these is accepted:
//	$volume.subvol	    (gets defaulted to \currentSystem.$v.sv)
//	system.$volume.subvol
//
// If fmt is VOL,
// either of these is accepted:
//	$volume		    (gets defaulted to \currentSystem.$v)
//	\system.$volume
//
// If fmt is SYS,
//	\system
//
ComMPLoc::ComMPLoc(const ComString &fileName, Format fmt)
: PREALLOCATE_STRING_MEMBERS
{
  parse(fileName, fmt);
}

// This method parses right-to-left.
// By default, it accepts as input true NSK format names only
// (i.e., with dots and dollar sign and backslash punctuation).
//
// The ctors use ONLY the default true NSK format parsing.
//
// If you want to follow the SHORTANSI scheme used by ODBC,
// you must do so in two steps, using the lightweight inline ctor first:
//	ComMPLoc loc;
//	loc.parse(nam, ComMPLoc::SUBVOL, TRUE);
// This will accept names in the ODBC SHORTANSI format only.
// In this format,
// - underscore '_' is the namepart delimiter instead of dot '.',
// - no '\\' or '$' punctuation appears in the system or volume namepart,
// - the system name does not get defaulted, all nameparts must be specified.
// This really makes sense only with fmt SUBVOL, in which only this is accepted:
//	system_volume_subvol		e.g.  SQUAW_DATA00_NSKPORT
// Note that after parsing, the class internally stores this as the equivalent
// vanilla MPLOC:
//	\system.$volume.subvol		e.g.  \SQUAW.$DATA00.NSKPORT
//
void ComMPLoc::parse(const ComString &fileName, Format fmt,
		     ComBoolean shortAnsi)
{
  ComBoolean err = FALSE;

  ComString nam(fileName);
  TrimNAStringSpace(nam);	// remove leading+trailing blanks  " $X.Y "
  nam.toUpper();

  char delim = shortAnsi ? '_' : '.';

  char sentinelBuf[] = ".";				// nul-terminated string
  sentinelBuf[0] = delim;				// overwrite w/ delim
  nam.prepend(sentinelBuf);				// sentinel delim

  Int32 namepartnum = ABS(fmt);
// Parser treats quoted string as it is.  To use reserved word as one part
// of the name separated by dots,  the reserved word must be enclosed in
// double quotes.  It's ok to remove all double quotes from the name here, 
// because unpaired double quote in the name is flagged as error.
  if (nam.index('"') != NA_NPOS) {                      // found a double quote
    Int32 quoted = -1;
    size_t i = nam.length();
    while (i--)
      if (nam[i] == '"') {
        nam.remove(i,1);                                // remove all "
        quoted = -quoted;
      }
    if (quoted > 0) err = TRUE;                         // " must be in pairs
  }
  size_t i = nam.length();

  while (i-- && !err) {
    char c = nam[i];
    if (!isalnum(c)) {
      if (c != delim) {					// eg. "Y.SUB%OL"
        if (nam[i-1] != delim)					err = TRUE;
	//else we are at either the '\\' or '$' in ".\X.$Y", which is okay,
	//or we are at                      '%' in ".\X.$Y.%UBVOL",
	//but on the next loop iter, the isalpha() below will catch this error.
      }
      else {
	ComString part(&nam.data()[i+1]);
	nam.remove(i);
	// Commented this out, to disallow  "\X .$Y . Z. F"
	//	if (!shortAnsi)
	//	  TrimNAStringSpace(part);
	if (part.isNull())					err = TRUE;
	else {
	  const char *p = part.data();
	  switch (namepartnum--) {
	    case 4:   if (!isalpha(*p))				err = TRUE;
		      file_   = part;
		      break;

	    case 3:   if (!isalpha(*p))				err = TRUE;
		      subvol_ = part;
		      break;

	    case 2:   if (!shortAnsi && *p++ != '$')		err = TRUE;
		      if (!isalpha(*p))				err = TRUE;
		      if (shortAnsi) part.prepend("$");
		      volume_ = part;
		      break;

	    case 1:   if (!shortAnsi && *p++ != '\\')		err = TRUE;
		      if (!isalpha(*p))				err = TRUE;
		      if (shortAnsi) part.prepend("\\");
		      system_ = part;
		      break;

	    default:  err = TRUE;
		      break;
	  }
	  if (part.length() > MAX_NAMEPART_LEN)			err = TRUE;
	}
      }	// c==delim
    }	// !isalnum
  }	// while

  if (fmt == FILE) {
    if (file_.isNull()) err = TRUE;
  }
  else if (!err) {
    // Default the system part to <currentSys>,
    // except if shortAnsi, which requires full specification w/o defaulting.
    if (fmt != SYS && system_.isNull() && !shortAnsi) {
      initializeSystemName();
      namepartnum--;
    }
    if (namepartnum) err = TRUE;	// must have all nameparts to my left
  }

  format_ = err ? INVALID : fmt;
  if (err) {
    ComString nam(fileName);
    TrimNAStringSpace(nam);
    nam.toUpper();
    system_ = nam;			// save original for getMPName, errmsgs
    sysdotvol_ = volume_ = subvol_ = file_ = "";
  }
  else {
    sysdotvol_ = system_;		// set to "" or "\sys"
    if (!sysdotvol_.isNull())
      sysdotvol_ += '.';		// literal dot, *not* delim
    sysdotvol_ += volume_;
  }
}

void ComMPLoc::initializeSystemName(ComBoolean ignoreNADefaults)
{
  static char mySysName[MAX_NAMEPART_LEN+1] = "";
  if (!SqlParser_Initialized() || ignoreNADefaults || !mySysName[0]) {
    if (!mySysName[0])
      ComRtGetOSClusterName(mySysName, sizeof(mySysName), NULL);
    if (mySysName[0] && mySysName[0] != ' ') {
      system_ = mySysName;
      if (*system_ != '\\') {
        system_.prepend("\\");
	strcpy(mySysName, system_);
      }
    }
  }
  else
    system_ = SqlParser_MPLOC.system_;
}

Int32 ComMPLoc::applyDefaults(const ComMPLoc &defaults,
			    ComBoolean emptySystemNameMatchCountsAsAMatch)
{
  if (!isValid(FILE)) return 0;

  if (system_.isNull())
    system_ = defaults.system_;

  if (volume_.isNull())
    volume_ = defaults.volume_;

  if (subvol_.isNull())
    subvol_ = defaults.subvol_;

  Int32 defaultMatchCount = 0;
  if (system_ == defaults.system_) {
    if (!system_.isNull() || emptySystemNameMatchCountsAsAMatch)
      defaultMatchCount++;
    if (volume_ == defaults.volume_) {
      defaultMatchCount++;
      if (subvol_ == defaults.subvol_)
        defaultMatchCount++;
    }
  }
  return defaultMatchCount;
}

ComString ComMPLoc::getMPName() const
{
  // Do NOT say "if (!isValid())" here -- infinite recursion!
  // Instead we just pass the name along as-is, if UNKNOWN.
  if (format_ == INVALID)
    return system_;		// original string, saved by parse() if error

  ComString n(ComMPLoc_NAMELEN(4));
  ComBoolean beg = FALSE;
  if (       hasSystemName()) { n += getSystemName(); n += '.'; beg = TRUE; }
  if (beg || hasVolumeName()) { n += getVolumeName(); n += '.'; beg = TRUE; }
  if (beg || hasSubvolName()) { n += getSubvolName(); n += '.'; beg = TRUE; }
  if (beg || hasFileName())     n += getFileName();

  // If this Loc was built using the left-to-right ctor
  // or with format SUBVOL or VOL or SYS,
  // it can have trailing parts missing, e.g. "\SYS.$VOL.SUBV.",
  // so now we remove any trailing dots, e.g. "\SYS.$VOL.SUBV"
  for (size_t i = n.length(); i-- && n[i] == '.'; )
    n.remove(i);

  return n;
}

ComString ComMPLoc::getOSSName() const
{
  // Do NOT say "if (!isValid())" here -- infinite recursion!
  // Instead we just pass the name along as-is, if UNKNOWN.
  if (format_ == INVALID)
    return system_;		// original string, saved by parse() if error

  static const ComString VSV_DEFAULT("SYSTEM");
  ComString n("/G/");
  n += hasVolumeName() ? ChopMPLocPrefix(getVolumeName()) : VSV_DEFAULT.data();
  n += '/';
  n += hasSubvolName() ? getSubvolName() : VSV_DEFAULT;
  if (hasFileName()) {
    n += '/';
    n += getFileName();
  }

  return n;
}

// Returns name, e.g.	\SYS.$DATA.SUBVOL.NAM
//			0123456789!123456789@
// and	lenArray[0] = 4,  for 4 name parts in this name
//	lenArray[1] = 21, length of entire name
//	lenArray[2] = 5,  offset of	$DATA.SUBVOL.NAM
//	lenArray[3] = 11, offset of	SUBVOL.NAM
//	lenArray[4] = 18, offset of	NAM
// This allows storing one single character string and a set of offsets into it,
// for equivalent ways of referencing an object --
// **this is meaningful ONLY IF the caller has called applyDefaults()
// **with the correct current defaults!
//
// See CollationInfo class's namelen + synonymOffset[3] size_t's.
//
ComString ComMPLoc::getMPName(size_t *lenArray /* array[5] */ ) const
{
  lenArray[0] = lenArray[1] = lenArray[2] = lenArray[3] = lenArray[4] = 0;
  ComString n = getMPName();
  ComASSERT(format_ != UNKNOWN);
  if (format_ != UNKNOWN &&			// safe to..
      ((ComMPLoc *)this)->isValid(FILE)) {	// ..cast away constness
    lenArray[1] = n.length();	// [1] = length of full name
    size_t a = 1;
    size_t i = 0;
    const char *s = n.data();
    for ( ; *s; i++, s++) {
      if (*s == '.') {
	lenArray[++a] = i + 1;	// [2] thru [4] = offsets of trailing name parts
      }
    }
    lenArray[0] = a;		// [0] = the count of size_t's being returned
  } 				// (length + up to 3 offsets)
				// (aka the number of nameparts)
  return n;			// Return the external-format string as well.
}

ComBoolean ComMPLoc::isValid(Format fmt)
{
  if (format_ == UNKNOWN)
    parse(getMPName(), fmt);

  return ((format_ != INVALID) &&
  	  (format_ == fmt ||
  	   ABS(format_) == fmt));	// format_ of FULLFILE matches FILE,
}					// but *not* vice versa!

ComBoolean ComMPLoc::getAsSubvol()
{
  if (isValid(SUBVOL)) return TRUE;
  if (hasSubvolName()) {
    file_ = "";
    format_ = SUBVOL;
    return TRUE;
  }
  return FALSE;
}

ostream &operator<< (ostream &s, const ComMPLoc &name)
{
  s << name.getMPName();
  return s;
}
