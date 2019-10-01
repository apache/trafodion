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
 *****************************************************************************
 *
 * File:         NADefaults.h
 * Description:  A class and implementation for supporting an internal
 *               table (as in hash, and not SQL) of Defaults values.
 *
 *
 * Created:      7/11/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#ifndef _NA_DEFAULTS_H
#define _NA_DEFAULTS_H


#include "Platform.h"
#include <ctype.h>
#include <iostream>
#include <sstream>

#include "BaseTypes.h"
#include "charinfo.h"
#include "Collections.h"
#include "ComTransInfo.h"
#include "DefaultConstants.h"
#include "NAStringDef.h"
#include "str.h"

// Forward references
class SqlParser_NADefaults;
struct DefaultDefault;

// In various methods, argument "errOrWarn" tells how to handle errors;
// it's an int functioning as an enum:
//	emit error	 = -1,
//	silent		 = 0,
//	emit warning	 = +1,
//	SilentIfSYSTEM	 = +2000 -- a special value used ONLY PRIVATELY
//				    by NADefaults and DefaultValidator!
// We also have the quasi-NABoolean return type int-instead-of-enum:
//	error i.e. FALSE = 0,
//	ok/valid   TRUE	 = 1 or -1,
//	SilentIfSYSTEM   = +2000 -- again, a special value signifying
//				 -- a different "flavor" of ok/valid/no-error.
//
const Int32 SilentIfSYSTEM = +2000;

enum NADefaultFlags { DEFAULT_FLAGS_OFF			= 0,
		      DEFAULT_IS_EXTERNALIZED		= 0x1,
		      DEFAULT_ALLOWS_SEPARATE_SYSTEM	= 0x2,
                      DEFAULT_CATALOG_SET_TO_USERID     = 0x4,
		      DEFAULT_SCHEMA_SET_TO_USERID      = 0x8,

		      // this default is used at runtime only.
		      // It is set by users using SET SESSION DEFAULT stmt.
		      // See cli/SessionDefaults.h/cpp for details.
		      DEFAULT_IS_SSD                    = 0x10,
                      DEFAULT_SCHEMA_SET_BY_NAMETYPE    = 0x20,
                      // goes with DEFAULT_IS_EXTERNALIZED,
                      // at most one of these two should be set
		      DEFAULT_IS_FOR_SUPPORT            = 0x40

		    };

// Class NADefaults
//
// This class represents a set of default values used in the SQL/ARK software.
// The two main events which this class supports are (1) reading the defaults
// values from defaults tables and converting the string values to internal
// values, and (2) providing the value of a default parameter to a caller.
//
// {Functions}
//
// * NABoolean getValue(enum DefaultConstants, NAString&)
//	 returns the value of the default specified by the enum.
//	 The value is returned in the NAString.
//	 The NABoolean function result is always TRUE/success.
// * const char * getValue(enum DefaultConstants)
//	 is an alternate way to get the same default value string.
//
// * NAString keyword(DefaultToken) returns
//       the string image of that DefaultToken.  It is an assertion failure
//       if the given DefaultToken is out of range.
// * DefaultToken token(NAString) maps a string to a token value.
//       If the given string doesn't match any of the keywords, then the
//       (negative!) value of DF_noSuchToken is returned.
//
// --- private member functions ---
//
// * loadDefaultDefaults() inserts default defaults into hashTable.  This
//       function uses insertHandlingOwnership() and removes any same-pre-
//       existing values.
//
// {Internals}
//
// * The main private data member is a Rogue Wave hash table which contains
//       NADefaultEntries using the long key value as the key.
//
// * The string values for the defaults ``tokens'' are stored as private
//       static members of this class.  An associated routine can map
//       a string to a token value, doing a binary search on an array of
//       keywords_.
//
// * There is an array of structs which supplies the default defaults.
//       The default constructor iterates over this array, inserting
//       entries into the hash table.  The refresh method will also
//       do this loop as part of preparing the hash table.

class NADefaults : public NABasicObject
{
  friend class OptimizerSimulator;
public:

  NADefaults   ( NAMemory * h );
  ~NADefaults   ();

  // This is an ORDERED enum:  SET_BY_CQD > READ_FROM_SQL_TABLE, etc.
  // This ordering is used by the "overwriteIfNotYet" logic.
  enum Provenance
  { UNINITIALIZED,
    INIT_DEFAULT_DEFAULTS, DERIVED, READ_FROM_SQL_TABLE, SET_BY_CQD, CQD_RESET_RESET, COMPUTED, 
    IMMUTABLE };

  enum Flags
  {
    // indicates that this default was set by user through a CQD stmt.
    USER_DEFAULT = 0x01
  };

  enum DefFlags
  {
    // set to indicate that seabase defaults table has been read.
    SEABASE_DEFAULTS_TABLE_READ = 0x01

  };

  // test read-only attributes, i.e., whether they can be changed by a CQD
  NABoolean        isReadonlyAttribute  (const char* attrName) const;

  // these defaults cannot be reset or set to FALSE through a cqd.
  NABoolean        isNonResetableAttribute(const char* attrName) const;

  // these defaults can be set only once by user.
  NABoolean        isSetOnceAttribute(Int32 attrEnum) const;

  // reset attributes which are set for the session based on 
  // the caller. These do not persist across multiple session.
  // For example, nvci( scripts), or dbtransporter defaults.
  // See also cli/SessionDefaults.h/resetSessionBasedDefaults.
  void             resetSessionOnlyDefaults();

  NABoolean        getValue	(Int32 attrEnum, NAString &result) const;
  const char *     getValue	(Int32 attrEnum) const;
  const char *	   getValueWhileInitializing (Int32 attrEnum);

  NABoolean        getFloat	(Int32 attrEnum, float &result) const;
  double	   getAsDouble	(Int32 attrEnum) const;
  Lng32		   getAsLong	(Int32 attrEnum) const;
  ULng32	   getAsULong	(Int32 attrEnum) const;

  NAString        getString(Int32 attrEnum) const;

  // get the number of configured ESPs per node.
  ULng32           getNumOfESPsPerNode() const;

  // get the number of configured ESPs per cluster.
  ULng32           getTotalNumOfESPsInCluster(NABoolean& fakeEnv) const;

  NABoolean	   domainMatch	(Int32 attrEnum,
  				 Int32 expectedDefaultValidatorType,
				 float *flt = NULL) const;

  static NAString  keyword	(DefaultToken tok);
  DefaultToken	   token	(Int32 attrEnum,
  				 NAString &value,
  				 NABoolean valueAlreadyGotten = FALSE,
				 Int32 errOrWarn = -1) const;

  DefaultToken	   getToken	(const Int32 attrEnum,
                                 const Int32 errOrWarn = -1) const;

  static const char *
  		   lookupAttrName	(Int32 attrEnum,
					 Int32 errOrWarn = -1);
  static DefaultConstants
   		   lookupAttrName	(const char *attrName,
					 Int32 errOrWarn = -1,
					 Int32 *position = NULL);

  // By default, a (CQD) setting supersedes any previous (CQD or less) setting.
  DefaultConstants validateAndInsert	(const char *attrName,
					 NAString &value,
					 NABoolean reset,
					 Int32 errOrWarn = -1,
				     Provenance overwriteIfNotYet = IMMUTABLE);

  // This method does not insert, because provenance < UNINITIALIZED is imposs
  DefaultConstants validate		(const char *attrName,
					 NAString &value,
					 NABoolean reset,
					 Int32 errOrWarn = -1)
  {return validateAndInsert(attrName, value, reset, errOrWarn, UNINITIALIZED);}

  DefaultConstants holdOrRestore	(const char *attrName,
					 Lng32 holdOrRestoreCQD);

  Int32		   validateFloat	(const char *value,
  					 float &result,
                          		 Int32 attrEnum,
					 Int32 errOrWarn = -1) const;

  NABoolean        getIsolationLevel    (TransMode::IsolationLevel &arg,
  					 DefaultToken tok = DF_noSuchToken) const;
  NABoolean        getIsolationLevel    (Int16 &arg,
  					 DefaultToken tok = DF_noSuchToken) const
  {
    TransMode::IsolationLevel il = (TransMode::IsolationLevel)arg;
    NABoolean ret = getIsolationLevel(il, tok);
    arg = il;
    return ret;
  }

  NABoolean        setCatalog		(NAString &value,
  					 Int32 errOrWarn = -1,
  					 NABoolean overwrite = TRUE,
					 NABoolean alreadyCanonical = FALSE);
  NABoolean        setSchema		(NAString &value,
  					 Int32 errOrWarn = -1,
  					 NABoolean overwrite = TRUE,
					 NABoolean alreadyCanonical = FALSE);
  // code not used
  NABoolean        setCatalogTrustedFast(NAString &value)
			       { return setCatalog(value, -1, TRUE, TRUE); }
  NABoolean        setSchemaTrustedFast	(NAString &value)
			       { return setSchema(value, -1, TRUE, TRUE); }
  void		   getCatalogAndSchema	(NAString &cat, NAString &sch);

  void		   setState		(Provenance s)	{ currentState_ = s; }
  Provenance	   getState		()		{ return currentState_;}

  Provenance	   getProvenance	(Int32 attrEnum) const;

  NABoolean        userDefault(Int32 attrEnum)
  {return (flags_[attrEnum] & USER_DEFAULT) != 0;}
  void             setUserDefault(Int32 attrEnum, NABoolean v)
  { (v ? flags_[attrEnum] |= USER_DEFAULT : flags_[attrEnum] &= ~USER_DEFAULT);
  }

  NABoolean seabaseDefaultsTableRead() const { return defFlags_ & SEABASE_DEFAULTS_TABLE_READ;}
  void setSeabaseDefaultsTableRead(NABoolean v)
  {
    (v ? defFlags_ |= SEABASE_DEFAULTS_TABLE_READ : defFlags_ &= ~SEABASE_DEFAULTS_TABLE_READ);
  }

  static size_t numDefaultAttributes();
  const char *getCurrentDefaultsAttrNameAndValue(size_t ix,
						 const char* &name,
						 const char* &value,
						 NABoolean userDefaultsOnly);
  void  readFromDefaultsTable(Provenance overwriteIfNotYet = SET_BY_CQD,
  				     Int32 errOrWarn = +1/*warning*/);

  NABoolean    catSetToUserID() {
               return (catSchSetToUserID_ & DEFAULT_CATALOG_SET_TO_USERID) !=0;
               }
  NABoolean    schSetToUserID() {
               return (catSchSetToUserID_ & DEFAULT_SCHEMA_SET_TO_USERID) !=0;
               }
  void         setCatUserID(NABoolean v)
		{(v ? catSchSetToUserID_ |= DEFAULT_CATALOG_SET_TO_USERID : catSchSetToUserID_ &= ~DEFAULT_CATALOG_SET_TO_USERID);}
  void         setSchUserID(NABoolean v)
		{(v ? catSchSetToUserID_ |= DEFAULT_SCHEMA_SET_TO_USERID : catSchSetToUserID_ &= ~DEFAULT_SCHEMA_SET_TO_USERID);}

  // for DEFAULT_SCHEMA_NAMETYPE
  void         setSchByNametype(NABoolean v)
		{(v ? catSchSetToUserID_ |= DEFAULT_SCHEMA_SET_BY_NAMETYPE : catSchSetToUserID_ &= ~DEFAULT_SCHEMA_SET_BY_NAMETYPE);}
  NABoolean    schSetByNametype() {
               return (catSchSetToUserID_ & DEFAULT_SCHEMA_SET_BY_NAMETYPE) !=0;
               }

  // for cqd's from cqd * reset
  void         setResetAll(NABoolean v) { resetAll_ = v; }
  NABoolean    isResetAll() { return resetAll_; }

  const SqlParser_NADefaults *getSqlParser_NADefaults();
  SqlParser_NADefaults *getSqlParser_NADefaults_Ptr()
         { return SqlParser_NADefaults_; }
  static void	   getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum);

  Lng32 packedLengthDefaults();
  Lng32 packDefaultsToBuffer(char * buffer);
  Lng32 unpackDefaultsFromBuffer(Lng32 numEntriesInBuffer, char * buffer);
  NABoolean isSameCQD(Lng32 numEntriesInBuffer, char * buffer, Lng32 bufLen);

  void setSchemaAsLdapUser(const NAString val="");


private:

  UInt32 defFlags_;

  void initCurrentDefaultsWithDefaultDefaults();
  void initCurrentDefaultsFromSavedDefaults();
  void saveCurrentDefaults();
  static void	   updateSystemParameters(NABoolean reInit = FALSE);
  void updateCurrentDefaultsForOSIM(DefaultDefault *,
                                    NABoolean validateFloatVal = TRUE);
  void		   resetAll		(NAString &value,
					 short reset,
  					 Int32 errOrWarn = -1);

  NABoolean	   insert               (Int32 attrEnum,
                                         const NAString &value,
					 Int32 errOrWarn = -1);

  NABoolean        setMPLoc		(const NAString &value,
  					 Int32 errOrWarn,
  					 Provenance overwriteIfNotYet);
  void		   deleteMe();

  static float computeNumESPsPerCore(NABoolean aggressive);

  // get the number of configured ESPs (in float) per node.
  float            getNumOfESPsPerNodeInFloat() const;

  static const char *keywords_[];	// attr_VALUE keywords (ON, OFF, ANSI...

  char		   *provenances_;	// Provenance enum needs two bits
  char             *flags_;             // various flags
  char             **resetToDefaults_;
  const char       **currentDefaults_;
  float            **currentFloats_;
  DefaultToken	   **currentTokens_;

  class HeldDefaults;  // forward reference for helper class used below

  // these default values were 'held' through a cqd HOLD stmt.
  HeldDefaults     **heldDefaults_;

  Provenance       currentState_;
  Int32   	   catSchSetToUserID_;

  SqlParser_NADefaults	*SqlParser_NADefaults_;

  NAMemory         *heap_;

  static const char **defaultsWithCQDsFromDefaultsTable_;
  static DefaultToken **tokensWithCQDsFromDefaultsTable_;
  static char *provenancesWithCQDsFromDefaultsTable_;	
  static float **floatsWithCQDsFromDefaultsTable_;	
  static NABoolean readFromDefaultsTable_;
  
  // cqd * reset
  NABoolean resetAll_;
};


inline
Lng32 ToErrorOrWarning(Lng32 sqlCode, Int32 eow)
{
  sqlCode = ABS(sqlCode);
  return (eow < 0) ? -sqlCode : (eow > 0) ? +sqlCode : 0;
}

#endif /* _NA_DEFAULTS_H */
