/*-------------------------------------------------------------------------
 *
 * json.h
 *    Declarations for JSON data type support.
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/json.h
 *
 *-------------------------------------------------------------------------
 */
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

#ifndef JSON_H
#define JSON_H

#include "ComJSONStringInfo.h"

#ifndef NULL
#define NULL	((void *) 0)
#endif

#ifndef true
#define true	((bool) 1)
#endif

#ifndef false
#define false	((bool) 0)
#endif

#define HIGHBIT					(0x80)
#define IS_HIGHBIT_SET(ch)		((unsigned char)(ch) & HIGHBIT)

typedef enum
{
    JSON_TOKEN_INVALID,
    JSON_TOKEN_STRING,
    JSON_TOKEN_NUMBER,
    JSON_TOKEN_OBJECT_START,
    JSON_TOKEN_OBJECT_END,
    JSON_TOKEN_ARRAY_START,
    JSON_TOKEN_ARRAY_END,
    JSON_TOKEN_COMMA,
    JSON_TOKEN_COLON,
    JSON_TOKEN_TRUE,
    JSON_TOKEN_FALSE,
    JSON_TOKEN_NULL,
    JSON_TOKEN_END
} JsonTokenType;

typedef enum					/* contexts of JSON parser */
{
    JSON_OK = 0,
    JSON_INVALID_TOKEN,
    JSON_INVALID_VALUE,			/* expecting a value */
    JSON_INVALID_STRING,			/* expecting a string (for a field name) */
    JSON_INVALID_ARRAY_START,		/* saw '[', expecting value or ']' */
    JSON_INVALID_ARRAY_NEXT,		/* saw array element, expecting ',' or ']' */
    JSON_INVALID_OBJECT_START,	/* saw '{', expecting label or '}' */
    JSON_INVALID_OBJECT_LABEL,	/* saw object label, expecting ':' */
    JSON_INVALID_OBJECT_NEXT,		/* saw object value, expecting ',' or '}' */
    JSON_INVALID_OBJECT_COMMA,	/* saw object ',', expecting next label */
    JSON_INVALID_END,				/* saw the end of a document, expect nothing */
    JSON_END_PREMATURELY,       /*the input ended prematurely*/
    JSON_UNEXPECTED_ERROR
} JsonReturnType;

/*
 * All the fields in this structure should be treated as read-only.
 *
 * If strval is not null, then it should contain the de-escaped value
 * of the lexeme if it's a string. Otherwise most of these field names
 * should be self-explanatory.
 *
 * line_number and line_start are principally for use by the parser's
 * error reporting routines.
 * token_terminator and prev_token_terminator point to the character
 * AFTER the end of the token, i.e. where there would be a nul byte
 * if we were using nul-terminated strings.
 */
typedef struct JsonLexContext
{
    char	   *input;
    int			input_length;
    char	   *token_start;
    char	   *token_terminator;
    char	   *prev_token_terminator;
    JsonTokenType token_type;
    int			lex_level;
    int			line_number;
    char	   *line_start;
    StringInfo	strval;
} JsonLexContext;

typedef JsonReturnType (*json_struct_action) (void *state);
typedef JsonReturnType (*json_ofield_action) (void *state, char *fname, bool isnull);
typedef JsonReturnType (*json_aelem_action) (void *state, bool isnull);
typedef JsonReturnType (*json_scalar_action) (void *state, char *token, JsonTokenType tokentype);

/*
 * Semantic Action structure for use in parsing json.
 * Any of these actions can be NULL, in which case nothing is done at that
 * point, Likewise, semstate can be NULL. Using an all-NULL structure amounts
 * to doing a pure parse with no side-effects, and is therefore exactly
 * what the json input routines do.
 *
 * The 'fname' and 'token' strings passed to these actions are malloc'd.
 * They are not free'd or used further by the parser, so the action function
 * is free to do what it wishes with them.
 */
typedef struct JsonSemAction
{
    void	   *semstate;
    json_struct_action object_start;
    json_struct_action object_end;
    json_struct_action array_start;
    json_struct_action array_end;
    json_ofield_action object_field_start;
    json_ofield_action object_field_end;
    json_aelem_action array_element_start;
    json_aelem_action array_element_end;
    json_scalar_action scalar;
} JsonSemAction;

/* functions in jsonfuncs.c */
extern JsonReturnType json_object_field_text(char *json, char *fieldName);
extern JsonReturnType json_extract_path(char **result, char *json, short nargs, ...);
extern JsonReturnType json_extract_path_text(char **result, char *json, short nargs, ...);

JsonLexContext *makeJsonLexContext(char *json, bool need_escapes);
JsonLexContext *makeJsonLexContextCstringLen(char *json, int len, bool need_escapes);
JsonReturnType pg_parse_json(JsonLexContext *lex, JsonSemAction *sem);
JsonReturnType json_count_array_elements(JsonLexContext *lex, int &count);
void escape_json(StringInfo buf, const char *str);


#endif   /* JSON_H */
