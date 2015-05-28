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
/* -*-C++-*-
****************************************************************************
*
* File:         ExRCB.cpp
* Description:  
*
* Created:      7/10/95
* Language:     C++
*
*
****************************************************************************
*/

/*
#include "hfs2rec"
#ifndef HFS2REC_INCLUDED
#define HFS2REC_INCLUDED

#ifndef FS2_RECORD_DEFINED
#define FS2_RECORD_DEFINED
*/

#define READTABLEDEF_IMPLEMENTATION  // kludge to define dfs2rec REC_BYTE_*_UP
#include "dfs2rec.h"

#include "ex_rcb.h"
#include "comrcb.h"
#include "ex_queue.h"
#pragma nowarn(260)  // warning elimination. To hide a warning from hcomslab
#include "hcomslab"
#pragma warn(260)    // warning elimination
#include "exp_attrs.h"

/*
#endif
#endif
*/

// to covert a MP record struct to ExRCB, called by filesystem.
// the rcb ptr is returned.
// sol 10-050324-5951 
// The memory cleanup code - usage of NADELETEBASIC, NADELETE is wrong in the 
// method for tuple_desc, attrsArray, colNameList. 
// Hence on error conditions in the method below, complete memory cleanup 
// is not done  this is cleaed up at a higher level in statement.cpp after 
// the requested  information is processed.
void * ExRcbFromMpLabelInfo (void * recordPtr, void * sqlLabel,
			      short entry_seq,
			      CollHeap *heap)
{
  rec_record_struct *rec = (rec_record_struct *)recordPtr;
  Int32 * defaultArea = (Int32 *)&rec->field[rec->num_entries];

  AttributesPtr * attrsArray = (AttributesPtr *) (new (heap) 
                               ( char[rec->num_entries * sizeof(AttributesPtr)]));

  Attributes::DefaultClass defclass;


  SLAB_External_Struct slab_external;

  Int32   slab_external_addr = (int_32)&slab_external;
  Int32   black_box_addr;
  Int16   black_box_size;
  Int32 error=0;

  Int32   la_charsetname_addr;
  Int16   la_charsetname_len;

  // SLAB_* functions (shared code) are used to unpack the sqllabel.
  SLAB_Init_External_Slab (slab_external_addr);
  error = SLAB_Unpack((int_32)sqlLabel, slab_external_addr);
  if (error != 0)
    {
      NADELETEBASIC(attrsArray,heap);
      return 0;
    };
#pragma nowarn(252)   // warning elimination
  error = SLAB_Get_Black_Box(slab_external_addr
                             ,SLAB_Char_Set_List
                             ,&black_box_addr
                             ,&black_box_size);
#pragma warn(252)  // warning elimination
  if (error != 0)
    {
      NADELETEBASIC(attrsArray,heap);
      return 0;
    };


  // build attributes entries based on the MP fields entries.
  Int32 i = 0;
  for (; i < rec->num_entries; i++)
    {
      rec_field_struct *field = &rec->field[i];

      switch ((short)field->default_offset)
	{
	case REC_SYSTEM_DEFAULT:
	  if (field->type == REC_DATETIME )
	    {
	      defclass =  Attributes::DEFAULT_CURRENT;
	    }
	  else
	    defclass = Attributes::DEFAULT_USER;
	  break;
	case REC_NO_DEFAULT:
	  defclass = Attributes::NO_DEFAULT;
	  break;
	case REC_NULL_DEFAULT:
	  defclass =  Attributes::DEFAULT_NULL;
	  break;
	default:
	  defclass = Attributes::DEFAULT_USER;
	  break;
	};

      NABoolean upshift = FALSE;
      short datatype = field->type;
      if (datatype == REC_BYTE_V_ASCII_UP ||  
      	  datatype == REC_BYTE_F_ASCII_UP)
	{
	  upshift = TRUE;

	  // convert the upshifted datatypes to regular char datatypes.
	  // The 'upshift' attribute is saved as a separate field in the
	  // SimpleType class, which is constructed in this method.
	  if (datatype == REC_BYTE_V_ASCII_UP)
	    datatype = REC_BYTE_V_ASCII;
	  else
	    datatype = REC_BYTE_F_ASCII;
	}

      short nullIndicatorLen = ((field->flags & REC_FIELD_NULLABLE) ? 2 : 0);
      short vcIndicatorLen = ((field->type == REC_BYTE_V_ASCII_UP || field->type == REC_BYTE_V_ASCII) ? 2 : 0);

      if (datatype == REC_DATETIME)
	{
	  // change the field attrs to correspond to the way catman
	  // sets them up in ReadTableDef.cpp::convertTreeMPtoARK.
	  // This is for sql/mp datetime fraction field support.
	  // The case of sql/mp DATETIME FRACTION to FRACTION is not
	  // supported in sql/mx. It is treated as a CHAR col.
	  // Leave that case as is since readtabledef doesn't change
	  // it either.
	  if ((field->getBeginType() != REC_DATE_FRACTION_MP) &&
	      (field->getEndType() == REC_DATE_FRACTION_MP))
	    field->setEndType(REC_DATE_SECOND);
	}

#pragma nowarn(1506)   // warning elimination 

      short precision;
      if (DFS2REC::isInterval(datatype))
	// get leading precision.
	// TBD: add a method to rec_field_struct class to get this value.
	precision = field->len_etc.interval_.lprecision;
      else if (DFS2REC::isFloat(datatype))
	// set up precision of tandem floats(note that this check
	// covers both ieee and tandem floats but this method we are currently
	// in is only called to convert sql/mp table label info)
	// the way they are set in ReadTableDef.cpp::convertTreeMPtoARK.
	precision = 0;
      else 
	precision = (short)field->getPrecision();

      Attributes * attr = attrsArray[i] = new (heap)
	SimpleType(datatype, 
		   (Lng32)field->getLength(),
		   (short)field->getScale(),
		   precision,
		   ExpTupleDesc::SQLMP_FORMAT,
		   0, /* alignment */
		   ((field->flags & REC_FIELD_NULLABLE) ? 1 : 0),
		   nullIndicatorLen,
		   vcIndicatorLen,
		   defclass,
		   upshift
		  );
#pragma warn(1506)  // warning elimination 

      if (DFS2REC::isAnyCharacter(datatype))
	{
          // fix 10-040720-8031 (Similarity check fails if an MP table has NCHAR column).
          //
          // get the charset name first
#pragma nowarn(1506)   // warning elimination
#pragma nowarn(252)   // warning elimination 
          error = SLAB_Read_Char_Set_Name ( black_box_addr
                                    , i
                                    , &la_charsetname_addr
                                    , &la_charsetname_len
                                    );
#pragma warn(1506)  // warning elimination
#pragma warn(252)  // warning elimination
          if (error!=0) {
             NADELETEBASIC(attrsArray,heap);
             return NULL;
          }

          char * charsetname = new (heap) char [la_charsetname_len + 1];
          memcpy (charsetname, (void *)la_charsetname_addr, la_charsetname_len);
          charsetname[la_charsetname_len] = '\0';

          // translate the name to the enum form
          CharInfo::CharSet cs = CharInfo::getCharSetEnum(charsetname);
          NADELETEBASIC(charsetname,heap);

          switch (cs) {
            case CharInfo::UnknownCharSet: // unknown in MP === ISO88591 in MX
               ((SimpleType*)attr)->setCharSet(CharInfo::ISO88591);
               break;
            default:
               if ( CharInfo::isCharSetSupported(cs) == FALSE ) {
                 NADELETEBASIC(attrsArray,heap);
                 return NULL;
               } else {
                  // set the charset if it is supported by MX
                  ((SimpleType*)attr)->setCharSet(cs);
               }
               break;
          }
	}

      if (defclass == Attributes::DEFAULT_USER)
	{
	  // set up default value
	  char * defval = new (heap) char [field->getLength() + nullIndicatorLen + vcIndicatorLen];

	  if (((short)field->default_offset) == REC_SYSTEM_DEFAULT)
	    { 
	      // system default
	      if (((short)field->type >= REC_MIN_CHARACTER) && 
		  ((short)field->type <= REC_MAX_CHARACTER))
		{
		  str_pad( defval, nullIndicatorLen+vcIndicatorLen, 0);
		  str_pad( &defval[nullIndicatorLen+vcIndicatorLen], 
			   field->getLength(), 
			   (vcIndicatorLen > 0 ? 0 : ' '));
		}
	      else if (((short)field->type >= REC_MIN_DECIMAL) && 
		       ((short)field->type <= REC_MAX_DECIMAL))
		{
		  str_pad( defval, nullIndicatorLen+vcIndicatorLen, 0);
		  str_pad( &defval[nullIndicatorLen+vcIndicatorLen], 
			   field->getLength(), '0');
		}
	      else  //numeric
		str_pad( defval, (field->getLength() + nullIndicatorLen), 0);
	    }
	  else
	    { 
	      // user supplied default
	      char * userdef = 
		(char *)((Int32)defaultArea + field->default_offset);
#pragma nowarn(1506)   // warning elimination 
	      Int16 defaultLen = field->getLength();
#pragma warn(1506)  // warning elimination 

	      str_pad( defval, nullIndicatorLen, 0);

	      if (((short)field->type >= REC_MIN_CHARACTER) && 
		  ((short)field->type <= REC_MAX_CHARACTER))
		{
		  memcpy((char *)&defaultLen, userdef, sizeof(short));
		  if (DFS2REC::isSQLVarChar(field->type))
		    {
		      str_pad( &defval[nullIndicatorLen+vcIndicatorLen], 
			       field->getLength(), 0);

		      memcpy( &defval[nullIndicatorLen], 
			      userdef, defaultLen+vcIndicatorLen);
		    }
		  else
		    {
		      str_pad( &defval[nullIndicatorLen], 
			       field->getLength(), ' ');

		      memcpy( &defval[nullIndicatorLen], 
			      &userdef[sizeof(short)],
			      defaultLen);
		    }
		}
	      else
		memcpy (&defval[nullIndicatorLen], userdef, defaultLen);	  
	    }

	  attr->setDefaultValue(defclass, defval);
	}

    }

  // now build the tuple_desc
  ExpTupleDesc *tuple_desc = new (heap)
                      ExpTupleDesc( rec->num_entries,
				    attrsArray,
				    rec->max_reclen,
				    ExpTupleDesc::SQLMP_FORMAT,
				    ExpTupleDesc::LONG_FORMAT
				    );

  // build the column name list from sqlLabel

  Queue * colNameList = new (heap) Queue (heap);

  Int32   la_colname_addr;
  Int16   la_colnamelen;

#pragma nowarn(252)  // warning elimination
  error = SLAB_Get_Black_Box(slab_external_addr
                             ,SLAB_Column_Names_List
                             ,&black_box_addr
                             ,&black_box_size);
#pragma warn(252)  // warning elimination
  if (error != 0)
    {
      NADELETE(tuple_desc,ExpTupleDesc,heap);
      NADELETEBASIC(attrsArray,heap);
      NADELETE(colNameList,Queue,heap);
      return 0;
    };

  for (i = 0 ; i < rec->num_entries; i++)
    {
#pragma nowarn(1506)   // warning elimination
#pragma nowarn(252)   // warning elimination 
      error = SLAB_Read_Column_Name ( black_box_addr 
				    , i
				    , &la_colname_addr
				    , &la_colnamelen
				    );
#pragma warn(1506)  // warning elimination 
#pragma warn(252)  // warning elimination 
      if (error!=0)
	{
	  NADELETE(tuple_desc,ExpTupleDesc,heap);
	  NADELETEBASIC(attrsArray,heap);
	  NADELETE(colNameList,Queue,heap);
	  return NULL;
	};
      char * colname = new (heap) char [la_colnamelen + 1];
      memcpy (colname, (void *)la_colname_addr, la_colnamelen);
      colname[la_colnamelen] = '\0'; 
      colNameList->insert( colname );
    };

  // next build the ExRCB
  ExRCB *rcb = new (heap) ExRCB();
  ULng32 total_len;

  rcb->initialize( NULL,                 // object_name
		   entry_seq,
		   0,                    // object_namelen
		   total_len,            // total_len
		   tuple_desc,
		   rec->max_reclen,
		   NULL,                 // key_encode_expr
		   0,                    // key_len
		   NULL,                 // partkey_encode_expr
		   0,                    // partkey_len
		   colNameList,
		   TRUE,                 // sqlmp_format
		   NULL                  // space
		 );
  return rcb;

}

short getExRcbSize()
{
  return sizeof (ExRCB);
};


// Do not propagate these dfs2rec symbols to rest of ExAll.cpp (NSK make)
#undef REC_BYTE_F_ASCII_UP
#undef REC_BYTE_V_ASCII_UP
