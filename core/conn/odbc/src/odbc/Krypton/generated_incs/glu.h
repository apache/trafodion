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
/*
 * glu.h
 *
 * Andrew Schofield, Thursday, the 6th of October 1994
 *
 * $Id: glu_head.h 1.14 1997/12/18 08:57:26 nagler PACKAGED $
 *
 * The Graceland Utilities (GLU) are a set of routines to
 * facilitate the use of the Graceland Idiom.
 *
 * The following functions are used to prepare
 * an interface description for use by the other
 * GLU functions:
 *
 * - "GLU_INTERFACE_PREPARE"
 *
 * - "GLU_INTERFACE_PREPARE_LENGTH"
 *
 * The following functions are used to pack/unpack requests/responses:
 *
 * - "GLU_HEAD_UNPACK"
 *
 * - "GLU_REQUEST_PACK"
 *
 * - "GLU_REQUEST_HEADER_UNPACK_BY_REF"
 *
 * - "GLU_REQUEST_BODY_UNPACK"
 *
 * - "GLU_RESPONSE_PACK"
 *
 * - "GLU_RESPONSE_HEADER_UNPACK"
 *
 * The following functions are used to copy requests/responses in memory:
 *
 * - "GLU_REQUEST_COPY"
 *
 * - "GLU_RESPONSE_COPY"
 *
 * The following functions are used to pack/unpack encapsulated
 * service context data:
 *
 * - GLU_ENCAPSULATION_PACK
 *
 * - GLU_ENCAPSULATION_UNPACK
 *
 * The following functions are used to obtain information
 * about a prepared interface (e.g. for use by CEE):
 *
 * - "GLU_GET_NR_OPERATIONS"
 *
 * - "GLU_GET_NR_PARAMETERS"
 *
 * - "GLU_GET_OPERATION_ID"
 *
 * - "GLU_GET_OPERATION_INDEX"
 *
 * 
 * PARAMETER VECTORS ("pv")
 * 
 * GLU uses the concept of a "parameter vector" or, simply, "pv" to pass
 * arbitrary object call parameters.  Synchronous and asynchronous calls have
 * different "pv" formats.  The "synch" parameter to the request/response
 * un/pack calls defines this format.  To aid the explanation the values
 * and types for CEE Example #1 TIM::Now operation are used in the diagrams
 * below.  The order of the parameters is the same as they were defined
 * in the IDL.
 *
 * A synchronous "pv" has the same format in the client and the server as
 * follows (C types in parentheses): 
 *
 *  Index   Description
 *        +-------------------------------------------------+
 *      0 | Pointer to Operation Synopsis or NULL           |
 *        | (IDL_unsigned long *)                           |
 *        +-------------------------------------------------+
 *      1 | Pointer to pointer to Operation's Exception     |
 *        | (struct **TIM_Now_exc_)                         |
 *        +-------------------------------------------------+
 *      2 | Pointer to Operation Result or First Parameter  |
 *        | (TimeZoneDef *)                                 |
 *        +-------------------------------------------------+
 *      3 | Pointer to Next Parameter			    |
 *        | (IDL_string **)                                 |
 *        +-------------------------------------------------+
 *
 * In the above layout, note that the second parameter (IDL_string) is
 * passed by reference, hence the extra level of indirection.  The
 * operation's exception structure is always passed by reference.
 *
 * The "pv" for asynchronous clients and servers is different, because
 * it only contains those parameters which are essential to the particular
 * action.
 *
 * Asynchronous (!synch) "GLU_REQUEST_PACK" and "GLU_REQUEST_BODY_UNPACK"
 * parameter vector (pv) layout:
 * 
 *  Index   Description
 *        +-------------------------------------------------+
 *      0 | Pointer to Operation Synopsis or NULL           |
 *        | (IDL_unsigned long *)                           |
 *        +-------------------------------------------------+
 *      1 | Pointer to First "in" or "inout" Parameter      |
 *        | (TimeZoneDef *)                                 |
 *        +-------------------------------------------------+
 *
 * Asynchronous (!synch) "GLU_RESPONSE_PACK" and "GLU_RESPONSE_BODY_UNPACK"
 * parameter vector (pv) layout:
 * 
 *  Index   Description
 *        +-------------------------------------------------+
 *      0 | Pointer to Operation Synopsis or NULL           |
 *        | (IDL_unsigned long *)                           |
 *        +-------------------------------------------------+
 *      1 | Pointer to pointer to Operation's Exception     |
 *        | (struct **TIM_Now_exc_)                         |
 *        +-------------------------------------------------+
 *      2 | Pointer to Operation Result                     |
 *        | or First "in" or "inout" Parameter              |
 *        | (TimeZoneDef *)                                 |
 *        +-------------------------------------------------+
 * 
 * MESSAGE FORMATS
 * 
 * All GLU messages begin with a GLU_header_def as follows:
 *
 * Absolute
 *  Offset  Description
 *        +-------------------------------------------------+
 *      0 | Magic Bytes: "GLU"                              |
 *        +-------------------------------------------------+
 *      4 | Version Bytes: "002"                            |
 *        +-------------------------------------------------+
 *      7 | Type Byte: 'Q', 'P', or 'C'                     |
 *        +-------------------------------------------------+
 *      8 | Total Msg Size: Big Endian ulong                |
 *        +-------------------------------------------------+
 *     12 | Request Id: Big Endian ulong                    |
 *        +-------------------------------------------------+
 *     16
 *
 * Cancel (type 'C') messages contain nothing but this header.
 * Request and Response messages are followed by an "encapsulation".
 * An encapsulation is an octet stream which begins with a
 * four-byte GLU_data_format_def as follows (offsets continued):
 *
 * Absolute
 *  Offset  Description
 *        +-------------------------------------------------+
 *     17 | Integer Format Byte: 1 or 2                     |
 *        +-------------------------------------------------+
 *     18 | Char Format Byte: 65                            |
 *        +-------------------------------------------------+
 *     19 | Real Format Byte: 33 or 34                      |
 *        +-------------------------------------------------+
 *     20 | Pack Format Byte: 97                            |
 *        +-------------------------------------------------+
 *     21
 *
 * The various formats are defined in pcu.h.
 *
 * A GLU_response_header_def or GLU_request_header_def follows the
 * encapsulation data format description.  Unlike the GLU_header_def
 * which always contains big endian integers, integers contained in the
 * encapsulation are in the order defined by the GLU_data_format_def
 * that begins the encapsulation.  Therefore, in the following layouts
 * only the type of the integer is specified.
 *
 * The data in requests and responses is variable length.  The
 * following is the layout of the request from CEE Example #1.
 * For this example, there service context list is empty.
 * Service contexts are discussed later.
 *
 * The GLU_request_header_def for a "Now" request to the "TIME" object
 * is laid out as follows (offsets continued):
 *
 * Absolute
 *  Offset  Description
 *        +-------------------------------------------------+
 *     21 | Service Context List Length: ulong (0)          |
 *        +-------------------------------------------------+
 *     25 | Object Id Length: ulong (4)                     |
 *        +-------------------------------------------------+
 *     29 | Object Id Chars: "TIME"                         |
 *        +-------------------------------------------------+
 *     33 | Operation Synopsis: long (1203756531)           |
 *        +-------------------------------------------------+
 *     37 | Operation Name Length: ulong (3)                |
 *        +-------------------------------------------------+
 *     41 | Operation Name Chars: "Now"                     |
 *        +-------------------------------------------------+
 *     44
 *
 * Immediately following the request come the "in" and "inout" parameters
 * in the order they were declared in the IDL file.  "TIM::Now" has one
 * in parameter, TimeZoneDef, an enum.  The parameters are laid out as
 * follows (offsets continued):
 *
 * Absolute
 *  Offset  Description
 *        +-------------------------------------------------+
 *     21 | TimeZoneDef: GMT (0) or LOCAL (1)               |
 *        +-------------------------------------------------+
 *     25
 *
 * A GLU reply contains a GLU_header_def and an encapsulation.  The
 * encapsulation begins with a GLU_data_format_def as described above
 * and contains a GLU_response_header_def, a GLU_exception_def, and
 * the result (if any) followed by the "out" and "inout" parameters.
 * In the event of a user exception, the GLU_exception_def is followed
 * by the marshalled user exception.  A standard exception consists
 * solely of the GLU_exception_def.
 *
 * Again, a response is variable length.  In this case, there is
 * no service context list and a normal response of "11:11:11" is
 * shown.  The GLU_response_header_def follows (offsets continued
 * from GLU_data_format_def):
 *
 * Absolute
 *  Offset  Description
 *        +-------------------------------------------------+
 *     21 | Service Context List Length: ulong (0)          |
 *        +-------------------------------------------------+
 *     25 | Response Status Byte: 0                         |
 *        +-------------------------------------------------+
 *     26
 *
 * The GLU_exception_def is laid out as follows (offsets continued):
 *
 * Absolute
 *  Offset  Description
 *        +-------------------------------------------------+
 *     26 | exception_nr: long (0)                          |
 *        +-------------------------------------------------+
 *     30 | exception_detail: long (0)                      |
 *        +-------------------------------------------------+
 *     34
 *
 * The result and "out" and "inout" parameters follow.  In this case,
 * there is no result.  There is a single out parameter, an unbounded
 * string:
 *
 * Absolute
 *  Offset  Description
 *        +-------------------------------------------------+
 *     26 | timeStr Length: ulong (8)                       |
 *        +-------------------------------------------------+
 *     30 | timeStr Chars: "11:11:11"                       |
 *        +-------------------------------------------------+
 *     38
 *
 * The Service Context is an unbound sequence of "tagged" sequences of octets.
 * A single element defines an implicit parameter associated with the request
 * or response.  A "tagged" sequence of octets is the term the OMG uses for a
 * type-specific datum.  The "tags" in this case are a "type" and some "flags".
 * The flags help the receiving ORB determine what to do with the datum without
 * actually having to unpack it.  In particular, if the "must use" flag isn't
 * set, the ORB is free to ignore the contents.  As new service context types
 * are added over time, older ORBs may not support (nor recognize) the new
 * facilities and therefore the ORB needs to know whether to accept or reject
 * the request based solely on the flags.  The "IOP" (CORBA/GIOP/IIOP/) flag
 * indicates the data is from another name space, managed by the OMG.  This
 * allows gateways to pass through uninterpretable service context elements.
 * The ultimate destination then can make a decision as to whether to accept or
 * reject the request.
 *
 * The layout of a Service Context List follows for the case of one
 * element which is 42 bytes long with type 13 and flags 0 (continued from
 * GLU_data_format_def):
 *
 * Absolute
 *  Offset  Description
 *        +-------------------------------------------------+
 *     21 | Service Context List Length: ulong (1)          |
 *        +-------------------------------------------------+
 *     25 | Service Context[0] Type: ulong (13)             |
 *        +-------------------------------------------------+
 *     29 | Service Context[0] Flags: ulong (0)             |
 *        +-------------------------------------------------+
 *     33 | Service Context[0] Data Length: ulong (42)      |
 *        +-------------------------------------------------+
 *     37 | Service Context[0] Data Bytes                   |
 *        +-------------------------------------------------+
 *     79
 *
 * A service context element (data) is an encapsulation.  As with all
 * encapsulations, it begins with a GLU_data_format_def.  The format of each
 * service context element may not necessarily be the same as the format of the
 * rest of the request or response header and body.  This allows gateways to
 * pass through the service context data without unpacking it.  The function
 * GLU_ENCAPSULATION_UNPACK first unpacks the GLU_data_format_def and
 * the encapsulated data.
 */
#ifndef GLU_H
#define GLU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <glu_rs.h>
#include <glu_stdexc.h>



#ifndef GLU_H_
#define GLU_H_
/*
 * Translation unit: GLU
 * Generated by CNPGEN(TANTAU CNPGEN TANTAU_AG_PC1 19991203.133909) on Mon Nov 20 10:39:05 2000
 */
#include <idltype.h>
#if IDL_TYPE_H_VERSION != 19971225
#error Version mismatch IDL_TYPE_H_VERSION != 19971225
#endif

/****************
 * Module 'GLU' *
 ****************/
#define GLU_INVALID_OPERATION_INDEX ((IDL_unsigned_long) 0)
struct GLU_exception_tag {
  IDL_long exception_nr;
  IDL_long exception_detail;
};
typedef struct GLU_exception_tag GLU_exception_def;
#define GLU_exception_def_cin_ ((char *) "b2+FF")
#define GLU_exception_def_csz_ ((IDL_unsigned_long) 5)
#define GLU_MAGIC_STRING ((IDL_string) "GLU ")
#define GLU_MAGIC_SIZE ((IDL_unsigned_long) 4)
#define GLU_VERSION_STRING ((IDL_string) "002")
#define GLU_VERSION_SIZE ((IDL_unsigned_long) 3)
struct GLU_header_tag {
  IDL_octet magic[4];
  IDL_octet version[3];
  IDL_octet type;
  IDL_unsigned_long size;
  IDL_unsigned_long request_id;
};
typedef struct GLU_header_tag GLU_header_def;
#define GLU_header_def_cin_ ((char *) "b5+a1+4+Ha1+3+HHKK")
#define GLU_header_def_csz_ ((IDL_unsigned_long) 18)
#define GLU_HEADER_SIZE ((IDL_unsigned_long) 16)
struct GLU_data_format_tag {
  IDL_octet integer;
  IDL_octet character;
  IDL_octet real;
  IDL_octet pack;
};
typedef struct GLU_data_format_tag GLU_data_format_def;
#define GLU_data_format_def_cin_ ((char *) "b4+HHHH")
#define GLU_data_format_def_csz_ ((IDL_unsigned_long) 7)
#define GLU_DATA_FORMAT_SIZE ((IDL_unsigned_long) 4)
struct GLU_service_context_tag {
  IDL_unsigned_long type;
  IDL_unsigned_long flags;
  struct GLU_service_context_tag_data_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    IDL_octet *_buffer;
    IDL_PTR_PAD(_buffer, 1)
  } data;
};
typedef struct GLU_service_context_tag GLU_service_context_def;
#define GLU_service_context_def_cin_ ((char *) "b3+KKc0+H")
#define GLU_service_context_def_csz_ ((IDL_unsigned_long) 9)
typedef struct GLU_service_context_list_def_seq_ {
  IDL_unsigned_long _length;
  char pad_to_offset_8_[4];
  GLU_service_context_def *_buffer;
  IDL_PTR_PAD(_buffer, 1)
} GLU_service_context_list_def;
#define GLU_service_context_list_def_cin_ ((char *) "c0+b3+KKc0+H")
#define GLU_service_context_list_def_csz_ ((IDL_unsigned_long) 12)
#define GLU_SERVICE_CONTEXT_FLAG_MUST_USE ((IDL_unsigned_long) 1)
#define GLU_SERVICE_CONTEXT_FLAG_IOP ((IDL_unsigned_long) 2)
#define GLU_TYPE_REQUEST ((IDL_char) 'Q')
struct GLU_request_header_tag {
  GLU_service_context_list_def service_context_list;
  struct GLU_request_header_tag_oid_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    IDL_char *_buffer;
    IDL_PTR_PAD(_buffer, 1)
  } oid;
  IDL_unsigned_long op_synopsis;
  char pad_to_offset_40_[4];
  struct GLU_request_header_tag_op_name_seq_ {
    IDL_unsigned_long _length;
    char pad_to_offset_8_[4];
    IDL_char *_buffer;
    IDL_PTR_PAD(_buffer, 1)
  } op_name;
};
typedef struct GLU_request_header_tag GLU_request_header_def;
#define GLU_request_header_def_cin_ ((char *) "b4+c0+b3+KKc0+Hc0+CKc0+C")
#define GLU_request_header_def_csz_ ((IDL_unsigned_long) 24)
typedef struct GLU_request_context_def_seq_ {
  IDL_unsigned_long _length;
  char pad_to_offset_8_[4];
  IDL_string *_buffer;
  IDL_PTR_PAD(_buffer, 1)
} GLU_request_context_def;
#define GLU_request_context_def_cin_ ((char *) "c0+d0+")
#define GLU_request_context_def_csz_ ((IDL_unsigned_long) 6)
#define GLU_TYPE_RESPONSE ((IDL_char) 'P')
struct GLU_response_header_tag {
  GLU_service_context_list_def service_context_list;
  IDL_octet status;
  char pad_to_size_24_[7];
};
typedef struct GLU_response_header_tag GLU_response_header_def;
#define GLU_response_header_def_cin_ ((char *) "b2+c0+b3+KKc0+HH")
#define GLU_response_header_def_csz_ ((IDL_unsigned_long) 16)
#define GLU_RESPONSE_STATUS_NONE ((IDL_unsigned_long) 0)
#define GLU_TYPE_REQUEST_CANCEL ((IDL_char) 'C')
/* End module: GLU */
/*
 * End translation unit: GLU
 */
#endif /* GLU_H_ */
/*
 * "GLU_PV_*" lays out a parameter vector.
 */
#define GLU_PV_SYNOPSIS			0
#define GLU_PV_EXCEPTION		(GLU_PV_SYNOPSIS + 1)
#define GLU_PV_ASYNCH_REQUEST_PARAM1	(GLU_PV_SYNOPSIS + 1)
#define GLU_PV_ASYNCH_RESPONSE_PARAM1	(GLU_PV_EXCEPTION + 1)
#define GLU_PV_SYNCH_PARAM1		GLU_PV_ASYNCH_RESPONSE_PARAM1

/*
 *
 * "GLU_INDEX_PARAMETER1" is the index of the first parameter.
 */
#define GLU_INDEX_PARAMETER1		1

/*
 *
 * "GLU_INDEX_OPERATION1" is the index of the first operation.
 */
#define GLU_INDEX_OPERATION1		1

/*
 *
 * "GLU_INDEX_EXCEPTION1" is the index of the first exception.
 */
#define GLU_INDEX_EXCEPTION1		1

/*
 * "GLU_GET_EXCEPTION_BY_REF" gets a exceptions name, direction, and
 * type (prep) given an operation index and except_index
 * (GLU_INDEX_EXCEPTION1 is first).
 *
 * <prep>               	contains the prepared CIN description as
 *				returned by "GLU_INTERFACE_PREPARE".
 *
 * <operation_index>    	which operation
 *
 * <except_index>		which exception
 * 				(starts with GLU_INDEX_EXCEPTION1)
 *
 * <except_id>			gets address of exception id in "prep"
 *
 * <except_id_len>		length of "*except_id"
 *
 * <except_pcu_prep>		gets the address of the PCU_PREPAREd cin
 *
 * <except_pcu_prep_len>	length of "except_pcu_prep" 
 */
extern GLU_status
GLU_GET_EXCEPTION_BY_REF(
  /* In  */ const void      		      	       *prep,
  /* In  */ IDL_unsigned_long           		operation_index,
  /* In  */ IDL_unsigned_long           		except_index,
  /* Out */ const char      	       		      **except_id,
  /* Out */ IDL_unsigned_long			       *except_id_len,
  /* Out */ const void				      **except_pcu_prep,
  /* Out */ IDL_unsigned_long			       *except_pcu_prep_len
  );

/*
 * "GLU_GET_INTERFACE_ID_BY_REF" is used to obtain the interface's ID.
 *
 * <prep>               	contains the prepared CIN description as 
 *                      	returned by "GLU_INTERFACE_PREPARE".
 *
 * <interface_id>		gets the address of the interface's ID
 *
 * <interface_id_len>    	length of "id"
 */
extern GLU_status
GLU_GET_INTERFACE_ID_BY_REF(
  /* In  */ const void      		      	       *prep,
  /* Out */ const char      	       		      **interface_id,
  /* Out */ IDL_unsigned_long  	        	       *interface_id_len);

/*
 * "GLU_GET_NR_EXCEPTIONS" is used to find out how many exceptions
 * an operation has. "GLU_GET_NR_EXCEPTIONS" returns
 * GLU_SUCCESS or a value indicating the reason for the failure.
 *
 * <prep>          	contains the prepared CIN description as returned by
 *                 	"GLU_INTERFACE_PREPARE".
 *
 * <operation_index>    specifies the operation whose exception count
 *                      is required.
 *                      Constants are generated for each operation
 *                      (suffix "_ldx_").
 *
 * <nr_exceptions>      gets the number of exceptions.
 */
extern GLU_status
GLU_GET_NR_EXCEPTIONS(
  /* In  */ const void  	       		       *prep,
  /* In  */ IDL_unsigned_long				operation_index,
  /* Out */ IDL_unsigned_long         		       *nr_exceptions);

/*
 * "GLU_GET_NR_OPERATIONS" is used to find out how many operations
 * are contained in an interface. "GLU_GET_NR_OPERATIONS" returns
 * GLU_SUCCESS or a value indicating the reason for the failure.
 *
 * <prep>          contains the prepared CIN description as returned by
 *                 "GLU_INTERFACE_PREPARE".
 *
 * <nr_operations> gets the number of operations in the interface.
 */
extern GLU_status
GLU_GET_NR_OPERATIONS(
  /* In  */ const void      	       		       *prep,
  /* Out */ IDL_unsigned_long         		       *nr_operations);

/*
 * "GLU_GET_NR_PARAMETERS" is used to find out how many parameters
 * an operation has. "GLU_get_nr_parameters" returns
 * GLU_SUCCESS or a value indicating the reason for the failure.
 *
 * <prep>          	contains the prepared CIN description as returned by
 *                 	"GLU_INTERFACE_PREPARE".
 *
 * <operation_index>    specifies the operation whose parameter count
 *                      is required.
 *                      Constants are generated for each operation
 *                      (suffix "_ldx_").
 *
 * <nr_parameters>      gets the number of parameters.
 *
 *
 *
 */
extern GLU_status
GLU_GET_NR_PARAMETERS(
  /* In  */ const void  	       		       *prep,
  /* In  */ IDL_unsigned_long				operation_index,
  /* Out */ IDL_unsigned_long         		       *nr_parameters);

/*
 * "GLU_GET_OPERATION_ID_BY_REF" is used to obtain an operation's ID and
 * synopsis given its index. "GLU_GET_OPERATION_ID_BY_REF" and
 * "GLU_GET_OPERATION_INDEX" can be used to match operations between
 * two different interfaces (for intracapsule calls).
 * "GLU_GET_OPERATION_ID_BY_REF" returns GLU_SUCCESS or a value indicating
 * the reason for the failure.
 *
 * <prep>               contains the prepared CIN description as returned
 *                      by "GLU_INTERFACE_PREPARE".
 *
 * <operation_index>    specifies the operation whose ID is required.
 *                      Constants are generated for each operation
 *                      (suffix "_ldx_").
 *
 * <operation_id>       gets the address of the operation's ID
 *
 * <operation_id_len>   length of operation_id (ignored if NULL)
 *
 * <operation_synopsis> gets the operation's synopsis.
 */
extern GLU_status
GLU_GET_OPERATION_ID_BY_REF(
  /* In  */ const void      		      	       *prep,
  /* In  */ IDL_unsigned_long           		operation_index,
  /* Out */ const char      	       		      **operation_id,
  /* Out  */ IDL_unsigned_long           	       *operation_id_len,
  /* Out */ IDL_long        	        	       *operation_synopsis);

/*
 * "GLU_GET_OPERATION_INDEX" gets an operation's index given its
 * ID and synopsis."GLU_GET_OPERATION_ID_BY_REF" and
 * "GLU_GET_OPERATION_INDEX" can be used to match operations between
 * two different interfaces (for intracapsule calls).
 * "GLU_GET_OPERATION_INDEX" returns
 * GLU_SUCCESS or a value indicating the reason for the failure.
 *
 * <prep>               contains the prepared CIN description as returned by
 *                     "GLU_INTERFACE_PREPARE".
 *
 * <operation_id>    	is the ID of the operation whose index is required.
 *
 * <operation_synopsis> is the synopsis of the operation whose index
 *                      is required.
 *
 * <operation_index>    gets the operations index.
 */
extern GLU_status
GLU_GET_OPERATION_INDEX(
  /* In  */ const void        	       		       *prep,
  /* In  */ const char      	       		       *operation_id,
  /* In  */ IDL_long         				operation_synopsis,
  /* Out */ IDL_unsigned_long 	       		       *operation_index);

/*
 * "GLU_GET_PARAMETER_BY_REF" gets a parameters name, direction, and
 * type (prep) given an operation index and param_index
 * (GLU_INDEX_PARAMETER1 is first).
 *
 * <prep>               	contains the prepared CIN description as
 *				returned by "GLU_INTERFACE_PREPARE".
 *
 * <operation_index>    	which operation
 *
 * <param_index>		which parameter
 * 				(starts with GLU_INDEX_PARAMETER1)
 *
 * <param_name>			gets address of parameter name in "prep"
 *
 * <param_name_len>		length of "*param_name"
 *
 * <param_direction>		one of the CIN_direction_* or
 *				CIN_function_result.  If there is a function
 *				result, it will be GLU_INDEX_PARAMETER1.
 *
 * <param_pcu_prep>		gets the address of the PCU_PREPAREd cin
 *
 * <param_pcu_prep_len>		length of "param_pcu_prep" 
 */
extern GLU_status
GLU_GET_PARAMETER_BY_REF(
  /* In  */ const void      		      	       *prep,
  /* In  */ IDL_unsigned_long           		operation_index,
  /* In  */ IDL_unsigned_long           		param_index,
  /* Out */ const char      	       		      **param_name,
  /* Out */ IDL_unsigned_long			       *param_name_len,
  /* Out */ char        	        	       *param_direction,
  /* Out */ const void				      **param_pcu_prep,
  /* Out */ IDL_unsigned_long			       *param_pcu_prep_len
  );

/*
 * "GLU_HEADER_UNPACK" unmarshals and validates the magic and version
 * of a GLU_header_def.
 *
 * <to_unpack_buf>	must pointer to a buffer which contains at least
 *			GLU_HEADER_SIZE bytes.  
 *
 * <header>   		all fields are set.
 */
extern GLU_status
GLU_HEADER_UNPACK(
  /* In    */ const IDL_octet			       *to_unpack_buf,
  /* Out   */ GLU_header_def			       *header);

/* Call this after you've packed the request/response_header and
   the body to pack the length, type, request_id, and data_format on
   the front of packed_buf. */
extern GLU_status
GLU_HEADER_PACK(
  /* In    */ IDL_unsigned_long 			request_id,
  /* In    */ char 					type,
  /* In    */ const GLU_data_format_def 	       *data_format,
  /* InOut */ IDL_octet 			       *packed_buf,
  /* In    */ IDL_unsigned_long 			packed_len
  );

/*
 * "GLU_INTERFACE_PREPARE" is used to prepare a CIN description
 * of an interface (generated by CNPGEN) for efficient use by
 * the other GLU routines.
 * "GLU_INTERFACE_PREPARE" returns
 * GLU_SUCCESS or a value indicating the reason for the failure.
 *
 * NOTE: Unlike PCU_PREPARE, "prep" may not be copied as it contains
 *       internal pointers.  To make a copy, call this routine again.
 *
 * <cinbuf>       is the generated CIN description of the interface.
 *
 * <cinlen>       is the size of CIN description contained in
 *                "cinbuf".
 *
 * <max_prep_len> is the maximum number of bytes that can be
 *                accommodated in "prep". The correct size can
 *                be obtained by calling "GLU_INTERFACE_PREPARE_length".
 *
 * <prep>         is the buffer to receive the prepared CIN.
 *
 * <prep_len> 	  gets the size of prepared CIN that will be
 *            	  produced by "GLU_INTERFACE_PREPARE".
 */
extern GLU_status
GLU_INTERFACE_PREPARE(
  /* In  */ const char        	       		       *cinbuf,
  /* In  */ IDL_unsigned_long  			  	cinlen,
  /* In  */ IDL_unsigned_long				max_prep_len,
  /* Out */ void              	       		       *prep,
  /* Out */ IDL_unsigned_long         		       *prep_len);

/*
 * "GLU_INTERFACE_PREPARE_LENGTH" is used to obtain the length of
 * the prepared interface description that will be produced by
 * "GLU_INTERFACE_PREPARE". This is useful if the memory for the
 * prepared description is to be allocated dynamically.
 * "GLU_INTERFACE_PREPARE_length" returns
 * GLU_SUCCESS or a value indicating the reason for the failure.
 *
 * <cinbuf>   is the generated CIN description of the interface.
 *
 * <cinlen>   is the size of CIN description contained in "cinbuf".
 *
 * <prep_len> gets the size of prepared CIN that will be
 *            produced by "GLU_INTERFACE_PREPARE".
 */
extern GLU_status
GLU_INTERFACE_PREPARE_LENGTH(
  /* In  */ const char  	       		       *cinbuf,
  /* In  */ IDL_unsigned_long        			cinlen,
  /* Out */ IDL_unsigned_long         		       *prep_len);

/*
 * "GLU_REQUEST_BODY_UNPACK" unpacks the parameters associated
 * with the request.  The "GLU_header_def" and "GLU_request_header_def"
 * must be unpacked by "GLU_HEADER_UNPACK" and
 * "GLU_REQUEST_HEADER_UNPACK_BY_REF", respectively.
 *
 * <data_format>		how to unmarshall the parameters.
 *				This value was returned by 
 * 				"GLU_REQUEST_HEADER_UNPACK_BY_REF".
 *				If NULL and request_header is NULL,
 *                              "GLU_HEADER_UNPACK" and
 *				"GLU_REQUEST_HEADER_UNPACK_BY_REF" will
 *				be called implicitly.
 *
 * <request_header>		defines the operation whose parameters
 *                              are being unpacked.  This value was returned
 *				by "GLU_REQUEST_HEADER_UNPACK_BY_REF".
 *				If NULL and data_format is NULL,
 *                              "GLU_HEADER_UNPACK" and
 *				"GLU_REQUEST_HEADER_UNPACK_BY_REF" will
 *				be called implicitly.
 *
 * <prep>			the prepared CIN which was returned by
 * 				"GLU_INTERFACE_PREPARE".  This value should
 *				be related to "request_header->oid".
 *
 * <synch>			is this a synchronous or asynchronous object
 *				call?  This value controls the interpretation
 * 				of "pv".
 *
 * <pv_len_max>			maximum number of elements in "pv"
 *
 * <to_unpack_buf>		marshalled request parameters.  This pointer
 * 				should point just after the
 * 				"GLU_request_header_def".  This value should be
 * 				computed by adding the "to_unpack_buf" and
 * 				"to_unpack_used" values passed to/from
 * 				"GLU_REQUEST_HEADER_UNPACK_BY_REF".
 *				If "GLU_HEADER_UNPACK" and
 *                              "GLU_REQUEST_HEADER_UNPACK_BY_REF" are
 *				to be called implicitly (see above),
 *				"to_unpack_buf" should point to
 *				"GLU_header_def.
 * 				
 * <to_unpack_len>		size of "to_unpack_buf".  This value should be
 * 				computed by subtracting the "to_unpack_len" and
 * 				"to_unpack_used" values passed to/from 
 * 				"GLU_REQUEST_HEADER_UNPACK_BY_REF".  
 * 				
 * <unpacked_len_max>		size of "unpacked_data".  This value should be
 * 				computed by subtracting the "unpacked_len_max"
 * 				and "unpacked_len" values passed to/from 
 * 				"GLU_REQUEST_HEADER_UNPACK_BY_REF".
 * 				
 * <unpacked_data>		buffer to unpack the parameters into.
 *
 * <unpacked_len>		sizeof of "unpacked_data" consumed by
 * 				this routine.
 *
 * <pv>				where to place the pointers to the unpacked
 * 				parameters.  For a detailed description,
 * 			 	see the section "PARAMETER VECTORS".
 * 				
 * <local_op_idx>		the index of the operation specified in
 * 				"request_header".
 * 				
 * <=GLU_SHORTOUTBUF>		the message could not be unpacked, because
 * 				"unpacked_len_max" was smaller than the space
 *				required.  "unpacked_len" is contains the
 *				appropriate length.  Reallocate "unpacked_data"
 * 				to at least "unpacked_len" bytes.
 * 				
 * <=GLU_BADPREPBUF>		"prep" is invalid.  Either the memory
 * 				is corrupt or "GLU_INTERFACE_PREPARE" was
 *				not called.
 *
 * <=GLU_NOSUCHOP>		the operation specified in "request_header"
 *				could not be found.  Either the name was 
 * 				not found or the operation synopsis did not
 * 				match, i.e. the operation's type signature
 *				defined in "prep" is not the same as the
 *				type signature used to pack the request.
 * 				
 * <=GLU_SHORTPV>		"pv_len_max" is shorter than required to
 * 				unpack all the parameters for the operation.
 *				"*local_op_idx" is valid.  The correct size
 *				should be two plus the parameter count
 *				for the operation.  GLU_GET_NR_PARAMETERS
 *				may be called to get the parameter count.
 *				Note that the "result" is treated as an
 *				"out" parameter and need not be handled
 *				specially.
 * 				
 * <=GLU_LONGINBUF>		the message was unpacked successfully, but
 *				"to_unpack_len" was greater than the data
 *				required to successfully unmarshal the
 * 				request.  This is probably a protocol
 * 				violation. 
 * 				
 * <=GLU_SHORTINBUF>		the message could not be unpacked, because
 * 				"to_unpack_len" was smaller than the data
 *				required.  This is probably a protocol
 * 				violation. 
 * 				
 * <=GLU_BADSEQSIZE>		while unmarshalling a parameter or exception,
 * 				a bounded sequence was encountered the
 *				length of which was greater than the bound.
 *				This is probably a protocol violation, because
 *				the sending ORB should have checked the length.
 * 				
 * <=GLU_BADSEQSIZE>		while unmarshalling a parameter or exception,
 * 				a bounded string was encountered the
 *				length of which was greater than the bound.
 *				This is probably a protocol violation, because
 *				the sending ORB should have checked the length.
 * 				
 * <=GLU_UNIMPLCONV>		while unmarshalling a parameter or exception,
 * 				a data type was encountered that could not
 *				be converted to the native format.  This is
 *				likely to be caused by a conversion between
 *				two different floating point (real) formats.
 *				For example, the client sent a NaN in IEEE
 *				format and the server's native format is
 *				Tandem format which doesn't have a
 * 				representation for NaN.
 *
 * <=GLU_NOTREQUEST>		the header type is not GLU_TYPE_REQUEST.
 * 				This status is only returned if
 *				"GLU_HEADER_DEF" is called implicitly.
 * 
 * <=GLU_UNKNOWN>		an unexpected PCU error occured.  This is
 *				probably a protocol violation.
 */
extern GLU_status
GLU_REQUEST_BODY_UNPACK(
  /* In  */ const GLU_data_format_def		       *data_format,
  /* In  */ const GLU_request_header_def	       *request_header,
  /* In  */ const void     	       		       *prep,
  /* In  */ IDL_boolean       				synch,
  /* In  */ IDL_unsigned_long 				pv_len_max,
  /* In  */ const IDL_octet			       *to_unpack_buf,
  /* In  */ IDL_unsigned_long				to_unpack_len,
  /* In  */ IDL_unsigned_long			       	unpacked_len_max,
  /* Out */ void				       *unpacked_data,
  /* Out */ IDL_unsigned_long			       *unpacked_len,
  /* Out */ void            			      **pv,
  /* Out */ IDL_unsigned_long 			       *local_op_idx);

extern GLU_status
GLU_REQUEST_COPY(
  /* In  */ const void      			       *prep,
  /* In  */ IDL_unsigned_long            		local_op_idx,
  /* In  */ IDL_boolean					synch_impl,
  /* In  */ const void *const 			       *pv,
  /* In  */ IDL_unsigned_long				max_msglen,
  /* Out */ void            			       *msgbuf,
  /* Out */ IDL_unsigned_long 			       *msglen);

/* If SHORTOUTBUF, then everything unpacked except service context */
extern GLU_status
GLU_REQUEST_HEADER_UNPACK_BY_REF(
  /* In  */ const IDL_octet			       *to_unpack_buf,
  /* In  */ IDL_unsigned_long				to_unpack_len,
  /* In  */ IDL_unsigned_long			       	unpacked_len_max,
  /* Out */ void				       *unpacked_data,
  /* Out */ IDL_unsigned_long			       *unpacked_len,
  /* Out */ GLU_data_format_def		       	       *data_format,
  /* Out */ GLU_request_header_def		       *request_header,
  /* Out */ IDL_unsigned_long			       *to_unpack_used);

extern GLU_status
GLU_REQUEST_PACK(
  /* In  */ IDL_unsigned_long				request_id,
  /* In  */ const GLU_service_context_list_def	       *service_context_list,
  /* In  */ const char				       *oid,
  /* In  */ IDL_unsigned_long			        oid_len,
  /* In  */ const void      	       		       *prep,
  /* In  */ IDL_unsigned_long 				local_op_idx,
  /* In  */ IDL_boolean      				synch,
  /* In  */ const void *const 	       		       *pv,
  /* In  */ IDL_unsigned_long				packed_len_max,
  /* Out */ IDL_octet         	       		       *packed_buf,
  /* Out */ IDL_unsigned_long 	       		       *packed_len);

/* If data_format AND response_header are NULL,
   GLU_RESPONSE_HEADER_UNPACK_BY_REF will be called. */
extern GLU_status
GLU_RESPONSE_BODY_UNPACK(
  /* In  */ const GLU_data_format_def		       *data_format,
  /* In  */ const GLU_response_header_def	       *response_header,
  /* In  */ const void     	       		       *prep,
  /* In  */ IDL_unsigned_long 			        local_op_idx,
  /* In  */ IDL_boolean       				synch,
  /* In  */ const IDL_octet			       *to_unpack_buf,
  /* In  */ IDL_unsigned_long				to_unpack_len,
  /* In  */ IDL_unsigned_long			       	unpacked_len_max,
  /* Out */ void				       *unpacked_data,
  /* Out */ IDL_unsigned_long			       *unpacked_len,
  /* Out */ void            			      **pv);

extern GLU_status
GLU_RESPONSE_COPY(
  /* In  */ const void      			       *prep,
  /* In  */ IDL_unsigned_long            		local_op_idx,
  /* In  */ IDL_boolean           			synch_impl,
  /* In  */ const void *const 			       *pv,
  /* In  */ IDL_unsigned_long				max_msglen,
  /* Out */ void            			       *msgbuf,
  /* Out */ IDL_unsigned_long			       *msglen);


/* If SHORTOUTBUF, then everything unpacked except service context */
extern GLU_status
GLU_RESPONSE_HEADER_UNPACK_BY_REF(
  /* In  */ const IDL_octet			       *to_unpack_buf,
  /* In  */ IDL_unsigned_long				to_unpack_len,
  /* In  */ IDL_unsigned_long			       	unpacked_len_max,
  /* Out */ void				       *unpacked_data,
  /* Out */ IDL_unsigned_long			       *unpacked_len,
  /* Out */ GLU_data_format_def		       	       *data_format,
  /* Out */ GLU_response_header_def		       *response_header,
  /* Out */ IDL_unsigned_long			       *to_unpack_used);

extern GLU_status
GLU_RESPONSE_PACK(
  /* In  */ IDL_unsigned_long				request_id,
  /* In  */ const GLU_service_context_list_def	       *service_context_list,
  /* In  */ char			        	status,
  /* In  */ const void      	       		       *prep,
  /* In  */ IDL_unsigned_long 				local_op_idx,
  /* In  */ IDL_boolean      				synch,
  /* In  */ const void *const 	       		       *pv,
  /* In  */ IDL_unsigned_long				packed_len_max,
  /* Out */ IDL_octet          	       		       *packed_buf,
  /* Out */ IDL_unsigned_long 	       		       *packed_len);

extern GLU_status
GLU_ENCAPSULATION_PACK(
  /* In  */ void				       *pcu_prep,
  /* In  */ void				       *to_pack_data,
  /* In  */ IDL_unsigned_long			       	packed_len_max,
  /* Out */ IDL_octet				       *packed_buf,
  /* Out */ IDL_unsigned_long			       *packed_len);

extern GLU_status
GLU_ENCAPSULATION_UNPACK(
  /* In  */ void				       *pcu_prep,
  /* In  */ const IDL_octet			       *to_unpack_buf,
  /* In  */ IDL_unsigned_long				to_unpack_len,
  /* In  */ IDL_unsigned_long			       	unpacked_len_max,
  /* Out */ void				       *unpacked_data,
  /* Out */ IDL_unsigned_long			       *unpacked_len);

extern GLU_status
GLU_ENCAPSULATION_UNPACK_SCATTER(
  /* In  */ void				       *pcu_prep,
  /* In  */ const IDL_octet			       *to_unpack_buf,
  /* In  */ IDL_unsigned_long				to_unpack_len,
  /* In  */ IDL_unsigned_long			       	unpacked_app_len_max,
  /* Out */ void				       *unpacked_base,
  /* Out */ void				       *unpacked_appendages,
  /* Out */ IDL_unsigned_long			       *unpacked_app_len);

/*
 * "GLU_STATUS_TO_TEXT" returns the address of a string
 * containing a description of the specified GLU status.
 * 
 * <sts>             is the return status code whose description is required.
 *
 * <=descr>          is the address of a zero terminated string containing
 *                   a description of "sts" 
 */
extern char *
GLU_STATUS_TO_TEXT(
  /* In  */ GLU_status  				sts);

/*
 * "GLU_STDEXC_TO_TEXT" returns the address of a string
 * containing a description of the specified standard exception number.
 * 
 * <exnr>             is the exception number whose description is required.
 *
 * <=descr>          is the address of a zero terminated string containing
 *                   a description of "exnr" 
 */
extern char *
GLU_STDEXC_TO_TEXT(
  /* In  */ GLU_exception_nr  		     		exnr);

extern GLU_status
GLU_STRUCT_REQUEST_PACK(
  /* In  */ IDL_unsigned_long				request_id,
  /* In  */ const GLU_service_context_list_def	       *service_context_list,
  /* In  */ const char				       *oid,
  /* In  */ IDL_unsigned_long			        oid_len,
  /* In  */ const void      	       		       *prep,
  /* In  */ IDL_unsigned_long 				local_op_idx,
  /* In  */ const void		 	       	       *to_pack_data,
  /* In  */ IDL_unsigned_long				packed_len_max,
  /* Out */ void            	       		       *packed_buf,
  /* Out */ IDL_unsigned_long 	       		       *packed_len);

extern GLU_status
GLU_STRUCT_REQUEST_BODY_UNPACK(
  /* In  */ const GLU_data_format_def		       *data_format,
  /* In  */ const GLU_request_header_def	       *request_header,
  /* In  */ const void      	       		       *prep,
  /* In  */ const IDL_octet			       *to_unpack_buf,
  /* In  */ IDL_unsigned_long				to_unpack_len,
  /* In  */ IDL_unsigned_long			       	unpacked_len_max,
  /* Out */ void				       *unpacked_data,
  /* Out */ IDL_unsigned_long			       *unpacked_len,
  /* Out */ IDL_unsigned_long 			       *local_op_idx);

extern GLU_status
GLU_STRUCT_RESPONSE_PACK(
  /* In  */ IDL_unsigned_long				request_id,
  /* In  */ const GLU_service_context_list_def	       *service_context_list,
  /* In  */ char			        	status,
  /* In  */ const void      	       		       *prep,
  /* In  */ IDL_unsigned_long 				local_op_idx,
  /* In  */ const void		 	       	       *to_pack_data,
  /* In  */ IDL_unsigned_long				packed_len_max,
  /* Out */ void            	       		       *packed_buf,
  /* Out */ IDL_unsigned_long 	       		       *packed_len);


extern GLU_status
GLU_STRUCT_RESPONSE_BODY_UNPACK(
  /* In  */ const GLU_data_format_def		       *data_format,
  /* In  */ const GLU_response_header_def	       *response_header,
  /* In  */ const void      	       		       *prep,
  /* In  */ IDL_unsigned_long 				local_op_idx,
  /* In  */ const IDL_octet			       *to_unpack_buf,
  /* In  */ IDL_unsigned_long				to_unpack_len,
  /* In  */ IDL_unsigned_long			       	unpacked_len_max,
  /* Out */ void				       *unpacked_data,
  /* Out */ IDL_unsigned_long			       *unpacked_len);

#ifdef __cplusplus
}
#endif
#endif
