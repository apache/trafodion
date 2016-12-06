/*-------------------------------------------------------------------------
 *
 * jsonfuncs.c
 *		Functions to process JSON data types.
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/jsonfuncs.c
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
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "str.h"

/* semantic action functions for json_get* functions */
static JsonReturnType get_object_start(void *state);
static JsonReturnType get_object_end(void *state);
static JsonReturnType get_object_field_start(void *state, char *fname, bool isnull);
static JsonReturnType get_object_field_end(void *state, char *fname, bool isnull);
static JsonReturnType get_array_start(void *state);
static JsonReturnType get_array_end(void *state);
static JsonReturnType get_array_element_start(void *state, bool isnull);
static JsonReturnType get_array_element_end(void *state, bool isnull);
static JsonReturnType get_scalar(void *state, char *token, JsonTokenType tokentype);

/* common worker function for json getter functions */
static JsonReturnType get_path_all(bool as_text, char *json, short nargs, va_list args, char **result);
static JsonReturnType get_worker(char *json, char **tpath, int *ipath, int npath,
                                 bool normalize_results, char **result);


/* semantic action functions for json_array_length */
static void alen_object_start(void *state);
static void alen_scalar(void *state, char *token, JsonTokenType tokentype);
static void alen_array_element_start(void *state, bool isnull);

/* common workers for json{b}_each* functions */

/* semantic action functions for json_each */
static void each_object_field_start(void *state, char *fname, bool isnull);
static void each_object_field_end(void *state, char *fname, bool isnull);
static void each_array_start(void *state);
static void each_scalar(void *state, char *token, JsonTokenType tokentype);

/* semantic action functions for json_array_elements */
static void elements_object_start(void *state);
static void elements_array_element_start(void *state, bool isnull);
static void elements_array_element_end(void *state, bool isnull);
static void elements_scalar(void *state, char *token, JsonTokenType tokentype);

/* semantic action functions for json_strip_nulls */
static void sn_object_start(void *state);
static void sn_object_end(void *state);
static void sn_array_start(void *state);
static void sn_array_end(void *state);
static void sn_object_field_start(void *state, char *fname, bool isnull);
static void sn_array_element_start(void *state, bool isnull);
static void sn_scalar(void *state, char *token, JsonTokenType tokentype);

/* state for json_object_keys */
typedef struct OkeysState
{
    JsonLexContext *lex;
    char	  **result;
    int			result_size;
    int			result_count;
    int			sent_count;
} OkeysState;

/* state for json_get* functions */
typedef struct GetState
{
    JsonLexContext *lex;
    char	   *tresult;
    char	   *result_start;
    bool		normalize_results;
    bool		next_scalar;
    int			npath;			/* length of each path-related array */
    char	  **path_names;		/* field name(s) being sought */
    int		   *path_indexes;	/* array index(es) being sought */
    bool	   *pathok;			/* is path matched to current depth? */
    int		   *array_cur_index;	/* current element index at each path level */
} GetState;

/* state for json_array_length */
typedef struct AlenState
{
    JsonLexContext *lex;
    int			count;
} AlenState;


/* state for json_array_elements */
typedef struct ElementsState
{
    JsonLexContext *lex;
    const char *function_name;
    char	   *result_start;
    bool		normalize_results;
    bool		next_scalar;
    char	   *normalized_scalar;
} ElementsState;

/* state for json_strip_nulls */
typedef struct StripnullState
{
    JsonLexContext *lex;
    StringInfo	strval;
    bool		skip_next_null;
} StripnullState;

JsonReturnType
json_extract_path(char **result, char *json, short nargs, ...)
{
    JsonReturnType ret = JSON_OK;
    va_list args;
    va_start(args, nargs);
    ret = get_path_all(true, json, nargs, args, result);
    va_end(args);
    return ret;
}

JsonReturnType
json_extract_path_text(char **result, char *json, short nargs, ...)
{
    JsonReturnType ret = JSON_OK;
    va_list args;
    va_start(args, nargs);
    ret = get_path_all(true, json, nargs, args, result);
    va_end(args);
    return ret;
}

/*
 * common routine for extract_path functions
 */
static JsonReturnType
get_path_all(bool as_text, char *json, short nargs, va_list args, char **result)
{
    bool	   *pathnulls;
    char	  **tpath;
    int		   *ipath;
    int			i;
    JsonReturnType ret = JSON_OK;

    tpath = (char **)malloc(nargs * sizeof(char *));
    ipath = (int *)malloc(nargs * sizeof(int));

    for (i = 0; i < nargs; i++)
    {
        tpath[i] = va_arg(args, char *);

        /*
         * we have no idea at this stage what structure the document is so
         * just convert anything in the path that we can to an integer and set
         * all the other integers to INT_MIN which will never match.
         */
        if (*tpath[i] != '\0')
        {
            long		ind;
            char	   *endptr;

            errno = 0;
            ind = strtol(tpath[i], &endptr, 10);
            if (*endptr == '\0' && errno == 0 && ind <= INT_MAX && ind >= INT_MIN)
                ipath[i] = (int) ind;
            else
                ipath[i] = INT_MIN;
        }
        else
            ipath[i] = INT_MIN;
    }

    ret = get_worker(json, tpath, ipath, nargs, as_text, result);

    if (tpath != NULL)
        free(tpath);
    if (ipath != NULL)
        free(ipath);
    return ret;
}

JsonReturnType json_object_field_text(char *json, char *fieldName, char **result)
{
    return get_worker(json, &fieldName, NULL, 1, true, result);
}

static JsonReturnType
get_worker(char *json,
           char **tpath,
           int *ipath,
           int npath,
           bool normalize_results,
           char **result)
{
    JsonLexContext *lex = makeJsonLexContext(json, true);
    JsonSemAction *sem = (JsonSemAction *)malloc(sizeof(JsonSemAction));
    GetState   *state = (GetState *)malloc(sizeof(GetState));
    JsonReturnType ret;

    memset(sem, 0, sizeof(JsonSemAction));
    memset(state, 0, sizeof(GetState));

    if(npath < 0)
        return JSON_UNEXPECTED_ERROR;

    state->lex = lex;
    /* is it "_as_text" variant? */
    state->normalize_results = normalize_results;
    state->npath = npath;
    state->path_names = tpath;
    state->path_indexes = ipath;
    state->pathok = (bool *)malloc(sizeof(bool) * npath);
    state->array_cur_index = (int *)malloc(sizeof(int) * npath);

    if (npath > 0)
        state->pathok[0] = true;

    sem->semstate = (void *) state;

    /*
     * Not all variants need all the semantic routines. Only set the ones that
     * are actually needed for maximum efficiency.
     */
    sem->scalar = get_scalar;
    if (npath == 0)
    {
        sem->object_start = get_object_start;
        sem->object_end = get_object_end;
        sem->array_start = get_array_start;
        sem->array_end = get_array_end;
    }
    if (tpath != NULL)
    {
        sem->object_field_start = get_object_field_start;
        sem->object_field_end = get_object_field_end;
    }
    if (ipath != NULL)
    {
        sem->array_start = get_array_start;
        sem->array_element_start = get_array_element_start;
        sem->array_element_end = get_array_element_end;
    }

    ret = pg_parse_json(lex, sem);
    if (ret == JSON_OK)
        *result = state->tresult;
    else
        *result = NULL;

    if (lex != NULL)
    {
        if (lex->strval != NULL)
        {
            if (lex->strval->data != NULL)
                free(lex->strval->data);
            free(lex->strval);
        }
        free(lex);
    }
    if (sem != NULL)
        free(sem);
    if (state != NULL)
        free(state);
    return ret;
}

static JsonReturnType
get_object_start(void *state)
{
    GetState   *_state = (GetState *) state;
    int			lex_level = _state->lex->lex_level;

    if (lex_level == 0 && _state->npath == 0)
    {
        /*
         * Special case: we should match the entire object.  We only need this
         * at outermost level because at nested levels the match will have
         * been started by the outer field or array element callback.
         */
        _state->result_start = _state->lex->token_start;
    }
    return JSON_OK;
}

static JsonReturnType
get_object_end(void *state)
{
    GetState   *_state = (GetState *) state;
    int			lex_level = _state->lex->lex_level;

    if (lex_level == 0 && _state->npath == 0)
    {
        /* Special case: return the entire object */
        char	   *start = _state->result_start;
        int			len = _state->lex->prev_token_terminator - start;

        //_state->tresult = cstring_to_text_with_len(start, len);
        _state->tresult = (char *)malloc(len + 1);

        memcpy(_state->tresult, start, len);
        _state->tresult[len] = '\0';
    }
    return JSON_OK;
}

static JsonReturnType
get_object_field_start(void *state, char *fname, bool isnull)
{
    GetState   *_state = (GetState *) state;
    bool		get_next = false;
    int			lex_level = _state->lex->lex_level;

    if (lex_level <= _state->npath &&
            _state->pathok[lex_level - 1] &&
            _state->path_names != NULL &&
            _state->path_names[lex_level - 1] != NULL &&
            strcmp(fname, _state->path_names[lex_level - 1]) == 0)
    {
        if (lex_level < _state->npath)
        {
            /* if not at end of path just mark path ok */
            _state->pathok[lex_level] = true;
        }
        else
        {
            /* end of path, so we want this value */
            get_next = true;
        }
    }

    if (get_next)
    {
        /* this object overrides any previous matching object */
        _state->tresult = NULL;
        _state->result_start = NULL;

        if (_state->normalize_results &&
                _state->lex->token_type == JSON_TOKEN_STRING)
        {
            /* for as_text variants, tell get_scalar to set it for us */
            _state->next_scalar = true;
        }
        else
        {
            /* for non-as_text variants, just note the json starting point */
            _state->result_start = _state->lex->token_start;
        }
    }
    return JSON_OK;
}

static JsonReturnType
get_object_field_end(void *state, char *fname, bool isnull)
{
    GetState   *_state = (GetState *) state;
    bool		get_last = false;
    int			lex_level = _state->lex->lex_level;

    /* same tests as in get_object_field_start */
    if (lex_level <= _state->npath &&
            _state->pathok[lex_level - 1] &&
            _state->path_names != NULL &&
            _state->path_names[lex_level - 1] != NULL &&
            strcmp(fname, _state->path_names[lex_level - 1]) == 0)
    {
        if (lex_level < _state->npath)
        {
            /* done with this field so reset pathok */
            _state->pathok[lex_level] = false;
        }
        else
        {
            /* end of path, so we want this value */
            get_last = true;
        }
    }

    /* for as_text scalar case, our work is already done */
    if (get_last && _state->result_start != NULL)
    {
        /*
         * make a text object from the string from the prevously noted json
         * start up to the end of the previous token (the lexer is by now
         * ahead of us on whatever came after what we're interested in).
         */
        if (isnull && _state->normalize_results)
            _state->tresult = (char *) NULL;
        else
        {
            char	   *start = _state->result_start;
            int			len = _state->lex->prev_token_terminator - start;

            //_state->tresult = cstring_to_text_with_len(start, len);

            _state->tresult = (char *)malloc(len + 1);

            memcpy(_state->tresult, start, len);
            _state->tresult[len] = '\0';
        }

        /* this should be unnecessary but let's do it for cleanliness: */
        _state->result_start = NULL;
    }
    return JSON_OK;
}

static JsonReturnType
get_array_start(void *state)
{
    GetState   *_state = (GetState *) state;
    int			lex_level = _state->lex->lex_level;

    if (lex_level < _state->npath)
    {
        /* Initialize counting of elements in this array */
        _state->array_cur_index[lex_level] = -1;

        /* INT_MIN value is reserved to represent invalid subscript */
        if (_state->path_indexes[lex_level] < 0 &&
                _state->path_indexes[lex_level] != INT_MIN)
        {
            /* Negative subscript -- convert to positive-wise subscript */
            int		nelements;
            JsonReturnType ret = json_count_array_elements(_state->lex, nelements);
            if (ret != JSON_OK)
                return ret;
            if (-_state->path_indexes[lex_level] <= nelements)
                _state->path_indexes[lex_level] += nelements;
        }
    }
    else if (lex_level == 0 && _state->npath == 0)
    {
        /*
         * Special case: we should match the entire array.  We only need this
         * at the outermost level because at nested levels the match will
         * have been started by the outer field or array element callback.
         */
        _state->result_start = _state->lex->token_start;
    }
    return JSON_OK;
}

static JsonReturnType
get_array_end(void *state)
{
    GetState   *_state = (GetState *) state;
    int			lex_level = _state->lex->lex_level;

    if (lex_level == 0 && _state->npath == 0)
    {
        /* Special case: return the entire array */
        char	   *start = _state->result_start;
        int			len = _state->lex->prev_token_terminator - start;

        //_state->tresult = cstring_to_text_with_len(start, len);

        _state->tresult = (char *)malloc(len + 1);

        memcpy(_state->tresult, start, len);
        _state->tresult[len] = '\0';
    }
    return JSON_OK;
}

static JsonReturnType
get_array_element_start(void *state, bool isnull)
{
    GetState   *_state = (GetState *) state;
    bool		get_next = false;
    int			lex_level = _state->lex->lex_level;

    /* Update array element counter */
    if (lex_level <= _state->npath)
        _state->array_cur_index[lex_level - 1]++;

    if (lex_level <= _state->npath &&
            _state->pathok[lex_level - 1] &&
            _state->path_indexes != NULL &&
            _state->array_cur_index[lex_level - 1] == _state->path_indexes[lex_level - 1])
    {
        if (lex_level < _state->npath)
        {
            /* if not at end of path just mark path ok */
            _state->pathok[lex_level] = true;
        }
        else
        {
            /* end of path, so we want this value */
            get_next = true;
        }
    }

    /* same logic as for objects */
    if (get_next)
    {
        _state->tresult = NULL;
        _state->result_start = NULL;

        if (_state->normalize_results &&
                _state->lex->token_type == JSON_TOKEN_STRING)
            _state->next_scalar = true;
        else
            _state->result_start = _state->lex->token_start;
    }
    return JSON_OK;
}

static JsonReturnType
get_array_element_end(void *state, bool isnull)
{
    GetState   *_state = (GetState *) state;
    bool		get_last = false;
    int			lex_level = _state->lex->lex_level;

    /* same tests as in get_array_element_start */
    if (lex_level <= _state->npath &&
            _state->pathok[lex_level - 1] &&
            _state->path_indexes != NULL &&
            _state->array_cur_index[lex_level - 1] == _state->path_indexes[lex_level - 1])
    {
        if (lex_level < _state->npath)
        {
            /* done with this element so reset pathok */
            _state->pathok[lex_level] = false;
        }
        else
        {
            /* end of path, so we want this value */
            get_last = true;
        }
    }

    /* same logic as for objects */
    if (get_last && _state->result_start != NULL)
    {
        if (isnull && _state->normalize_results)
            _state->tresult = (char *) NULL;
        else
        {
            char	   *start = _state->result_start;
            int			len = _state->lex->prev_token_terminator - start;

            //_state->tresult = cstring_to_text_with_len(start, len);

            _state->tresult = (char *)malloc(len + 1);

            memcpy(_state->tresult, start, len);
            _state->tresult[len] = '\0';
        }

        _state->result_start = NULL;
    }
    return JSON_OK;
}

static JsonReturnType
get_scalar(void *state, char *token, JsonTokenType tokentype)
{
    GetState   *_state = (GetState *) state;
    int			lex_level = _state->lex->lex_level;

    /* Check for whole-object match */
    if (lex_level == 0 && _state->npath == 0)
    {
        if (_state->normalize_results && tokentype == JSON_TOKEN_STRING)
        {
            /* we want the de-escaped string */
            _state->next_scalar = true;
        }
        else if (_state->normalize_results && tokentype == JSON_TOKEN_NULL)
            _state->tresult = (char *) NULL;
        else
        {
            /*
             * This is a bit hokey: we will suppress whitespace after the
             * scalar token, but not whitespace before it.  Probably not worth
             * doing our own space-skipping to avoid that.
             */
            char	   *start = _state->lex->input;
            int			len = _state->lex->prev_token_terminator - start;

            //_state->tresult = cstring_to_text_with_len(start, len);

            _state->tresult = (char *)malloc(len + 1);

            memcpy(_state->tresult, start, len);
            _state->tresult[len] = '\0';
        }
    }

    if (_state->next_scalar)
    {
        /* a de-escaped text value is wanted, so supply it */
        //_state->tresult = cstring_to_text(token);
        //_state->tresult = token;
        int len = str_len(token);
        _state->tresult = (char *)malloc(len + 1);

        memcpy(_state->tresult, token, len);
        _state->tresult[len] = '\0';
        /* make sure the next call to get_scalar doesn't overwrite it */
        _state->next_scalar = false;
    }
    return JSON_OK;
}

/*
 * These next two checks ensure that the json is an array (since it can't be
 * a scalar or an object).
 */

static void
alen_object_start(void *state)
{
    AlenState  *_state = (AlenState *) state;

    /* json structure check */
    if (_state->lex->lex_level == 0)
        return;
}

static void
alen_scalar(void *state, char *token, JsonTokenType tokentype)
{
    AlenState  *_state = (AlenState *) state;

    /* json structure check */
    if (_state->lex->lex_level == 0)
        return;
}

static void
alen_array_element_start(void *state, bool isnull)
{
    AlenState  *_state = (AlenState *) state;

    /* just count up all the level 1 elements */
    if (_state->lex->lex_level == 1)
        _state->count++;
}


static void
elements_object_start(void *state)
{
    ElementsState *_state = (ElementsState *) state;

    /* json structure check */
    if (_state->lex->lex_level == 0)
        return;
}

static void
elements_scalar(void *state, char *token, JsonTokenType tokentype)
{
    ElementsState *_state = (ElementsState *) state;

    /* json structure check */
    if (_state->lex->lex_level == 0)
        return;

    /* supply de-escaped value if required */
    if (_state->next_scalar)
        _state->normalized_scalar = token;
}


/*
 * Semantic actions for json_strip_nulls.
 *
 * Simply repeat the input on the output unless we encounter
 * a null object field. State for this is set when the field
 * is started and reset when the scalar action (which must be next)
 * is called.
 */

static void
sn_object_start(void *state)
{
    StripnullState *_state = (StripnullState *) state;

    appendStringInfoCharMacro(_state->strval, '{');
}

static void
sn_object_end(void *state)
{
    StripnullState *_state = (StripnullState *) state;

    appendStringInfoCharMacro(_state->strval, '}');
}

static void
sn_array_start(void *state)
{
    StripnullState *_state = (StripnullState *) state;

    appendStringInfoCharMacro(_state->strval, '[');
}

static void
sn_array_end(void *state)
{
    StripnullState *_state = (StripnullState *) state;

    appendStringInfoCharMacro(_state->strval, ']');
}

static void
sn_object_field_start(void *state, char *fname, bool isnull)
{
    StripnullState *_state = (StripnullState *) state;

    if (isnull)
    {
        /*
         * The next thing must be a scalar or isnull couldn't be true, so
         * there is no danger of this state being carried down into a nested
         * object or array. The flag will be reset in the scalar action.
         */
        _state->skip_next_null = true;
        return;
    }

    if (_state->strval->data[_state->strval->len - 1] != '{')
        appendStringInfoCharMacro(_state->strval, ',');

    /*
     * Unfortunately we don't have the quoted and escaped string any more, so
     * we have to re-escape it.
     */
    escape_json(_state->strval, fname);

    appendStringInfoCharMacro(_state->strval, ':');
}

static void
sn_array_element_start(void *state, bool isnull)
{
    StripnullState *_state = (StripnullState *) state;

    if (_state->strval->data[_state->strval->len - 1] != '[')
        appendStringInfoCharMacro(_state->strval, ',');
}

static void
sn_scalar(void *state, char *token, JsonTokenType tokentype)
{
    StripnullState *_state = (StripnullState *) state;

    if (_state->skip_next_null)
    {
        _state->skip_next_null = false;
        return;
    }

    if (tokentype == JSON_TOKEN_STRING)
        escape_json(_state->strval, token);
    else
        appendStringInfoString(_state->strval, token);
}
