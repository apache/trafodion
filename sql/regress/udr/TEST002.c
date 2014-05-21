// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2000-2014 Hewlett-Packard Development Company, L.P.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <math.h>
#include "sqlcli.h"
#include "sqludr.h"
//#include "hdfs.h"

/*
tokenizer (TABLE doc_table, char pattern)
*/


SQLUDR_LIBFUNC SQLUDR_INT32 TOKENIZER(
  SQLUDR_CHAR           *in_data,
  SQLUDR_TMUDF_TRAIL_ARGS)
{
  
  char            *inData = NULL;
  SQLUDR_PARAM    *inParam = NULL;
  unsigned long    inDataLen = 0;

  char            *outData = NULL;
  SQLUDR_PARAM    *outParam = NULL;
  unsigned long   outDataLen = 0;

  char            pattern[5] ;
  unsigned int    i;

  char            inText[4096];
  char            *token;
  int             len = 0;

  SQLUDR_Q_STATE  qstate;
  SQLUDR_GetNextRow      getR = NULL;
  SQLUDR_EmitRow         emitR= NULL;
  SQLUDR_INT32 retcode = SQLUDR_SUCCESS;

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  /* For this test program, SQLUDR_CALLTYPE_INITIAL and */
  /* SQLUDR_CALLTYPE_NORMAL are the same. */

  /* store getRow and emitRow function pointers */
  /* passed as arguments */
  if((getRow == NULL) || (emitRow == NULL))
    return SQLUDR_ERROR;
  getR = getRow;
  emitR =emitRow; 

  /* extract all the scalar values */
  for(i= 0; i < udrinfo->num_inputs; i++)
  {
    inParam = &(udrinfo->inputs[i]);
    inData = in_data + inParam->data_offset;
    inDataLen = inParam->data_len; 
    
    /* expect scalar values to be input in desired order */
    switch(i)
    {
      case 0: /* scalar argument 1, pattern */
	{
	  if (inDataLen > 4)
	    return SQLUDR_ERROR;
	  memcpy(pattern, inData, inDataLen);
	  pattern[inDataLen] = 0;
	  break;
	}

      default:
	strncpy(msgtext,"The Table Mapping UDF TOKENIZER expects only "
                "one scalar input parameter", 72);
        return SQLUDR_ERROR;
    }
  }

  /* Determine length, type and address of single column in table input */
  for(i = 0; i < udrinfo->table_inputs[0].num_params; i++)
  {
    inParam = &udrinfo->table_inputs[0].params[i];
    inData = rowDataSpace1 + inParam->data_offset;
    inDataLen = inParam->data_len; 

    if (i != 0)
    {
      strncpy(msgtext,"The Table Mapping UDF TOKENIZER expects only "
              "one column in its table input", 75);
      /* expect only one column inputs? */
      return SQLUDR_ERROR;
    }
  }

  /* determine length and address of single column in table output */
  for(i = 0; i < udrinfo->num_return_values; i++)
  {
    outParam = &udrinfo->return_values[i];
    outData = rowDataSpace2 + outParam->data_offset;
    outDataLen = outParam->data_len; 
    if (i != 0)
    {
      strncpy(msgtext,"The Table Mapping UDF TOKENIZER will produce only "
              "one column in its output", 75);
      /* I informed MXCMP that I will produce only 1 value. */
      return SQLUDR_ERROR;
    }
  }
  
  do
  {
    /* Get one row from table input */
    getR(rowDataSpace1,0, &qstate);
    
    /* A row is expected only when qstate is SQLUDR_Q_MORE */
    if(qstate != SQLUDR_Q_MORE)
      break;
    /* row length is part of input row and has to be read in for each
       input row */
    if (inParam->datatype == SQLTYPECODE_VARCHAR_WITH_LENGTH)
    {
      char *vcInd = ((char *) rowDataSpace1) + inParam->vc_ind_offset;
      unsigned long vcIndLen = inParam->vc_ind_len;
      if (vcIndLen == 2)
        inDataLen = *((unsigned short *) vcInd);
      else
        inDataLen = *((unsigned long *) vcInd);
    }
    memset(inText, ' ', 4096);
    memcpy(inText, inData, inDataLen); 
    inText[inDataLen] = 0;
    
    /* Now process the data */
    token = strtok(inText, pattern);
    while (token != NULL)
    {
      memset(rowDataSpace2, 0, udrinfo->out_row_length);

      len = (int) strlen(token);
      memset(outData, ' ', outDataLen);
      memcpy(outData, token, len < outDataLen? len: outDataLen);
      /* Emit each token in table input row as a separate output row */
      emitR(rowDataSpace2,0,&qstate);
      if(qstate != SQLUDR_Q_MORE)
        break;
      token = strtok(NULL, pattern);
    }/* while end of row */
  }while(qstate == SQLUDR_Q_MORE);

  switch(qstate)
  {
    case SQLUDR_Q_EOD:
      /* emit row to indicate EOD. */
      emitR(rowDataSpace2,0,&qstate);
      if(qstate == SQLUDR_Q_CANCEL) 
      {
        strncpy(msgtext,"The Table Mapping UDF TOKENIZER received "
                "an unexpected CANCEL while emitting the last row", 90);
        return SQLUDR_ERROR;
      }
      retcode = SQLUDR_SUCCESS;
      break;
      
    case SQLUDR_Q_CANCEL:
      strncpy(msgtext,"The Table Mapping UDF TOKENIZER received "
              "an unexpected CANCEL",62);
      retcode = SQLUDR_ERROR;
      break;

    case SQLUDR_Q_MORE:
    default:
      /* we should not reach here. Means something is wrong. */
      strncpy(msgtext,"The Table Mapping UDF TOKENIZER encountered "
              "an internal error in MXUDR",71);
      retcode = SQLUDR_ERROR;
      break;
  };

  return retcode;

}/* main */

// SQLUDR_LIBFUNC SQLUDR_INT32 TOKENIZER_ValidateScalarInputs(
//    SQLUDR_CHAR         **in_data,                /* IN */
//    SQLUDR_INT16        *null_in_data,            /* IN */
//    SQLUDR_TMUDFINFO    *tmudfInfo,               /* IN */
//    SQLUDR_PARAM        *input_values,            /* OUT */
//    SQLUDR_CHAR          sqlstate[],              /* OUT */
//    SQLUDR_CHAR          msgtext[],               /* OUT */
//    SQLUDR_STATEAREA    *statearea                /* OUT */
//  ) 
// {
//   SQLUDR_INT32 retcode = SQLUDR_SUCCESS;
// 
//   if (!tmudfInfo)
//     return SQLUDR_ERROR;
// 
//   if (tmudfInfo->num_inputs == 1)
//   {
//     if (!input_values)
//       retcode = SQLUDR_ERROR;
//     if(input_values[0].datatype == SQLTYPECODE_CHAR)
//       retcode = SQLUDR_SUCCESS;
//     else
//     {
//       strcpy(msgtext, "Input parameter is not of character type.");
//       retcode = SQLUDR_ERROR;
//     }
//   }
//   else
//   {
//     strcpy(msgtext, "Too many input parameters.");
//     retcode = SQLUDR_ERROR;
//   }
//   return retcode;
// }

// SQLUDR_LIBFUNC SQLUDR_INT32 TOKENIZER_DescribeMaxOutputs(  
//   SQLUDR_CHAR           **in_data,               /* IN */
//   SQLUDR_INT16          *null_in_data,           /* IN */
//   SQLUDR_TMUDFINFO      *tmudfInfo,              /* IN */
//   SQLUDR_UINT32         *num_return_values,      /* OUT */
//   SQLUDR_PARAM          *return_values,          /* OUT */
//   SQLUDR_CHAR           sqlstate[],              /* OUT */
//   SQLUDR_CHAR           msgtext[],               /* OUT */
//   SQLUDR_STATEAREA      *statearea               /* OUT */
//  ) 
// {
//   if ((!tmudfInfo)||(!num_return_values)||(!return_values))
//     return SQLUDR_ERROR;
// 
//   if ((tmudfInfo->num_table_inputs != 1)||(!(tmudfInfo->table_inputs)))
//   {
//     strcpy(msgtext, "Expecting one table input");
//     return SQLUDR_ERROR;
//   }
// 
// 
//   *(num_return_values) = 1;
//   return_values[0].datatype = SQLTYPECODE_CHAR;
//   return_values[0].data_len = 40;
//   strcpy(return_values[0].name, "OUTVAL");
//   return_values[0].u1.character_set = SQLCHARSETCODE_ISO88591;
//   return_values[0].u2.collation = SQLUDR_COLLATION_DEFAULT;
//   return_values[0].ind_offset = 0;
// 
//   return SQLUDR_SUCCESS;
// }

SQLUDR_LIBFUNC SQLUDR_INT32 TOKENIZER_DescribeInputsAndOutputs(  
  SQLUDR_UINT32         num_table_inputs,         /* IN */
  SQLUDR_CHAR           **in_data,                /* IN */
  SQLUDR_TMUDFINFO      *tmudfInfo,               /* IN */
  SQLUDR_UINT32         num_return_values,        /* IN/OUT */
  SQLUDR_TABLE_PARAM    *table_inputs,            /* IN/OUT */
  SQLUDR_PARAM          *return_values,           /* IN/OUT */
  SQLUDR_CHAR           sqlstate[],               /* OUT */
  SQLUDR_CHAR           msgtext[],                /* OUT */
  SQLUDR_STATEAREA      *statearea                /* OUT */
 ) 
{return 0; }

SQLUDR_LIBFUNC SQLUDR_INT32 TOKENIZER_DescribeInputPartitionAndOrder(
  SQLUDR_UINT32         num_table_inputs ,         /* IN */ 
  SQLUDR_TABLE_PARAM    *table_inputs,             /* IN/OUT */
  SQLUDR_CHAR           **in_data,                 /* IN */
  SQLUDR_TMUDFINFO      *tmudfInfo,                /* IN */
  SQLUDR_CHAR           sqlstate[],                /* OUT */
  SQLUDR_CHAR           msgtext[],                 /* OUT */
  SQLUDR_STATEAREA      *statearea                 /* OUT */
  )
{return 0; }

SQLUDR_LIBFUNC SQLUDR_INT32 TOKENIZER_PredicatePushDown(
  SQLUDR_UINT32         num_table_inputs ,          /* IN */ 
  SQLUDR_TABLE_PARAM    *table_inputs,              /* IN/OUT */
  SQLUDR_CHAR           **in_data,                  /* IN */
  SQLUDR_TMUDFINFO      *tmudfInfo,                 /* IN */
  SQLUDR_CHAR           sqlstate[],                 /* OUT */
  SQLUDR_CHAR           msgtext[],                  /* OUT */
  SQLUDR_STATEAREA      *statearea                  /* OUT */
  )
{return 0; }

SQLUDR_LIBFUNC SQLUDR_INT32 TOKENIZER_Cardinality(
  SQLUDR_CHAR           **in_data,                  /* IN */
  SQLUDR_TMUDFINFO      *tmudfInfo,                 /* IN */
  SQLUDR_UINT32         num_out_rows,               /* OUT */
  SQLUDR_UINT32         *out_param_uecs,            /* OUT */
  SQLUDR_CHAR           sqlstate[],                 /* OUT */
  SQLUDR_CHAR           msgtext[],                  /* OUT */
  SQLUDR_STATEAREA      *statearea                  /* OUT */
  )
{return 0; }

SQLUDR_LIBFUNC SQLUDR_INT32 TOKENIZER_Constraints(
  SQLUDR_CHAR           **in_data,                  /* IN */
  SQLUDR_TMUDFINFO      *tmudfInfo,                 /* IN */
  SQLUDR_UINT32         num_constraints,            /* IN/OUT */
  SQLUDR_CONSTRAINT     *constraints,               /* IN/OUT */
  SQLUDR_CHAR           sqlstate[],                 /* OUT */
  SQLUDR_CHAR           msgtext[],                  /* OUT */
  SQLUDR_STATEAREA      *statearea                  /* OUT */
  )
{return 0; }

SQLUDR_LIBFUNC SQLUDR_INT32 TOKENIZER_Cost(
  SQLUDR_CHAR           **in_data,                  /* IN */
  SQLUDR_TMUDFINFO      *tmudfInfo,                 /* IN */
  SQLUDR_DOUBLE         cost_per_row,               /* OUT */
  SQLUDR_CHAR           sqlstate[],                 /* OUT */
  SQLUDR_CHAR           msgtext[],                  /* OUT */
  SQLUDR_STATEAREA      *statearea                  /* OUT */
  )
{return 0; }

SQLUDR_LIBFUNC SQLUDR_INT32 TOKENIZER_DegreeOfParallelism(
  SQLUDR_CHAR           **in_data,                  /* IN */
  SQLUDR_TMUDFINFO      *tmudfInfo,                 /* IN */
  SQLUDR_UINT32         degree_of_parallelism,      /* OUT */
  SQLUDR_CHAR           sqlstate[],                 /* OUT */
  SQLUDR_CHAR           msgtext[],                  /* OUT */
  SQLUDR_STATEAREA      *statearea                  /* OUT */
  )
{return 0; }

SQLUDR_LIBFUNC SQLUDR_INT32 TOKENIZER1(
  SQLUDR_CHAR           *in_data,
  SQLUDR_TMUDF_TRAIL_ARGS)
{
  
  char            *inData = NULL;
  SQLUDR_PARAM    *inParam = NULL;
  int             inDataLen = 0;

  char            *outData = NULL;
  SQLUDR_PARAM    *outParam = NULL;
  int             outDataLen = 0;

  char            pattern[5];
  unsigned int    i;
  char            inText[4096];

  FILE            *fp;
  char            *eof = NULL;

  char            *token;
  int             len = 0;

  SQLUDR_Q_STATE  qstate;
  SQLUDR_GetNextRow      getR = NULL;
  SQLUDR_EmitRow         emitR= NULL;
  SQLUDR_INT32 retcode = SQLUDR_SUCCESS;

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  /* For this test program, SQLUDR_CALLTYPE_INITIAL and */
  /* SQLUDR_CALLTYPE_NORMAL are the same. */

  /* store getRow and emitRow function pointers */
  /* passed as arguments */
  if((getRow == NULL) || (emitRow == NULL))
    return SQLUDR_ERROR;
  getR = getRow;
  emitR =emitRow; 

  /* extract all the scalar values */
  for(i= 0; i < udrinfo->num_inputs; i++)
  {
    inParam = &(udrinfo->inputs[i]);
    inData = in_data + inParam->data_offset;
    inDataLen = (int) inParam->data_len; 
    
    /* expect scalar values to be input in desired order */
    switch(i)
    {
      case 0: /* scalar argument 1, pattern */
	{
	  if (inDataLen > 4)
	    return SQLUDR_ERROR;
	  memcpy(pattern, inData, inDataLen);
	  pattern[inDataLen] = 0;
	  break;
	}
      default:
	strncpy(msgtext,"The Table Mapping UDF TOKENIZER1 expects only "
                "one scalar input parameter", 73);
        return SQLUDR_ERROR;
    }
  }

  /* Determine length, type and address of single column in table input */
  for(i = 0; i < udrinfo->table_inputs[0].num_params; i++)
  {
    inParam = &udrinfo->table_inputs[0].params[i];
    inData = rowDataSpace1 + inParam->data_offset;
    inDataLen = (int) inParam->data_len; 

    if (i != 0)
    {
      strncpy(msgtext,"The Table Mapping UDF TOKENIZER1 expects only "
              "one column in its table input", 76);
      /* expect only one column inputs? */
      return SQLUDR_ERROR;
    }
  }

  /* determine length and address of single column in table output */
  for(i = 0; i < udrinfo->num_return_values; i++)
  {
    outParam = &udrinfo->return_values[i];
    outData = rowDataSpace2 + outParam->data_offset;
    outDataLen = (int) outParam->data_len; 
    if (i != 0)
    {
      strncpy(msgtext,"The Table Mapping UDF TOKENIZER will produce only "
              "one column in its output", 75);
      /* I informed MXCMP that I will produce only 1 value. */
      return SQLUDR_ERROR;
    }
  }
  
  do
  {
    /* Get one row from table input */
    getR(rowDataSpace1,0, &qstate);
    
    /* A row is expected only when qstate is SQLUDR_Q_MORE. */
    if(qstate != SQLUDR_Q_MORE)
      break;
    /* row length is part of input row and has to be read in for each
       input row */
    if (inParam->datatype == SQLTYPECODE_VARCHAR_WITH_LENGTH)
    {
      char *vcInd = ((char *) rowDataSpace1) + inParam->vc_ind_offset;
      unsigned long vcIndLen = inParam->vc_ind_len;
      if (vcIndLen == 2)
        inDataLen = (int) *((unsigned short *) vcInd);
      else
        inDataLen = (int) *((unsigned long *) vcInd);
    }
    memset(inText, ' ', 4096);
    memcpy(inText, inData, inDataLen); 
    inText[inDataLen] = 0;

    /* open the file */
    fp=fopen(inText, "r");
    if (fp == 0)
    {
      strncpy(msgtext,"The Table Mapping UDF TOKENIZER1 encountered "
              "an error while opening a file", 75);
      return SQLUDR_ERROR;
    }

    do
    {

      memset(inText, ' ', 4096);
      /* read a line from file */
      eof = fgets(inText, 4050, fp);
      if (eof == 0) break;
      len = (int) strlen(inText);
      inText[len-1] = 0; /* remove new line character also */

      /* Now process the data */
      token = strtok(inText, pattern);
      while (token != NULL)
      {
        memset(rowDataSpace2, 0, udrinfo->out_row_length);
      
	len = (int) strlen(token);
	memset(outData, ' ', outDataLen);
	memcpy(outData, token, len < outDataLen? len: outDataLen);
	/* Emit each token in table input row as a separate output row */
	emitR(rowDataSpace2,0,&qstate);
	if(qstate != SQLUDR_Q_MORE)
	  break;
	token = strtok(NULL, pattern);
      }/* while end of row */
    } while(eof != NULL); /* end of file */
    fclose(fp);
  }while(qstate == SQLUDR_Q_MORE); /* end of table input */

  switch(qstate)
  {
    case SQLUDR_Q_EOD:
      /* emit row to indicate EOD. */
      emitR(rowDataSpace2,0,&qstate);
      if(qstate == SQLUDR_Q_CANCEL) 
      {
        strncpy(msgtext,"The Table Mapping UDF TOKENIZER1 received "
                "an unexpected CANCEL while emitting the last row", 91);
        return SQLUDR_ERROR;
      }
      retcode = SQLUDR_SUCCESS;
      break;
      
    case SQLUDR_Q_CANCEL:
      strncpy(msgtext,"The Table Mapping UDF TOKENIZER1 received "
              "an unexpected CANCEL",63);
      retcode = SQLUDR_ERROR;
      break;
      
    case SQLUDR_Q_MORE:
    default:
      /* we should not reach here. Means something is wrong. */
      strncpy(msgtext,"The Table Mapping UDF TOKENIZER1 encountered "
              "an internal error in MXUDR",72);
      retcode = SQLUDR_ERROR;
  }
  
  return retcode;
}/* main */

SQLUDR_LIBFUNC SQLUDR_INT32 TOKENIZER1_ValidateScalarInputs(  
  SQLUDR_CHAR         **in_data,                /* IN */
  SQLUDR_INT16        *null_in_data,            /* IN */
  SQLUDR_TMUDFINFO    *tmudfInfo,               /* IN */
  SQLUDR_PARAM        *input_values,            /* OUT */
  SQLUDR_CHAR          sqlstate[],              /* OUT */
  SQLUDR_CHAR          msgtext[],               /* OUT */
  SQLUDR_STATEAREA    *statearea                /* OUT */
 ) 
{
  SQLUDR_INT32 retcode = SQLUDR_SUCCESS;

  if (!tmudfInfo)
    return SQLUDR_ERROR;

  if (tmudfInfo->num_inputs == 1)
  {
    if (!input_values)
      retcode = SQLUDR_ERROR;
    if(input_values[0].datatype == SQLTYPECODE_CHAR)
      retcode = SQLUDR_SUCCESS;
    else
    {
      strcpy(msgtext, "Input parameter is not of character type.");
      retcode = SQLUDR_ERROR;
    }
  }
  else
  {
    strcpy(msgtext, "Too many input parameters.");
    retcode = SQLUDR_ERROR;
  }
  return retcode;
}

SQLUDR_LIBFUNC SQLUDR_INT32 TOKENIZER1_DescribeMaxOutputs(  
  SQLUDR_CHAR           **in_data,               /* IN */
  SQLUDR_INT16          *null_in_data,           /* IN */
  SQLUDR_TMUDFINFO      *tmudfInfo,              /* IN */
  SQLUDR_UINT32         *num_return_values,      /* OUT */
  SQLUDR_PARAM          *return_values,          /* OUT */
  SQLUDR_CHAR           sqlstate[],              /* OUT */
  SQLUDR_CHAR           msgtext[],               /* OUT */
  SQLUDR_STATEAREA      *statearea               /* OUT */
 ) 
{
  if ((!tmudfInfo)||(!num_return_values)||(!return_values))
    return SQLUDR_ERROR;

  if ((tmudfInfo->num_table_inputs != 1)||(!(tmudfInfo->table_inputs)))
  {
    strcpy(msgtext, "Expecting one table input");
    return SQLUDR_ERROR;
  }


  *(num_return_values) = 1;
  return_values[0].datatype = SQLTYPECODE_CHAR;
  return_values[0].data_len = 40;
  strcpy(return_values[0].name, "OUTVAL");
  return_values[0].u1.character_set = SQLCHARSETCODE_ISO88591;
  return_values[0].u2.collation = SQLUDR_COLLATION_DEFAULT;
  return_values[0].ind_offset = 0;

  return SQLUDR_SUCCESS;
}

