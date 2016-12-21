/*-------------------------------------------------------------------------
 *
 * json.c
 *		JSON data type support.
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/json.c
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

#include "ComJSONStringInfo.h"
#include "ComJSON.h"
#include <string.h>
#include <stdlib.h>
#include "str.h"

/*
 * The context of the parser is maintained by the recursive descent
 * mechanism, but is passed explicitly to the error reporting routine
 * for better diagnostics.
 */
typedef enum					/* contexts of JSON parser */
{
    JSON_PARSE_VALUE,			/* expecting a value */
    JSON_PARSE_STRING,			/* expecting a string (for a field name) */
    JSON_PARSE_ARRAY_START,		/* saw '[', expecting value or ']' */
    JSON_PARSE_ARRAY_NEXT,		/* saw array element, expecting ',' or ']' */
    JSON_PARSE_OBJECT_START,	/* saw '{', expecting label or '}' */
    JSON_PARSE_OBJECT_LABEL,	/* saw object label, expecting ':' */
    JSON_PARSE_OBJECT_NEXT,		/* saw object value, expecting ',' or '}' */
    JSON_PARSE_OBJECT_COMMA,	/* saw object ',', expecting next label */
    JSON_PARSE_END				/* saw the end of a document, expect nothing */
} JsonParseContext;

typedef enum					/* type categories for datum_to_json */
{
    JSONTYPE_NULL,				/* null, so we didn't bother to identify */
    JSONTYPE_BOOL,				/* boolean (built-in types only) */
    JSONTYPE_NUMERIC,			/* numeric (ditto) */
    JSONTYPE_DATE,				/* we use special formatting for datetimes */
    JSONTYPE_TIMESTAMP,
    JSONTYPE_TIMESTAMPTZ,
    JSONTYPE_JSON,				/* JSON itself (and JSONB) */
    JSONTYPE_ARRAY,				/* array */
    JSONTYPE_COMPOSITE,			/* composite */
    JSONTYPE_CAST,				/* something with an explicit cast to JSON */
    JSONTYPE_OTHER				/* all else */
} JsonTypeCategory;

static inline JsonReturnType json_lex(JsonLexContext *lex);
static inline JsonReturnType json_lex_string(JsonLexContext *lex);
static inline JsonReturnType json_lex_number(JsonLexContext *lex, char *s,
        bool *num_err, int *total_len);
static inline JsonReturnType parse_scalar(JsonLexContext *lex, JsonSemAction *sem);
static JsonReturnType parse_object_field(JsonLexContext *lex, JsonSemAction *sem);
static JsonReturnType parse_object(JsonLexContext *lex, JsonSemAction *sem);
static JsonReturnType parse_array_element(JsonLexContext *lex, JsonSemAction *sem);
static JsonReturnType parse_array(JsonLexContext *lex, JsonSemAction *sem);
static JsonReturnType report_parse_error(JsonParseContext ctx, JsonLexContext *lex);


/* the null action object used for pure validation */
static JsonSemAction nullSemAction =
{
    NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL
};

/* Recursive Descent parser support routines */

/*
 * lex_peek
 *
 * what is the current look_ahead token?
*/
static inline JsonTokenType
lex_peek(JsonLexContext *lex)
{
    return lex->token_type;
}

/*
 * lex_accept
 *
 * accept the look_ahead token and move the lexer to the next token if the
 * look_ahead token matches the token parameter. In that case, and if required,
 * also hand back the de-escaped lexeme.
 *
 * returns true if the token matched, false otherwise.
 */
static inline JsonReturnType
lex_accept(JsonLexContext *lex, JsonTokenType token, char **lexeme, bool &ismatched)
{
    JsonReturnType ret = JSON_OK;
    ismatched = false;

    if (lex->token_type == token)
    {
        if (lexeme != NULL)
        {
            if (lex->token_type == JSON_TOKEN_STRING)
            {
                if (lex->strval != NULL)
                    *lexeme = strdup(lex->strval->data);
            }
            else
            {
                int			len = (lex->token_terminator - lex->token_start);
                char	   *tokstr = (char *)malloc(len + 1);

                memcpy(tokstr, lex->token_start, len);
                tokstr[len] = '\0';
                *lexeme = tokstr;
            }
        }
        ret = json_lex(lex);
        if (ret == JSON_OK)
            ismatched = true;
    }
    return ret;
}

/*
 * lex_accept
 *
 * move the lexer to the next token if the current look_ahead token matches
 * the parameter token. Otherwise, report an error.
 */
static inline JsonReturnType
lex_expect(JsonParseContext ctx, JsonLexContext *lex, JsonTokenType token)
{
    bool ismatched = false;
    JsonReturnType ret = lex_accept(lex, token, NULL, ismatched);
    if (ret != JSON_OK)
        return ret;
    else if (!ismatched)
        return JSON_INVALID_TOKEN;
    else
        return JSON_OK;
}


/* chars to consider as part of an alphanumeric token */
#define JSON_ALPHANUMERIC_CHAR(c)  \
	(((c) >= 'a' && (c) <= 'z') || \
	 ((c) >= 'A' && (c) <= 'Z') || \
	 ((c) >= '0' && (c) <= '9') || \
	 (c) == '_' || \
	 IS_HIGHBIT_SET(c))

/*
 * Utility function to check if a string is a valid JSON number.
 *
 * str is of length len, and need not be null-terminated.
 */
bool
IsValidJsonNumber(const char *str, int len)
{
    bool		numeric_error;
    int			total_len;
    JsonLexContext dummy_lex;
    int ret = 0;

    if (len <= 0)
        return false;

    /*
     * json_lex_number expects a leading  '-' to have been eaten already.
     *
     * having to cast away the constness of str is ugly, but there's not much
     * easy alternative.
     */
    if (*str == '-')
    {
        dummy_lex.input = (char *) str + 1;
        dummy_lex.input_length = len - 1;
    }
    else
    {
        dummy_lex.input = (char *) str;
        dummy_lex.input_length = len;
    }

    ret = json_lex_number(&dummy_lex, dummy_lex.input, &numeric_error, &total_len);
    if (ret != JSON_OK)
        return false;

    return (!numeric_error) && (total_len == dummy_lex.input_length);
}

/*
 * makeJsonLexContext
 *
 * lex constructor, with or without StringInfo object
 * for de-escaped lexemes.
 *
 * Without is better as it makes the processing faster, so only make one
 * if really required.
 *
 * If you already have the json as a text* value, use the first of these
 * functions, otherwise use  makeJsonLexContextCstringLen().
 */
JsonLexContext *
makeJsonLexContext(char *json, bool need_escapes)
{
    return makeJsonLexContextCstringLen(json, str_len(json), need_escapes);
}

JsonLexContext *
makeJsonLexContextCstringLen(char *json, int len, bool need_escapes)
{
    JsonLexContext *lex = (JsonLexContext *)malloc(sizeof(JsonLexContext));
    memset(lex, 0, sizeof(JsonLexContext));
    lex->input = lex->token_terminator = lex->line_start = json;
    lex->line_number = 1;
    lex->input_length = len;

    if (need_escapes)
        lex->strval = makeStringInfo();
    return lex;
}

/*
 * pg_parse_json
 *
 * Publicly visible entry point for the JSON parser.
 *
 * lex is a lexing context, set up for the json to be processed by calling
 * makeJsonLexContext(). sem is a strucure of function pointers to semantic
 * action routines to be called at appropriate spots during parsing, and a
 * pointer to a state object to be passed to those routines.
 */
JsonReturnType
pg_parse_json(JsonLexContext *lex, JsonSemAction *sem)
{
    JsonTokenType tok;
    JsonReturnType ret = JSON_OK;

    /* get the initial token */
    json_lex(lex);

    tok = lex_peek(lex);

    /* parse by recursive descent */
    switch (tok)
    {
    case JSON_TOKEN_OBJECT_START:
        ret = parse_object(lex, sem);
        break;
    case JSON_TOKEN_ARRAY_START:
        ret = parse_array(lex, sem);
        break;
    default:
        ret = parse_scalar(lex, sem);		/* json can be a bare scalar */
    }
    if (ret != JSON_OK)
        return ret;
    return lex_expect(JSON_PARSE_END, lex, JSON_TOKEN_END);

}

/*
 * json_count_array_elements
 *
 * Returns number of array elements in lex context at start of array token
 * until end of array token at same nesting level.
 *
 * Designed to be called from array_start routines.
 */
JsonReturnType
json_count_array_elements(JsonLexContext *lex, int &count)
{
    JsonLexContext copylex;
    JsonReturnType ret;
    /*
     * It's safe to do this with a shallow copy because the lexical routines
     * don't scribble on the input. They do scribble on the other pointers
     * etc, so doing this with a copy makes that safe.
     */
    memcpy(&copylex, lex, sizeof(JsonLexContext));
    copylex.strval = NULL;		/* not interested in values here */
    copylex.lex_level++;

    count = 0;
    ret = lex_expect(JSON_PARSE_ARRAY_START, &copylex, JSON_TOKEN_ARRAY_START);
    if (ret != 0 )
        return ret;
    if (lex_peek(&copylex) != JSON_TOKEN_ARRAY_END)
    {
        bool ismatched;
        do
        {
            count++;
            ret = parse_array_element(&copylex, &nullSemAction);
            if (ret != JSON_OK )
                return ret;
            ret = lex_accept(&copylex, JSON_TOKEN_COMMA, NULL, ismatched);
        }
        while (ret == JSON_OK && ismatched);
        if (ret != JSON_OK)
            return ret;
    }
    ret = lex_expect(JSON_PARSE_ARRAY_NEXT, &copylex, JSON_TOKEN_ARRAY_END);
    if (ret != JSON_OK)
        return ret;
    return JSON_OK;
}

/*
 *	Recursive Descent parse routines. There is one for each structural
 *	element in a json document:
 *	  - scalar (string, number, true, false, null)
 *	  - array  ( [ ] )
 *	  - array element
 *	  - object ( { } )
 *	  - object field
 */
static inline JsonReturnType
parse_scalar(JsonLexContext *lex, JsonSemAction *sem)
{
    char	   *val = NULL;
    json_scalar_action sfunc = sem->scalar;
    char	  **valaddr;
    JsonReturnType ret = JSON_OK;
    bool ismatched;
    JsonTokenType tok = lex_peek(lex);

    valaddr = sfunc == NULL ? NULL : &val;

    /* a scalar must be a string, a number, true, false, or null */
    switch (tok)
    {
    case JSON_TOKEN_TRUE:
        ret = lex_accept(lex, JSON_TOKEN_TRUE, valaddr, ismatched);
        break;
    case JSON_TOKEN_FALSE:
        ret = lex_accept(lex, JSON_TOKEN_FALSE, valaddr, ismatched);
        break;
    case JSON_TOKEN_NULL:
        ret = lex_accept(lex, JSON_TOKEN_NULL, valaddr, ismatched);
        break;
    case JSON_TOKEN_NUMBER:
        ret = lex_accept(lex, JSON_TOKEN_NUMBER, valaddr, ismatched);
        break;
    case JSON_TOKEN_STRING:
        ret = lex_accept(lex, JSON_TOKEN_STRING, valaddr, ismatched);
        break;
    default:
        return report_parse_error(JSON_PARSE_VALUE, lex);
    }
    if (ret != JSON_OK)
    {
        if( *valaddr != NULL)
        {
            free(*valaddr);
            *valaddr = NULL;
        }
        return ret;
    }

    if (sfunc != NULL)
        ret = (*sfunc) (sem->semstate, val, tok);
    if( *valaddr != NULL)
    {
        free(*valaddr);
        *valaddr = NULL;
    }
    return ret;
}

static JsonReturnType
parse_object_field(JsonLexContext *lex, JsonSemAction *sem)
{
    JsonReturnType ret = JSON_OK;
    /*
     * An object field is "fieldname" : value where value can be a scalar,
     * object or array.  Note: in user-facing docs and error messages, we
     * generally call a field name a "key".
     */

    char	   *fname = NULL;	/* keep compiler quiet */
    json_ofield_action ostart = sem->object_field_start;
    json_ofield_action oend = sem->object_field_end;
    bool		isnull;
    bool ismatched;
    char	  **fnameaddr = NULL;
    JsonTokenType tok;

    if (ostart != NULL || oend != NULL)
        fnameaddr = &fname;

    ret = lex_accept(lex, JSON_TOKEN_STRING, fnameaddr, ismatched);
    if (ret != JSON_OK || !ismatched)
        ret = JSON_INVALID_TOKEN;
    if (ret != JSON_OK)
    {
        if( *fnameaddr != NULL)
            free(*fnameaddr);
        return ret;
    }

    ret = lex_expect(JSON_PARSE_OBJECT_LABEL, lex, JSON_TOKEN_COLON);
    if (ret != JSON_OK )
    {
        if( *fnameaddr != NULL)
            free(*fnameaddr);
        return ret;
    }
    tok = lex_peek(lex);
    isnull = tok == JSON_TOKEN_NULL;

    if (ostart != NULL)
    {
        ret = (*ostart) (sem->semstate, fname, isnull);
        if (ret != JSON_OK)
        {
            if( *fnameaddr != NULL)
                free(*fnameaddr);
            return ret;
        }
    }

    switch (tok)
    {
    case JSON_TOKEN_OBJECT_START:
        ret = parse_object(lex, sem);
        break;
    case JSON_TOKEN_ARRAY_START:
        ret = parse_array(lex, sem);
        break;
    default:
        ret = parse_scalar(lex, sem);
    }
    if (ret != JSON_OK)
    {
        if( *fnameaddr != NULL)
            free(*fnameaddr);
        return ret;
    }

    if (oend != NULL)
        ret = (*oend) (sem->semstate, fname, isnull);

    if( *fnameaddr != NULL)
        free(*fnameaddr);
    return ret;
}

static JsonReturnType
parse_object(JsonLexContext *lex, JsonSemAction *sem)
{
    JsonReturnType ret = JSON_OK;
    bool ismatched;
    /*
     * an object is a possibly empty sequence of object fields, separated by
     * commas and surrounded by curly braces.
     */
    json_struct_action ostart = sem->object_start;
    json_struct_action oend = sem->object_end;
    JsonTokenType tok;

    if (ostart != NULL)
    {
        ret = (*ostart) (sem->semstate);
        if (ret != JSON_OK)
            return ret;
    }

    /*
     * Data inside an object is at a higher nesting level than the object
     * itself. Note that we increment this after we call the semantic routine
     * for the object start and restore it before we call the routine for the
     * object end.
     */
    lex->lex_level++;

    /* we know this will succeeed, just clearing the token */
    ret = lex_expect(JSON_PARSE_OBJECT_START, lex, JSON_TOKEN_OBJECT_START);
    if (ret != JSON_OK )
        return ret;
    tok = lex_peek(lex);
    switch (tok)
    {
    case JSON_TOKEN_STRING:
        ret = parse_object_field(lex, sem);
        if (ret != JSON_OK)
            return ret;
        ret = lex_accept(lex, JSON_TOKEN_COMMA, NULL, ismatched);
        while (ret == JSON_OK && ismatched)
        {
            ret = parse_object_field(lex, sem);
            if (ret != JSON_OK)
                return ret;
            ret = lex_accept(lex, JSON_TOKEN_COMMA, NULL, ismatched);
        }

        if (ret != JSON_OK)
            return ret;
        break;
    case JSON_TOKEN_OBJECT_END:
        break;
    default:
        /* case of an invalid initial token inside the object */
        return report_parse_error(JSON_PARSE_OBJECT_START, lex);;
    }

    ret = lex_expect(JSON_PARSE_OBJECT_NEXT, lex, JSON_TOKEN_OBJECT_END);
    if (ret != JSON_OK )
        return ret;
    lex->lex_level--;

    if (oend != NULL)
        ret = (*oend) (sem->semstate);
    return ret;
}

static JsonReturnType
parse_array_element(JsonLexContext *lex, JsonSemAction *sem)
{
    json_aelem_action astart = sem->array_element_start;
    json_aelem_action aend = sem->array_element_end;
    JsonTokenType tok = lex_peek(lex);
    JsonReturnType ret = JSON_OK;
    bool		isnull;

    isnull = tok == JSON_TOKEN_NULL;

    if (astart != NULL)
    {
        ret = (*astart) (sem->semstate, isnull);
        if (ret != JSON_OK)
            return ret;
    }

    /* an array element is any object, array or scalar */
    switch (tok)
    {
    case JSON_TOKEN_OBJECT_START:
        ret = parse_object(lex, sem);
        break;
    case JSON_TOKEN_ARRAY_START:
        ret = parse_array(lex, sem);
        break;
    default:
        ret = parse_scalar(lex, sem);
    }
    if (ret != JSON_OK)
        return ret;

    if (aend != NULL)
        ret = (*aend) (sem->semstate, isnull);

    return ret;
}

static JsonReturnType
parse_array(JsonLexContext *lex, JsonSemAction *sem)
{
    JsonReturnType ret = JSON_OK;
    bool ismatched;
    /*
     * an array is a possibly empty sequence of array elements, separated by
     * commas and surrounded by square brackets.
     */
    json_struct_action astart = sem->array_start;
    json_struct_action aend = sem->array_end;

    if (astart != NULL)
    {
        ret = (*astart) (sem->semstate);
        if (ret != JSON_OK)
            return ret;
    }

    /*
     * Data inside an array is at a higher nesting level than the array
     * itself. Note that we increment this after we call the semantic routine
     * for the array start and restore it before we call the routine for the
     * array end.
     */
    lex->lex_level++;

    ret = lex_expect(JSON_PARSE_ARRAY_START, lex, JSON_TOKEN_ARRAY_START);
    if (ret != JSON_OK)
        return ret;
    if (lex_peek(lex) != JSON_TOKEN_ARRAY_END)
    {
        ret = parse_array_element(lex, sem);
        if (ret != JSON_OK)
            return ret;
        ret = lex_accept(lex, JSON_TOKEN_COMMA, NULL, ismatched);
        while (ret == JSON_OK && ismatched)
        {
            parse_array_element(lex, sem);
            if (ret != JSON_OK)
                return ret;
            ret = lex_accept(lex, JSON_TOKEN_COMMA, NULL, ismatched);
        }
    }
    if (ret != JSON_OK)
        return ret;

    ret = lex_expect(JSON_PARSE_ARRAY_NEXT, lex, JSON_TOKEN_ARRAY_END);
    if (ret != JSON_OK)
        return ret;
    lex->lex_level--;

    if (aend != NULL)
    {
        ret = (*aend) (sem->semstate);
        if (ret != JSON_OK)
            return ret;
    }
    return JSON_OK;
}

/*
 * Lex one token from the input stream.
 */
static inline JsonReturnType
json_lex(JsonLexContext *lex)
{
    char	   *s;
    int			len;
    JsonReturnType ret = JSON_OK;

    /* Skip leading whitespace. */
    s = lex->token_terminator;
    len = s - lex->input;
    while (len < lex->input_length &&
            (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r'))
    {
        if (*s == '\n')
            ++lex->line_number;
        ++s;
        ++len;
    }
    lex->token_start = s;

    /* Determine token type. */
    if (len >= lex->input_length)
    {
        lex->token_start = NULL;
        lex->prev_token_terminator = lex->token_terminator;
        lex->token_terminator = s;
        lex->token_type = JSON_TOKEN_END;
    }
    else
        switch (*s)
        {
        /* Single-character token, some kind of punctuation mark. */
        case '{':
            lex->prev_token_terminator = lex->token_terminator;
            lex->token_terminator = s + 1;
            lex->token_type = JSON_TOKEN_OBJECT_START;
            break;
        case '}':
            lex->prev_token_terminator = lex->token_terminator;
            lex->token_terminator = s + 1;
            lex->token_type = JSON_TOKEN_OBJECT_END;
            break;
        case '[':
            lex->prev_token_terminator = lex->token_terminator;
            lex->token_terminator = s + 1;
            lex->token_type = JSON_TOKEN_ARRAY_START;
            break;
        case ']':
            lex->prev_token_terminator = lex->token_terminator;
            lex->token_terminator = s + 1;
            lex->token_type = JSON_TOKEN_ARRAY_END;
            break;
        case ',':
            lex->prev_token_terminator = lex->token_terminator;
            lex->token_terminator = s + 1;
            lex->token_type = JSON_TOKEN_COMMA;
            break;
        case ':':
            lex->prev_token_terminator = lex->token_terminator;
            lex->token_terminator = s + 1;
            lex->token_type = JSON_TOKEN_COLON;
            break;
        case '"':
            /* string */
            ret = json_lex_string(lex);
            lex->token_type = JSON_TOKEN_STRING;
            break;
        case '-':
            /* Negative number. */
            ret = json_lex_number(lex, s + 1, NULL, NULL);
            lex->token_type = JSON_TOKEN_NUMBER;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            /* Positive number. */
            ret = json_lex_number(lex, s, NULL, NULL);
            lex->token_type = JSON_TOKEN_NUMBER;
            break;
        default:
        {
            char	   *p;

            /*
             * We're not dealing with a string, number, legal
             * punctuation mark, or end of string.  The only legal
             * tokens we might find here are true, false, and null,
             * but for error reporting purposes we scan until we see a
             * non-alphanumeric character.  That way, we can report
             * the whole word as an unexpected token, rather than just
             * some unintuitive prefix thereof.
             */
            for (p = s; p - s < lex->input_length - len && JSON_ALPHANUMERIC_CHAR(*p); p++)
                /* skip */ ;

            /*
             * We got some sort of unexpected punctuation or an
             * otherwise unexpected character, so just complain about
             * that one character.
             */
            if (p == s)
            {
                lex->prev_token_terminator = lex->token_terminator;
                lex->token_terminator = s + 1;
                return JSON_INVALID_TOKEN;
            }

            /*
             * We've got a real alphanumeric token here.  If it
             * happens to be true, false, or null, all is well.  If
             * not, error out.
             */
            lex->prev_token_terminator = lex->token_terminator;
            lex->token_terminator = p;
            if (p - s == 4)
            {
                if (memcmp(s, "true", 4) == 0)
                    lex->token_type = JSON_TOKEN_TRUE;
                else if (memcmp(s, "null", 4) == 0)
                    lex->token_type = JSON_TOKEN_NULL;
                else
                    return JSON_INVALID_TOKEN;
            }
            else if (p - s == 5 && memcmp(s, "false", 5) == 0)
                lex->token_type = JSON_TOKEN_FALSE;
            else
                return JSON_INVALID_TOKEN;

        }
        }						/* end of switch */
    return ret;
}

/*
 * The next token in the input stream is known to be a string; lex it.
 */
static inline JsonReturnType
json_lex_string(JsonLexContext *lex)
{
    char	   *s;
    int			len;
    int			hi_surrogate = -1;

    if (lex->strval != NULL)
        resetStringInfo(lex->strval);

    //ASSERT(lex->input_length > 0);
    s = lex->token_start;
    len = lex->token_start - lex->input;
    for (;;)
    {
        s++;
        len++;
        /* Premature end of the string. */
        if (len >= lex->input_length)
        {
            lex->token_terminator = s;
            return JSON_INVALID_TOKEN;
        }
        else if (*s == '"')
            break;
        else if ((unsigned char) *s < 32)
        {
            /* Per RFC4627, these characters MUST be escaped. */
            /* Since *s isn't printable, exclude it from the context string */
            return JSON_INVALID_STRING;
        }
        else if (*s == '\\')
        {
            /* OK, we have an escape character. */
            s++;
            len++;
            if (len >= lex->input_length)
            {
                lex->token_terminator = s;
                return JSON_INVALID_TOKEN;
            }
            else if (*s == 'u')
            {
                int			i;
                int			ch = 0;

                for (i = 1; i <= 4; i++)
                {
                    s++;
                    len++;
                    if (len >= lex->input_length)
                    {
                        lex->token_terminator = s;
                        return JSON_INVALID_TOKEN;
                    }
                    else if (*s >= '0' && *s <= '9')
                        ch = (ch * 16) + (*s - '0');
                    else if (*s >= 'a' && *s <= 'f')
                        ch = (ch * 16) + (*s - 'a') + 10;
                    else if (*s >= 'A' && *s <= 'F')
                        ch = (ch * 16) + (*s - 'A') + 10;
                    else
                        return JSON_INVALID_STRING;
                }
                if (lex->strval != NULL)
                {
                    char		utf8str[5];
                    int			utf8len;

                    if (ch >= 0xd800 && ch <= 0xdbff)
                    {
                        if (hi_surrogate != -1)
                            return JSON_INVALID_STRING;
                        hi_surrogate = (ch & 0x3ff) << 10;
                        continue;
                    }
                    else if (ch >= 0xdc00 && ch <= 0xdfff)
                    {
                        if (hi_surrogate == -1)
                            return JSON_INVALID_STRING;
                        ch = 0x10000 + hi_surrogate + (ch & 0x3ff);
                        hi_surrogate = -1;
                    }

                    if (hi_surrogate != -1)
                        return JSON_INVALID_STRING;

                    /*
                     * For UTF8, replace the escape sequence by the actual
                     * utf8 character in lex->strval. Do this also for other
                     * encodings if the escape designates an ASCII character,
                     * otherwise raise an error.
                     */

                    if (ch == 0)
                    {
                        /* We can't allow this, since our TEXT type doesn't */
                        return JSON_INVALID_STRING;
                    }
                    else if (ch <= 0x007f)
                    {
                        /*
                         * This is the only way to designate things like a
                         * form feed character in JSON, so it's useful in all
                         * encodings.
                         */
                        appendStringInfoChar(lex->strval, (char) ch);
                    }
                    else
                        return JSON_INVALID_STRING;

                }
            }
            else if (lex->strval != NULL)
            {
                if (hi_surrogate != -1)
                    return JSON_INVALID_STRING;

                switch (*s)
                {
                case '"':
                case '\\':
                case '/':
                    appendStringInfoChar(lex->strval, *s);
                    break;
                case 'b':
                    appendStringInfoChar(lex->strval, '\b');
                    break;
                case 'f':
                    appendStringInfoChar(lex->strval, '\f');
                    break;
                case 'n':
                    appendStringInfoChar(lex->strval, '\n');
                    break;
                case 'r':
                    appendStringInfoChar(lex->strval, '\r');
                    break;
                case 't':
                    appendStringInfoChar(lex->strval, '\t');
                    break;
                default:
                    /* Not a valid string escape, so error out. */
                    return JSON_INVALID_STRING;
                }
            }
            else if (strchr("\"\\/bfnrt", *s) == NULL)
            {
                return JSON_INVALID_STRING;
            }

        }
        else if (lex->strval != NULL)
        {
            if (hi_surrogate != -1)
                return JSON_INVALID_STRING;

            appendStringInfoChar(lex->strval, *s);
        }

    }

    if (hi_surrogate != -1)
        return JSON_INVALID_STRING;

    /* Hooray, we found the end of the string! */
    lex->prev_token_terminator = lex->token_terminator;
    lex->token_terminator = s + 1;
    return JSON_OK;
}

/*
 * The next token in the input stream is known to be a number; lex it.
 *
 * In JSON, a number consists of four parts:
 *
 * (1) An optional minus sign ('-').
 *
 * (2) Either a single '0', or a string of one or more digits that does not
 *	   begin with a '0'.
 *
 * (3) An optional decimal part, consisting of a period ('.') followed by
 *	   one or more digits.  (Note: While this part can be omitted
 *	   completely, it's not OK to have only the decimal point without
 *	   any digits afterwards.)
 *
 * (4) An optional exponent part, consisting of 'e' or 'E', optionally
 *	   followed by '+' or '-', followed by one or more digits.  (Note:
 *	   As with the decimal part, if 'e' or 'E' is present, it must be
 *	   followed by at least one digit.)
 *
 * The 's' argument to this function points to the ostensible beginning
 * of part 2 - i.e. the character after any optional minus sign, or the
 * first character of the string if there is none.
 *
 * If num_err is not NULL, we return an error flag to *num_err rather than
 * raising an error for a badly-formed number.  Also, if total_len is not NULL
 * the distance from lex->input to the token end+1 is returned to *total_len.
 */
static inline JsonReturnType
json_lex_number(JsonLexContext *lex, char *s,
                bool *num_err, int *total_len)
{
    bool		error = false;
    int			len = s - lex->input;

    /* Part (1): leading sign indicator. */
    /* Caller already did this for us; so do nothing. */

    /* Part (2): parse main digit string. */
    if (len < lex->input_length && *s == '0')
    {
        s++;
        len++;
    }
    else if (len < lex->input_length && *s >= '1' && *s <= '9')
    {
        do
        {
            s++;
            len++;
        }
        while (len < lex->input_length && *s >= '0' && *s <= '9');
    }
    else
        error = true;

    /* Part (3): parse optional decimal portion. */
    if (len < lex->input_length && *s == '.')
    {
        s++;
        len++;
        if (len == lex->input_length || *s < '0' || *s > '9')
            error = true;
        else
        {
            do
            {
                s++;
                len++;
            }
            while (len < lex->input_length && *s >= '0' && *s <= '9');
        }
    }

    /* Part (4): parse optional exponent. */
    if (len < lex->input_length && (*s == 'e' || *s == 'E'))
    {
        s++;
        len++;
        if (len < lex->input_length && (*s == '+' || *s == '-'))
        {
            s++;
            len++;
        }
        if (len == lex->input_length || *s < '0' || *s > '9')
            error = true;
        else
        {
            do
            {
                s++;
                len++;
            }
            while (len < lex->input_length && *s >= '0' && *s <= '9');
        }
    }

    /*
     * Check for trailing garbage.  As in json_lex(), any alphanumeric stuff
     * here should be considered part of the token for error-reporting
     * purposes.
     */
    for (; len < lex->input_length && JSON_ALPHANUMERIC_CHAR(*s); s++, len++)
        error = true;

    if (total_len != NULL)
        *total_len = len;

    if (num_err != NULL)
    {
        /* let the caller handle any error */
        *num_err = error;
    }
    else
    {
        /* return token endpoint */
        lex->prev_token_terminator = lex->token_terminator;
        lex->token_terminator = s;
        /* handle error if any */
        if (error)
            return JSON_INVALID_TOKEN;
    }
    return JSON_OK;
}

/*
 * Produce a JSON string literal, properly escaping characters in the text.
 */
void
escape_json(StringInfo buf, const char *str)
{
    const char *p;

    appendStringInfoCharMacro(buf, '\"');
    for (p = str; *p; p++)
    {
        switch (*p)
        {
        case '\b':
            appendStringInfoString(buf, "\\b");
            break;
        case '\f':
            appendStringInfoString(buf, "\\f");
            break;
        case '\n':
            appendStringInfoString(buf, "\\n");
            break;
        case '\r':
            appendStringInfoString(buf, "\\r");
            break;
        case '\t':
            appendStringInfoString(buf, "\\t");
            break;
        case '"':
            appendStringInfoString(buf, "\\\"");
            break;
        case '\\':
            appendStringInfoString(buf, "\\\\");
            break;
        default:
            if ((unsigned char) *p < ' ')
            	appendStringInfo(buf, "\\u%04x", (int) *p);
            else
            	appendStringInfoCharMacro(buf, *p);
            break;
        }
    }
    appendStringInfoCharMacro(buf, '\"');
}


/*
 * Report a parse error.
 *
 * lex->token_start and lex->token_terminator must identify the current token.
 */
static JsonReturnType
report_parse_error(JsonParseContext ctx, JsonLexContext *lex)
{
    /* Handle case where the input ended prematurely. */
    if (lex->token_start == NULL || lex->token_type == JSON_TOKEN_END)
        return JSON_END_PREMATURELY;

    switch (ctx)
    {
    case JSON_PARSE_VALUE:
        return JSON_INVALID_VALUE;
        break;
    case JSON_PARSE_STRING:
        return JSON_INVALID_STRING;
        break;
    case JSON_PARSE_ARRAY_START:
        return JSON_INVALID_ARRAY_START;
        break;
    case JSON_PARSE_ARRAY_NEXT:
        return JSON_INVALID_ARRAY_NEXT;
        break;
    case JSON_PARSE_OBJECT_START:
        return JSON_INVALID_OBJECT_START;
        break;
    case JSON_PARSE_OBJECT_LABEL:
        return JSON_INVALID_OBJECT_LABEL;
        break;
    case JSON_PARSE_OBJECT_NEXT:
        return JSON_INVALID_OBJECT_NEXT;
        break;
    case JSON_PARSE_OBJECT_COMMA:
        return JSON_INVALID_OBJECT_COMMA;
        break;
    case JSON_PARSE_END:
        return JSON_INVALID_END;
        break;
    default:
        return JSON_UNEXPECTED_ERROR;
    }
}

