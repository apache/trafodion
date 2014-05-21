/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         CliMxArlib.cpp
 * Description:  Methods that help TMFARLB2 to interpret SQL/MX
 *               audit records.
 *               
 * Created:      10/01/02
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
// ss_cc_changes .This file is obsolete on Seaquest 
//LCOV_EXCL_START
#include "cli_stdh.h"
#include "Cli.h"
#include "ComCextdecs.h"
#include "exp_attrs.h"
#include "ComRCB.h"
#include "fs/feerrors.h"
#include "hfs2dm"
#include "CliMxArLib.h"
#include "ModifiedFieldMap.h"
#include "exp_datetime.h"
#include "exp_interval.h"
#include "exp_clause_derived.h"

//
// Notes on how we retrieve the resource fork, September 2003
// - The code was originally written to use the ArkFsRTMD interface.
// - ArkFsRTMD does not return all the version numbers we need for
//   version checks on NSK.
// - Now on NSK only, we call a system library routine to get the 
//   resource fork and all the version numbers we need. This routine
//   is SQLMX_GETOBJECTINFO_ and it does all of its reading via a
//   single DP2 request.
// - The MXARLIB_USE_ARKFSRTMD preprocessor define is used to create
//   two code paths, one that calls ArkFsRTMD and the other calls
//   SQLMX_GETOBJECTINFO_. 
// - The MXARLIB_USE_ARKFSRTMD define does not create two independent
//   code paths. Most code from each path gets compiled regardless
//   of how the define is set. This is done only as a convenience to
//   reduce the number of ifdefs in this file.
// - The helper class CliMxArLibRfork provides a C++ wrapper
//   around SQLMX_GETOBJECTINFO_.
//
 #include "dmxrtmd.h"
 extern "C"_callable short SQLMX_GETOBJECTINFO_ (SqlmxRtmdDisplayStruct *);

#define MAX2(a,b) ((a) > (b) ? (a) : (b))
#define MIN2(a,b) ((a) < (b) ? (a) : (b))

// Debugging macros. In the debug build they serve as printf wrappers
// and in the release build they are no-ops.
#ifdef NA_DEBUG_C_RUNTIME
static NABoolean MXARLIB_DO_DEBUG = FALSE;
#define MXARLIB_DEBUG0(fmt) \
  { if (MXARLIB_DO_DEBUG) fprintf(stderr, (fmt)); }
#define MXARLIB_DEBUG1(fmt,a) \
  { if (MXARLIB_DO_DEBUG) fprintf(stderr, (fmt), (a)); }
#define MXARLIB_DEBUG2(fmt,a,b) \
  { if (MXARLIB_DO_DEBUG) fprintf(stderr, (fmt), (a), (b)); }
#define MXARLIB_DEBUG3(fmt,a,b,c) \
  { if (MXARLIB_DO_DEBUG) fprintf(stderr, (fmt), (a), (b), (c)); }
#else
#define MXARLIB_DEBUG0(fmt)
#define MXARLIB_DEBUG1(fmt,a)
#define MXARLIB_DEBUG2(fmt,a,b)
#define MXARLIB_DEBUG3(fmt,a,b,c)
#endif

// -----------------------------------------------------------------------
// Methods for class CliMxArLibCacheEntry
// -----------------------------------------------------------------------

CliMxArLibCacheEntry::CliMxArLibCacheEntry(
     NAHeap           *heap,
     const char       *ansiName,
     const char       ansiNameSpace[CLIARL_ANSI_NAME_SPACE_LEN],
     const char       *logicalPartitionName,
     Int64            crvsn,
     const char       *guardianName,
     Int32              numAttrs,
     AttributesPtrPtr attrs)
  : heap_(heap),
    crvsn_(crvsn),
    lastAccessTime_(0)
{
  // This constructor makes a deep copy of the attributes passed in

  assert(ansiName);
  assert(guardianName);
  assert(logicalPartitionName);

  Lng32 ansiNameLen = str_len(ansiName);
  Lng32 guardianNameLen = str_len(guardianName);
  Lng32 partitionNameLen = str_len(logicalPartitionName);

  Int32 colNum;
  Int32 colIx;
  Int32 nextColIx;
  Int32 i;

  // variables to help in calculating move information
  NABoolean reachedEndOfFixedFields = FALSE;
  NABoolean bulkMoveStillPossible = TRUE;
  Lng32 highWaterMarkSrc = 0; // offset of first byte after last column in
                             // the original record so far
  Lng32 highWaterMarkDst = 0; // offset of first byte after last column in
                             // the output record so far
  Lng32 fieldLowWaterMarkSrc; // offset of first byte of this column
  Lng32 fieldLowWaterMarkDst; // offset of first byte of this column
  Lng32 offsetAdjForConv = 0; // offset adjustment for data conversions

  // To avoid the overhead of multiple memory allocations we will
  // allocate a single "names" buffer to store the ANSI name, logical
  // partition name, and Guardian name.
  Lng32 namesBufferLen = ansiNameLen + 1 +
                        partitionNameLen + 1 +
                        guardianNameLen + 1;
  namesBuffer_ = new(heap_) char[namesBufferLen];
  ansiName_ = namesBuffer_;
  partitionName_ = &namesBuffer_[ansiNameLen + 1];
  guardianName_ = &namesBuffer_[ansiNameLen + 1 + partitionNameLen + 1];
  str_cpy_all(ansiName_, ansiName, ansiNameLen+1);
  str_cpy_all(partitionName_, logicalPartitionName, partitionNameLen+1);
  str_cpy_all(guardianName_, guardianName, guardianNameLen+1);

  str_cpy_all(ansiNameSpace_, ansiNameSpace, CLIARL_ANSI_NAME_SPACE_LEN);

  imageInfoLen_ = (Lng32) (sizeof(MXARLibImageInfo) +
                          (numAttrs-1) * sizeof(MXARLibColumnInfo));
  imageInfo_ = (MXARLibImageInfo *) (new(heap_) char[imageInfoLen_]);
  imageInfo_->columnCount_ = numAttrs;
  imageInfo_->encodedKeyLength_ = -1; // gets set by caller
  str_pad(imageInfo_->filler_, sizeof(imageInfo_->filler_), 0);
  bitmapLen_ =
    (numAttrs + CLIARL_BITS_PER_ENTRY-1) / CLIARL_BITS_PER_ENTRY;
  bitmapForAllFields_ = new(heap_) ULng32[bitmapLen_];
  str_pad((char *) bitmapForAllFields_, (Lng32) sizeof(Lng32) * bitmapLen_, 0);
  firstFixedField_          = -1;
  firstVariableField_       = -1;
  firstStoredField_         = -1;
  lengthOfAdminFields_      = -1;
  firstVariableFieldOffset_ = -1;
  bulkMoveLen_              = 0;
  numBulkMoveFields_        = 0;
  flags_                    = 0;
  moveInfo_ = (CliMxArLibMoveInfo *) new(heap_)
    char[numAttrs * sizeof(CliMxArLibMoveInfo)];

  // find the first variable and the first fixed field, if they exist
  for (colNum=0; colNum < numAttrs; colNum++)
    {
      // is this a variable-length field?
      if (attrs[colNum]->getVCIndicatorLength())
	{
	  if (firstVariableField_ < 0)
	    firstVariableField_ = colNum;
	}
      else
	{
	  if (firstFixedField_ < 0)
	    firstFixedField_ = colNum;
	}
    }

  // find the field with the lowest offset, that is either the first fixed
  // field if one exists, otherwise it is the first (variable) field
  if (firstFixedField_ >= 0)
    firstStoredField_ = firstFixedField_;
  else
    firstStoredField_ = firstVariableField_;
  assert(firstStoredField_ >= 0);

  // walk columns of the table, but in ascending offset order, not
  // in field order
  for (i=0, colIx = firstStoredField_;
       i < numAttrs && colIx != UINT_MAX;
       i++, colIx=nextColIx)
    {
      Attributes        &a   = *(attrs[colIx]);
      MXARLibColumnInfo &ci  = imageInfo_->columnInfoArray_[colIx];
      CliMxArLibMoveInfo &mi = moveInfo_[colIx];
      Lng32 offsetAdjThisField = 0;
      mi.convertSrcLength_ = 0;

      mi.defaultClass_ = a.getDefaultClass();
      if (mi.defaultClass_ == Attributes::NO_DEFAULT)
      {
        mi.defaultValueLength_ = 0;
        mi.defaultValue_ = NULL;
      }
      else
      {
        Lng32 defaultLen = a.getStorageLength();
        mi.defaultValueLength_ = defaultLen;
        mi.defaultValue_ = new (heap_) char[defaultLen];
        str_cpy_all(mi.defaultValue_, a.getDefaultValue(), defaultLen);

#ifdef NA_DEBUG_C_RUNTIME
        MXARLIB_DEBUG1("[MXARLIB] Default value for column %d\n", (Lng32) i);
        MXARLIB_DEBUG1("[MXARLIB]   length %d\n", (Lng32) defaultLen);
        MXARLIB_DEBUG0("[MXARLIB]   ");
        for (Int32 xx = 0; xx < defaultLen; xx++)
        {
          MXARLIB_DEBUG1("%02X ", (unsigned char) mi.defaultValue_[xx]);
        }
        MXARLIB_DEBUG0("\n");
#endif
      }

      bitmapForAllFields_[colIx/CLIARL_BITS_PER_ENTRY] |=
	(ULng32) 0x1 << (CLIARL_BITS_PER_ENTRY - 1 -
				(colIx % CLIARL_BITS_PER_ENTRY));

      // see also method ExpGenerator::convertNATypeToAttributes
      // in file generator/GenExpGenerator.cpp and method
      // ExpTupleDesc::computeOffsets (SQLMX_FORMAT case)
      ci.columnNum_        = colIx;
      ci.columnAnsiType_   = Descriptor::ansiTypeFromFSType(a.getDatatype());
      ci.columnFsType_     = a.getDatatype();
      ci.keyField_         = -1; // caller uses setKeyField() to change this
      ci.dataOffset_       = -1; // initialize
      ci.dataLength_       = a.getLength();
      ci.nullIndOffset_    = -1; // initialize
      ci.nullIndLength_    = a.getNullIndicatorLength();
      ci.varLenOffset_     = -1; // initialize
      ci.varLenLength_     = a.getVCIndicatorLength();
      ci.datetimeIntCode_  = 0;
      ci.datetimeFsCode_   = 0;
      ci.leadingPrecision_ = 0;
      ci.precision_        = a.getPrecision();
      ci.scale_            = a.getScale();
      ci.characterSet_     = 0;
      ci.futureCollation_  = 0; // for now
      str_pad(ci.filler_, sizeof(ci.filler_), 0);

      // set, reset or override fields that are datatype-specific
      if (ci.columnFsType_ == REC_DATETIME)
	{
	  Lng32 convertedLength;
	  ci.datetimeFsCode_   = a.getPrecision();
	  ci.datetimeIntCode_  = Descriptor::datetimeIntCodeFromTypePrec(
	                                                  ci.columnFsType_,
	                                                  ci.datetimeFsCode_);
	  ci.precision_        = 0;

	  // we convert the internal datetime format to a string of
	  // a different length, this makes bulk move impossible and
	  // changes the offsets of following columns
	  bulkMoveStillPossible = FALSE;
	  mi.convertSrcLength_  = ci.dataLength_;

	  // compute length of the ASCII datetime value
	  switch (ci.datetimeFsCode_)
	    {
	    case REC_DTCODE_DATE:
	      convertedLength = 10;   // yyyy-mm-dd
	      break;
	    case REC_DTCODE_TIME:
	      convertedLength = 8;    // hh:mm:ss
	      break;
	    case REC_DTCODE_TIMESTAMP:
	      convertedLength = 19;   // yyyy-mm-dd hh:mm:ss
	      break;
	    default:
	      convertedLength = 0; // Avoid Reasoning defect
	      assert(0); // Internal error if we reach here

	    }

	  // add length needed for fractional seconds ".ffffff"
	  if (a.getScale())
	    convertedLength += 1 + a.getScale();

	  // compute the new length of the converted item and the
	  // difference between source and converted length (this
	  // will have to be added to all future offsets)
	  ci.dataLength_ = convertedLength;
	  offsetAdjThisField = ci.dataLength_ - mi.convertSrcLength_;
	}
      else if (Descriptor::isIntervalFSType(ci.columnFsType_))
	{
          // INTERVAL types have two precisions, one called leading
          // precision and the other fractional precision. We
          // interpret the scale fields in our structures as
          // fractional precision. Leading precision is stored as
          // precision in an Attributes object and as
          // leadingPrecision_ in the MXARLibColumnInfo structure.

	  ci.leadingPrecision_ = a.getPrecision();
	  ci.datetimeIntCode_  = Descriptor::datetimeIntCodeFromTypePrec(
	                                               ci.columnFsType_,
	                                               ci.leadingPrecision_);
	  ci.precision_        = 0;

          // we convert the internal interval format to a string of
          // a different length, this makes bulk move impossible and
          // changes the offsets of following columns
          bulkMoveStillPossible = FALSE;
          mi.convertSrcLength_  = ci.dataLength_;

	  // compute the new length of the converted item and the
	  // difference between source and converted length (this
	  // will have to be added to all future offsets)
	  ci.dataLength_ =
            ExpInterval::getDisplaySize(ci.columnFsType_,
                                        (short) ci.leadingPrecision_,
                                        (short) ci.scale_);
	  offsetAdjThisField = ci.dataLength_ - mi.convertSrcLength_;
	}
      else if (Descriptor::isCharacterFSType(ci.columnFsType_))
	{
	  ci.characterSet_     = a.getScale();
	  ci.scale_            = 0; // reset from previously assigned value
	}


      // compute starting offset for this field
      if (i == 0)
	{
	  // The first field starts after the VOA, calculate this
	  // from the lowest of the used offsets. Note that if the
	  // first stored field is a variable field, its offsets
	  // are still set.
	  if (ci.nullIndLength_)
	    fieldLowWaterMarkSrc = a.getNullIndOffset();
	  else if (ci.varLenLength_)
	    fieldLowWaterMarkSrc = a.getVCLenIndOffset();
	  else
	    fieldLowWaterMarkSrc = (Lng32) a.getOffset();
	  fieldLowWaterMarkDst = fieldLowWaterMarkSrc;

	  // now we know the length of the administrative fields
	  lengthOfAdminFields_ = fieldLowWaterMarkSrc;
	}
      else
	{
	  fieldLowWaterMarkSrc = highWaterMarkSrc;
	  fieldLowWaterMarkDst = highWaterMarkDst;
	}

      // compute offsets and field move information
      if (ci.varLenLength_ == 0)
	{
	  // fixed field, set offsets
	  if (ci.nullIndLength_)
	    ci.nullIndOffset_ = a.getNullIndOffset() + offsetAdjForConv;
	  if (ci.varLenLength_)
	    ci.varLenOffset_  = a.getVCLenIndOffset() + offsetAdjForConv;
	  ci.dataOffset_ = (Lng32) (a.getOffset() + offsetAdjForConv);

	  // should see fields in ascending offset order and
	  // shouldn't see fixed fields after first varchar
	  assert(!reachedEndOfFixedFields &&
		 ci.dataOffset_ >= highWaterMarkDst);
	}
      else
	{
	  if (!reachedEndOfFixedFields)
	    {
	      reachedEndOfFixedFields = 1;
	      // ExpTupleDesc::computeOffsets sets the offset of the
	      // first variable field
	      if (ci.nullIndLength_)
		ci.nullIndOffset_ = a.getNullIndOffset() + offsetAdjForConv;
	      if (ci.varLenLength_)
		ci.varLenOffset_  = a.getVCLenIndOffset() + offsetAdjForConv;
	      ci.dataOffset_ = (Lng32) (a.getOffset() + offsetAdjForConv);
	      firstVariableFieldOffset_ = fieldLowWaterMarkSrc;
	    }
	  else
	    {
	      // adjust offset of varchars with no fixed offset for
	      // an exploded record
	      if (ci.nullIndLength_)
		{
		  ci.nullIndOffset_  = highWaterMarkDst;
		  highWaterMarkDst  += ci.nullIndLength_;
		}
	      if (ci.varLenLength_)
		{
		  ci.varLenOffset_   = highWaterMarkDst;
		  highWaterMarkDst  += ci.varLenLength_;
		}
	      ci.dataOffset_   = highWaterMarkDst;
	    }
	  bulkMoveStillPossible = FALSE;
	}

      highWaterMarkDst = ci.dataOffset_ + ci.dataLength_;
      highWaterMarkSrc = (Lng32) (a.getOffset() + a.getLength());

      // see whether we can still perform a bulk move of the columns so
      // far (we won't attempt that for any reordered columns)
      if (i != colIx)
	bulkMoveStillPossible = FALSE;
      else if (bulkMoveStillPossible)
	{
	  bulkMoveLen_ = highWaterMarkDst;
	  numBulkMoveFields_++;
	}

      // finalize move info for this individual column
      mi.moveOffset_ = fieldLowWaterMarkDst;
      // we can move this column as a single byte move if it is not a
      // variable length col and if it needs no conversion
      if (ci.varLenLength_ == 0  &&  !mi.convertSrcLength_)
	mi.moveLength_ = highWaterMarkDst - fieldLowWaterMarkDst;
      else
	mi.moveLength_ = 0;

      // prepare for the next iteration
      offsetAdjForConv += offsetAdjThisField;
      nextColIx = (Int32) attrs[colIx]->getNextFieldIndex();
      if (nextColIx == UINT_MAX)
	nextColIx = -1; // doesn't really do anything on a 32 bit machine...
      mi.nextColumnInOffsetOrder_ = nextColIx;

    } // loop over attributes in storage order

  // make sure we walked the list completely
  assert(i == numAttrs && colIx == UINT_MAX);

  // finally we know the length of the entire record
  imageBufferLen_ = highWaterMarkDst;
  if (firstVariableFieldOffset_ < 0)
    firstVariableFieldOffset_ = highWaterMarkSrc;
}

CliMxArLibCacheEntry::~CliMxArLibCacheEntry()
{
  // Release resources held by the moveInfo_ array
  if (moveInfo_)
  {
    Lng32 numColumns = imageInfo_->columnCount_;
    for (Lng32 i = 0; i < numColumns; i++)
    {
      CliMxArLibMoveInfo &mi = moveInfo_[i];
      const char *defaultValue = mi.defaultValue_;
      if (defaultValue)
      {
        NADELETEBASIC(defaultValue, heap_);
      }
    }
  }

  NADELETEBASIC(namesBuffer_, heap_);
  NADELETEBASIC(imageInfo_, heap_);
  NADELETEBASIC(moveInfo_, heap_);
  NADELETEBASIC(bitmapForAllFields_, heap_);
}

void CliMxArLibCacheEntry::setKeyField(Lng32 colNumber, Lng32 keyColNumber)
{
  if (colNumber >= 0 && colNumber < imageInfo_->columnCount_)
    imageInfo_->columnInfoArray_[colNumber].keyField_ = keyColNumber;
}

NABoolean CliMxArLibCacheEntry::isIndex() const
{
  return (str_cmp(getAnsiNameSpace(),
		  "IX",
		  CLIARL_ANSI_NAME_SPACE_LEN) == 0);
}

NABoolean CliMxArLibCacheEntry::isRFork() const
{
  // The Guardian names of resource forks end in an odd character
  // (e.g. "01") as opposed to table names which end in an even
  // character (e.g. "00"). To determine whether a given Guardian name
  // represents a resource fork we simply check to see if the last
  // character has an even or odd ASCII value.
  return (guardianName_[str_len(guardianName_)-1] % 2 != 0);
}

short CliMxArLibCacheEntry::moveAllFields(
     char *dst,
     const char *src,
     Lng32 srcLen,
     Lng32 &numAddedCols)
{
  // Move all fields from an audit record to a destination buffer that
  // may be owned by the user (non-priv).

  // Note that the CLI call that invoked this has bounds-checked the buffer,
  // but that this method needs to perform additional checking to make sure
  // we don't read outside the buffer (when we interpret variable length
  // fields, for example). Generally, we should not trust any contents of
  // the source buffer while it is ok to trust the contents of the
  // cache entry (*this). The same comment applies to method moveFields().

  // This is the assumed record layout:
  //
  //   Values derived from the src record and parameters:
  //
  //   srcLen ---------------------------------------------------------------+
  //   offsetPastFixedFieldsFromRec ----------------------------+            |
  //   lengthOfAdminFieldsFromRec ---------------+              |            |
  //                                             |              |            |
  // |<------- administrative fields ----------->v              v            v
  // +-----------------------+------+-----+------+--------------+------------+
  // | first fixed fld offs. | VO1  | ... | VOn  | fixed fields | var fields |
  // +-----------------------+------+-----+------+--------------+------------+
  //                                             ^              ^            ^
  //   Values derived from RCB:                  |              |            |
  //                                             |              |            |
  //   lengthOfAdminFields_ ---------------------+              |            |
  //   offsetPastFixedFields -----------------------------------+            |
  //   imageBufferLen_ (interpreted as offset) ------------------------------+
  //
  // VOA (VO1...n), fixed fields, and var fields are optional.

  // Have to have at least the first fixed field offset. However, this
  // fixed field offset is 0 when no fixed fields are present.
  if (srcLen < sizeof(Lng32))
    return FEBOUNDSERR;

  // ---------------------------------------------------------------------
  // check for missing columns in the full audit record
  // (which has first fixed field offset and VOA included)
  // ---------------------------------------------------------------------

  //  long lengthOfAdminFieldsFromRec = *((long *) src);
  Lng32 lengthOfAdminFieldsFromRec = *((Lng32 *)src);
    // BSV:  FIX
    // (long)Attributes::getFirstFixedFieldOffset((char*)src);

  // customized bulk move information for this record
  Lng32 actualBulkMoveLen = bulkMoveLen_;
  Int32  actualNumBulkMoveFields = numBulkMoveFields_;
  numAddedCols = 0;

  // calculate length of the administrative fields
  if (lengthOfAdminFieldsFromRec < sizeof(Lng32))
    {
      // Without fixed fields the length of the administrative fields
      // at the beginning of the record is indicated by the first VOA
      // field.	Check for unexpected values.
      if (lengthOfAdminFieldsFromRec != 0 || srcLen < 2 * sizeof(Lng32))
	return FEINVALOP;
      // use the value of VO1 (see picture above)
      lengthOfAdminFieldsFromRec = *(((Lng32 *) src) + 1);
    }

  // do a sanity check on the length of the administrative fields
  if (lengthOfAdminFieldsFromRec < sizeof(Lng32) ||
      lengthOfAdminFieldsFromRec > srcLen)
    return FEINVALOP;
  
  // check for added variable-length columns, indicated by a missing
  // VOA entry for the record that was passed in
  if (lengthOfAdminFieldsFromRec != lengthOfAdminFields_)
    {
      if (lengthOfAdminFieldsFromRec < lengthOfAdminFields_ &&
	  getHasAddedColumn() &&
	  ((lengthOfAdminFields_ -
	    lengthOfAdminFieldsFromRec) % sizeof(Lng32) == 0))
	{
	  // This case is plausible, a variable-length column could
	  // have been added to the table since the audit record was
	  // created. This shifted all the fixed columns in the record,
	  // to the left, so move all fields individually.
	  actualBulkMoveLen       = 0;
	  actualNumBulkMoveFields = 0;
	  numAddedCols += (lengthOfAdminFields_ -
			   lengthOfAdminFieldsFromRec) / sizeof(Lng32);
	}
      else
	{
	  // That's very inconsistent. Since we don't have
	  // a DROP COLUMN command the current table can't have
	  // fewer variable columns than the audit record. Also,
	  // if there is any difference in the number of variable
	  // columns, the table has to have added columns. Finally,
	  // we can only add whole entries to the VOA, which come
	  // as 4 byte integers.
	  // The audit record is not compatible with the table,
	  // return with an error.
	  // (Note that since the likelihood that we reuse the same
	  // GUARDIAN name for a different table is extremely low,
	  // it should be virtually impossible to reach here unless
	  // some data got corrupted).
	  return FEINVALOP;
	}
    } // difference in first fixed field offset

  // check for added fixed columns
  if (firstFixedField_ >= 0)
    {
      // Calculate the offset of the first byte just past the fixed fields
      // in the records (get the value from the record and also the value
      // of what we think it should be). This calculation depends on
      // whether there are variable columns. Note that
      // firstVariableFieldOffset_ is set regardless of whether there are
      // variable fields present or not (w/o variable fields it indicates
      // the first byte after the fixed fields).
      Lng32 offsetPastFixedFields = firstVariableFieldOffset_;
      Lng32 offsetPastFixedFieldsFromRec;

      if (lengthOfAdminFieldsFromRec > sizeof(Lng32))
	{
	  // have to have at least one entry in the VOA
	  if (srcLen < 2*sizeof(Lng32))
	    return FEBOUNDSERR;
	  
	  // look at the offset in the first VOA entry, it should point
	  // to the byte right after the fixed fields
	  offsetPastFixedFieldsFromRec = *(((Lng32 *) src) + 1);
	  if (offsetPastFixedFieldsFromRec < lengthOfAdminFieldsFromRec ||
	      offsetPastFixedFieldsFromRec > srcLen)
	    return FEINVALOP;
	}
      else
	{
	  // if there are no variable columns, the fixed fields are at
	  // the end of the record
	  offsetPastFixedFieldsFromRec = srcLen;
	}

      // check for added fixed fields, indicated by a mismatch in the
      // length of the fixed portion of the record
      Lng32 fixedPartDiff =
	(offsetPastFixedFieldsFromRec - lengthOfAdminFieldsFromRec) -
	(offsetPastFixedFields - lengthOfAdminFields_);

      if (fixedPartDiff)
	{
	  // The length of the fixed part of the record does not match
	  if (fixedPartDiff < 0 && getHasAddedColumn())
	    {
	      // walk through the fixed fields and find those that
	      // are outside the audit record we got
	      // (they must have been added in the meantime)
	      //
	      //      |-- ad flds --|--- fixed fields of audit rec ---|
	      //                    |                                 |
	      //                    v                                 v
	      // |-- admin fields --|--- fixed fields of current layout ---|
	      Lng32 offsetOfFirstMissingField =            // here:    ^
		offsetPastFixedFieldsFromRec - lengthOfAdminFieldsFromRec +
		lengthOfAdminFields_;
	      Lng32 numAddedVarCols = numAddedCols;

	      for (Int32 c = firstFixedField_;
		   c >= 0  &&  c != firstVariableField_;
		   c = moveInfo_[c].nextColumnInOffsetOrder_)
		{
		  if (imageInfo_->columnInfoArray_[c].dataOffset_ >=
		      offsetOfFirstMissingField)
		    {
		      // this column is not present in the audit record
		      numAddedCols++;
		      if (actualNumBulkMoveFields > c)
			// don't include in bulk move, note that bulk
			// move is done only if col # in offset order is
			// equal to user col #
			actualNumBulkMoveFields = c;
		    }
		}		     
	      
	      // there should have been at least one added fixed column
	      // if the fixed field parts differ, and we can't have added
	      // all the columns
	      if (numAddedCols == numAddedVarCols ||
		  numAddedCols >= imageInfo_->columnCount_)
		return FEINVALOP;

	      // set bulk move length back to at most the fixed
	      // field part in the record (note that there is no
	      // bulk move if the admin fields differ)
	      if (actualBulkMoveLen > offsetPastFixedFieldsFromRec)
		actualBulkMoveLen = offsetPastFixedFieldsFromRec;
	    }
	  else
	    {
	      // The table doesn't have added columns or the audit
	      // record's fixed fields are longer than those of the
	      // table, refuse to interpret this audit record as a
	      // record of this table
	      return FEINVALOP;
	    }
	} // fixed field parts have different lengths
    } // fixed fields present

  // ---------------------------------------------------------------------
  // Start the actual movement of data
  // ---------------------------------------------------------------------

  if (actualNumBulkMoveFields)
    {
      if (actualBulkMoveLen > srcLen ||
	  lengthOfAdminFieldsFromRec > actualBulkMoveLen)
	return FEBOUNDSERR;

      // exclude the administrative fields from the bulk move
      str_cpy_all(dst+lengthOfAdminFields_,
		  src+lengthOfAdminFieldsFromRec,
		  actualBulkMoveLen-lengthOfAdminFieldsFromRec);
    }

  // anything left?
  if (actualNumBulkMoveFields < imageInfo_->columnCount_ - numAddedCols)
    {
      const char *remainder;
      Lng32 moveStartField;
      Lng32 moveEndField;
      Lng32 lastFieldToMove = imageInfo_->columnCount_ - numAddedCols - 1;
      short retcode = FEOK;
      
      // Move those columns that weren't included in the bulk move but
      // are present in the audit record. The fields must be moved
      // in offset order, not in column order, so break up the fields
      // remaining into contiguous ranges if necessary.

      // Calculate the starting byte of the next field in the source, and
      // find the first field (in offset order) to move. This is either
      // the next field (in offset order) from where we finished the
      // bulk move or it is the first fixed field in the record
      // (if existent), or the first variable field.
      if (actualNumBulkMoveFields)
	{
	  // skip buffer that was already moved
	  remainder = src + actualBulkMoveLen;
	  moveStartField =
	    moveInfo_[actualNumBulkMoveFields-1].nextColumnInOffsetOrder_;
	}
      else
	{
	  // skip administrative fields
	  remainder = src + lengthOfAdminFieldsFromRec;
	  if (firstFixedField_ >= 0)
	    moveStartField = firstFixedField_;
	  else
	    moveStartField = firstVariableField_;
	}

      // In each iteration of the loop below we'll move a range of
      // columns that are contiguous. We do this until we have reached
      // the end of the columns. This process ends once we have reached
      // the last column in offset order (moveStartField becomes -1)
      while (moveStartField >= 0)
	{
	  if (moveStartField <= lastFieldToMove)
	    {
	      // start with a range of a single field
	      moveEndField = moveStartField;
	      
	      // find columns with contiguous column numbers that are also
	      // contiguous in the record and add them to the range
	      while (moveInfo_[moveEndField].nextColumnInOffsetOrder_ ==
		     moveEndField+1 &&
		     moveEndField < lastFieldToMove)
		moveEndField++;
	      
	      retcode = moveFields(dst, remainder, src+srcLen,
				   moveStartField, moveEndField,
                                   NULL, // optional bitmap
                                   0);   // bitmap length
	      if (retcode)
		return retcode;

	      // get ready for the next move
	      moveStartField =
		moveInfo_[moveEndField].nextColumnInOffsetOrder_;
	    }
	  else
	    {
	      // skip this field, it is outside of the range of
	      // columns provided in the audit record
	      moveStartField =
		moveInfo_[moveStartField].nextColumnInOffsetOrder_;
	    }
	      
	}
    } // anything left to move

  // ---------------------------------------------------------------------
  // Fill in missing columns if needed
  // ---------------------------------------------------------------------

  if (numAddedCols)
    {
      // make up the default values for added columns that are missing
      // in the source
      for (Int32 i = imageInfo_->columnCount_ - numAddedCols;
	   i < imageInfo_->columnCount_;
	   i++)
	{
	  short retcode2 = moveDefaultValue(dst,i);
	  if (retcode2)
	    return retcode2;
	}
    }

  return FEOK;
}

short CliMxArLibCacheEntry::moveFields(
     char *destBuffer,
     const char *&firstSrcField,
     const char *firstBytePastSrc,
     Lng32 startField,
     Lng32 endField,
     Lng32 *replyBitmap,
     Lng32 bitmapLength)
{
  // This method moves a range of contiguous fields from the audit
  // record into the destination buffer.  As a side effect, the
  // firstSrcField pointer is advanced to point to the next field
  // after those that have been moved. Note that destBuffer is not
  // side-effected in the same way, it always points to the beginning
  // of the entire destination buffer.

  // startField and endField are logical field numbers. However the
  // method does not move all logical fields between startField and
  // endField, because adjacent logical fields (for example, the 1st
  // and 2nd columns specified in a CREATE TABLE statement) are not
  // always contiguous. We will instead move the contiguous range of
  // fields whose first logical field number is startField and whose
  // last logical field number is endField. The value of
  // moveInfo_[N].nextColumnInOffsetOrder_ is used to determine the
  // next contiguous field after logical field N.

  // The replyBitmap parameter is optional and a value of NULL means
  // the caller does not need a bitmap updated. The bitmapLength is
  // specified in 4-byte units, not in bytes.

  // See comment in method moveAllFields() about bounds checking requirements.

  // Here is a list of assumptions made in this code, not necessarily
  // complete. See also the external spec. The assumptions don't just
  // apply to this method, they apply to the entire file.
  //
  // - The modified field map is built such that field ranges are actually
  //   contiguous in the record, even if fields got reordered by moving
  //   varchars to the end of the record.
  // - When we ship the entire record to the audit image that includes the
  //   administrative fields at the beginning (first fixed field offset + voa)
  // - When we use a modified field map we don't include the administrative
  //   fields
  // - We assume that there are no fillers between null indicator,
  //   var len and fields, and that relOffset is the only offset used,
  //   not null indicator, varlen, and data offset, and that the other
  //   offsets are calculated from the null indicator and varlen indicator
  //   lengths
  // - The totlen_before_frags and totlen_after_frags fields in the modified
  //   field map don't account for any intervening fillers (see assumption
  //   about no internal fillers above). Also, DP2 seems to recompute
  //   this value at run time.
  // - For a range of fields (entry 10 of a modified field map for a table
  //   with no added col), we take the rel/voa offset of the first field
  //   in the range as the starting byte and the rel/voa offset of the last
  //   field plus the last field's length (w/o intervening fillers, again)
  //   as the end (already outside)

  Lng32 nextField = startField;
  while (nextField >= 0)
  {
    // Get the column number and CliMxArLibMoveInfo instance for this
    // iteration of the loop
    Lng32 current = nextField;
    const CliMxArLibMoveInfo &mi = moveInfo_[current];

    // Get the column number for the next iteration of the loop. If we
    // are currently processing endField, then set nextField to -1 so
    // we do not iterate again. If we are currently processing the
    // last physical column, then nextField will become -1 and we will
    // not iterate again.
    if (current == endField)
      nextField = -1;
    else
      nextField = mi.nextColumnInOffsetOrder_;

    // Deal with the bitmap
    if (replyBitmap)
    {
      if ((current / CLIARL_BITS_PER_ENTRY) < bitmapLength)
      {
        replyBitmap[current / CLIARL_BITS_PER_ENTRY] |=
          (ULng32) 0x1 << (CLIARL_BITS_PER_ENTRY - 1 -
                                  (current % CLIARL_BITS_PER_ENTRY));
      }
    }

    Lng32 ml = mi.moveLength_;

    if (ml)
    {
      // bounds check
      if (firstSrcField + ml > firstBytePastSrc)
        return FEBOUNDSERR;
      
      str_cpy_all(&destBuffer[mi.moveOffset_], firstSrcField, ml);
      firstSrcField += ml;
    }
    else
    {
      const MXARLibColumnInfo &ci  = imageInfo_->columnInfoArray_[current];
      Lng32 dataLen = ci.dataLength_;
      NABoolean srcValueIsNull = FALSE;

      // bounds check for null indicator / var len indicator
      if (firstSrcField + ci.nullIndLength_ + ci.varLenLength_ >
          firstBytePastSrc)
        return FEBOUNDSERR;
      
      // move NULL indicator
      if (ci.nullIndLength_)
      {
        str_cpy_all(&destBuffer[ci.nullIndOffset_], 
                    firstSrcField,
                    ci.nullIndLength_);
                                                    
        srcValueIsNull = (firstSrcField[1] & NEG_BIT_MASK);

        firstSrcField += ci.nullIndLength_;
      }
      
      if (ci.varLenLength_)
      {
        assert(ci.varLenLength_ == 4); // for now

        // move varLen indicator and read its value
        dataLen = *((Lng32 *) firstSrcField);
        if (dataLen < 0 ||
            dataLen > ci.dataLength_)
          // reading corrupted data image, return with an error
          return FEINVALOP;
        
        str_cpy_all(&destBuffer[ci.varLenOffset_], 
                    firstSrcField,
                    ci.varLenLength_);
        
        firstSrcField += ci.varLenLength_;
        // pad the empty space in the destination buffer with zeroes
        str_pad(&destBuffer[ci.dataOffset_ + dataLen],
                ci.dataLength_ - dataLen,
                0);
      }
      
      //
      // Now we move the actual data. Two cases to consider:
      //  a) conversion to presentation format is required. This is
      //     indicated by a non-zero value in the mi.convertSrcLength_
      //     field.
      //  b) no conversion required
      // 

      if (mi.convertSrcLength_)
      {
        // conversion needed
        
        // first bounds check for data
        if (firstSrcField + mi.convertSrcLength_ > firstBytePastSrc)
          return FEBOUNDSERR;
        
        if (ci.columnFsType_ == REC_DATETIME)
        {
          if (srcValueIsNull)
          {
            // Pad with zeroes
            str_pad(&destBuffer[ci.dataOffset_], ci.dataLength_, 0);
          }
          else
          {
            ComDiagsArea *dummyDiags = NULL;
            ExpDatetime srcDatetimeOpType;
            
            // Setup attribute for the source.
            //
            srcDatetimeOpType.setPrecision(ci.datetimeFsCode_);
            srcDatetimeOpType.setScale((short) ci.scale_);
            
            // Convert the datetime value to ASCII in the
            // DEFAULT format.
            Lng32 targetLen = 
              srcDatetimeOpType.convDatetimeToASCII(
                  (char *) firstSrcField,
                  &destBuffer[ci.dataOffset_],
                  dataLen,
                  ExpDatetime::DATETIME_FORMAT_DEFAULT,
		  NULL,
                  heap_,
                  &dummyDiags);
            
            if (dummyDiags)
            {
              // SQL error during conversion, get rid of
              // diags area
              dummyDiags->decrRefCount();
            }
            
            // targetLen = -1 also indicates an error
            if (dummyDiags || targetLen != dataLen)
              return FEINVALOP;

          } // if (srcValueIsNull) else ...            
        } // conversion of datetime values
        
        else if (Descriptor::isIntervalFSType(ci.columnFsType_))
        {
          if (srcValueIsNull)
          {
            // Pad with zeroes
            str_pad(&destBuffer[ci.dataOffset_], ci.dataLength_, 0);
          }
          else
          {
            ex_expr::exp_return_type expResult = 
              convDoIt((char *) firstSrcField,       // char *source
                       mi.convertSrcLength_,         // long sourceLen
                       (short) ci.columnFsType_,     // short sourceType
                       ci.leadingPrecision_,         // long sourcePrecision
                       ci.scale_,                    // long sourceScale
                       &destBuffer[ci.dataOffset_],  // char *target
                       dataLen,                      // long targetLen
                       REC_BYTE_F_ASCII,             // short targetType
                       0,                            // long targetPrecision
                       0,                            // long scale
                       NULL,                         // char *varCharLen
                       0);                           // long varCharLenSize
            
            if (expResult != ex_expr::EXPR_OK)
            {
              return FEINVALOP;
            }
            
          } // if (srcValueIsNull)
        } // conversion of interval values
        
        else
        {
          // don't know about conversion of this data type
          assert(0);
          break;
        }
        
        firstSrcField += mi.convertSrcLength_;
        
      } // if a conversion is needed
      
      else
      {
        // no conversion needed, just move dataLen bytes
        
        // first bounds check for data
        if (firstSrcField + dataLen > firstBytePastSrc)
          return FEBOUNDSERR;
        
        str_cpy_all(&destBuffer[ci.dataOffset_], 
                    firstSrcField,
                    dataLen);
        
        firstSrcField += dataLen;

      }

      // End of if-then-else blocks for different types of
      // presentation format conversions
      
    } // ml == 0, move field the complicated way
  } // while (nextField <= 0)
  
  return FEOK;
}

short CliMxArLibCacheEntry::moveDefaultValue(
     char *dst,
     Lng32 fieldNum)
{
  // Notes about this method:
  //
  // fieldNum is a 0-based column number in field order not offset order
  //
  // This method returns an error if the field has no default
  // value. The reason is that we should only be trying to move
  // defaults for added columns and added columns are required to have
  // default values. If we enter this method for a column having no
  // default value, the likely cause is that the audit record being
  // processed is incorrectly formatted.
  //
  // The moveFields() call below will side-effect its second parameter
  // and for that reason we pass in a local variable (i.e. a temporary
  // copy of the pointer value) rather than a reference to a class or
  // struct member

  MXARLIB_DEBUG1("[MXARLIB] Moving default value for field %d\n", fieldNum);
  short retcode = FEOK;
  CliMxArLibMoveInfo &mi = moveInfo_[fieldNum];
  
  if (mi.defaultClass_ != Attributes::NO_DEFAULT)
  {
    const char *defaultValue = mi.defaultValue_;
    Lng32 defaultLen = mi.defaultValueLength_;
    retcode = moveFields(dst, defaultValue, defaultValue + defaultLen,
                         fieldNum, fieldNum,
                         NULL, // optional bitmap
                         0);   // bitmap length
  }
  else
  {
    retcode = FEINVALOP;
  }

  return retcode;
}

// -----------------------------------------------------------------------
// Methods for class CliMxArLibCache
// -----------------------------------------------------------------------
CliMxArLibCache::CliMxArLibCache(NAHeap *heap)
  : heap_(heap),
    entries_(NULL),
    cacheLimitBytes_(5 * 1024 * 1024),
    cacheLimitSetInStone_(FALSE),
    accessCounter_(0)
{
  entries_ = new (heap_) HashQueue(heap_);

#ifdef NA_DEBUG_C_RUNTIME
  // Set global flag that turns debugging on and off
  MXARLIB_DO_DEBUG = (getenv("MXARLIB_DEBUG") != NULL);

  // See if the environment contains a setting to control cache size
  const char *cacheLimit = getenv("MXARLIB_CACHE_LIMIT");
  if (cacheLimit && cacheLimit[0])
  {
    cacheLimitBytes_ = atol(cacheLimit) * 1024 * 1024;
  }
#endif
}

CliMxArLibCache::~CliMxArLibCache()
{
  clearCache();
  delete entries_;
}

void CliMxArLibCache::clearCache()
{
  while (entries_->numEntries() > 0)
  {
    entries_->position();
    CliMxArLibCacheEntry *e = (CliMxArLibCacheEntry *) entries_->getNext();
    entries_->remove(e);
    delete e;
  }
}

short CliMxArLibCache::getMxArLibCacheEntryByName(
     CliGlobals            *cliGlobals,
     const char            *guardianName,
     Lng32                  guardianNameLen,
     CliMxArLibCacheEntry  *&entry)
{
  // Initialize result to NULL
  entry = NULL;

  // Convert to a fully qualified name
  char normalizedName[MAX_EXTERNAL_FNAME_LENGTH+1];
  short rc = normalizeName(guardianName,
                           guardianNameLen,
                           normalizedName,
                           MAX_EXTERNAL_FNAME_LENGTH+1);
  if (rc)
    return rc;

  // See if the entry is already in the cache
  Int32 normalizedNameLen = str_len(normalizedName);
  entry = findEntry(normalizedName, normalizedNameLen);

  if (entry != NULL)
  {
    // This entry is already in the cache, just update the timestamp
    MXARLIB_DEBUG1("[MXARLIB] UPDATE TIMESTAMP %s\n", guardianName);
    entry->lastAccessTime_ = ++accessCounter_;
  }
  else
  {
    // Create a new cache entry
    rc = createMxArLibCacheEntry(cliGlobals,
                                 normalizedName,
                                 normalizedNameLen,
                                 entry);
    if (rc == 0)
    {
      MXARLIB_DEBUG1("[MXARLIB] ADD %s\n", guardianName);
      entry->lastAccessTime_ = ++accessCounter_;
      entries_->insert(normalizedName, normalizedNameLen, (void *) entry);

      // We added a new cache entry. We now delete entries in LRU
      // order until cache size is within the application's chosen
      // limit. We will NOT delete the newly created entry even if
      // that entry alone consumes more than the allowed number of
      // bytes.
      while (entries_->numEntries() > 1 &&
             heap_->getAllocSize() > cacheLimitBytes_)
      {
        CliMxArLibCacheEntry *oldest = findOldestEntry();
        assert(oldest && oldest != entry);

        MXARLIB_DEBUG1("[MXARLIB] REMOVE OLDEST %s\n",
                       oldest->getGuardianName());
        removeEntry(oldest);
      }

    } // if (rc == 0)
  } // if (entry already in cache) else ...

  // Verify that entry and rc (the two outputs of this method) are in
  // sync.
  if (rc == 0)
  {
    assert(entry != NULL);
  }
  else
  {
    assert(entry == NULL);
  }

  return rc;
}

CliMxArLibCacheEntry *CliMxArLibCache::findOldestEntry()
{
  CliMxArLibCacheEntry *curr = NULL;
  CliMxArLibCacheEntry *oldest = NULL;

  entries_->position();
  while ((curr = (CliMxArLibCacheEntry *) entries_->getNext()) != NULL)
  {
    if (!oldest || (curr->lastAccessTime_ < oldest->lastAccessTime_))
    {
      oldest = curr;
    }
  }

  return oldest;
}

void CliMxArLibCache::removeEntry(CliMxArLibCacheEntry *e)
{
  // Do a lookup and then a removal in the hash queue. 
  entries_->position(e->getGuardianName(),
                     e->getGuardianNameLen());
  
  CliMxArLibCacheEntry *hashQueueEntry;
  NABoolean found = FALSE;
  while ((hashQueueEntry = (CliMxArLibCacheEntry *) entries_->getNext()))
  {
    if (hashQueueEntry == e)
    {
      entries_->remove(e);
      delete e;
      
      found = TRUE;
      break;
    }
  }
  
  assert(found);
}

CliMxArLibCacheEntry *CliMxArLibCache::findEntry(const char *guardianName,
                                                 Lng32 nameLen)
{
  CliMxArLibCacheEntry *e;

  // All public interfaces that use the cache will call this
  // method. The first time through this method we set the
  // cacheLimitSetInStone_ flag to TRUE which will cause us to ignore
  // any future attempts to change the cache size.
  cacheLimitSetInStone_ = TRUE;

  entries_->position(guardianName, nameLen);
  while ((e = (CliMxArLibCacheEntry *) entries_->getNext()) != NULL)
  {
    if (nameLen == e->getGuardianNameLen() &&
        str_cmp(guardianName, e->getGuardianName(), nameLen) == 0)
        
    {
      MXARLIB_DEBUG1("[MXARLIB] HIT %s\n", guardianName);
      return e;
    }
  }

  MXARLIB_DEBUG1("[MXARLIB] MISS %s\n", guardianName);
  return NULL;
}

void CliMxArLibCache::removeEntryByName(const char *guardianName, Lng32 nameLen)
{
  CliMxArLibCacheEntry *e = findEntry(guardianName, nameLen);

  if (e)
  {
    // A note about the removal from the entries_ hash queue. Removing
    // an entry requires a lookup (which consists of a position() and
    // getNext() call) followed by a remove(e) call. In this code path
    // the lookup was performed inside the findEntry() call above.
    MXARLIB_DEBUG1("[MXARLIB] REMOVE %s\n", guardianName);
    entries_->remove(e);
    delete e;
  }
}

short CliMxArLibCache::createMxArLibCacheEntry(
     CliGlobals            *cliGlobals,
     const char            *guardianName,
     Lng32                  guardianNameLen,
     CliMxArLibCacheEntry  *&entry)
{
  // Initialize result to NULL
  entry = NULL;

  short rc;

#ifdef MXARLIB_USE_ARKFSRTMD

  // split Guardian name into parts
  char outBuffer[4 * (FILENAME_SUBPART_LENGTH + 1)];
  char * parts[4];
  Lng32 numParts;

  // split the name into parts
  if ((LateNameInfo::extractParts((char *)guardianName, outBuffer, numParts, 
				  parts, FALSE)) ||
      (numParts != 4))
    return FEBADPARMVALUE;

  ArkFsRTMD fsRTMD(DM_ANSI_TABLE, heap_);  // check RTMD type requirements

  fsRTMD.setGuardianName(ArkFsGuardianName(
			      parts[0] /*sysName*/, 
			      parts[1] /*volName*/, 
			      parts[2] /*subvolName*/,
			      parts[3] /*fileName*/));

  ArkFsFetchList fetchList(heap_);
  fetchList.setAnsiNameOpcode();
  fetchList.setAnsiNameSpaceOpcode();
  fetchList.setLogicalPartitionNameOpcode();
  fetchList.setRecExprOpcode();
  fetchList.setKeyDescOpcode();
  // fetchList.setPartitionArrayOpcode();  // $$$$ read once for all partns
  
  // Fetch label info for this table.
  Lock_Protocol  lp = NOLOCK;
  Lock_State     ls = SHARED;

  Lng32 defaultTimeout = 6000;

  rc = fsRTMD.fetch(fetchList, lp, ls, defaultTimeout);
  if (rc)
    return rc;

  // move information from fsRTMD to cache entry
  ExRCB rcbVp;
  ExRCB * rcb;
  ExpTupleDesc *tupleDesc;
  ArkFsKeyClass *keys;

  if ((rcb = (ExRCB *)((fsRTMD.getRecordExpression())->getData())) == NULL)
    return FENOTFOUND;

  rcb->driveUnpack((void *)rcb,&rcbVp,NULL);

  if ((tupleDesc = rcb->getTupleDesc()) == NULL)
    return FENOTFOUND;

  if ((keys = fsRTMD.getKeyDesc()) == NULL)
    return FENOTFOUND;

  const char *ansiName = fsRTMD.getAnsiName();
  const char *ansiNameSpace = fsRTMD.getAnsiNameSpace();
  const char *partitionName = fsRTMD.getLogicalPartitionName();
  Int64 crvsn = 0;
  UInt32 numKeyFields = keys->getNumKeyFieldEntries();

#else // if MXARLIB_USE_ARKFSRTMD

  CliMxArLibRfork rfork(guardianName, heap_);
  rc = rfork.fetch();
  if (rc)
    return rc;

  ExRCB *rcb = rfork.getRCB();
  if (rcb == NULL)
    return FENOTFOUND;

  ExpTupleDesc *tupleDesc = rfork.getTupleDesc();
  if (tupleDesc == NULL)
    return FENOTFOUND;

  const char *ansiName = rfork.getAnsiName();
  const char *ansiNameSpace = rfork.getAnsiNameSpace();
  const char *partitionName = rfork.getLogicalPartitionName();
  Int64 crvsn = rfork.getCRVSN();

  SqlmxKeyDescriptor *keyDesc = rfork.getKeyDesc();
  UInt32 numKeyFields = (keyDesc ? keyDesc->numberOfEntries : 0);

#endif // if MXARLIB_USE_ARKFSRTMD else ...

  // $$$$ MXARLIB operations are not yet supported for aligned format
  if (tupleDesc->isSQLMXAlignedTable())
    return FEINVALOP;

  ULng32 numAttrs = tupleDesc->numAttrs();
  AttributesPtrPtr attrs = tupleDesc->attrs();

  if ( numAttrs == 0 || *attrs == (AttributesPtr)NULL )
    return FENOTFOUND;

  entry = new (heap_) CliMxArLibCacheEntry(heap_,
                                           ansiName,
                                           ansiNameSpace,
                                           partitionName,
                                           crvsn,
                                           guardianName,
                                           (Lng32) numAttrs,
                                           attrs);
  
  // add information on clustering key to the cache entry

  NABoolean isIndex = entry->isIndex();
  
  for (UInt32 k = 0; k < numKeyFields; k++)
  {
    // the keyFieldIndex_ entries of an index refer to the base
    // column numbers and the assumption is that key column
    // #k of the index is also its column #k 
    UInt32 keyIndex;
#ifdef MXARLIB_USE_ARKFSRTMD
    keyIndex = (isIndex ? k : keys->getKeyFieldEntry(k)->keyFieldIndex_);
#else
    const SqlmxKeyEntry &e = keyDesc->entry_()[k];
    keyIndex = (isIndex ? k : e.columnNumber);
#endif
    entry->setKeyField((Lng32) keyIndex, (Lng32) k);
  }

  entry->setEncodedKeyLength((Lng32) rcb->getKeyLen());

  // remember if there are any added columns
  if (tupleDesc->addedFieldPresent())
    entry->setHasAddedColumn();

  // for now create an error if we encounter an RFORK
  if (entry->isRFork())
  {
    delete entry;
    entry = NULL;
    return FEINVALOP;
  }

  return FEOK;
}
     
short CliMxArLibCache::normalizeName(
     const char          *guardianName,
     Lng32                guardianNameLen,
     char                *normalizedName,
     Lng32                normalizedNameMaxLen)
{
  short rc;
  short outBufferLen;
  short validNameLen;

  // check and resolve the Guardian name
  rc = FILENAME_SCAN_((char *) guardianName,
		      (short) guardianNameLen,
		      &validNameLen);
  if (rc)
    return rc;
			 
  rc = FILENAME_RESOLVE_((char *) guardianName,
			 validNameLen,
			 normalizedName,
			 (short) normalizedNameMaxLen,
			 &outBufferLen);
  if (rc)
    return rc;

  // add a NUL terminator, note that we do not return the length
  // of the normalized string, the caller needs to find out with str_len
  normalizedName[outBufferLen] = 0;

  return FEOK;
}

short CliMxArLibCache::getBeforeAfterData(
     CliGlobals            *cliGlobals,
     MXARLibAuditParams    *arlibParams,
     const Lng32            *requestBitmap,
     Lng32                  *replyBitmap,
     Lng32                  bitmapLengthInBytes,
     Lng32                  *imageBuffer,
     Lng32                  imageBufferLength,
     Lng32                  *endImageDataOffset,
     Lng32                  *replyHint,
     Lng32                  isAfterData)
{
  short                 retcode = FEOK;
  CliMxArLibCacheEntry  *ent = NULL;
  Lng32                  bitmapLength = bitmapLengthInBytes / 4;

  // Right now we support the first and only version of the MXARLIB
  // interface
  if (arlibParams->version_ != MXARLibCurrentVersion)
    return FEBADPARMVALUE;

  // Make sure the MX version of this DML audit record is
  // supported. For release 2 FCS we support the first and only audit
  // version. The error codes used here were suggested by TMFARLB2
  // development.
  Lng32 actual = arlibParams->sqlmxAuditVersion_;

  if (actual < COM_AUDIT_VERS_OLDEST_SUPPORTED)
    return FEINVALIDVERSION;

  if (actual > COM_AUDIT_VERS_HIGHEST_SUPPORTED)
    return FESERVERVERSIONTOOLOW;

  // Now find the cache entry. This call will read a new entry in from
  // disk if necessary.
  retcode = CliMxArLibCache::getMxArLibCacheEntryByName(
	      cliGlobals,
              arlibParams->guardianName_,
              arlibParams->guardianNameLen_,
              ent);

  // Check the CRVSN if one was passed in and it is not zero. This
  // only works on NSK. On Windows DP2 does not return the CRVSN to
  // us.
  if (retcode == FEOK)
  {
    assert(ent);
  }

  if (retcode == FEOK)
  {
    if (ent->getImageBufferLen() > imageBufferLength)
    {
      if (replyHint)
        *replyHint = ent->getImageBufferLen();
      retcode = FEBUFTOOSMALL;
    }
    else
    {
      ModifiedFieldMap *mfm =
        (ModifiedFieldMap *) arlibParams->modifiedFieldMap_;
      const char * src = arlibParams->auditImage_;
      Lng32 srcLen = arlibParams->auditImageLen_;
        
      if (mfm && arlibParams->encodedKeyLen_ < 0)
      {
        // If the caller sets the encoded key length to -1 that
        // means that the caller doesn't know that length and
        // that we are supposed to calculate the location of the
        // audit image from the key length (obtained from the
        // label which almost certainly matches the audit
        // record's key length) and from the indication whether
        // we need the before or after data. We assume the
        // following layout:
        // |-- encoded key --|-- after image --|-- before image --|
          
        // audit image ptr must point to key buffer in this case
        src = arlibParams->encodedKey_ + ent->getEncodedKeyLength();
        if (isAfterData)
        {
          srcLen = mfm->totlenAfterFrags_;
        }
        else
        {
          src += mfm->totlenAfterFrags_;
          srcLen = mfm->totlenBeforeFrags_;
        }
          
        // check whether the srcLen bytes of buffer starting at
        // src are still within the original audit image that
        // was passed in (auditImageLen_ bytes starting at
        // auditImage_)
        if (src < arlibParams->auditImage_ ||
            (src+srcLen) > (arlibParams->auditImage_ +
                            arlibParams->auditImageLen_))
          return FEBADPARMVALUE; // probably bad mfm

      } // if (mfm && arlibParams->encodedKeyLen_ < 0)
        
      if (mfm && mfm->range_[0].endFldnum_ != -1)
      {
        // We are using a modified field map, and there was no added
        // column at the time the mfm was created. The audit record
        // contains only the columns specified in the mfm, without
        // any VOA fields.
        // (-1 in the first end col indicates that the table had an added
        //  column and that the entire record got included in the audit)
          
        const char *srcLimit = src+srcLen;
          
        if (replyBitmap)
          for (Int32 i=0; i<bitmapLength; i++)
            replyBitmap[i] = 0x0;
          
        // Move field by field as specified by the modified field
        // map. Note that a range in the modified field map does not
        // include all logical columns from start to end. It includes
        // all physical columns, in offset order, from start to end.
        for (Int32 i=0; i<(Int32)mfm->numEntries_ && retcode==FEOK; i++)
        {
          Lng32 start = mfm->range_[i].begFldnum_;
          Lng32 end = mfm->range_[i].endFldnum_;

          retcode = ent->moveFields((char *) imageBuffer,
                                    src, srcLimit,
                                    start, end,
                                    replyBitmap, bitmapLength);
        }
          
        // In the future we may want to un-encode the key value
        // arlibParams->encodedKey_ to get key column
        // values. For now we just make sure that the key
        // columns are always added to the modified field map

      } // if (mfm && mfm->range_[0].endFldnum_ != -1)

      else
      {
        // assume we got an entire record (including VOA) and move
        // all fields
        Lng32 numAddedCols;
        retcode = ent->moveAllFields((char *) imageBuffer,
                                     src,
                                     srcLen,
                                     numAddedCols);
        if (replyBitmap && retcode == FEOK)
        {
          for (Int32 i=0; i<bitmapLength; i++)
            if (i < ent->getBitmapLen())
              replyBitmap[i] = (Lng32) (ent->getBitmapForAllFields()[i]);
            else
              replyBitmap[i] = 0x0;
        }

      } // if (mfm && mfm->range_[0].endFldnum_ != -1) else ...
        
      // For now set endImageDataOffset and replyHint to the
      // values for an entire record. In the future we may
      // calculate these values more accurately based on the
      // fields in the modified field map
      if (endImageDataOffset)
        *endImageDataOffset = ent->getImageBufferLen();
      if (replyHint)
        *replyHint = ent->getNumCols()-1;

    } // if (ent->getImageBufferLen() > imageBufferLength) else ...
  } // if (retcode == FEOK)
  
  return retcode;
}

short CliMxArLibCache::invalidate(const char *guardianName, Lng32 nameLen)
{
  // Convert to a fully qualified name
  char normalizedName[MAX_EXTERNAL_FNAME_LENGTH+1];
  short rc = normalizeName(guardianName,
                           nameLen,
                           normalizedName,
                           MAX_EXTERNAL_FNAME_LENGTH+1);
  if (rc)
  {
    return rc;
  }

  // Remove the entry from the cache
  Int32 normalizedNameLen = str_len(normalizedName);
  removeEntryByName(normalizedName, normalizedNameLen);

  return rc;
}

short CliMxArLibCache::setCacheLimit(ULng32 cacheLimitMB)
{
  if (!cacheLimitSetInStone_)
  {
    cacheLimitBytes_ = cacheLimitMB * 1024 * 1024;
  }
  return FEOK;
}

// -----------------------------------------------------------------------
// Methods for class CliMxArLibRforkData
// -----------------------------------------------------------------------

CliMxArLibRfork::CliMxArLibRfork(const char *guardianName,
                                 NAHeap *heap) :
  objectInfo_(NULL),
  heap_(heap),
  crvsn_(0),
  rcb_(NULL)
{
  Int32 len = str_len(guardianName);
  Int32 bytesToCopy = (len > MAX_EXTERNAL_FNAME_LENGTH ?
                     MAX_EXTERNAL_FNAME_LENGTH : len);
  str_cpy_all(guardianName_, guardianName, bytesToCopy);
  guardianName_[bytesToCopy] = 0;

  ansiName_[0] = 0;
  ansiNameSpace_[0] = 0;
  logicalPartitionName_[0] = 0;
}

CliMxArLibRfork::~CliMxArLibRfork()
{
  if (objectInfo_)
    NADELETEBASIC(objectInfo_, heap_);
}

short CliMxArLibRfork::fetch()
{
  // split Guardian name into parts
  char outBuffer[4 * (FILENAME_SUBPART_LENGTH + 1)];
  char *parts[4];
  Lng32 numParts;
  short rc;

  // split the name into parts
  if ((LateNameInfo::extractParts((char *)guardianName_, outBuffer, numParts, 
				  parts, FALSE)) ||
      (numParts != 4))
  {
    return FEBADPARMVALUE;
  }

  rc = fetchObjectInfo(parts[0], parts[1], parts[2], parts[3]);
  if (rc)
    return rc;

  assert(objectInfo_);
  assert(rcb_);

  return FEOK;
}

ExpTupleDesc *CliMxArLibRfork::getTupleDesc() const
{
  if (rcb_)
    return rcb_->getTupleDesc();
  return NULL;
}

SqlmxKeyDescriptor *CliMxArLibRfork::getKeyDesc() const
{
  SqlmxKeyDescriptor *result = NULL;
  SqlmxRtmdDisplayOutputStruct *rtmd =
    (SqlmxRtmdDisplayOutputStruct *) objectInfo_;

  if (rtmd && (rtmd->keyDesc).length > 0)
    result = (SqlmxKeyDescriptor *) (rtmd->keyDesc).ptr;

  return result;
}

short CliMxArLibRfork::fetchObjectInfo(const char *sysName,
                                       const char *volName,
                                       const char *subvolName,
                                       const char *objectName)
{
  SqlmxRtmdDisplayStruct display;

  // setup eyecatcher, timeout and version.
  str_cpy_all((char *) display.eyeCatcher, SQLMX_EYECATCHER, 2);
  display.version = SQLMX_DISPLAY_STRUCT_VERSION;
  display.timeout = 6000; // units are milliseconds

  // setup lock protocol and state
  display.lockProtocol = MX_BROWSE_ACCESS;
  display.lockState    = MX_SHARE;

  // setup rtmd section flags
  display.MX_FETCH_RCB = 1;
  display.MX_FETCH_KCB = 1;
  display.MX_FETCH_CONSTRAINTS = 0;
  display.MX_FETCH_IXMAPARRAY = 0;
  display.MX_FETCH_PARTARRAY = 0;
  display.MX_FETCH_SECURITY = 0;
  display.MX_FETCH_EXTENTMAPARRAY = 0;
  display.MX_FETCH_TRIGGERARRAY = 0;
  display.MX_FETCH_VERTICALPARTARRAY = 0;
  display.MX_FETCH_LOGICALPARTNAME = 1;
  display.MX_FETCH_PARENT_ANSINAME = 0;
  display.MX_FETCH_ANSI_UIDS = 0;

  // setup table name
  Int32 len;

  len = str_len(sysName);
  str_pad((char *) display.sysName, 8, ' ');
  str_cpy_all((char *) display.sysName, sysName, len > 8 ? 8 : len);

  len = str_len(volName);
  str_pad((char *) display.volName, 8, ' ');
  str_cpy_all((char *) display.volName, volName, len > 8 ? 8 : len);

  len = str_len(subvolName);
  str_pad((char *) display.subvolName, 8, ' ');
  str_cpy_all((char *) display.subvolName, subvolName, len > 8 ? 8 : len);

  len = str_len(objectName);
  str_pad((char *) display.objectName, 8, ' ');
  str_cpy_all((char *) display.objectName, objectName, len > 8 ? 8 : len);

  display.filler1 = 0;
  display.filler2 = 0;
  display.filler3 = 0;
  display.filler4 = 0;
  str_pad((char *) display.filler5, sizeof(display.filler5), 0);

  if (objectInfo_)
  {
    NADELETEBASIC(objectInfo_, heap_);
    objectInfo_ = NULL;
  }

  Int32 outBufLen = 20000;
  objectInfo_ = new (heap_) short[outBufLen];
  display.bufLength = outBufLen;
  display.bufPtr = (void *) &objectInfo_[0];

  short rc = SQLMX_GETOBJECTINFO_(&display);
  
  if (rc)
  {
    if (rc == FEBADCOUNT)
    {
      Lng32 neededBufLen = display.feedback.requiredBufferLength;
      NADELETEBASIC(objectInfo_, heap_);
      objectInfo_ = NULL;
      objectInfo_ = new (heap_) short[neededBufLen];
      display.bufLength = neededBufLen;
      display.bufPtr = (void *) &objectInfo_[0];

      MXARLIB_DEBUG1("[MXARLIB] Retry SQLMX_GETOBJECTINFO_ with %d bytes\n",
                     (Lng32) (neededBufLen * sizeof(short)));

      rc = SQLMX_GETOBJECTINFO_(&display);
    }
  }
  
  SqlmxRtmdDisplayOutputStruct *rtmd =
    (SqlmxRtmdDisplayOutputStruct *) objectInfo_;

  // Notes on version checks
  // - OSV must be compatible with the current software
  // - The plan version in the RCB must be compatible
  // - MXV is the MX software version on the node where the object
  //   resides. We do not check this field.
  // - We are not concerned with OFV. It is typically only used when
  //   a schema is downgraded or when restoring/DUPing objects and 
  //   does not impact our ability to interpret a resource fork or
  //   audit records.


  // Set up the RCB
  if (rc == 0)
  {
    rcb_ = NULL;
    if ((rtmd->recordDesc).length)
      rcb_ = (ExRCB *) ((rtmd->recordDesc).ptr);
    
    if (rcb_ == NULL)
    {
      rc = FENOTFOUND;
    }
    else
    {
      ExRCB dummyRCB;
      rcb_->driveUnpack(rcb_, &dummyRCB, NULL);


    }
  }

  // Copy ANSI names and the CRVSN into this instance
  if (rc == 0)
  {
    Int32 nameLen;
    Int32 bytesToCopy;
    
    nameLen = (rtmd->ansiName).length;
    if (nameLen > 0)
    {
      bytesToCopy = MIN2(nameLen, ANSINAME_SUBPART_LENGTH * 3 + 2);
      str_cpy_all(ansiName_, (char *) (rtmd->ansiName).ptr, bytesToCopy);
      ansiName_[bytesToCopy] = 0;
      
      str_cpy_all(ansiNameSpace_, (char *) rtmd->ansiNameSpace,
                  CLIARL_ANSI_NAME_SPACE_LEN);
      ansiNameSpace_[CLIARL_ANSI_NAME_SPACE_LEN] = 0;
    }
    
    nameLen = (rtmd->logicalPartName).length;
    if (nameLen > 0)
    {
      bytesToCopy = MIN2(nameLen, ANSINAME_SUBPART_LENGTH);
      str_cpy_all(logicalPartitionName_, (char *) (rtmd->logicalPartName).ptr, 
                  bytesToCopy);
      logicalPartitionName_[bytesToCopy] = 0;
    }

    // The 6-byte CRVSN from the resource fork becomes the low order 6
    // bytes of the 64-bit CRVSN for this instance. We assume the
    // incoming value is an array of 3 unsigned 16-bit values, most
    // significant first. This big-endian assumption is safe on NSK,
    // and also harmless on Windows (because we do not get CRVSNs from
    // DP2 on Windows, and actually never even call this function).

    unsigned short *incoming = (unsigned short *) &(rtmd->crvsn[0]);
    const ULng32 shift = 0x10000;  // equal to 2^16
    crvsn_ = incoming[0];                 // put first element in low 16 bits
    crvsn_ *= shift;                      // shift left 16 bits
    crvsn_ += incoming[1];                // put next element in low 16 bits
    crvsn_ *= shift;                      // shift left 16 bits
    crvsn_ += incoming[2];                // put next element in low 16 bits
  }

  return rc;
}
//LCOV_EXCL_STOP
