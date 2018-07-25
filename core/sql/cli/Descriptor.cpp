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
 * File:         Descriptor.cpp
 * Description:  Procedures to handle descriptors.
 *
 *
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"


#ifdef _DEBUG
#include <assert.h>
#endif

//try to remove the following line later on
#include <stdio.h>
#include "str.h"
#include "ComSizeDefs.h"
#include "ComRtUtils.h"
#include "charinfo.h"
#include "cli_stdh.h"
#include "Descriptor.h"
#include "dfs2rec.h"
#include "sql_id.h"
#include "exp_clause_derived.h"

// WARNING: assuming varchar length indicator of sizeof(long) == 4
#define VCPREFIX_LEN sizeof(Int32)

// extern declaration
extern short
convertTypeToText_basic(char * text,
                        Lng32 fs_datatype,
                        Lng32 length,
                        Lng32 precision,
                        Lng32 scale,
                        rec_datetime_field datetimestart,
                        rec_datetime_field datetimeend,
                        short datetimefractprec,
                        short intervalleadingprec,
                        short upshift,
			short caseinsensitive,
                        CharInfo::CharSet charSet,
                        const char * collation_name,
                        const char * displaydatatype,
			short displayCaseSpecific);

// The length of an indicator host variable depends on its type
Lng32 Descriptor::setIndLength(desc_struct &descItem)
{
  switch (descItem.ind_datatype)
    {
    case REC_BIN16_SIGNED:
    case REC_BIN16_UNSIGNED:
      descItem.ind_length = 2;
      break;
    case REC_BIN32_SIGNED:
    case REC_BIN32_UNSIGNED:
      descItem.ind_length = 4;
      break;
    case REC_BIN64_SIGNED:
    case REC_BIN64_UNSIGNED:
      descItem.ind_length = 8;
      break;
    default:
      descItem.ind_length = 0;
    }
  return descItem.ind_length;
}

Lng32 Descriptor::setVarLength(desc_struct &descItem)
{
  switch (descItem.datatype)
    {
    case REC_BIN16_SIGNED:
    case REC_BIN16_UNSIGNED:
      descItem.length = 2;
      break;
    case REC_BIN32_SIGNED:
    case REC_BIN32_UNSIGNED:
    case REC_FLOAT32:
      descItem.length = 4;
      break;
    case REC_BIN64_SIGNED:
    case REC_BIN64_UNSIGNED:
    case REC_FLOAT64:
      descItem.length = 8;
      break;
    default:
      descItem.length = 0;
    }
  return descItem.length;
}

Descriptor::Descriptor(SQLDESC_ID * descriptor_id_,
		       Lng32 max_entries_,
		       ContextCli *context) : context_(context)
{
  ContextCli   & currContext = *context;
  NAHeap       & heap        = *(currContext.exHeap());

  init_SQLCLI_OBJ_ID(&descriptor_id);
  init_SQLMODULE_ID(&module);

  descriptor_id.module = &module;

  descriptor_id.identifier = 0;
  descriptor_id.handle = descriptor_id_->handle;
  descriptor_id.name_mode = descriptor_id_->name_mode;

  switch (descriptor_id.name_mode)
  {
  case desc_name:
    {
      Lng32 module_nm_len = getModNameLen(descriptor_id_->module);

      // use of a descriptor name doesn't necessitate use of a module name.
      if ( module_nm_len > 0 ) 
	{
	  char * modname = (char *) heap.allocateMemory(module_nm_len + 1);

	  str_cpy_all(modname,
		      descriptor_id_->module->module_name, module_nm_len);

	  modname[module_nm_len] = 0;
	  module.module_name = modname;
	}
      else
	{
	  module.module_name = 0;
	}

      module.module_name_len = module_nm_len;

      Lng32 identifier_len = getIdLen(descriptor_id_);

      char * id = (char *)heap.allocateMemory(identifier_len + 1);

      str_cpy_all(id, descriptor_id_->identifier, identifier_len);
      id[identifier_len] = 0;
      descriptor_id.identifier = id;
      descriptor_id.identifier_len = identifier_len;
    }
  break;

  case desc_handle:
    {
      descriptor_id.handle = (void*)context_->getNextDescriptorHandle();
    }
    break;

  case desc_via_desc:
    // the following implementation does 'early' binding of the name.
    // This is the correct implementation, in that we want to get the
    // name currently in the descriptor, not what may be there at a
    // later time.  If we search for this descriptor again, we can use the
    // name of a descriptor (bound to a host variable containing the
    // name); it can and probably will be the same descriptor that
    // we allocated with...

    {
    SQLDESC_ID tmpDescId;
    init_SQLCLI_OBJ_ID(&tmpDescId);

    tmpDescId.name_mode  = desc_via_desc;
    tmpDescId.identifier = descriptor_id_->identifier;
    tmpDescId.identifier_len = getIdLen(descriptor_id_);
    tmpDescId.module     = descriptor_id_->module;


    // get the name via the descriptor
    //   NOTE: Descriptor::GetNameViaDesc() allocates the returned string
    //         from context->exHeap()

    SQLCLI_OBJ_ID* id = (SQLCLI_OBJ_ID *)Descriptor::GetNameViaDesc(&tmpDescId,context,heap);
    // fix memory leak (genesis case 10-981230-3244) 
    // caused by not freeing a non-null id.

    if (id) {
      // transfer id->identifier's ownership from id to descriptor_id
       char * id2 = (char *)
	 heap.allocateMemory(id->identifier_len + 1);

       str_cpy_all(id2,id->identifier,
                  id->identifier_len);
       id2[id->identifier_len] = 0;
       descriptor_id.identifier = id2;
       descriptor_id.identifier_len = id -> identifier_len;
      // id is now useless; free it.
      heap.deallocateMemory(id);
    }

    if (descriptor_id.identifier == 0)
    {
        // error
    }

    Lng32 module_nm_len = getModNameLen(descriptor_id_->module);

    char * mn = (char*) heap.allocateMemory(module_nm_len + 1);
    str_cpy_all(mn,
           descriptor_id_->module->module_name, module_nm_len);
    mn[module_nm_len] = 0;
    module.module_name = mn;
    module.module_name_len = module_nm_len;

    // now that the name is set... we don't look in the descriptor
    // again.  The name_mode is now *_name *NOT* *_via_desc.

    descriptor_id.name_mode = desc_name;
    }
    break;

  default:
      // error
      break;
  }

  dyn_alloc_flag     = -1;
  rowsetSize         = 0;
  rowsetStatusPtr    = 0;
  rowsetNumProcessed = 0;
  rowsetHandle       = 0;
  compoundStmtsInfo_ = 0;
  rowwiseRowsetSize  = 0;

  max_entries        = max_entries_;
  used_entries       = 0;
  desc               = 0;

  bmInfo_            = NULL;
  bulkMoveStmt_      = NULL;
  flags_             = 0;
  reComputeBulkMoveInfo();

  str_pad(filler_, sizeof(filler_), '\0');
}

Descriptor::~Descriptor()
{
  NAHeap & heap = *(context_->exHeap());

  if (descriptor_id.module->module_name)
    {
    heap.deallocateMemory((char*)descriptor_id.module->module_name);
    SQLMODULE_ID * m = (SQLMODULE_ID*)descriptor_id.module;
    m->module_name = 0;
    }

  if (descriptor_id.identifier)
    {
    heap.deallocateMemory((char*)descriptor_id.identifier);
    descriptor_id.identifier = 0;
    }

  if (desc) dealloc();
  deallocBulkMoveInfo();
}

NABoolean Descriptor::operator ==(Descriptor &other)
{
  if (getUsedEntryCount() != other.getUsedEntryCount())
    return FALSE;

  Int32 i = 0;
  while (i < getUsedEntryCount())
    {
      desc_struct &descItem = desc[i];
      desc_struct &otherDescItem = other.desc[i];

      if ((descItem.datatype != otherDescItem.datatype) ||
	  (descItem.datetime_code != otherDescItem.datetime_code) ||
	  (descItem.length != otherDescItem.length) ||
	  (descItem.nullable != otherDescItem.nullable) ||
	  (descItem.charset != otherDescItem.charset) ||
	  (descItem.scale != otherDescItem.scale) ||
	  (descItem.precision != otherDescItem.precision) ||
	  (descItem.int_leading_precision != otherDescItem.int_leading_precision) ||
	  (descItem.output_name && (! otherDescItem.output_name)) ||
	  ((!descItem.output_name) && otherDescItem.output_name) ||
	  (descItem.output_name && otherDescItem.output_name &&
	   ((*(Lng32*)descItem.output_name != *(Lng32*)otherDescItem.output_name) ||
	    (memcmp(&(descItem.output_name[sizeof(Lng32)]), &(otherDescItem.output_name[sizeof(Lng32)]),
		    *(Lng32*)descItem.output_name) != 0))) ||
	  (descItem.heading && (! otherDescItem.heading)) ||
	  ((!descItem.heading) && otherDescItem.heading) ||
	  (descItem.heading && otherDescItem.heading &&
	   ((*(Lng32*)descItem.heading != *(Lng32*)otherDescItem.heading) ||
	    (memcmp(&(descItem.heading[sizeof(Lng32)]), &(otherDescItem.heading[sizeof(Lng32)]),
		    *(Lng32*)descItem.heading) != 0))) ||
	  (descItem.generated_output_name != otherDescItem.generated_output_name))
	return FALSE;

      i++;
    }

  return TRUE;
}

char *Descriptor::getVarItem(desc_struct &descItem, Lng32 idxrow)
{
  char *ptr;

  if (descItem.var_data)
    ptr = descItem.var_data + VCPREFIX_LEN;
  else
  {
    ptr = (char *)descItem.var_ptr;

    // The size for rowset SQLVarChars is the sum of  
    // (length of the val part) and (length of the len part) 
    if (descItem.rowsetVarLayoutSize > 0) {
      if ( descItem.datatype == REC_BYTE_V_ASCII) {
	// special case for COBOL VARCHARs, the length parts
	// have to be aligned.
	if ((descItem.rowsetVarLayoutSize % 2) == 0) {
	  ptr += idxrow * (descItem.rowsetVarLayoutSize + descItem.vc_ind_length);
	}
	else {
	  ptr += idxrow * (descItem.rowsetVarLayoutSize + 1 + descItem.vc_ind_length);
	}
      }
      else if (DFS2REC::isSQLVarChar(descItem.datatype)) {
	  ptr += idxrow * (descItem.rowsetVarLayoutSize + descItem.vc_ind_length);
      }
      else if (descItem.datatype == REC_BLOB || descItem.datatype == REC_CLOB) {
          ptr += idxrow * (descItem.rowsetVarLayoutSize + 4);
      }
      else {
	ptr += idxrow * descItem.rowsetVarLayoutSize;
      }
    }
  } 
  return ptr;
}

char *Descriptor::getIndItem(desc_struct &descItem, Lng32 idxrow)
{
  char *ptr;
  
  if (descItem.ind_data)
    ptr = (char *)&descItem.ind_data;
  else
  {
    ptr = (char *)descItem.ind_ptr;
    if (descItem.rowsetVarLayoutSize > 0)
        ptr += idxrow * descItem.rowsetIndLayoutSize;
  }
  return ptr;
}

char *Descriptor::getVarData(Lng32 entry)
{
  assert(entry >= 1 && entry <= used_entries);

  return getVarItem(desc[entry - 1], rowsetHandle); // Zero Base
}

char *Descriptor::getVarData(Lng32 entry, Lng32 idxrow)
{
  assert(entry  >= 1 && entry  <= used_entries);
  assert(idxrow >= 1 && idxrow <= rowsetSize);

  return getVarItem(desc[entry - 1], idxrow -1); // Zero Base
}

char *Descriptor::getIndData(Lng32 entry)
{
  assert(entry >= 1 && entry <= used_entries);

  return getIndItem(desc[entry - 1], rowsetHandle); // Zero Base
}

char *Descriptor::getIndData(Lng32 entry, Lng32 idxrow)
{
  assert(entry  >= 1 && entry  <= used_entries);
  assert(idxrow >= 1 && idxrow <= rowsetSize);

  return getIndItem(desc[entry - 1], idxrow -1); // Zero Base
}

Int32 Descriptor::getVarDataLength(Lng32 entry)
{
  register Lng32 entryZB = entry - 1;
  return desc[entryZB].length;
}

Int32 Descriptor::getVarIndicatorLength(Lng32 entry)
{
  register Lng32 entryZB = entry - 1;
  return desc[entryZB].vc_ind_length;
}

Int32 Descriptor::getIndLength(Lng32 entry)
{
  register Lng32 entryZB = entry - 1;
  return desc[entryZB].ind_length;
}

const char* Descriptor::getVarDataCharSet(Lng32 entry)
{
  register Lng32 entryZB = entry - 1;
  return CharInfo::getCharSetName((CharInfo::CharSet)desc[entryZB].charset);
}

Lng32 Descriptor::ansiTypeFromFSType(Lng32 datatype)
{
  return getAnsiTypeFromFSType(datatype);
}

const char * Descriptor::ansiTypeStrFromFSType(Lng32 datatype)
{
  return getAnsiTypeStrFromFSType(datatype);
}

Lng32 Descriptor::datetimeIntCodeFromTypePrec(Lng32 datatype, Lng32 precision)
{
  if (datatype == _SQLDT_DATETIME)
    {
      return precision;
    }
  else if (isIntervalFSType(datatype))
    return getDatetimeCodeFromFSType(datatype);
  else
    return 0; // not a defined value, indicates not a DT/INT
}

short Descriptor::isIntervalFSType(Lng32 datatype)
{
   return ((datatype >= REC_MIN_INTERVAL) && (datatype <= REC_MAX_INTERVAL));
}

static
void setVCLength(char * tgt, Lng32 len, size_t vcPrefixLength)
{
   if (!tgt)
   {
#ifdef _DEBUG
       // TBD: better error handling
       fprintf(stderr, "setVCLength: tgt invalid\n");
       fflush(stderr);
#else
       assert(0);
#endif
       return;
   }

   if (vcPrefixLength == sizeof(Int32))
   {
      str_cpy_all(tgt, (char *)&len, sizeof(Int32));
   }
   else if (vcPrefixLength == sizeof(short))
   {
     assert(len <= USHRT_MAX);
     unsigned short temp = (unsigned short)len;
      // this handles big vs. little endian, etc...
      str_cpy_all(tgt, (char *)&temp, sizeof(short));
   }
   else
   {
      // error?  should be either 2 or 4 bytes...
   }
}

static
Lng32 getVCLength(const char * source_string, size_t vcPrefixLength)
{
   Lng32 returned_len = 0L;

   if (!source_string)
   {
       // a source string that is NULL is considered a string with length 0
       return 0L;
   }

   if (vcPrefixLength == sizeof(Int32))
   {
      str_cpy_all((char *)&returned_len, source_string, sizeof(Int32));
   }
   else if (vcPrefixLength == sizeof(short))
   {
      unsigned short temp;

      str_cpy_all((char *)&temp, source_string, sizeof(short));

      returned_len = temp;
   }
   else
   {
      // error?  should be either 2 or 4 bytes...
   }
   return returned_len;
}

static
Lng32 desc_set_string_value_from_varchar(char * string_value,
                                   Lng32   max_string_len,
                                   char * source_string)
{
   Lng32 returned_len;

   if (!string_value)
     {
#ifdef _DEBUG
       // TBD: better error handling
       fprintf(stderr, "desc_set_string_value: string_value invalid\n");
       fflush(stderr);
#else
       assert(0);
#endif
       returned_len = max_string_len;
     }
   else if (!source_string)
     {
       memset(string_value, 0, max_string_len);
       returned_len = max_string_len;
     }
   else
     {
       returned_len = getVCLength(source_string, VCPREFIX_LEN);

       if (max_string_len < returned_len)
          returned_len = max_string_len;
       str_cpy_all(string_value, source_string+VCPREFIX_LEN, returned_len);
      // How does the caller know that there is more data to retrive ?
     }

   return returned_len;
}

//
// this function sets the content for an ASCII CHAR host variable.
// 
// source: the content to be passed into the host variable. It is
// organized by a length field and followed by the data.
//
// host_var_buf and host_var_buf_size: host var
//
// returned_len: actually bytes wriiten to the host var
//
// info_desc and info_desc_index: describing the host variable in
// details (such as its type).
//
static RETCODE
setCharHostVar(char* source,
	       char * host_var_buf, Lng32 host_var_buf_size, 
	       Lng32 * returned_len,
	       Descriptor* info_desc = 0, Int32 info_desc_index = 0
              )
{
      Lng32 target_type;

      if ( info_desc == 0  ||
           info_desc->getDescItem(info_desc_index+1,
                                    SQLDESC_TYPE_FS,
                                    &target_type,
                                    NULL, 0, NULL, 0) < 0
         )
      {
// assume fixed ASCII format
         target_type = REC_BYTE_F_ASCII;
      }

// If the host variable's type is not char, raise an exception.
      //##NCHAR: Shouldn't this be
      //		if (! DFS2REC::isAnyCharacter(target_type))	?##
      if ( target_type != REC_BYTE_F_ASCII &&
           target_type != REC_BYTE_V_ASCII  &&
           target_type != REC_BYTE_V_ANSI
         )
      {
         return ERROR;
      }

      char* target = host_var_buf;
      Lng32 target_len = host_var_buf_size;

// The source type is hard-wired!
      Lng32 source_type = REC_BYTE_F_ASCII;
      Lng32 source_len = getVCLength(source, VCPREFIX_LEN);

      *returned_len = target_len;

      if (target)
      {
         if (source == 0)
         {
           if ( target_type == REC_BYTE_V_ANSI )
              target[0] = 0;
           else 
              memset(target, ' ', target_len);
         }
         else {
   
            if (target_len > source_len )
               *returned_len = source_len;
   
            ex_expr::exp_return_type expRetcode = 
   	    convDoIt(source+VCPREFIX_LEN, source_len, (short)source_type, 0, 0,
                        target, target_len, (short)target_type, 0, 0,
                        NULL, 0);
   
            if (expRetcode != ex_expr::EXPR_OK)
              return ERROR;
   
         }
      }
   return SUCCESS;
}

RETCODE Descriptor::getDescItemPtr(Lng32 entry, Lng32 what_to_get,
                                   char **string_ptr, Lng32 *returned_len)
{
  desc_struct &descItem = desc[entry - 1]; // Zero base
  *returned_len = 0;

  NAHeap *heap = context_->exHeap();
  ComDiagsArea & diags = context_->diags();

  switch(what_to_get)
    {
     case SQLDESC_CHAR_SET_NAM:
	if (descItem.charset_name)
          {
            *string_ptr = descItem.charset_name+VCPREFIX_LEN;
            *returned_len = getVCLength(descItem.charset_name, VCPREFIX_LEN);
          }
        else
          *string_ptr = NULL;
        break;

      case SQLDESC_CHAR_SET_CAT:
        if (descItem.charset_catalog) 
          {
            *string_ptr = descItem.charset_catalog+VCPREFIX_LEN;
            *returned_len = getVCLength(descItem.charset_catalog, VCPREFIX_LEN);
          }
        else
          *string_ptr = NULL;
        break;

      case SQLDESC_CHAR_SET_SCH:
        if (descItem.charset_schema)
          {
            *string_ptr = descItem.charset_schema+VCPREFIX_LEN;
            *returned_len = getVCLength(descItem.charset_schema, VCPREFIX_LEN);
          }
        else
          *string_ptr = NULL;
        break;

      case SQLDESC_COLL_NAM:
        if (descItem.coll_name)
          {
            *string_ptr = descItem.coll_name+VCPREFIX_LEN;
            *returned_len = getVCLength(descItem.coll_name, VCPREFIX_LEN);
          }
        else
          *string_ptr = NULL;
        break;

      case SQLDESC_COLL_CAT:
	if (descItem.coll_catalog)
          {
            *string_ptr = descItem.coll_catalog+VCPREFIX_LEN;
            *returned_len = getVCLength(descItem.coll_catalog, VCPREFIX_LEN);
          }
        else
          *string_ptr = NULL;
        break;

      case SQLDESC_COLL_SCH:
        if (descItem.coll_schema)
          {
            *string_ptr = descItem.coll_schema+VCPREFIX_LEN;
            *returned_len = getVCLength(descItem.coll_schema, VCPREFIX_LEN);
          }
        else
          *string_ptr = NULL;
        break;


      case SQLDESC_NAME:
        if (descItem.output_name)
          {
            *string_ptr = descItem.output_name+VCPREFIX_LEN;
            *returned_len = getVCLength(descItem.output_name, VCPREFIX_LEN);
          }
        else
          *string_ptr = NULL;
        break;

      case SQLDESC_HEADING:
        if (descItem.heading)
          {
            *string_ptr = descItem.heading+VCPREFIX_LEN;
            *returned_len = getVCLength(descItem.heading, VCPREFIX_LEN);
          }
        else
          *string_ptr = NULL;
        break;

      case SQLDESC_VAR_DATA:
        if (descItem.var_data)
          {
            *string_ptr = descItem.var_data+VCPREFIX_LEN;
            *returned_len = getVCLength(descItem.var_data, VCPREFIX_LEN);
          }
        else
          *string_ptr = NULL;
        break;
    case SQLDESC_TABLE_NAME:
        if (descItem.table_name)
          {
            *string_ptr = descItem.table_name+VCPREFIX_LEN;
            *returned_len = getVCLength(descItem.table_name, VCPREFIX_LEN);
          }
        else
          *string_ptr = NULL;
        break;
    case SQLDESC_SCHEMA_NAME:
        if (descItem.schema_name)
          {
            *string_ptr = descItem.schema_name+VCPREFIX_LEN;
            *returned_len = getVCLength(descItem.schema_name, VCPREFIX_LEN);
          }
        else
          *string_ptr = NULL;
        break;
    case SQLDESC_CATALOG_NAME:
        if (descItem.catalog_name)
          {
            *string_ptr = descItem.catalog_name+VCPREFIX_LEN;
            *returned_len = getVCLength(descItem.catalog_name, VCPREFIX_LEN);
          }
        else
          *string_ptr = NULL;
        break;

    case SQLDESC_TEXT_FORMAT:
        if (! descItem.text_format)
          set_text_format(descItem);

        if (descItem.text_format)
          {
            *string_ptr = descItem.text_format+VCPREFIX_LEN;
            *returned_len = getVCLength(descItem.text_format, VCPREFIX_LEN);
          }
        else 
          *string_ptr = NULL;

        break;

      default:
        {
          diags << DgSqlCode(-CLI_INTERNAL_ERROR);
          return ERROR;
        }
        break;
    }

  return SUCCESS;
}

/*
static NABoolean unicodeDataType (long type)
{
  NABoolean unicode = FALSE;

  switch (type)
    {
    case REC_BYTE_F_DOUBLE:
    case REC_BYTE_V_ANSI_DOUBLE:
    case REC_BYTE_V_DOUBLE:
      unicode = TRUE;
      break;

    default :
     break;
    }

  return (unicode);
}
*/

RETCODE Descriptor::getDescItem(Lng32 entry, Lng32 what_to_get,
                                void * numeric_value, char * string_value,
                                Lng32 max_string_len, Lng32 * returned_len,
				Lng32 /*start_from_offset*/,
				Descriptor* info_desc, Int32 info_desc_index
			        )
{
  desc_struct &descItem = desc[entry - 1]; // Zero base
  Lng32        idxrow = rowsetHandle;

  switch (what_to_get)
    {
    case SQLDESC_TYPE:
      *(Int32 *)numeric_value = ansiTypeFromFSType(descItem.datatype);
      // In case the flags bit 1 is set, then we set it in SetDescItem to 
      // indicate that this is really a numeric type.
      if (descItem.desc_flags & descItem.IS_NUMERIC)
	*(Int32 *)numeric_value = SQLTYPECODE_NUMERIC;
      else if (descItem.desc_flags & descItem.IS_NUMERIC_UNSIGNED)
	*(Int32 *)numeric_value = SQLTYPECODE_NUMERIC_UNSIGNED;
      
      break;

    case SQLDESC_DATETIME_CODE:
#if 0
      *(Int32 *)numeric_value = descItem.datetime_code;
#else
      // assuming that type is correctly set...
      // if not, the datetime code is probably not right either
      if (isIntervalFSType(descItem.datatype))
	{
          *(Int32 *)numeric_value = getDatetimeCodeFromFSType(descItem.datatype);
	}
      else
	{
          *(Int32 *)numeric_value = descItem.datetime_code;
	}
#endif
      break;

    case SQLDESC_TYPE_FS:
      *(Int32 *)numeric_value = descItem.datatype;
       
      break;
     
    case SQLDESC_LENGTH:
     {
       // Compiler may report length as negative if greater than INT_MAX
       if (descItem.length < 0)
         return ERROR;
       *(Int32 *)numeric_value = descItem.length;
       if ( descItem.charset == CharInfo::UNICODE  ||
            CharInfo::is_NCHAR_MP((CharInfo::CharSet)descItem.charset)
          )
         *(Int32 *)numeric_value /= SQL_DBCHAR_SIZE;
     }
     break;

    case SQLDESC_OCTET_LENGTH:
      // Compiler may report length as negative if greater than INT_MAX
      if (descItem.length < 0)
        return ERROR;
      *(Int32 *)numeric_value = descItem.length;
      break;

    case SQLDESC_PRECISION:
      *(Int32 *)numeric_value = descItem.precision;
      break;

    case SQLDESC_SCALE:
      *(Int32 *)numeric_value = descItem.scale;
      break;

    case SQLDESC_INT_LEAD_PREC:
      *(Int32 *)numeric_value = descItem.int_leading_precision;
      break;

    case SQLDESC_NULLABLE:
      *(Int32 *)numeric_value = descItem.nullable;
      break;

    case SQLDESC_CASEINSENSITIVE:
      if (descItem.desc_flags & descItem.IS_CASE_INSENSITIVE)
	*(Int32 *)numeric_value = 1;
      else
	*(Int32 *)numeric_value = 0;
      break;

    // internal use only
    case SQLDESC_CHAR_SET:
      *(Int32 *)numeric_value = descItem.charset;
      break;

    case SQLDESC_COLLATION:
         *(Int32 *)numeric_value = descItem.coll_seq;
      // *returned_len = desc_set_string_value_from_varchar(string_value,
      //                                              max_string_len,
      //                                              descItem.coll_seq);
      break;
     
    case SQLDESC_NAME:
      {
         return 
            setCharHostVar(descItem.output_name, 
			   string_value, max_string_len, returned_len, 
			   info_desc, info_desc_index 
                          );
      }
      break;

    case SQLDESC_UNNAMED:
      *(Int32 *)numeric_value = descItem.generated_output_name;
      break;

    case SQLDESC_HEADING:
      if (descItem.heading) // heading is present
        {
        *returned_len = desc_set_string_value_from_varchar(string_value,
                                                      max_string_len,
	                                              descItem.heading);
        }
      else
        {
	  *returned_len = 0;
        }
      break;
     
    case SQLDESC_IND_TYPE:
      *(Int32 *)numeric_value = descItem.ind_datatype;
      break;

    case SQLDESC_IND_LENGTH:
      *(Int32 *)numeric_value = descItem.ind_length;
      break;

    case SQLDESC_VC_IND_LENGTH:
      *(Int32 *)numeric_value = descItem.vc_ind_length;
      break;

    case SQLDESC_ALIGNED_LENGTH:
      // Compiler may report length as negative if greater than INT_MAX
      if ((descItem.aligned_length < 0) || (descItem.length < 0))
        return ERROR;
      *(Int32 *)numeric_value = descItem.aligned_length;
      break;

    case SQLDESC_DATA_OFFSET:
      *(Int32 *)numeric_value = descItem.data_offset;
      break;

    case SQLDESC_NULL_IND_OFFSET:
      *(Int32 *)numeric_value = descItem.null_ind_offset;
      break;

    case SQLDESC_VAR_PTR:
    {
      Long var_ptr = descItem.var_ptr;
      if (rowsetSize > 0 && descItem.rowsetVarLayoutSize > 0) {
	// The size for rowset SQLVarChars is the sum of  
        // (length of the val part) and (length of the len part) 
        if (descItem.rowsetVarLayoutSize > 0) {
    	  if ( descItem.datatype == REC_BYTE_V_ASCII) {
	    // special case for COBOL VARCHARs, the length parts
	    // have to be aligned.
	    if ((descItem.rowsetVarLayoutSize % 2) == 0) {
	      var_ptr += idxrow * (descItem.rowsetVarLayoutSize + descItem.vc_ind_length);
	    }
	    else {
	      var_ptr += idxrow * (descItem.rowsetVarLayoutSize + 1 + descItem.vc_ind_length);
	    }
	  }
	  else if (DFS2REC::isSQLVarChar(descItem.datatype)) {
	    var_ptr += idxrow * (descItem.rowsetVarLayoutSize + descItem.vc_ind_length);
	  }
	  else {
	    var_ptr += idxrow * descItem.rowsetVarLayoutSize;
	  }
	}
      }
      *(Long *)numeric_value = var_ptr;
    }
      break;

    case SQLDESC_IND_PTR:
    {
      Long ind_ptr = descItem.ind_ptr;
      if (rowsetSize > 0 && descItem.rowsetIndLayoutSize > 0) {
         ind_ptr += idxrow * descItem.rowsetIndLayoutSize;
      }
      *(Long *)numeric_value = ind_ptr;
    }
      break;

    case SQLDESC_RET_LEN :
      *(Int32 *)numeric_value = getVCLength(descItem.var_data, VCPREFIX_LEN);
      if ( CharInfo::maxBytesPerChar((CharInfo::CharSet)descItem.charset) == SQL_DBCHAR_SIZE )
        *(Int32 *)numeric_value = *(Int32 *)numeric_value/SQL_DBCHAR_SIZE;
      break;

    case SQLDESC_RET_OCTET_LEN :
      *(Int32 *)numeric_value = getVCLength(descItem.var_data, VCPREFIX_LEN);
      break;

    case SQLDESC_VAR_DATA:
      *returned_len = desc_set_string_value_from_varchar(string_value,
                                                    max_string_len,
                                                    descItem.var_data);
      break;
     
    case SQLDESC_IND_DATA:
      *(Int32 *)numeric_value = descItem.ind_data;
      break;

    case SQLDESC_ROWSET_VAR_LAYOUT_SIZE:
      *(Int32 *)numeric_value = descItem.rowsetVarLayoutSize;
      break;

    case SQLDESC_ROWSET_IND_LAYOUT_SIZE:
      *(Int32 *)numeric_value = descItem.rowsetIndLayoutSize;
      break;

    case SQLDESC_ROWSET_SIZE:
      *(Int32 *)numeric_value = rowsetSize;
      break;
      
    case SQLDESC_ROWWISE_ROWSET_SIZE:
      if (NOT rowwiseRowset())
	return ERROR;

      *(Int32 *)numeric_value = rowwiseRowsetSize;
      break;
      
    case SQLDESC_ROWWISE_ROWSET_ROW_LEN:
      if (NOT rowwiseRowset())
	return ERROR;

      *(Int32 *)numeric_value = rowwiseRowsetRowLen;
      break;
      
    case SQLDESC_ROWWISE_ROWSET_PTR:
      if (NOT rowwiseRowset())
	return ERROR;

      *(Long *)numeric_value = rowwiseRowsetPtr;
      break;
      
    case SQLDESC_ROWWISE_ROWSET_PARTN_NUM:
      if (NOT rowwiseRowset())
	return ERROR;

      *(Int32 *)numeric_value = rowwiseRowsetPartnNum;
      break;
      
    case SQLDESC_ROWSET_STATUS_PTR:
      *(Long *)numeric_value = (Long) rowsetStatusPtr;

    case SQLDESC_ROWSET_NUM_PROCESSED:
      *(Int32 *)numeric_value = rowsetNumProcessed;
      break;

    case SQLDESC_ROWSET_HANDLE:
      *(Int32 *)numeric_value = rowsetHandle;
      break;

    case SQLDESC_TABLE_NAME:
      if (descItem.table_name) // heading is present
        {
        *returned_len = desc_set_string_value_from_varchar(string_value,
                                                      max_string_len,
	                                              descItem.table_name);
        }
      else
        {
	  *returned_len = 0;
        }
      break;

    case SQLDESC_SCHEMA_NAME:
      if (descItem.schema_name) // heading is present
        {
        *returned_len = desc_set_string_value_from_varchar(string_value,
                                                      max_string_len,
	                                              descItem.schema_name);
        }
      else
        {
	  *returned_len = 0;
        }
      break;

    case SQLDESC_CATALOG_NAME:
      if (descItem.catalog_name) // heading is present
        {
        *returned_len = desc_set_string_value_from_varchar(string_value,
                                                      max_string_len,
	                                              descItem.catalog_name);
        }
      else
        {
	  *returned_len = 0;
        }
      break;

    case SQLDESC_PARAMETER_INDEX:

	*(Int32 *)numeric_value = descItem.parameterIndex;
	break;

    case SQLDESC_DESCRIPTOR_TYPE:
	if(isDescTypeWide())
	  *(Int32 *)numeric_value = (Lng32)DESCRIPTOR_TYPE_WIDE;
	else
	  *(Int32 *)numeric_value = (Lng32)DESCRIPTOR_TYPE_NARROW;
	break;

      // Vicz: add two new desc item to support call stmt

    case SQLDESC_PARAMETER_MODE:

      *(Int32 *)numeric_value = descItem.parameterMode;

      break;

    case SQLDESC_ORDINAL_POSITION:
      
      *(Int32 *)numeric_value = descItem.ordinalPosition;

      break;

    case SQLDESC_TEXT_FORMAT:
      if (! descItem.text_format)
        set_text_format(descItem);

      if (descItem.text_format)
      {
        *returned_len = desc_set_string_value_from_varchar(string_value,
                                                     max_string_len,
  	                                             descItem.text_format);
      }
      else
      {
        *returned_len = 0;
      }

      break;

#ifdef _DEBUG
// - start testing logic *****
    case SQLDESC_ROWWISE_VAR_OFFSET:
      *(Int32 *)numeric_value = descItem.rowwise_var_offset;
      break;

    case SQLDESC_ROWWISE_IND_OFFSET:
      *(Int32 *)numeric_value = descItem.rowwise_ind_offset;
      break;
// - end testing logic *****
#endif

    default:
      return ERROR;
      break;
     
    }
 
  return SUCCESS;
}

// Retrieve the main information about an item descriptor. The returned
// addresses of the indicator variables are related to the first row in the
// rowset if any. For other rows, the addresses can be found calling the
// function getVarData and getIndData. Any specific information can be 
// retrived by calling the getDescItem function.

RETCODE Descriptor::getDescItemMainVarInfo(Lng32 entry,
                                           short &var_isnullable,
                                           Lng32  &var_datatype,
                                           Lng32  &var_length,
                                           void   ** var_ptr,
                                           Lng32  &ind_datatype,
                                           Lng32  &ind_length,
                                           void   ** ind_ptr)
{
  desc_struct &descItem = desc[entry - 1]; // Zero base

  var_isnullable = descItem.nullable;
  var_datatype   = descItem.datatype;
  var_length     = descItem.length;
  ind_datatype   = descItem.ind_datatype;
  ind_length     = descItem.ind_length;

  if (descItem.var_data)
    {
      *var_ptr        = (void *)descItem.var_data;
      *ind_ptr        = (void *)((long)descItem.ind_data);
    }
  else
    {
      *var_ptr        = (void *)descItem.var_ptr;
      *ind_ptr        = (void *)descItem.ind_ptr;
    }
 
  return SUCCESS; 
}


// if length = 0, source is a varchar. Otherwise, source is a fixed
// char with length = length.
// Allocate a varchar target. Length bytes = 4.
// WARNING - assuming that varchar indicator is 4 bytes - i.e. sizeof(long)
static
char * desc_varchar_alloc_and_copy(const char   * src,
                                   NAHeap & heap,
                                   Lng32     length = 0,
                                   size_t   vcIndLen = VCPREFIX_LEN)
{
  char * tgt;

  Lng32 len = (length > 0) ? length : getVCLength(src, vcIndLen);

  tgt = (char *)(heap.allocateMemory((size_t)(vcIndLen + len + 1)));

  setVCLength(tgt, len, vcIndLen);
 
  str_cpy_all(tgt+vcIndLen, src+((length > 0) ? 0 : vcIndLen), len);

  // null terminate it just for good measure...
  tgt[len + vcIndLen] = '\0';

  return tgt;
}


static
char * desc_var_data_alloc(char   *source,
                          Lng32     sourceType,
                          Lng32     sourceLen,
                          CharInfo::CharSet sourceCharSet,
                          char   **target,
                          Lng32     targetType,
                          Lng32    *targetLen,
                          CharInfo::CharSet targetCharSet,
                          char   **targetVCLen,
                          Lng32    *targetVCLenSize,
                          NAHeap  &heap)
{

  char * tgt;
  Lng32 sourceDataLen, reqLen;
  short tempSourceDataLen;

  *targetVCLenSize = 0;
  switch (sourceType) {
  case REC_BYTE_V_ASCII:
  case REC_BYTE_V_DOUBLE:
  case REC_BYTE_V_ASCII_LONG:
      str_cpy_all((char *)&tempSourceDataLen, source, sizeof(short));
      sourceDataLen = (Lng32)tempSourceDataLen;
      break;
  case REC_BYTE_V_ANSI:
      if ( CharInfo::is_NCHAR_MP(sourceCharSet) == FALSE )
        sourceDataLen = str_len(source);
      else
        sourceDataLen = na_wcslen((NAWchar*)source) * SQL_DBCHAR_SIZE;
      break;
  case REC_NCHAR_V_ANSI_UNICODE:
      sourceDataLen = na_wcslen((NAWchar*)source) * SQL_DBCHAR_SIZE;
      break;
  case REC_BYTE_F_ASCII:
  case REC_NCHAR_F_UNICODE:
      sourceDataLen = sourceLen;
      break;
  default:
      sourceDataLen = *targetLen;
  }
  switch (targetType) {
  case REC_BYTE_V_ASCII:
  case REC_BYTE_V_DOUBLE:
  case REC_BYTE_V_ASCII_LONG:
      *targetVCLenSize = sizeof(short);
      reqLen = sourceDataLen + *targetVCLenSize;
      break;
  case REC_BYTE_V_ANSI:
      reqLen = sourceDataLen + CharInfo::maxBytesPerChar(targetCharSet);
      break;
  case REC_NCHAR_V_ANSI_UNICODE:
      reqLen = sourceDataLen + SQL_DBCHAR_SIZE;
      break;
  case REC_BYTE_F_ASCII:
  case REC_NCHAR_F_UNICODE:
      reqLen = *targetLen;
      break;
  default:
      reqLen = sourceDataLen;
  }
  *targetLen = MINOF(reqLen, *targetLen + *targetVCLenSize);
  tgt = (char *)(heap.allocateMemory((size_t)(VCPREFIX_LEN + *targetLen)));
  setVCLength(tgt, *targetLen, VCPREFIX_LEN);
  *target = tgt + VCPREFIX_LEN + *targetVCLenSize;
  *targetVCLen = *targetVCLenSize == 0 ? NULL : tgt + VCPREFIX_LEN;
  *targetLen -= *targetVCLenSize;
  return tgt;
}


short Descriptor::isCharacterFSType(Lng32 datatype)
{
   return ( DFS2REC::isDoubleCharacter(datatype) ||
            DFS2REC::is8bitCharacter(datatype) );
}

short Descriptor::isIntegralFSType(Lng32 datatype)
{
   switch (datatype)
   {
   case REC_BIN8_SIGNED:
   case REC_BIN8_UNSIGNED:
   case REC_BIN16_SIGNED:
   case REC_BIN16_UNSIGNED:
   case REC_BIN32_SIGNED:
   case REC_BIN32_UNSIGNED:
   case REC_BIN64_SIGNED:
   case REC_BIN64_UNSIGNED:
      return TRUE;

   default:
      break;
   }

   return FALSE;
}

short Descriptor::isFloatFSType(Lng32 datatype)
{
   switch (datatype)
   {
   case REC_FLOAT32:
   case REC_FLOAT64:
      return TRUE;

   default:
      break;
   }

   return FALSE;
}

short Descriptor::isNumericFSType(Lng32 datatype)
{
   switch (datatype)
   {
   case REC_DECIMAL_UNSIGNED:
   case REC_DECIMAL_LSE:
      return TRUE;

   default:
      break;
   }

   return FALSE;
}

short Descriptor::isDatetimeFSType(Lng32 datatype)
{
   switch (datatype)
   {
   case REC_DATETIME:
      return TRUE;

   default:
      break;
   }

   return FALSE;
}

short Descriptor::isBitFSType(Lng32 datatype)
{
   switch (datatype)
   {
   case REC_BPINT_UNSIGNED:
      return TRUE;

   default:
      break;
   }
   return FALSE;
}

Lng32 Descriptor::DefaultPrecision(Lng32 datatype)
{
   Lng32 precision = 0;

   switch (datatype)
   {
   case SQLTYPECODE_IEEE_REAL:
   case SQLTYPECODE_TDM_REAL:
   case REC_FLOAT32:
      precision = 22;
      break;

   case SQLTYPECODE_IEEE_DOUBLE:
   case SQLTYPECODE_TDM_DOUBLE:
   case REC_FLOAT64:
      precision = 54;
      break;
   case SQLTYPECODE_NUMERIC:
      precision = 9;
      break;
   case SQLTYPECODE_NUMERIC_UNSIGNED:
      precision = 9;
      break;
   default:
      break;
   }

   return precision;
}

Lng32 Descriptor::DefaultScale(Lng32 datatype)
{
   Lng32 scale = 0;

   switch (datatype)
   {
   case REC_DECIMAL_UNSIGNED:
   case REC_DECIMAL_LSE:
      // TBD: what is the default scale for these?
      break;

   default:
      break;
   }

   return scale;
}

static short isIntervalANSIType(Lng32 datatype)
{
   return (datatype == SQLTYPECODE_INTERVAL);
}

static short isIntervalType(Lng32 datatype)
{
   return (isIntervalANSIType(datatype) ||
	   Descriptor::isIntervalFSType(datatype));
}

void Descriptor::DescItemDefaultsForDatetimeCode(
                                    /*INOUT*/ desc_struct & descItem,
                                    /*IN*/           Lng32   datetime_interval)
{
  // sanity should dictate that descItem.datetime_code == datetime_interval
  // we'll assume it's true and use them interchangable in here...

  // Tandem internal Intervals are stored as different 'types'
  //  whereas ANSI Intervals need a datetime interval code to fully
  //  identify the type of the interval...
  // In ANSI case, we can't set the internal Tandem datatype until we've
  //  seen the datetime interval code, which we're seeing right here!
  
  if (isIntervalANSIType(descItem.datatype))
    {
      descItem.datatype = getFSTypeFromDatetimeCode(descItem.datetime_code);

      if (descItem.datatype == -1)
        {
          // ERROR!
          // at this point the descriptor's datatype isn't valid...
          // and may not ever be...
          descItem.datatype = SQLTYPECODE_INTERVAL;
        }
    }

  // now for the meat...

  if (isIntervalType(descItem.datatype))
    {
      // 17.5 - GR 5-j
      descItem.int_leading_precision = 2;

      switch (datetime_interval)
        {
        case SQLINTCODE_SECOND:
        case SQLINTCODE_MINUTE_SECOND:
        case SQLINTCODE_HOUR_SECOND:
        case SQLINTCODE_DAY_SECOND:
          // 17.5 - GR 5-j-i
          descItem.precision          = 9;
          break;

        default:
          // 17.5 - GR 5-j-ii
          descItem.precision          = 0;
          break;
        }
    }
  else if (isDatetimeFSType(descItem.datatype))
    {
      descItem.int_leading_precision = 0;
      // 17.5 - GR 5-i
      switch (datetime_interval)
        {
        case SQLDTCODE_TIMESTAMP:
        // none with Timezone yet
          // 17.5 - GR 5-i-i
          descItem.precision          = 9;
          break;

        case SQLDTCODE_TIME:
        case SQLDTCODE_DATE:
        default:
        // none with Timezone yet
          // 17.5 - GR 5-i-ii
          descItem.precision          = 0;
          break;
        }
    }
  else
    {
      // TBD: this is an error - we shouldn't be setting this for
      //      non-datetime or non-interval types...
      descItem.int_leading_precision = 0;
      descItem.precision             = 0;
    }
}

// A new argument to NAHeap is needed because memory will be allocated and
// deallocated to set character data type related desc
void Descriptor::DescItemDefaultsForType(/*INOUT*/ desc_struct & descItem,
                                         /*IN*/           Lng32   datatype,
                                         /*IN*/ NAHeap & heap
                                        )
{
   // the scheme for the following is...
   // extra indentation -
   //    indicates that the value is being set in an implementation-dependent
   //    fashion
   // normal indentation -
   //    indicates that the value is being set due to an ANSI/ISO standard
   //    rule

   if (isCharacterFSType(datatype))
   {
       // implementation dependent...

       // 17.5 - GR 5-a
         

         if (descItem.coll_name)
	   heap.deallocateMemory(descItem.coll_name);
	 descItem.coll_name = desc_varchar_alloc_and_copy("DEFAULT", heap, str_len("DEFAULT"));
         
         if ( DFS2REC::isDoubleCharacter(datatype) == TRUE ) 
         {
	    descItem.charset = SQLCHARSETCODE_UCS2; // default
            descItem.length  = SQL_DBCHAR_SIZE;
         } else  // ascii
         {
	    descItem.charset  = SQLCHARSETCODE_ISO88591; // default
            descItem.length  = 1;
         } 

         const char* charset_name = 
              CharInfo::getCharSetName((CharInfo::CharSet)descItem.charset);
         if (descItem.charset_name)
	   heap.deallocateMemory(descItem.charset_name);
	 descItem.charset_name = desc_varchar_alloc_and_copy(charset_name, heap, str_len(charset_name));

         descItem.coll_seq              = 1;

         descItem.ind_datatype          = 0;  // ??
         descItem.ind_length            = 0;
	 descItem.vc_ind_length         = 0;
	 descItem.aligned_length        = 0;
         descItem.datetime_code         = 0;
         descItem.ind_data              = 0;
         descItem.ind_ptr               = 0;
         descItem.int_leading_precision = 0;
	 // 17.5 - GR 5-a

         descItem.nullable              = 0;
         descItem.precision             = 0;
         descItem.scale                 = 0;
         descItem.string_format         = 0;  // ??
         descItem.var_ptr               = 0;
         descItem.var_data              = 0;

	 descItem.data_offset                    = 0;
	 descItem.null_ind_offset                = 0;

   }
   else if (isIntegralFSType(datatype))
   {
	 // 17.5 - GR 5-f - All fields implementation dependent
         // implementation dependent...
         descItem.charset               = 0;
         descItem.coll_seq              = 0;
         descItem.ind_datatype          = 0;
         descItem.ind_length            = 0;
	 descItem.vc_ind_length         = 0;
	 descItem.aligned_length        = 0;
         descItem.datetime_code         = 0;
         descItem.ind_data              = 0;
         descItem.ind_ptr               = 0;
         descItem.int_leading_precision = 0;
         setVarLength(descItem);
         descItem.nullable              = 0;
         descItem.precision             = 0;
         descItem.scale                 = 0;
         descItem.string_format         = 0;
         descItem.var_ptr               = 0;
         descItem.var_data              = 0;
	 descItem.data_offset                    = 0;
	 descItem.null_ind_offset                = 0;
   }
   else if (isNumericFSType(datatype))
   {
         // implementation dependent...
         descItem.charset               = 0;
         descItem.coll_seq              = 0;
         descItem.ind_datatype          = 0;
         descItem.ind_length            = 0;
	 descItem.vc_ind_length         = 0;
	 descItem.aligned_length        = 0;
         descItem.datetime_code         = 0;
         descItem.ind_data              = 0;
         descItem.ind_ptr               = 0;
         descItem.int_leading_precision = 0;
         // it would be nice to be able to calculate the length anyway
         descItem.length                = 0;
         descItem.nullable              = 0;
	 // 17.5 - GR 5-e
	 descItem.precision             = Descriptor::DefaultPrecision(datatype);
	 // 17.5 - GR 5-e
	 descItem.scale                 = Descriptor::DefaultScale(datatype);
         descItem.string_format         = 0;
         descItem.var_ptr               = 0;
         descItem.var_data              = 0;

	 descItem.data_offset                    = 0;
	 descItem.null_ind_offset                = 0;
   }
   else if (isFloatFSType(datatype))
   {
         // implementation dependent...
         descItem.charset               = 0;
         descItem.coll_seq              = 0;
         descItem.ind_datatype          = 0;
         descItem.ind_length            = 0;
	 descItem.vc_ind_length         = 0;
	 descItem.aligned_length        = 0;
         descItem.datetime_code         = 0;
         descItem.ind_data              = 0;
         descItem.ind_ptr               = 0;
         descItem.int_leading_precision = 0;
         // it would be nice to be able to calculate the length anyway
         setVarLength(descItem);
         descItem.nullable              = 0;
	 // 17.5 - GR 5-g and GR 5-h
	 descItem.precision             = Descriptor::DefaultPrecision(datatype);
         descItem.scale                 = 0;
         descItem.string_format         = 0;
         descItem.var_ptr               = 0;
         descItem.var_data              = 0;

	 descItem.data_offset                    = 0;
	 descItem.null_ind_offset                = 0;
   }
   else if (isDatetimeFSType(datatype))
   {
         // implementation dependent...
         descItem.charset               = 0;
         descItem.coll_seq              = 0;
         descItem.ind_datatype          = 0;
         descItem.ind_length            = 0;
         descItem.ind_length            = 0;
	 descItem.vc_ind_length         = 0;
         descItem.datetime_code         = 0;
         descItem.ind_data              = 0;
         descItem.ind_ptr               = 0;
         descItem.int_leading_precision = 0;
         descItem.length                = 0;
         descItem.nullable              = 0;
	 // 17.5 - GR 5-c

         descItem.precision             = 0;
         descItem.scale                 = 0;
         descItem.string_format         = 0;
         descItem.var_ptr               = 0;
         descItem.var_data              = 0;

	 descItem.data_offset                    = 0;
	 descItem.null_ind_offset                = 0;
   }
   else if (isIntervalType(datatype))
   {
         // implementation dependent...
         descItem.charset               = 0;
         descItem.coll_seq              = 0;
         descItem.ind_datatype          = 0;
         descItem.ind_length            = 0;
	 descItem.vc_ind_length         = 0;
	 descItem.aligned_length        = 0;

	 if (isIntervalANSIType(datatype))
	   {
	     descItem.datetime_code         = 0;
	   }
	 else // internal Tandem FS interval datatype
	   {
	     descItem.datetime_code = getDatetimeCodeFromFSType(datatype);
	     DescItemDefaultsForDatetimeCode(descItem,
					     descItem.datetime_code);
	   }
         descItem.ind_data              = 0;
         descItem.ind_ptr               = 0;
	 // 17.5 - GR 5-d

         descItem.int_leading_precision = 2;
         descItem.length                = 0;
         descItem.nullable              = 0;
         descItem.precision             = 0;
         descItem.scale                 = 0;
         descItem.string_format         = 0;
         descItem.var_ptr               = 0;
         descItem.var_data              = 0;

	 descItem.data_offset                    = 0;
	 descItem.null_ind_offset                = 0;
   }
   else if (isBitFSType(datatype))
   {
         // implementation dependent...
         descItem.charset               = 0;
         descItem.coll_seq              = 0;
         descItem.ind_datatype          = 0;
         descItem.ind_length            = 0;
	 descItem.vc_ind_length         = 0;
	 descItem.aligned_length        = 0;
         descItem.datetime_code         = 0;
         descItem.ind_data              = 0;
         descItem.ind_ptr               = 0;
         descItem.int_leading_precision = 0;
	 // 17.5 - GR 5-b

         descItem.length                = 1;
         descItem.nullable              = 0;
         descItem.precision             = 0;
         descItem.scale                 = 0;
         descItem.string_format         = 0;
         descItem.var_ptr               = 0;
         descItem.var_data              = 0;
	 descItem.data_offset                    = 0;
	 descItem.null_ind_offset                = 0;
   }
   else
   {
         // implementation dependent...
         descItem.charset               = 0;
         descItem.coll_seq              = 0;
         descItem.ind_datatype          = 0;
         descItem.ind_length            = 0;
	 descItem.vc_ind_length         = 0;
	 descItem.aligned_length        = 0;
         descItem.datetime_code         = 0;
         descItem.ind_data              = 0;
         descItem.ind_ptr               = 0;
         descItem.int_leading_precision = 0;
         descItem.length                = 0;
         descItem.nullable              = 0;
         descItem.precision             = 0;
         descItem.scale                 = 0;
         descItem.string_format         = 0;
         descItem.var_ptr               = 0;
         descItem.var_data              = 0;
	 descItem.data_offset                    = 0;
	 descItem.null_ind_offset                = 0;
   }
}

//
// this function gets char string content from an ASCII CHAR host variable.
//
// info_desc and info_desc_index provide additional info about the
// host variable, such as its type. 
//
// theSQLDesc_option indicates the role of the host variable
// in relation to a SQL descriptor entry (e.g., a NAME option).
//
char *
Descriptor::getCharDataFromCharHostVar(ComDiagsArea & diags, 
                                       NAHeap& heap,
			               char * host_var_string_value, 
			               Lng32 host_var_string_value_length,
			               const char* the_SQLDESC_option,
			               Descriptor* info_desc,
                                       Int32 info_desc_index,
                                       short target_type)
{
      Lng32 source_type = 0;

      if ( info_desc == 0 ||
           info_desc -> 
             getDescItem(info_desc_index+1, 
                         SQLDESC_TYPE_FS, &source_type, 0, 0, 0, 0) < 0 
         ) 
      {  // assume fixed CHAR.
         source_type = REC_BYTE_F_ASCII;
      }

      //##NCHAR: Shouldn't this be
      //		if (! DFS2REC::isAnyCharacter(source_type))	?##
      if ( source_type != REC_BYTE_F_ASCII &&
           source_type != REC_BYTE_V_ASCII  &&
           source_type != REC_BYTE_V_ANSI
         )
      { // Host variable should be of ASCII char type!
         diags << DgSqlCode(-CLI_NOT_ASCII_CHAR_TYPE) 
	       << DgString0(the_SQLDESC_option);
         return 0;
      }

      char * source = host_var_string_value;
      Lng32 source_len = host_var_string_value_length;

      enum SQLDESC_option_enum	{ dont_care_option,
      				  COLUMN_NAME_option,
				  CURSOR_NAME_option };
      SQLDESC_option_enum the_SQLDESC_option_enum = dont_care_option;

      if (strcmp(the_SQLDESC_option, "NAME") == 0)
        the_SQLDESC_option_enum = COLUMN_NAME_option;
      else if (strcmp(the_SQLDESC_option, "CURSOR NAME") == 0)
        the_SQLDESC_option_enum = CURSOR_NAME_option;

      if (the_SQLDESC_option_enum == COLUMN_NAME_option)
        {
          if (source_len == 0)  
            {
              source_len = getVCLength(source, VCPREFIX_LEN);
              source += VCPREFIX_LEN;
            }
        }
      else if (the_SQLDESC_option_enum == CURSOR_NAME_option)
        {
          if (source_type == REC_BYTE_V_ASCII)
            {
              source_len = getVCLength(source, sizeof(short));
              source += sizeof(short);
            }
        }
      else
        {
          assert(0);
        }

      Lng32 target_len = source_len;
      char * target = 0;
      ex_expr::exp_return_type expRetcode = ex_expr::EXPR_OK;

      if (the_SQLDESC_option_enum == COLUMN_NAME_option)
        {
          // Target is a default unnamed data type with a VCPREFIX_LEN
          // length prefix.  The caller should not specify the type.
          assert(target_type == -1);

          target = (char *)(heap.allocateMemory((size_t)(VCPREFIX_LEN + target_len + 1)));

          setVCLength(target, target_len, VCPREFIX_LEN);

          // target data is stored as the length field followed by the data.
          expRetcode = 
	    convDoIt(source, source_len, (short)source_type, 0, 0,
                     target+VCPREFIX_LEN, target_len, (short)REC_BYTE_F_ASCII, 
		     0, 0, NULL, 0
                     );
        }
      else
        {
          // Target is the data type specified by target_type.

          if (target_type == REC_BYTE_V_ANSI) target_len++;

          target = (char *)(heap.allocateMemory((size_t)(target_len)));

          expRetcode = 
	    convDoIt(source, source_len, (short)source_type, 0, 0,
                     target, target_len, target_type, 
		     0, 0, NULL, 0
                     );
        }

      if (expRetcode != ex_expr::EXPR_OK) {
         diags << DgSqlCode(-CLI_INVALID_DESC_INFO_REQUEST);
         return 0;
      }

   return target;
}

RETCODE Descriptor::processNumericDatatype(desc_struct &descItem,
					   Lng32 numeric_value)
{
  // In case of exact numeric type, we need to set the flags bit 1 to indicate
  // that this is really a NUMERIC and not SMALLINT, LARGEINT or INT type.
  if (numeric_value == SQLTYPECODE_NUMERIC)
    descItem.desc_flags |= descItem.IS_NUMERIC;
  else if (numeric_value == SQLTYPECODE_NUMERIC_UNSIGNED)
    descItem.desc_flags |= descItem.IS_NUMERIC_UNSIGNED;
  else
    {
      // reset the flag
      descItem.desc_flags &= ~descItem.IS_NUMERIC;
      descItem.desc_flags &= ~descItem.IS_NUMERIC_UNSIGNED;
    }
  return SUCCESS;
}

RETCODE Descriptor::processNumericDatatypeWithPrecision(desc_struct &descItem,
							ComDiagsArea &diags)
{
  if (descItem.desc_flags & descItem.IS_NUMERIC) 
    // Then it is a numeric type
    // Set by SQLDESC_ANSITYPE.There,we only knew that it was NUMERIC.
    // we didn't know whether it should
    // internally be represented as smallint, int or largeint.
    // So here we check the precision and we will know how to 
    // represent it internally.
    {
      if ((descItem.precision >= 0 ) && (descItem.precision <= 4))
	descItem.datatype = 
	  getFSTypeFromANSIType(SQLTYPECODE_SMALLINT);
      else
	if ((descItem.precision > 4 )&&(descItem.precision <=9))
	  descItem.datatype = 
	    getFSTypeFromANSIType(SQLTYPECODE_INTEGER);
	else
	  if ((descItem.precision >=10) && (descItem.precision < 19))
	    descItem.datatype = 
	      getFSTypeFromANSIType(SQLTYPECODE_LARGEINT);
	else
	  if (descItem.precision >=19)
	    descItem.datatype = 
	      getFSTypeFromANSIType(REC_NUM_BIG_SIGNED);
    }
  else if (descItem.desc_flags & descItem.IS_NUMERIC_UNSIGNED)
    {
      if ((descItem.precision >= 0 ) && (descItem.precision <= 4))
	descItem.datatype = 
	  getFSTypeFromANSIType(SQLTYPECODE_SMALLINT_UNSIGNED);
      else
	if ((descItem.precision > 4 )&&(descItem.precision <=9))
	  descItem.datatype = 
	    getFSTypeFromANSIType(SQLTYPECODE_INTEGER_UNSIGNED);
	else
	  if (descItem.precision >=10)
	    {
	      descItem.datatype = 
		getFSTypeFromANSIType(REC_NUM_BIG_UNSIGNED);

	      // unsigned numeric type with precision >= 10 is
	      // not supported. It has to be a signed type.
	      /*diags << DgSqlCode(-3008) << DgString0("NUMERIC")
		    << DgInt0(descItem.precision);
	      return ERROR;
	      */
	    }
    }
  
  // If the TYPE was set by SETDESC_TYPE,it will be in internal datatype
  // form. So if we set it earlier to smallint, int or largeint and it
  // actually has some precision then it is really a NUMERIC type.
  if (descItem.precision >= 0)
    {
      if ((descItem.datatype == REC_BIN16_SIGNED) ||
	  (descItem.datatype == REC_BIN32_SIGNED) ||
	  (descItem.datatype == REC_BIN64_SIGNED) ||
	  (descItem.datatype == REC_NUM_BIG_SIGNED))
	{
	  if (((descItem.datatype == REC_BIN16_SIGNED) &&
	       (descItem.precision > 4)) ||
	      ((descItem.datatype == REC_BIN32_SIGNED) &&
	       (descItem.precision > 9)) ||
	      ((descItem.datatype == REC_BIN64_SIGNED) &&
	       (descItem.precision > 18)) ||
	      ((descItem.datatype == REC_NUM_BIG_SIGNED) &&
	       (descItem.precision > 128)))
	    {
	      Lng32 maxPrec;
	      if (descItem.datatype == REC_BIN16_SIGNED)
		maxPrec = 4;
	      else if (descItem.datatype == REC_BIN32_SIGNED)
		maxPrec = 9;
	      else if (descItem.datatype == REC_BIN64_SIGNED)
		maxPrec = 18;
	      else
		maxPrec = 128;
	      diags << DgSqlCode(-3014) 
		    << DgInt0(descItem.precision)
		    << DgInt1(maxPrec);
	      return ERROR;
	    }

	  // Set the flag to indicate that it is NUMERIC.
	  descItem.desc_flags |= descItem.IS_NUMERIC; 
	}
      else if ((descItem.datatype == REC_BIN16_UNSIGNED) ||
	       (descItem.datatype == REC_BIN32_UNSIGNED) ||
	       (descItem.datatype == REC_BIN64_UNSIGNED) ||
	       (descItem.datatype == REC_NUM_BIG_UNSIGNED))
	{
	  if (((descItem.datatype == REC_BIN16_UNSIGNED) &&
	       (descItem.precision > 4)) ||
	      ((descItem.datatype == REC_BIN32_UNSIGNED) &&
	       (descItem.precision > 9)) ||
	      ((descItem.datatype == REC_BIN64_UNSIGNED) &&
	       (descItem.precision > 20)) ||
	      ((descItem.datatype == REC_NUM_BIG_UNSIGNED) &&
	       (descItem.precision > 128)))
	    {
	      Lng32 maxPrec;
	      if (descItem.datatype == REC_BIN16_UNSIGNED)
		maxPrec = 4;
	      else if (descItem.datatype == REC_BIN32_UNSIGNED)
		maxPrec = 9;
	      else if (descItem.datatype == REC_BIN64_UNSIGNED)
		maxPrec = 19;
	      else
		maxPrec = 128;
	      diags << DgSqlCode(-3008) 
		    << DgString0("NUMERIC")
		    << DgInt0(descItem.precision)
		    << DgInt1(maxPrec);
	      return ERROR;
	    }

	  // Set the flag to indicate that it is NUMERIC unsigned.
	  descItem.desc_flags |= descItem.IS_NUMERIC_UNSIGNED;
	  
	  /*if (descItem.precision >=10)
	    {
	      // unsigned numeric type with precision >= 10 is
	      // not supported. It has to be a signed type.
	      diags << DgSqlCode(-3008) << DgString0("NUMERIC")
		    << DgInt0(descItem.precision);
	      return ERROR;
	    }
	    */
	}
      else
	{
	  // reset the flag
	  descItem.desc_flags &= ~descItem.IS_NUMERIC;
	  descItem.desc_flags &= ~descItem.IS_NUMERIC_UNSIGNED;
	}
    }	
  return SUCCESS;
}

RETCODE Descriptor::setDescItem(Lng32 entry, Lng32 what_to_set,
			     Long numeric_value, char * string_value,
			     Descriptor* info_desc, Int32 info_desc_index)
{
  RETCODE rc = SUCCESS;

  desc_struct  & descItem = ((entry > 0) 
                             ? desc[entry - 1] : desc[0]); // Zero base

  Lng32 sourceType = 0, sourceLen = 0, sourcePrecision = 0, sourceScale = 0;
  Lng32 targetType = 0, targetLen = 0, targetPrecision = 0, targetScale = 0;
  Lng32 targetVCLenSize = 0;
  char *target = NULL, *targetVCLen = NULL, *source = NULL;
  Lng32 whichPrecision, whichScale;

  CharInfo::CharSet newCharSet = CharInfo::UnknownCharSet;
  char* charset_name = NULL;

  ComDiagsArea & diags       = context_->diags();
  ComDiagsArea * diagsArea   = context_->getDiagsArea();
  NAHeap       & heap        = *context_->exHeap();
  CollHeap     * collHeap    = context_->exCollHeap();

  // Do some small checks to make sure we are not setting items
 // in a descriptor that does not have any items
 switch (what_to_set)
 {
  
  case SQLDESC_ROWSET_SIZE:
  case SQLDESC_ROWSET_HANDLE : 
  case SQLDESC_ROWSET_NUM_PROCESSED:
  case SQLDESC_ROWSET_ADD_NUM_PROCESSED :
  case SQLDESC_ROWSET_STATUS_PTR        :
  case SQLDESC_DESCRIPTOR_TYPE          :
  case SQLDESC_ROWSET_TYPE          :
  case SQLDESC_ROWWISE_ROWSET_SIZE:
  case SQLDESC_ROWWISE_ROWSET_ROW_LEN:
  case SQLDESC_ROWWISE_ROWSET_PTR:

    // In the above cases there is no need to check for desc because
   // these items are not bound to specific columns i.e they are not
   // part of the small desc structure.
 
    break;
  default:
      if (desc == NULL)
     {
      diags << DgSqlCode(-CLI_INVALID_DESC_ENTRY);
      return ERROR;
     }
 }
  
  // var_data is undefined if anything other than var_data is set.
  // and if we're about to set var_data then we can just unset it
  // for now...  though we need to deallocate the memory it points to...


  if (desc && entry > 0 && descItem.var_data) // data already exists
  {
     // delete it
     heap.deallocateMemory(descItem.var_data);
     descItem.var_data = 0;
  }

  // Setup for data type conversion
  // Do it only for variable data for now
  if (info_desc != NULL) {
      switch (what_to_set) {
      case SQLDESC_VAR_DATA:
      case SQLDESC_IND_DATA:
      case SQLDESC_TYPE:
      case SQLDESC_TYPE_FS:
      case SQLDESC_DATETIME_CODE:
      case SQLDESC_INT_LEAD_PREC:
      case SQLDESC_PRECISION:
      case SQLDESC_SCALE:
      case SQLDESC_CHAR_SET_NAM:
      case SQLDESC_LENGTH:
      case SQLDESC_IND_TYPE:
       info_desc->getDescItem(
                     info_desc_index + 1,
                     SQLDESC_TYPE_FS,
                     &sourceType, NULL, 0, NULL, 0);
       info_desc->getDescItem(
                     info_desc_index + 1,
                     SQLDESC_OCTET_LENGTH,
                     &sourceLen, NULL, 0, NULL, 0);
       if (sourceType >= REC_MIN_INTERVAL && sourceType <= REC_MAX_INTERVAL) {
          whichPrecision = SQLDESC_INT_LEAD_PREC;
          whichScale = SQLDESC_PRECISION;
       }
       else if (sourceType == REC_DATETIME) {
          whichPrecision = SQLDESC_DATETIME_CODE;
          whichScale = SQLDESC_PRECISION;
       }
       else {
        whichPrecision = SQLDESC_PRECISION;
        whichScale = SQLDESC_SCALE;
       }
       info_desc->getDescItem(
                     info_desc_index + 1,
                     whichPrecision,
                     &sourcePrecision, NULL, 0, NULL, 0);
       info_desc->getDescItem(
                     info_desc_index + 1,
                     whichScale,
                     &sourceScale, NULL, 0, NULL, 0);
      }

      switch (what_to_set) {
        case SQLDESC_VAR_DATA:
         {
          // if VAR_DATA is set for a character desc item, make sure the two have 
          // the same charset.  Do the test only for non-Unicode hostvars because
          // Unicode ones can be relaxed.
          if ( DFS2REC::isAnyCharacter(sourceType) && 
               DFS2REC::isAnyCharacter(descItem.datatype) ) {
            Lng32 sourceCharSet;
            info_desc->getDescItem(
                          info_desc_index + 1,
                          SQLDESC_CHAR_SET,
                          &sourceCharSet, NULL, 0, NULL, 0);
  
            if ( sourceCharSet != CharInfo::UNICODE && 
                 sourceCharSet != descItem.charset )
            {
            // Error 8896: The character set $0~string0 of a host variable does not match $1~string1 of the corresponding descriptor item (entry $2~Int0).
              diags << DgSqlCode(-CLI_CHARSET_MISMATCH)
  		<< DgString0(CharInfo::getCharSetName((CharInfo::CharSet)sourceCharSet))
  		<< DgString1(CharInfo::getCharSetName((CharInfo::CharSet)descItem.charset))
  		<< DgInt0(entry);
  	    return ERROR;
            }
         }
        }
         break;
        default:
         break;
      }

      switch (what_to_set) {
      case SQLDESC_VAR_DATA:
       targetType = descItem.datatype;
       targetLen = descItem.length;
       if (targetType >= REC_MIN_INTERVAL && targetType <= REC_MAX_INTERVAL) {
          targetPrecision = descItem.int_leading_precision;
          targetScale = descItem.precision;
       }
       else if (targetType == REC_DATETIME) {
          targetPrecision = descItem.datetime_code;
          targetScale = descItem.precision;
       }
       else {
          targetPrecision = descItem.precision;
          targetScale = descItem.scale;
       }
       break;
      case SQLDESC_IND_DATA:
      case SQLDESC_TYPE:
      case SQLDESC_TYPE_FS:
      case SQLDESC_DATETIME_CODE:
      case SQLDESC_INT_LEAD_PREC:
      case SQLDESC_PRECISION:
      case SQLDESC_SCALE:
      case SQLDESC_LENGTH:
      case SQLDESC_IND_TYPE:
        targetType = REC_BIN32_SIGNED;
        targetLen = sizeof(Int32);
        targetPrecision = 31;
        targetScale = 0;
        break;
      case SQLDESC_CHAR_SET_NAM:
        targetType = REC_BYTE_V_ANSI;
        targetLen = sourceLen+1;
     }
  }

  switch (what_to_set)
    {
    case SQLDESC_TYPE:
      if (string_value != NULL && info_desc != NULL) {
        source = string_value;
        target = (char *)&descItem.datatype;
      }
      else {
        Lng32 datatype = getFSTypeFromANSIType(numeric_value);
	// Here is case of NUMERIC types , the above procedure, sets 
	// the internal datatype to the default type - INT. 
	// Further down, we will check for the precision and may re assign
	// the correct internal datatype ( could be smallint or largeint )depending 
	// on the precision 
	if (numeric_value != datatype)
	  {
	    reComputeBulkMoveInfo();
	    descItem.datatype = datatype;
	  }
	
	DescItemDefaultsForType(descItem,
				descItem.datatype, heap);

	processNumericDatatype(descItem, numeric_value);

     }
    break;
    
    case SQLDESC_TYPE_FS:
      if (string_value != NULL && info_desc != NULL) {
        source = string_value;
        target = (char *)&descItem.datatype;
      }
      else {
	if (numeric_value != descItem.datatype)
	{
	  reComputeBulkMoveInfo();
	  descItem.datatype = numeric_value;
	}

	DescItemDefaultsForType(descItem,
				descItem.datatype, heap);
       }

      // Further down we will check for precision to see if this is a NUMERIC
      // type or not.
      // Right now, reset the flag to make it non-NUMERIC.
      descItem.desc_flags &= ~descItem.IS_NUMERIC;
      descItem.desc_flags &= ~descItem.IS_NUMERIC_UNSIGNED;

      break;

    case SQLDESC_DATETIME_CODE:
      // TBD: verify that datatype is DATETIME or INTERVAL type
      if (string_value != NULL && info_desc != NULL) {
        source = string_value;
        target = (char *)&descItem.datetime_code;
      }
      else {
        descItem.datetime_code = numeric_value;
        DescItemDefaultsForDatetimeCode(descItem,
                                      descItem.datetime_code);
      }
      break;
     
    case SQLDESC_COLLATION:
      if (string_value != NULL && info_desc != NULL) {
        source = string_value;
        target = (char *)&descItem.coll_seq;
      }
      else {
	  descItem.coll_seq = numeric_value;
      }
      break;

    case SQLDESC_LENGTH:
      if (string_value != NULL && info_desc != NULL) {
        source = string_value;
        target = (char *)&descItem.length;
      }
      else {
        if (numeric_value != descItem.length)
	{
	  reComputeBulkMoveInfo();
	  descItem.length = numeric_value;
	}

      }
      break;

    case SQLDESC_PRECISION:
      if (string_value != NULL && info_desc != NULL) {
        source = string_value;
        target = (char *)&descItem.precision;
      }
      else { 
         if (numeric_value != descItem.precision)
	{
	  reComputeBulkMoveInfo();
	  descItem.precision = numeric_value;

	  rc = processNumericDatatypeWithPrecision(descItem, diags);
	  if (rc == ERROR)
	    return rc;
	}
      }
      break;

    case SQLDESC_SCALE:
      if (string_value != NULL && info_desc != NULL) {
        source = string_value;
        target = (char *)&descItem.scale;
      }
      else {
       if (numeric_value != descItem.scale)
	{
	  reComputeBulkMoveInfo();
	  descItem.scale = numeric_value;
	}
      }
      break;

    case SQLDESC_INT_LEAD_PREC:
      if (string_value != NULL && info_desc != NULL) {
        source = string_value;
        target = (char *)&descItem.int_leading_precision;
      }
      else
        descItem.int_leading_precision = numeric_value;
      break;

    case SQLDESC_NULLABLE:
      descItem.nullable = (short)numeric_value;
      break;

    case SQLDESC_CASEINSENSITIVE:
      if (numeric_value != 0)
	descItem.desc_flags |= descItem.IS_CASE_INSENSITIVE;
      else
	descItem.desc_flags &= ~descItem.IS_CASE_INSENSITIVE;
      break;

    // internal use only
    case SQLDESC_CHAR_SET:
      newCharSet = (CharInfo::CharSet)numeric_value;
      break;

    case SQLDESC_CHAR_SET_NAM:
      if (string_value) {
          source = string_value;
          if ( info_desc == NULL ) {
            // need to set type, len, precision and scale for both
            // source and target
            sourceType = REC_BYTE_V_ANSI;
            sourceLen = strlen(source) + 1; // max source length including
                                            // the null at the end
            sourcePrecision = sourceScale = 0;

            targetType = REC_BYTE_V_ANSI;
            targetLen = sourceLen;
            targetPrecision = targetScale = 0;
          }
          target = (char *)heap.allocateMemory(targetLen);
      }
      break;

// cannot set the following items.
     case SQLDESC_CHAR_SET_CAT:
     case SQLDESC_CHAR_SET_SCH:
     case SQLDESC_COLL_CAT:
     case SQLDESC_COLL_SCH:
     case SQLDESC_COLL_NAM:
      /* Error 8829: Invalid desc_items to set. */
      diags << DgSqlCode(-8829); 
      return ERROR;
      break;

    case SQLDESC_NAME:
    {
      if (descItem.output_name) // data already exists
	heap.deallocateMemory(descItem.output_name);

      descItem.output_name = getCharDataFromCharHostVar
				(diags, heap, string_value, numeric_value,
				 "NAME", info_desc, info_desc_index);
    }
      break;

    case SQLDESC_UNNAMED:
      descItem.generated_output_name = (short)numeric_value;
      break;

    case SQLDESC_HEADING:
      if (descItem.heading) // data already exists
	heap.deallocateMemory(descItem.heading);
      descItem.heading =
          desc_varchar_alloc_and_copy(string_value, heap);
      break;

    case SQLDESC_IND_TYPE:
      if (string_value != NULL && info_desc != NULL) {
        source = string_value;
        target = (char *)&descItem.ind_datatype;
      }
      else {
        descItem.ind_datatype = numeric_value;
        setIndLength(descItem);
      }
     break;

    case SQLDESC_IND_LENGTH:
      if (descItem.ind_length == 0) {
        switch (numeric_value) {
        case 2:
          descItem.ind_datatype = REC_BIN16_SIGNED;
          break;
        case 4:
          descItem.ind_datatype = REC_BIN32_SIGNED;
          break;
        case 8:
          descItem.ind_datatype = REC_BIN64_SIGNED;
          break;
        default:
          // Error for now assume length 2
          descItem.ind_datatype = 0;
          break;
        }
        setIndLength(descItem);
      } else
        if (descItem.ind_length != numeric_value) 
          {
#ifdef _DEBUG
            // It should match.. Likely an error
            fprintf(stderr, "setDescItem: indicator length of %d"
                    "must match %d\n", (Int32) numeric_value, descItem.ind_length);
            fflush(stderr);
#else
	    assert(0);
#endif
          }
      break;

    case SQLDESC_VC_IND_LENGTH:
      {
	if (descItem.vc_ind_length == 0)
	  descItem.vc_ind_length = numeric_value;
	else if (descItem.vc_ind_length != numeric_value)
          {
#ifdef _DEBUG
            // It should match.. Likely an error
            fprintf(stderr, "setDescItem: indicator length of %d"
                    "must match %d\n", (Int32) numeric_value, descItem.ind_length);
            fflush(stderr);
#else
	    assert(0);
#endif
          }
      }
    break;

    case SQLDESC_VAR_PTR:
      if (numeric_value != descItem.var_ptr)
	{
	  reComputeBulkMoveInfo();

	  descItem.var_ptr = numeric_value;
	}
      break;

    case SQLDESC_IND_PTR:
      descItem.ind_ptr = numeric_value;
     
      // ind_data becomes undefined.
      descItem.ind_data = 0;
      if (descItem.ind_ptr != 0 && descItem.ind_length == 0) {
        // It is likely defined by the application
        descItem.ind_datatype = REC_BIN32_SIGNED;
        setIndLength(descItem);
      }
      // Note the validation of code-point values for Unicode char 
      // variable data is done in InputOutputExpr::inputValues().
      break;

    case SQLDESC_VAR_DATA:

    // Here is a description on how the var_data is converted.
    //
    // source: string_value,
    // sourceType: info_desc.datatype (=type(:hv assigned to VARIABLE_DATA))
    // sourceLen: info_desc.length (=arraysize(:hv assigned to VARIABLE_DATA)),
    //
    // target: newly allocated memory,
    // targetType: descItem.datatype
    // targetLen: descItem.length
    //
    // With Phase I work, desc.charset is aligned with desc.datatype as only
    // core charset is allowed. So we do not have to worry about desc.chars et.
    // In PII, this does not hold anymore. Both info_desc.charset and
    // descItem.charset should be used to construct the sourceType and
    // targetType in order for the convDoIt() above to work properly.
    //
    // Also, the validation of code-point values for Unicode char variable data
    // is done in InputOutputExpr::inputValues().

    // Var Data should not be used for rowset arrays.  

    if (string_value) // some real data area passed in.
      {
        if (info_desc != NULL) {

          Lng32 sourceCharSet;

          info_desc->getDescItem(info_desc_index + 1,
                                 SQLDESC_CHAR_SET, &sourceCharSet, NULL, 0, NULL, 0);

          descItem.var_data = desc_var_data_alloc(
                                    string_value,
                                    sourceType,
                                    sourceLen,
                                    (CharInfo::CharSet)sourceCharSet,
                                    &target,
                                    targetType,
                                    &targetLen,
                                    DFS2REC::isAnyCharacter(descItem.datatype) ? 
                                              (CharInfo::CharSet)descItem.charset : 
                                              CharInfo::ISO88591,
                                    &targetVCLen,
                                    &targetVCLenSize,
                                    heap);
          source = string_value;
        }
        else
          descItem.var_data =
                desc_varchar_alloc_and_copy(string_value, heap, numeric_value);
      }
    // var_ptr becomes undefined.
    descItem.var_ptr = 0;
    break;

    case SQLDESC_IND_DATA:
      if (string_value != NULL && info_desc != NULL) {
        source = string_value;
        target = (char *)&descItem.ind_data;
      }
      else
        descItem.ind_data = numeric_value;
      break;

    case SQLDESC_ROWSET_VAR_LAYOUT_SIZE:
      // We better have a rowset
      if (rowsetSize < 0)
        diags << DgSqlCode(-CLI_INTERNAL_ERROR);
      else
        descItem.rowsetVarLayoutSize = numeric_value;
      break;

    case SQLDESC_ROWSET_IND_LAYOUT_SIZE:
      // We better have a rowset
      if (rowsetSize < 0)
        diags << DgSqlCode(-CLI_INTERNAL_ERROR);
      else
        descItem.rowsetIndLayoutSize = numeric_value;
      break;

    case SQLDESC_ROWSET_SIZE:
      rowsetSize = numeric_value;
      break;
      
    case SQLDESC_ROWWISE_ROWSET_SIZE:
      if (NOT rowwiseRowset())
	return ERROR;

      rowwiseRowsetSize = numeric_value;
      break;
      
    case SQLDESC_ROWWISE_ROWSET_ROW_LEN:
      if (NOT rowwiseRowset())
	return ERROR;

      rowwiseRowsetRowLen = numeric_value;
      break;
      
    case SQLDESC_ROWWISE_ROWSET_PTR:
      if (NOT rowwiseRowset())
	return ERROR;

      rowwiseRowsetPtr = numeric_value;
      break;
      
    case SQLDESC_ROWWISE_ROWSET_PARTN_NUM:
      if (NOT rowwiseRowset())
	return ERROR;

      rowwiseRowsetPartnNum = numeric_value;
      break;
      
    case SQLDESC_ROWSET_STATUS_PTR:
      if (rowsetSize <= 0)
        diags << DgSqlCode(-CLI_INTERNAL_ERROR);
      else {
        rowsetStatusPtr = (void *)numeric_value;
      }
      break;

    case SQLDESC_ROWSET_NUM_PROCESSED:
      if (((NOT rowwiseRowset()) &&
	   (numeric_value > rowsetSize)) ||
	  ((rowwiseRowset()) &&
	   (numeric_value > rowwiseRowsetSize)))
        diags << DgSqlCode(-CLI_INTERNAL_ERROR);
      else
        rowsetNumProcessed = numeric_value;
      break;

    case SQLDESC_ROWSET_ADD_NUM_PROCESSED:
      if (rowsetNumProcessed >= rowsetSize)
        diags << DgSqlCode(-CLI_INTERNAL_ERROR);
      else {
        rowsetNumProcessed++;
        rowsetHandle++;
      }
      break;

    case SQLDESC_ROWSET_HANDLE:
      if (numeric_value > rowsetSize)
        diags << DgSqlCode(-30002) << DgInt0((Int32)(numeric_value)) << DgInt1((Int32)rowsetSize);
      else
        rowsetHandle = numeric_value;
      break;
    
      
    case SQLDESC_TABLE_NAME:
      if (descItem.table_name) // data already exists
	heap.deallocateMemory(descItem.table_name);
      descItem.table_name =
          desc_varchar_alloc_and_copy(string_value, heap);
      break;
    case SQLDESC_SCHEMA_NAME:
      if (descItem.schema_name) // data already exists
	heap.deallocateMemory(descItem.schema_name);
      descItem.schema_name =
          desc_varchar_alloc_and_copy(string_value, heap);
      break;
    case SQLDESC_CATALOG_NAME:
      if (descItem.catalog_name) // data already exists
	heap.deallocateMemory(descItem.catalog_name);
      descItem.catalog_name =
          desc_varchar_alloc_and_copy(string_value, heap);
      break;
   
    case SQLDESC_PARAMETER_MODE:
      descItem.parameterMode = numeric_value;
      break;

    case SQLDESC_ORDINAL_POSITION:
      descItem.ordinalPosition = numeric_value;
      break;

    case SQLDESC_PARAMETER_INDEX:
	descItem.parameterIndex = numeric_value;
	break;

    case SQLDESC_DESCRIPTOR_TYPE:
	{
	enum DESCRIPTOR_TYPE descType = (enum DESCRIPTOR_TYPE)numeric_value;
	if(descType == DESCRIPTOR_TYPE_WIDE)
	  setDescTypeWide(TRUE);
	else
	  setDescTypeWide(FALSE);
	} 
	break;

    case SQLDESC_ROWSET_TYPE:
	{
	enum ROWSET_TYPE rowsetType = (enum ROWSET_TYPE)numeric_value;
	if(rowsetType == ROWSET_ROWWISE_V1)
	  setRowwiseRowsetV1(TRUE);
	else
	  setRowwiseRowsetV1(FALSE);
	if(rowsetType == ROWSET_ROWWISE_V2)
	  setRowwiseRowsetV2(TRUE);
	else
	  setRowwiseRowsetV2(FALSE);
	} 
	break;

#ifdef _DEBUG
// - start testing logic *****
    case SQLDESC_ROWWISE_VAR_OFFSET:
      descItem.rowwise_var_offset = numeric_value;
      break;
          
    case SQLDESC_ROWWISE_IND_OFFSET:
      descItem.rowwise_ind_offset = numeric_value;
      break;
// - end testing logic *****
#endif

    default:
      diags << DgSqlCode(-CLI_INVALID_DESC_INFO_REQUEST);
      return ERROR;
    }

  if (source != NULL) 
    {
      if (DFS2REC::isSQLVarChar(sourceType))
	{
	  // the first 2 bytes of data are actually the variable length indicator
	  short VCLen;
	  str_cpy_all((char *) &VCLen, source, sizeof(short));
	  sourceLen = (Lng32) VCLen;
	  source = &source[sizeof(short)];
#ifdef _DEBUG
	  static UInt32 asserted = 0;
	  if (!asserted++) {
	    assert(SQL_VARCHAR_HDR_SIZE == sizeof(short));
	  }
#endif
	}
      else if ((what_to_set == SQLDESC_VAR_DATA) &&
	       ((DFS2REC::isDateTime(sourceType)) ||
		(DFS2REC::isInterval(sourceType))))
	{
	  // datetime and interval values are saved as ascii string.
	  // Change the datatype to be ascii to convDoIt can move the data
	  // correctly.
	  sourceType = targetType = REC_BYTE_F_ASCII;
	  sourceScale = targetScale = 0;
	  sourcePrecision = targetPrecision = 0;
	}

      if (::convDoIt(source,
                     sourceLen,
                     (short) sourceType,
                     sourcePrecision,
                     sourceScale,
                     target,
                     targetLen,
                     (short) targetType,
                     targetPrecision,
                     targetScale,
                     targetVCLen,
                     targetVCLenSize,
                     collHeap,
                     &diagsArea) !=  ex_expr::EXPR_OK) {

	if (what_to_set == SQLDESC_CHAR_SET_NAM && string_value)  // deallocate the memory
	  heap.deallocateMemory(target);

	return ERROR;
      }

      if ( DFS2REC::isANSIVarChar(targetType) && 
           CharInfo::is_NCHAR_MP((CharInfo::CharSet)descItem.charset) ) {
         target[strlen(target)+1] = 0; // make it a NAWchar NULL
      }

      if (targetType >= REC_MIN_NUMERIC &&
          targetType <= REC_MAX_NUMERIC &&
          sourceScale != targetScale &&
          ::scaleDoIt(target,
                      targetLen,
                      targetType,
                      sourceScale,
                      targetScale,
                      sourceType,
                      collHeap) != ex_expr::EXPR_OK) {
	return ERROR;
      }

      switch (what_to_set) {
      case SQLDESC_TYPE:
	{
	  Lng32 ansiType = descItem.datatype;
	  descItem.datatype = getFSTypeFromANSIType(ansiType);
	  DescItemDefaultsForType(descItem,
				  descItem.datatype, heap);
	  processNumericDatatype(descItem, ansiType);
	}
	break;
	
      case SQLDESC_TYPE_FS:
	DescItemDefaultsForType(descItem,
				descItem.datatype, heap);
	break;
      case SQLDESC_PRECISION:
	{
	  rc = processNumericDatatypeWithPrecision(descItem, diags);
	  if (rc == ERROR)
	    return rc;
	}
	break;
      case SQLDESC_DATETIME_CODE:
	DescItemDefaultsForDatetimeCode(descItem,
					descItem.datetime_code);
	break;
      case SQLDESC_IND_TYPE:
	setIndLength(descItem);
	break;
      case SQLDESC_IND_DATA:
	// ind_ptr becomes undefined.
	descItem.ind_ptr = 0;
	if (descItem.ind_data != 0 && descItem.ind_length == 0) {
	  // It is likely defined by the system
	  descItem.ind_datatype = REC_BIN16_SIGNED;
	  setIndLength(descItem);
	}
	break;
      
        case SQLDESC_CHAR_SET_NAM: 
         {
          charset_name = string_value;
          if (target) charset_name = target;
      
          newCharSet = CharInfo::getCharSetEnum(charset_name);
         }

         default:
          break;
      } // switch
    } // source != NULL

  // final processing for char-typed or rowset items
  switch (what_to_set) {
     case SQLDESC_CHAR_SET_NAM: 
        // the name has been converted and translated to numeric value form
        // in variable newCharSet. The name (null-terminator form) is pointed
        // at by charset_name. If a conversion is involved, the variable 
        // 'target' has the null-terminator name. Need to delete 'target'
        // after the following code has been executed.

        // fall through

     case SQLDESC_CHAR_SET: 
         {
          if ( charset_name == NULL )
             charset_name = (char*)CharInfo::getCharSetName(newCharSet);

          if (( !DFS2REC::isAnyCharacter(descItem.datatype)  || 
               !CharInfo::isCharSetSupported(newCharSet)  
                ) && !((descItem.datatype != REC_BLOB) ||(descItem.datatype != REC_CLOB)))
          {
              //Error 8895: An invalid character set name for the descriptor item SQLDESC_CHAR_SET.
      	      diags << DgSqlCode(-CLI_INVALID_CHARSET_FOR_DESCRIPTOR) << DgString0(charset_name);
              return ERROR;
          }
      
      
          if ( newCharSet != descItem.charset ) {
      
           // The following check verifies the datatype and the charset fields are
           // compatible.  This method is called after the charset field is
           // set through CHARACTER_SET_NAME.
           //
           // Character Type Invariant (CTI):
           //   datatype is always in sync with charset:
           //       charset                              datatype
           //       ISO88591     REC_BYTE_F_ASCII, REC_BYTE_V_ASCII,
           //                    REC_BYTE_V_ASCII_LONG, or REC_BYTE_V_ANSI
           //       UNICODE      REC_BYTE_F_DOUBLE, REC_BYTE_V_DOUBLE, or
           //                    REC_BYTE_V_ANSI_DOUBLE
           //
           //   The length field always records the length in octets regardless of
           //   charset.
           //
           //   According to ANSI, the unit for SQLDESC_LENGTH is always characters,
           //   and that for SQLDESC_OCTET_LENGTH is 8-bit byte.
           //
             if ( newCharSet == CharInfo::ISO88591 )
             {
                switch ( descItem.charset )
                {
                 case CharInfo::UNICODE:
                   // adjust the datatype and the length fields.
                    switch ( descItem.datatype )
                    {
                       case REC_BYTE_F_DOUBLE:
                           descItem.datatype = REC_BYTE_F_ASCII;
                           break;
                       case REC_BYTE_V_DOUBLE:
                           descItem.datatype = REC_BYTE_V_ASCII;
                           break;
                       case REC_BYTE_V_ANSI_DOUBLE:
                           descItem.datatype = REC_BYTE_V_ANSI;
                           break;
                       default:
          	           diags << DgSqlCode(-CLI_INVALID_CHARSET_FOR_DESCRIPTOR) << DgString0(charset_name);
                           return ERROR;
                    }
                    // fall through
                  case CharInfo::KANJI_MP:
                  case CharInfo::KSC5601_MP:
                    descItem.length /= SQL_DBCHAR_SIZE;
                     break;
                  default:
                     break;
                }
             } else
             if ( newCharSet == CharInfo::UNICODE )
             {
                 if ( DFS2REC::is8bitCharacter(descItem.datatype) ) 
                 // ISO88591, KANJI, and KSC5601
                 {
                   switch ( descItem.datatype )
                   {
                      case REC_BYTE_F_ASCII:
                          descItem.datatype = REC_BYTE_F_DOUBLE;
                          break;
                      case REC_BYTE_V_ASCII_LONG:
                      case REC_BYTE_V_ASCII:
                          descItem.datatype = REC_BYTE_V_DOUBLE;
                          break;
                      case REC_BYTE_V_ANSI:
                          descItem.datatype = REC_BYTE_V_ANSI_DOUBLE;
                          break;
                      default:
         	          diags << DgSqlCode(-CLI_INVALID_CHARSET_FOR_DESCRIPTOR) << DgString0(charset_name);
                          return ERROR;
                   }
                   if (descItem.charset == CharInfo::ISO88591) 
                      descItem.length *= SQL_DBCHAR_SIZE;
                } 
             } else
                if ( CharInfo::is_NCHAR_MP(newCharSet) )
                {
                  if (descItem.charset == CharInfo::ISO88591) 
                     descItem.length *= SQL_DBCHAR_SIZE;
                  else
                  if (descItem.charset == CharInfo::UNICODE ) 
                  {
                     switch ( descItem.datatype )
                     {
                      case REC_BYTE_F_DOUBLE:
                          descItem.datatype = REC_BYTE_F_ASCII;
                          break;
                      case REC_BYTE_V_DOUBLE:
                          descItem.datatype = REC_BYTE_V_ASCII;
                          break;
                      case REC_BYTE_V_ANSI_DOUBLE:
                          descItem.datatype = REC_BYTE_V_ANSI;
                          break;
                      default:
      	                  diags << DgSqlCode(-CLI_INVALID_CHARSET_FOR_DESCRIPTOR) << DgString0(charset_name);
                          return ERROR;
                     }
                  }
                } 
      
             // set descItem.charset_name (char* with length at beginning)
	     if (descItem.charset_name)
	         heap.deallocateMemory(descItem.charset_name);
             descItem.charset_name = 
                 desc_varchar_alloc_and_copy(charset_name, heap, 
                                             str_len(charset_name)
                                            );
             // set descItem.charset 
             descItem.charset = newCharSet;
          }


          if ( what_to_set == SQLDESC_CHAR_SET_NAM && string_value )  
             heap.deallocateMemory(target); // deallocate the memory. Have to
                                            // it after the name is used in
                                            // the main block of code above.
       
      }
      break;

     case SQLDESC_LENGTH:
      {
        if ( descItem.charset == CharInfo::UNICODE ||
             CharInfo::is_NCHAR_MP((CharInfo::CharSet)descItem.charset) 
           )
          descItem.length *= SQL_DBCHAR_SIZE;
      }
      break;

      case SQLDESC_ROWSET_VAR_LAYOUT_SIZE:
      case SQLDESC_ROWSET_IND_LAYOUT_SIZE:
      case SQLDESC_VAR_DATA:
      case SQLDESC_IND_DATA:
	{
	  // VAR_DATA and IND_DATA not supported for rowsets. For ind_data, here we only
	  // catch those cases where ind_data is set o a non-zero value by the user.
	  // The zero case is caught during execution in inputValues() by checking for
	  // the absence of ind_ptr. This is because the value 0 for ind_data is also 
	  // used to denote no indicataor data.
	  if ((descItem.var_data || (descItem.ind_data != 0)) &&
	      (descItem.rowsetVarLayoutSize > 0 || descItem.rowsetIndLayoutSize > 0))
	  {
	    diags << DgSqlCode(-EXE_ROWSET_VARDATA_OR_INDDATA_ERROR);
	    return ERROR;
	  }
	}
	break;

     default:
      break;
  }
  return SUCCESS;
}

RETCODE Descriptor::setDescItemInternal(Lng32 entry, Lng32 what_to_set,
					Lng32 numeric_value, char * string_value)
{
  RETCODE rc = SUCCESS;
  
  desc_struct  & descItem = ((entry > 0) 
                             ? desc[entry - 1] : desc[0]); // Zero base

  ComDiagsArea & diags       = context_->diags();
  ComDiagsArea * diagsArea   = context_->getDiagsArea();
  NAHeap       & heap        = *context_->exHeap();
  CollHeap     * collHeap    = context_->exCollHeap();

  switch (what_to_set)
    {
    case SQLDESC_DATA_OFFSET:
      {
	descItem.data_offset = numeric_value;
      }
    break;

    case SQLDESC_NULL_IND_OFFSET:
      {
	descItem.null_ind_offset = numeric_value;
      }
    break;

    case SQLDESC_ALIGNED_LENGTH:
      {
	descItem.aligned_length = numeric_value;
      }
    break;

    default:
      {
	diags << DgSqlCode(-CLI_INVALID_DESC_ENTRY);
	return ERROR;
      }
    break;
    }

  return SUCCESS;
}

RETCODE Descriptor::alloc(Lng32 used_entries_)
{
  ComDiagsArea & diags       = context_->diags();
  NAHeap       & heap        = *(context_->exHeap());

  if (used_entries_ > max_entries)
    {
      diags << DgSqlCode(- CLI_DATA_OUTOFRANGE)
            << DgInt0(max_entries);
      return ERROR;
    }

  used_entries = used_entries_;
  desc = (desc_struct *)
         heap.allocateMemory((size_t)used_entries * sizeof(desc_struct));
 
  // initialize entries. Maybe desc_struct should be a class
  // and call constructor here.
  for (Int32 i = 0; i < used_entries_; i++)
    { 
     // desc_struct &descItem = desc[i];

      // initialize everything
      memset((char*)&desc[i], 0, sizeof(struct desc_struct));
/*
      descItem.rowsetVarLayoutSize   = 0;
      descItem.rowsetIndLayoutSize   = 0;
      descItem.datatype              = 0;
      descItem.datetime_code         = 0;
      descItem.length                = 0;
      descItem.nullable              = 0;
      descItem.charset               = 0;
      descItem.charset_schema        = 0;
      descItem.charset_catalog	     = 0;
      descItem.charset_name          = 0;
      descItem.coll_seq              = 0;
      descItem.coll_schema           = 0;
      descItem.coll_catalog          = 0;
      descItem.coll_name             = 0;
      descItem.scale                 = 0;
      descItem.precision             = 0;
      descItem.int_leading_precision = 0; 
      descItem.output_name           = 0;
      descItem.generated_output_name = 0;
      descItem.table_name            = 0;
      descItem.schema_name           = 0;
      descItem.catalog_name          = 0;
      descItem.heading               = 0;
      descItem.string_format         = 0;
      descItem.var_ptr               = 0;
      descItem.var_data              = 0;
      descItem.ind_ptr               = 0; 
      descItem.ind_data              = 0;
      descItem.ind_datatype          = 0;
      descItem.ind_length            = 0;
      descItem.desc_flags	     = 0;
*/

      //  See SQL92, subclause 17.5, General Rules.
      //  There may also be work in the generator for
      //  static descriptors
    }
 
  return SUCCESS;
}

RETCODE Descriptor::dealloc()
{
  if (desc)
    {
      NAHeap * heap = context_->exHeap();
       for (Int32 i = 0; i < used_entries; i++)
	{
	  if (desc[i].output_name)
	    heap->deallocateMemory(desc[i].output_name);
	  if (desc[i].heading)
	    heap->deallocateMemory(desc[i].heading);
	  if (desc[i].var_data)
	    heap->deallocateMemory(desc[i].var_data);
	  if (desc[i].table_name)
	    heap->deallocateMemory(desc[i].table_name);
	  if (desc[i].schema_name)
	    heap->deallocateMemory(desc[i].schema_name);
	  if (desc[i].catalog_name)
	    heap->deallocateMemory(desc[i].catalog_name);
          if (desc[i].charset_schema)
            heap->deallocateMemory(desc[i].charset_schema);
          if (desc[i].charset_catalog)
            heap->deallocateMemory(desc[i].charset_catalog);
          if (desc[i].charset_name)
            heap->deallocateMemory(desc[i].charset_name);
          if (desc[i].coll_schema)
            heap->deallocateMemory(desc[i].coll_schema);
          if (desc[i].coll_catalog)
	    heap->deallocateMemory(desc[i].coll_catalog);
          if (desc[i].coll_name)
            heap->deallocateMemory(desc[i].coll_name); 
          if (desc[i].text_format)
            heap->deallocateMemory(desc[i].text_format); 
	}

      heap->deallocateMemory(desc);
    }
  used_entries = 0;
  desc = NULL;

  return SUCCESS;
}

RETCODE Descriptor::allocBulkMoveInfo()
{
  NAHeap * heap = context_->exHeap();

  if ((! bulkMoveInfo()) ||
      ((Lng32)bulkMoveInfo()->maxEntries() < used_entries))
    {
      if (bmInfo_)
	heap->deallocateMemory(bmInfo_);

      bmInfo_ = (BulkMoveInfo *)
	heap->allocateMemory(sizeof(BulkMoveInfo) +
			     (size_t)(used_entries-1) *
			     sizeof(BulkMoveInfo::BMInfoStruct));

      bmInfo_->maxEntries_ = used_entries;
    }

  bmInfo_->flags_ = 0;
  bmInfo_->usedEntries_ = 0;

  for (Int32 i=0; i<(used_entries-1); i++)
    bmInfo_->bmiArray_[i].bmiFlags_ = 0;
 
  return SUCCESS;
}

ULng32 Descriptor::getCompoundStmtsInfo() const 
{
  return compoundStmtsInfo_;
}

void Descriptor::setCompoundStmtsInfo(ULng32 info) 
{
  compoundStmtsInfo_ = info;
}

Lng32 Descriptor::getUsedEntryCount()
{
  return used_entries;
}

NABoolean Descriptor::isDescTypeWide() 
{ 
  return (flags_ & DESC_TYPE_WIDE) != 0; 
}

NABoolean Descriptor::doSlowMove() 
{ 
  return (flags_ & DO_SLOW_MOVE) != 0; 
}

NABoolean Descriptor::bulkMoveSetup()
{ 
  return (flags_ & BULK_MOVE_SETUP) != 0;
}

NABoolean Descriptor::bulkMoveDisabled() 
{ 
  return (flags_ & BULK_MOVE_DISABLED) != 0; 
}


void BulkMoveInfo::addEntry(ULng32 length, char * descDataPtr,
			    short exeAtpIndex, NABoolean isExePtr,
			    Long  exeOffset,
			    short firstEntryNum,short lastEntryNum,
                            NABoolean isVarchar, NABoolean isNullable)
{
  if (usedEntries() >= maxEntries())
    assert(usedEntries() < maxEntries());
  
  bmiArray_[usedEntries()].length_ = length;
  bmiArray_[usedEntries()].descDataPtr_ = descDataPtr;
  bmiArray_[usedEntries()].exeAtpIndex_ = exeAtpIndex;
  bmiArray_[usedEntries()].setIsExePtr(isExePtr);
  bmiArray_[usedEntries()].exeOffset_ = exeOffset;
  bmiArray_[usedEntries()].firstEntryNum_ = firstEntryNum;
  bmiArray_[usedEntries()].lastEntryNum_ = lastEntryNum;
  bmiArray_[usedEntries()].setIsVarchar(isVarchar);
  bmiArray_[usedEntries()].setIsNullable(isNullable);
  
  usedEntries_++;
}

RETCODE Descriptor::deallocBulkMoveInfo()
{
  NAHeap * heap = context_->exHeap();

  if (bmInfo_)
    heap->deallocateMemory(bmInfo_);

  bmInfo_ = NULL;

  return SUCCESS;
}


void Descriptor::setUsedEntryCount(Lng32 used_entries_)
{
  used_entries = used_entries_;
}

RETCODE Descriptor::addEntry(Lng32 entry)
{
  Lng32 currUsedEntries   = used_entries;
  desc_struct * currDesc = desc;
      
  alloc(entry);

  if (currDesc)
    {

      // if entry is less than the previously allocated entries,
      // then we just deallocate the previous entries.
      // Otherwise, copy them to the first currUsedEntries of the
      // newly allocated desc.

      if (entry > currUsedEntries)
         {
          for (Int32 i = 0; i < currUsedEntries; i++)
            {
              str_cpy_all((char *)(&desc[i]), (char *)(&currDesc[i]),
                          sizeof(desc_struct));
            }
         }
      
      // deallocate the previously allocated desc array.
      context_->exHeap()->deallocateMemory(currDesc);
    }

  return SUCCESS;
}

void stripBlanks(char * buf, Lng32& len)
{
// NOTE: buf is assumed to be non-NULL so it will not be checked again

   char* p = 0; // for locating the first non-blank char.

// find the real length
   Int32 i=0;
   for ( ; i<len; i++ ) {
      if ( buf[i] == 0 )
         break;

      if ( p == 0 && buf[i] != ' ' )
        p = buf+i;
   }

// all chars are blanks!
   if ( p == 0 ) {
     buf[0] = 0; len = 0; return;
   }

// get the length
   len = i;
   if ( p ) len -= (p - buf);

// skip the trailing blanks.
   register char * endbuf = p + len - 1;
   while ((endbuf >= p ) && (*endbuf == ' '))
     {
          *endbuf = '\0';
          --endbuf;
	  len--;
     }

// copy the striped substring back into the beginning of the buf
   if ( buf != p ) {
     str_cpy_all(buf, p, len);
     buf[len] = 0;
   }

}

void upperCase(char * buf)
{
  // VO, April 2004: Moved this functionality to ComRtUtils
  ComRt_Upshift (buf);
}

//
//  Get the name of a statement, cursor, or descriptor from a
//  descriptor id.  The descriptor containing the name must, of
//  course, be of a character type.  It is used to implement
//  getting the name/identifier from SQLDESC_ID and SQLSTMT_IDs
//  where the name_mode is stmt_via_desc, curs_via_desc, or desc_via_desc.
//  The function is shared/used by the Statement (incl. cursors)
//  and Descriptor constructors and by the ContextCli::getDescriptor()
//  and ContextCli::getStatement() methods.
//
//  It was made a global function because it is not code unique to
//  any of the three classes where it is used.  It was placed with
//  the code for the Descriptor class, because it is decoding a
//  Descriptor.
//
//  This function returns a dynamically allocated SQL_OBJ_ID and the
//  clients of this function are responsible for deallocating that string.
//

SQLCLI_OBJ_ID* Descriptor::GetNameViaDesc(SQLDESC_ID *desc_id, ContextCli *context, NAHeap &heap)
{
  ComDiagsArea *diagsArea = context->getDiagsArea();
  SQLDESC_ID tmpDescId;
  
  init_SQLCLI_OBJ_ID (&tmpDescId,
		      SQLCLI_CURRENT_VERSION,
		      desc_name,
		      desc_id->module,
		      desc_id->identifier,
		      0,
		      SQLCHARSETSTRING_ISO88591,
		      (Lng32) getIdLen(desc_id)
		  );

  Descriptor *desc = context->getDescriptor(&tmpDescId);
  if ( desc == NULL)
    {
      return NULL;
    }	
  ComDiagsArea & diags = desc->getContext()->diags();
  // there should only be one entry - otherwise it's an error
  if (desc->getUsedEntryCount() != 1)
    {
      return NULL;
    }
  
  Lng32 nullable;
  
  desc->getDescItem(1, SQLDESC_NULLABLE, &nullable, 0, 0, 0, 0);
  
  if (nullable)
    {
      Lng32 * isnullp;
      desc->getDescItem(1, SQLDESC_IND_PTR, &isnullp, 0, 0, 0, 0);
      if ((isnullp == 0) || *isnullp )
	return NULL;
    }
  
  char * data     = 0;
  Lng32   datatype = 0L;
  Lng32   length   = 0L;
  char * buf      = 0;
  
  desc->getDescItem(1, SQLDESC_TYPE_FS, &datatype, 0, 0, 0, 0);   
  desc->getDescItem(1, SQLDESC_VAR_PTR, &data, 0, 0, 0, 0);
  desc->getDescItem(1, SQLDESC_LENGTH,  &length, 0, 0, 0, 0);
  buf = (char *)heap.allocateMemory(length+1);
  
  if (!((datatype>=REC_MIN_CHARACTER) &&
	(datatype <= REC_MAX_CHARACTER)))
    {
      *diagsArea << DgSqlCode(-CLI_INVALID_SQL_ID);
      return NULL;
    }          

  short retcode = convDoIt(
       data,
       length,
       datatype,
       0,
       0,
       buf,
       length+1,
       REC_BYTE_V_ANSI,
       0,
       0,
       0,
       0,
       &heap,
       &diagsArea);

  if (retcode != ex_expr::EXPR_OK)
    {
      return NULL;
    }
  
  
  char *ansi_id = (char *)heap.allocateMemory(length+1);
  if (buf)
    {
      str_strip_blanks(buf,length);
      if (str_to_ansi_id(buf,ansi_id,length))
	{
	  diags << DgSqlCode(-CLI_INVALID_SQL_ID)
      		<< DgString0(buf);
	  return NULL; 
	}
      heap.deallocateMemory(buf);
    }
  
  
  // return allocated buffer containing name for stmt,cursor or descriptor
  //
  //   NOTE: space allocated from context_->exHeap()
  //         must be freed via:
  //           context_->exHeap()->deallocateMemory()
  //         or
  //           delete (context_->exHeap())
  
  SQLCLI_OBJ_ID* obj_id =
    (SQLCLI_OBJ_ID *)(heap.allocateMemory(sizeof(SQLCLI_OBJ_ID)));
  
  switch(desc_id->name_mode)
    {
    case curs_via_desc:
      
      init_SQLCLI_OBJ_ID(obj_id, SQLCLI_CURRENT_VERSION,
			 cursor_name, 0, ansi_id, 0, 
			 SQLCHARSETSTRING_ISO88591, length);
      break;
    case stmt_via_desc:
      
      init_SQLCLI_OBJ_ID(obj_id, SQLCLI_CURRENT_VERSION,
			 stmt_name, 0, ansi_id, 0,
			 SQLCHARSETSTRING_ISO88591, length);
      break;
    case desc_via_desc:
      
      init_SQLCLI_OBJ_ID(obj_id, SQLCLI_CURRENT_VERSION,
			 desc_name, 0, ansi_id, 0,
			 SQLCHARSETSTRING_ISO88591, length);
      break;  
    default:
      return NULL;
    }
 
  return obj_id;
 
}

/* This method is defined in file CliExpExchange.cpp.
NABoolean Descriptor::isBulkMovePossible(short entry, Attributes * op,
                                         long &varPtr);
*/


// Methods to lock and unlock a Descriptor for use by a pending
// no-wait operation. $$$ Note that these are not thread-safe. For
// the moment, we are depending on the CLI semaphore to guarantee
// safety. This works correctly except for the Cancel thread on
// NT. For a product implementation, we will need a semaphore for
// this locking activity.


RETCODE Descriptor::lockForNoWaitOp(void)
  {
  // return ERROR if already locked, SUCCESS if not
  RETCODE rc = lockedForNoWaitOp_ ? ERROR : SUCCESS;

  lockedForNoWaitOp_ = TRUE;

  return rc;
  }

RETCODE Descriptor::unlockForNoWaitOp(void)
  {
  // return ERROR if not already locked, SUCCESS if it was locked
  RETCODE rc = lockedForNoWaitOp_ ? SUCCESS : ERROR;

  lockedForNoWaitOp_ = FALSE;

  return rc;
}

// call convertTypeToText_basic and set text_format field.
void Descriptor::set_text_format(desc_struct &descitem)
{
  NAHeap& heap = *(context_->exHeap());

  rec_datetime_field dt_start = REC_DATE_UNKNOWN;
  rec_datetime_field dt_end   = REC_DATE_UNKNOWN;
  switch (descitem.datatype)
  {
    case REC_DATETIME:
      switch (descitem.datetime_code)
      {
        case SQLDTCODE_DATE:
          dt_start = REC_DATE_YEAR;
          dt_end = REC_DATE_DAY;
          break;

        case SQLDTCODE_TIME:
          dt_start = REC_DATE_HOUR;
          dt_end = REC_DATE_SECOND;
          break;

        case SQLDTCODE_TIMESTAMP:
          dt_start = REC_DATE_YEAR;
          dt_end = REC_DATE_SECOND;
          break;
      }
      break;

    case REC_INT_YEAR:
      dt_start = REC_DATE_YEAR;
      dt_end = REC_DATE_YEAR;
      break;

    case REC_INT_MONTH:
      dt_start = REC_DATE_MONTH;
      dt_end = REC_DATE_MONTH;
      break;

    case REC_INT_YEAR_MONTH:
      dt_start = REC_DATE_YEAR;
      dt_end = REC_DATE_MONTH;
      break;

    case REC_INT_DAY:
      dt_start = REC_DATE_DAY;
      dt_end = REC_DATE_DAY;
      break;

    case REC_INT_HOUR:
      dt_start = REC_DATE_HOUR;
      dt_end = REC_DATE_HOUR;
      break;

    case REC_INT_DAY_HOUR:
      dt_start = REC_DATE_DAY;
      dt_end = REC_DATE_HOUR;
      break;

    case REC_INT_MINUTE:
      dt_start = REC_DATE_MINUTE;
      dt_end = REC_DATE_MINUTE;
      break;

    case REC_INT_HOUR_MINUTE:
      dt_start = REC_DATE_HOUR;
      dt_end = REC_DATE_MINUTE;
      break;

    case REC_INT_DAY_MINUTE:
      dt_start = REC_DATE_DAY;
      dt_end = REC_DATE_MINUTE;
      break;

    case REC_INT_SECOND:
      dt_start = REC_DATE_SECOND;
      dt_end = REC_DATE_SECOND;
      break;

    case REC_INT_MINUTE_SECOND:
      dt_start = REC_DATE_MINUTE;
      dt_end = REC_DATE_SECOND;
      break;

    case REC_INT_HOUR_SECOND:
      dt_start = REC_DATE_HOUR;
      dt_end = REC_DATE_SECOND;
      break;

    case REC_INT_DAY_SECOND:
      dt_start = REC_DATE_DAY;
      dt_end = REC_DATE_SECOND;
      break;
  }

  // 256 bytes must be more than enough for any type
  char temp_text_format[256];
  short retcode =
    convertTypeToText_basic(temp_text_format,
  	   	            descitem.datatype,
  		            descitem.length,
  		            descitem.precision,
		            descitem.scale,
		            dt_start,      // rec_datetime_field
		            dt_end,        // rec_datetime_field
		            (short) descitem.precision,
		            (short) descitem.int_leading_precision,
		            (short) 0,          // descitem.upshift,
			    (short) 0,          // caseinsensitive
		            (CharInfo::CharSet) descitem.charset,
		            descitem.coll_name ?
                              (descitem.coll_name + VCPREFIX_LEN) : NULL,
                            NULL,
			    0);

  if (retcode == 0)
  {
    descitem.text_format =
      desc_varchar_alloc_and_copy(temp_text_format,
                                  heap,
                                  str_len(temp_text_format));
  }
  else
  {
    descitem.text_format = NULL;
  }

  return;
}
