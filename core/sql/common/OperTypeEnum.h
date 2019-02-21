/* -*-C++-*- */
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

#ifndef OPERTYPEENUM_H
#define OPERTYPEENUM_H
/* -*-C++-*-
******************************************************************************
*
* File:         OperTypeEnum.h
* Description:  Expression nodes (relational nodes and item expression nodes)
* Created:      4/28/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "BaseTypes.h"

// -----------------------------------------------------------------------
// An enumeration type for the different operators of the Noah dataflow
// model. This includes both relational and item expressions. Operators
// can also be wildcards (to be used only in the "pattern" expression
// of a rule). Wildcard operators have the string "ANY" or "FORCE" in them.
// The enum datatype is divided into different ranges for relational,
// item, statement, and DDL operators, allowing some operators to be
// changed or added without recompiling all components of SQL/ARK.
//
// NOTE on side effects of rounding mode on datetime arithmetic:
// Datetime arithmetic is supposed to ignore any rounding mode in effect.
// For now, this is accomplished by propagating OrigOpType_ from source node
// to any arithmetic operation introduced in the Generator and disabling
// rounding in Binder when OrigOpType_ is one of datetime functions.
// Currently it is known that the range of operators between
// ITM_DATE_TRUNC_YEAR and ITM_DATEDIFF_WEEK need this treatment and they
// are handled in BiAirth::bindNode(). If any new datetime operator is
// introduced that may be subject to scaling (generates any arithmetic
// nodes below it self) they need to be handled in BiArith::bindNode().
// -----------------------------------------------------------------------
enum OperatorTypeEnum {
  NO_OPERATOR_TYPE = FALSE,
  INVALID_OPERATOR_TYPE = FALSE,

                        DML_FIRST_DML_OP = 1000,
                        ANY_REL_OR_ITM_OP,
                        // relational operators
                        REL_FIRST_REL_OP,
                        // relational operators with arity 0 or 1
                        REL_ANY_GEN_UPDATE,
                        REL_ANY_UPDATE_DELETE,
                        REL_ANY_DELETE,
                        // leaf relational operators (arity 0)
                        REL_ANY_LEAF_OP,
                        REL_ANY_LEAF_GEN_UPDATE,
                        REL_LEAF_INSERT,
                        REL_LEAF_UPDATE,
                        REL_LEAF_DELETE,
                        REL_FORCE_ANY_SCAN,
                        REL_SCAN,
                        REL_STORED_PROC,
                        REL_INTERNALSP,
                        REL_UTIL_INTERNALSP,
                        REL_ROUTINE,
                        REL_ANY_ROUTINE,
                        REL_ANY_SCALAR_UDF_ROUTINE,
                        REL_TABLE_VALUED_FUNCTION,
                        REL_BUILTIN_TABLE_VALUED_FUNCTION,
                        REL_TABLE_VALUED_UDF,
                        REL_LEAF_TABLE_MAPPING_UDF,
                        REL_UNARY_TABLE_MAPPING_UDF,
                        REL_BINARY_TABLE_MAPPING_UDF,
                        REL_TABLE_MAPPING_BUILTIN_LOG_READER,
                        REL_TABLE_MAPPING_BUILTIN_TIMESERIES,
                        REL_TABLE_MAPPING_BUILTIN_SERIES, //series
                        REL_TABLE_MAPPING_BUILTIN_JDBC,
                        REL_ANY_TABLE_MAPPING_UDF,
                        REL_ANY_LEAF_TABLE_MAPPING_UDF,
                        REL_ANY_UNARY_TABLE_MAPPING_UDF,
                        REL_ANY_BINARY_TABLE_MAPPING_UDF,
                        REL_ISOLATED_NON_TABLE_UDR,
                        REL_ISOLATED_SCALAR_UDF,
                        REL_FORCE_ANY_SCALAR_UDF,
                        REL_EXPLAIN,
                        REL_HIVEMD_ACCESS,
                        REL_HBASE_ACCESS,
                        REL_HBASE_DELETE,
                        REL_HBASE_UPDATE,
                        REL_HBASE_INSERT,
                        REL_HBASE_BULK_LOAD,
                        REL_HBASE_COPROC_AGGR,
                        REL_ANY_HBASE,
                        REL_ANY_HBASE_GEN_UPDATE,
                        REL_TUPLE,
                        REL_TUPLE_LIST,
                        REL_INSERT,
                        REL_ROWSET,
                        REL_ROWSETFOR,
                        REL_ROWSET_INTO,
                        // relational operators with arity 1
                        REL_ANY_UNARY_OP,
                        REL_ANY_GROUP,
                        REL_ANY_UNARY_GEN_UPDATE,
                        REL_FORCE_EXCHANGE,
                        REL_UNARY_INSERT,
                        REL_UNARY_UPDATE,
                        REL_UNARY_DELETE,
                        REL_GROUPBY,
                        REL_AGGREGATE,
                        REL_RENAME,
                        REL_RENAME_TABLE,
                        REL_RENAME_REFERENCE,
                        REL_BEFORE_TRIGGER,
                        REL_BINDER_ONLY,
                        REL_REFRESH,
                        REL_MVLOG,
                        REL_FILTER,
                        REL_MAP_VALUEIDS,
                        REL_MATERIALIZE,
                        REL_TRANSPOSE,
                        REL_PACK,
                        REL_UNPACKROWS,
                        REL_SEQUENCE,
                        REL_SAMPLE,
                        REL_FIRST_N,
                        // relational operators with arity 2
                        REL_ANY_BINARY_OP,
                        REL_ANY_JOIN,
                        REL_ANY_INNER_JOIN,
                        REL_ANY_NON_TS_INNER_JOIN,
                        REL_ANY_TSJ,
                        REL_ANY_NON_TSJ_JOIN,
                        REL_ANY_SEMIJOIN,
                        REL_ANY_SEMITSJ,
                        REL_ANY_ANTI_SEMIJOIN,
                        REL_ANY_ANTI_SEMITSJ,
                        REL_ANY_LEFT_JOIN,
                        REL_ANY_RIGHT_JOIN,
                        REL_ANY_LEFT_TSJ,
                        REL_ANY_FULL_JOIN,
                        REL_ANY_NESTED_JOIN,
                        REL_ANY_HASH_JOIN,
                        REL_ANY_MERGE_JOIN,
                        REL_FORCE_JOIN,
                        REL_FORCE_NESTED_JOIN,
                        REL_FORCE_HASH_JOIN,
                        REL_FORCE_HYBRID_HASH_JOIN,
                        REL_FORCE_ORDERED_HASH_JOIN,
                        REL_FORCE_MERGE_JOIN,
                        REL_FORCE_ORDERED_CROSS_PRODUCT,
                        REL_JOIN,
                        REL_LEFT_JOIN,
                        REL_RIGHT_JOIN,
                        REL_FULL_JOIN,
                        REL_UNION_JOIN,
                        REL_ROUTINE_JOIN,
                        REL_TSJ,
                        REL_TSJ_FLOW,
                        REL_INDEX_JOIN,
                        REL_SEMIJOIN,
                        REL_ANTI_SEMIJOIN,
                        REL_SEMITSJ,
                        REL_ANTI_SEMITSJ,
                        REL_LEFT_TSJ,
                        REL_UNION,
                        REL_INTERSECT,
                        REL_EXCEPT,
                        REL_CHOICE,
                        REL_UPDATE,
                        REL_DELETE,
                        // relational operators with variable arity
                        REL_MULTI_JOIN,
                        REL_FILE_SCAN,
                        REL_HDFS_SCAN,
                        REL_DDL,
                        REL_DESCRIBE,
                        REL_EXE_UTIL,

                        REL_SORT_LOGICAL,

                        // phys with arity 1
                        REL_SORT,
                        REL_EXCHANGE,
                        REL_PARTITION_ACCESS,
                        REL_ORDERED_GROUPBY,
                        REL_HASHED_GROUPBY,
                        REL_SHORTCUT_GROUPBY,
                        REL_INSERT_CURSOR,
                        REL_DELETE_CURSOR,
                        REL_UPDATE_CURSOR,
                        REL_MATERIALIZE_SIMPLE,
                        REL_MATERIALIZE_HASHED,
                        REL_MATERIALIZE_ORDERED,
                        REL_PROBE_CACHE,
                        REL_ROOT,

                        REL_NO_LOGICAL, // has no logical equivalent
                        // phys with arity 2
                        REL_NESTED_JOIN,
                        REL_LEFT_NESTED_JOIN,
                        REL_NESTED_SEMIJOIN,
                        REL_NESTED_ANTI_SEMIJOIN,
                        REL_NESTED_JOIN_FLOW,
                        REL_MERGE_JOIN,
                        REL_LEFT_MERGE_JOIN,
                        REL_MERGE_SEMIJOIN,
                        REL_MERGE_ANTI_SEMIJOIN,
                        // HASH
                        REL_HASH_JOIN,
                        REL_LEFT_HASH_JOIN,
                        REL_HASH_SEMIJOIN,
                        REL_HASH_ANTI_SEMIJOIN,
                        // Left ORDERED_HASH
                        REL_ORDERED_HASH_JOIN,
                        REL_LEFT_ORDERED_HASH_JOIN,
                        REL_ORDERED_HASH_SEMIJOIN,
                        REL_ORDERED_HASH_ANTI_SEMIJOIN,
                        // HYBRID_HASH
                        REL_HYBRID_HASH_JOIN,
                        REL_LEFT_HYBRID_HASH_JOIN,
                        REL_FULL_HYBRID_HASH_JOIN,
                        REL_HYBRID_HASH_SEMIJOIN,
                        REL_HYBRID_HASH_ANTI_SEMIJOIN,

                        REL_MERGE_UNION,
                        // phys with arity > 2
                        REL_VP_JOIN,
                        // DCL (preparable set, control, ...)
                        REL_CONTROL_QUERY_SHAPE,
                        REL_CONTROL_QUERY_DEFAULT,
                        REL_CONTROL_TABLE,
                        REL_CONTROL_SESSION,
                        REL_SET_SESSION_DEFAULT,
                        // lock and unlock statements
                        REL_LOCK,
                        REL_UNLOCK,
                        // Transaction operation.
                        // Used for BEGIN/COMMIT/ROLLBACK WORK and
                        // SET TRANSACTION statements
                        REL_TRANSACTION,

                        //  Operator for SET TABLE TIMEOUT
                        REL_SET_TIMEOUT,

                        // PSM/3GL operators.
                        REL_COMPOUND_STMT,
                        REL_WHILE,
                        // special-purpose operators (optimizer)
                        REL_CUT_OP,
                        REL_TREE_OP,
                        REL_STATISTICS,

                        REL_CALLSP,   // CALL statement
                        REL_SP_PROXY, // Stored procedure result set proxy

                        // Parallel extract consumer
                        REL_EXTRACT_SOURCE,

                        // Cancel, Suspend, Activate
                        REL_CONTROL_RUNNING_QUERY,
                        REL_FAST_EXTRACT,

                        REL_HIVE_INSERT,
                        REL_ANY_EXTRACT,
                        REL_BULK_UNLOAD,

                        REL_COMMON_SUBEXPR_REF,

                        REL_LAST_REL_OP = 1999,

                        // item operators (predicates)
                        // ---------------------------------------------------
                        // NOTE: the numbers for ITM operator types are also
                        // used for expressions in the executor.
                        // ---------------------------------------------------
                        ITM_FIRST_ITEM_OP = 2000,
                        // binary logic operators
                        ITM_AND = 2001,
                        ITM_OR = 2002,

                        // unary logic operators
                        ITM_NOT = 2010,
                        ITM_IS_TRUE = 2011,
                        ITM_IS_FALSE = 2012,
                        ITM_IS_NULL = 2013,
                        ITM_IS_NOT_NULL = 2014,
                        ITM_IS_UNKNOWN = 2015,
                        ITM_IS_NOT_UNKNOWN = 2016,

                        // binary comparison operators
                        ITM_EQUAL = 2020,
                        ITM_NOT_EQUAL = 2021,
                        ITM_LESS = 2022,
                        ITM_LESS_EQ = 2023,
                        ITM_GREATER = 2024,
                        ITM_GREATER_EQ = 2025,
                        // new operator type used in the hash anti semi join optimization project
                        //
                        ITM_NOT_IN = 2026,

                        ITM_ANY_COMP = 2027,

                        // unary arithmetic operators
                        ITM_NEGATE = 2030,
                        ITM_INVERSE = 2031,

                        // binary arithmetic operators
                        ITM_PLUS = 2040,
                        ITM_MINUS = 2041,
                        ITM_TIMES = 2042,
                        ITM_DIVIDE = 2043,
                        ITM_EXPONENT = 2044,

                        // aggregate functions
                        ITM_AVG = 2050,
                        ITM_MAX = 2051,
                        ITM_MIN = 2052,
                        ITM_MAX_ORDERED = 2053,
                        ITM_MIN_ORDERED = 2054,
                        ITM_SUM = 2055,
                        ITM_COUNT = 2056,        // COUNT(*)
                        ITM_COUNT_NONULL = 2057, // COUNT(A) count non-null A's
                        ITM_STDDEV = 2058,
                        ITM_VARIANCE = 2059,
                        ITM_BASECOL = 2060,  // no aggregate function

                        ITM_ONE_ROW = 2070,
                        ITM_ONE_TRUE = 2071,
                        ITM_ANY_TRUE = 2072,
                        ITM_ANY_TRUE_MAX = 2073,
                        ITM_MAX_INCL_NULL = 2074,
                        ITM_AGGR_MIN_MAX = 2075,
                        ITM_ONEROW = 2076,
  
                        ITM_PIVOT_GROUP = 2077,

                        ITM_GROUPING = 2078,
                        ITM_AGGR_GROUPING_FUNC = 2079,
                        ITM_GROUPING_ID = 2080,

                        // custom functions
                        ITM_USER_DEF_FUNCTION = 2100,
                        ITM_BETWEEN = 2101,
                        ITM_LIKE = 2102,
                        ITM_CURRENT_TIMESTAMP = 2103,
                        ITM_CURRENT_USER = 2104,
                        ITM_SESSION_USER = 2105,
                        ITM_USER = 2106,

                        ITM_BOOL_RESULT = 2107,
                        ITM_NO_OP = 2108,

                        ITM_CASE = 2109,
                        ITM_IF_THEN_ELSE = 2110,
                        ITM_RETURN_TRUE = 2111,
                        ITM_RETURN_FALSE = 2112,
                        ITM_RETURN_NULL = 2113,

                        ITM_COMP_ENCODE = 2114,
                        ITM_HASH = 2115,

                        ITM_REPLACE_NULL = 2116,
                        ITM_PACK_FUNC = 2117,
                        ITM_BITMUX = 2118,
                        ITM_OVERLAPS = 2119,
                        ITM_RAISE_ERROR = 2120,

                        ITM_USERID = 2121,

                        // sequence functions
                        ITM_DIFF1             = 2122,
                        ITM_DIFF2             = 2123,
                        ITM_LAST_NOT_NULL     = 2124,
                        ITM_MOVING_COUNT      = 2125,
                        ITM_MOVING_SUM        = 2126,
                        ITM_MOVING_AVG        = 2127,
                        ITM_MOVING_MAX        = 2128,
                        ITM_MOVING_MIN        = 2129,
                        ITM_MOVING_SDEV       = 2130,
                        ITM_MOVING_VARIANCE   = 2131,
                        ITM_OFFSET            = 2132,
                        ITM_RUNNING_COUNT     = 2133,
                        ITM_ROWS_SINCE        = 2134,
                        ITM_RUNNING_SUM       = 2135,
                        ITM_RUNNING_AVG       = 2136,
                        ITM_RUNNING_MAX       = 2137,
                        ITM_RUNNING_MIN       = 2138,
                        ITM_RUNNING_SDEV      = 2139,
                        ITM_RUNNING_VARIANCE  = 2140 ,
                        ITM_THIS              = 2141 ,
                        ITM_NOT_THIS          = 2142,

                        // flow control
                        ITM_DO_WHILE          = 2143,
                        ITM_BLOCK             = 2144,
                        ITM_WHILE             = 2145,

                        // scalar min/max

                        ITM_SCALAR_MIN        = 2146,
                        ITM_SCALAR_MAX        = 2147,

                        ITM_CURRENT_TIMESTAMP_RUNNING = 2148,

                        // return null if argument is zero
                        ITM_NULLIFZERO        = 2149,

                        // returns operand2 if operand1 is null
                        ITM_NVL               = 2150,

                        // return NULL if 2 arguments match, else return 1st arg
                        ITM_NULLIF            = 2152,

                        // return 0 if argument is null
                        ITM_ZEROIFNULL        = 2153,

                        // converts values to user specified format
                        ITM_FORMAT            = 2154,

                        ITM_COALESCE          = 2155,

                        ITM_RUNNING_RANK      = 2156,
                        ITM_MOVING_RANK       = 2157,
                        ITM_RUNNING_CHANGE    = 2158,

                        ITM_CURRENT_ROLE      = 2159,

                        ITM_RUNNING_DRANK       = 2160,
                        ITM_MOVING_DRANK       = 2161,

                        ITM_OLAP_COUNT        = 2162,
                        ITM_OLAP_SUM          = 2163,
                        ITM_OLAP_AVG          = 2164,
                        ITM_OLAP_MAX          = 2165,
                        ITM_OLAP_MIN          = 2166,
                        ITM_OLAP_SDEV         = 2167,
                        ITM_OLAP_VARIANCE     = 2168 ,
                        ITM_OLAP_RANK         = 2169 ,
                        ITM_OLAP_DRANK        = 2170 ,
                
                        // interpret authIDs
                        ITM_AUTHNAME = 2171,
                        ITM_AUTHTYPE = 2172,

                        ITM_CURRENT_TIMESTAMP_UTC = 2173,
                        ITM_CURRENT_TIME_UTC = 2174,

                        ITM_COMP_DECODE = 2175,
                        ITM_OLAP_LEAD = 2176,
                        ITM_OLAP_LAG = 2177,

                        // Regular Expression
                        ITM_REGEXP = 2178,
			ITM_UNIX_TIMESTAMP = 2179,
			ITM_SLEEP = 2180,
			ITM_UNIQUE_SHORT_ID = 2181,

                        // numeric functions
                        ITM_ABS = 2200,
                        ITM_CEIL = 2201,
                        ITM_COS = 2202,
                        ITM_COSH = 2203,
                        ITM_FLOOR = 2204,
                        ITM_LOG = 2205,
                        ITM_LOG10 = 2206,
                        ITM_MOD = 2207,
                        ITM_POWER = 2209,
                        ITM_ROUND = 2210,
                        ITM_SIGN = 2211,
                        ITM_SIN = 2212,
                        ITM_SINH = 2213,
                        ITM_SQRT = 2214,
                        ITM_TAN = 2215,
                        ITM_TANH = 2216,
                        ITM_ROUND_ROBIN = 2217,
                        ITM_ACOS = 2218,
                        ITM_ASIN = 2219,
                        ITM_ATAN = 2220,
                        ITM_ATAN2 = 2221,
                        ITM_DEGREES = 2222,
                        ITM_EXP = 2223,
                        ITM_PI = 2224,
                        ITM_RADIANS = 2225,
                        ITM_SCALE_TRUNC = 2226,
                        ITM_MASK_CLEAR = 2227,
                        ITM_MASK_SET = 2228,
                        ITM_SHIFT_RIGHT = 2229,
                        ITM_SHIFT_LEFT = 2230,
                        ITM_BITAND = 2231,
                        ITM_BITOR = 2232,
                        ITM_BITXOR = 2233,
                        ITM_BITNOT = 2234,
                        ITM_BITEXTRACT = 2235,
                        ITM_CONVERTTOBITS = 2236,
                        ITM_LOG2= 2237,
                        // JSON fuctions
                        ITM_JSONOBJECTFIELDTEXT = 2241,

                        // string functions
                        ITM_SPLIT_PART = 2249,
                        ITM_TRUNC = 2250,
                        ITM_ASCII = 2251,
                        ITM_POSITION = 2252,
                        ITM_CHAR_LENGTH = 2253,
                        ITM_INSERT_STR = 2254,
                        ITM_OCTET_LENGTH = 2255,
                        ITM_LOWER = 2256,
                        ITM_LPAD = 2257,
                        ITM_LTRIM = 2258,
                        ITM_REPLACE = 2259,
                        ITM_RPAD = 2260,
                        ITM_RTRIM = 2261,
                        ITM_SOUNDEX = 2262,
                        ITM_SUBSTR = 2263,
                        ITM_TRIM = 2264,
                        ITM_UPPER = 2265,
                        ITM_CHAR = 2266,
                        ITM_CONCAT = 2267,
                        ITM_UNPACKCOL = 2268,
                        ITM_EXPLODE_VARCHAR = 2269,
                        ITM_REPEAT = 2270,
                        ITM_RIGHT = 2271,
                        ITM_CONVERTTOHEX = 2272,
                        ITM_CONVERTFROMHEX = 2273,

                        // UNICODE/DOUBLEBYTE charsets built-in functions
                        ITM_SUBSTR_DOUBLEBYTE = 2274,
                        ITM_TRIM_DOUBLEBYTE = 2275,
                        ITM_CHAR_LENGTH_DOUBLEBYTE = 2276,
                        ITM_POSITION_DOUBLEBYTE= 2277,
                        ITM_LIKE_DOUBLEBYTE = 2278,
                        ITM_UPPER_UNICODE = 2279,
                        ITM_LOWER_UNICODE = 2280,
                        ITM_REPEAT_UNICODE = 2281,
                        ITM_REPLACE_UNICODE = 2282,
                        ITM_UNICODE_CODE_VALUE = 2283,

                        // translate function
                        ITM_TRANSLATE = 2284,

                        ITM_UNICODE_CHAR = 2285,

                        // RowSet expression functions
                        ITM_ROWSETARRAY_SCAN =  2286,
                        ITM_ROWSETARRAY_ROWID = 2287,
                        ITM_ROWSETARRAY_INTO  = 2288,

                        // more string functions
                        ITM_LEFT = 2289,
                        ITM_SPACE = 2290,
                        ITM_ODBC_LENGTH = 2291,
                        ITM_TOKENSTR = 2292,
                        ITM_CODE_VALUE = 2293,
                        ITM_REVERSE = 2294,
                        ITM_OVERLAY = 2295,

                        // datetime functions
                        ITM_CONVERTTIMESTAMP = 2300,
                        ITM_DATEFORMAT = 2301,
                        ITM_DAYOFWEEK = 2302,
                        ITM_EXTRACT = 2303,
                        ITM_INITCAP = 2304,
                        ITM_JULIANTIMESTAMP = 2305,
                        ITM_EXTRACT_ODBC = 2306,
                        ITM_DAYNAME = 2307,
                        ITM_MONTHNAME = 2308,
                        ITM_QUARTER = 2309,
                        ITM_WEEK = 2310,
                        ITM_DAYOFYEAR = 2311,
                        ITM_FIRSTDAYOFYEAR = 2312,
                        ITM_INTERNALTIMESTAMP = 2313, // ++ Triggers,
                        ITM_DAYOFMONTH = 2314,

                        ITM_DATE_TRUNC_YEAR    = 2315,
                        ITM_DATE_TRUNC_MONTH   = 2316,
                        ITM_DATE_TRUNC_DAY     = 2317,
                        ITM_DATE_TRUNC_HOUR    = 2318,
                        ITM_DATE_TRUNC_MINUTE  = 2319,
                        ITM_DATE_TRUNC_SECOND  = 2320,
                        ITM_DATE_TRUNC_CENTURY = 2321,
                        ITM_DATE_TRUNC_DECADE  = 2322,
                        ITM_DATEDIFF_YEAR      = 2323,
                        ITM_DATEDIFF_MONTH     = 2324,
                        ITM_DATEDIFF_DAY       = 2325,
                        ITM_DATEDIFF_HOUR      = 2326,
                        ITM_DATEDIFF_MINUTE    = 2327,
                        ITM_DATEDIFF_SECOND    = 2328,
                        ITM_DATEDIFF_QUARTER   = 2329,
                        ITM_DATEDIFF_WEEK      = 2330,
                        // the following 8 items are for timestampdiff
                        ITM_TSI_YEAR           = 2331,
                        ITM_TSI_MONTH          = 2332,
                        ITM_TSI_DAY            = 2333,
                        ITM_TSI_HOUR           = 2334,
                        ITM_TSI_MINUTE         = 2335,
                        ITM_TSI_SECOND         = 2336,
                        ITM_TSI_QUARTER        = 2337,
                        ITM_TSI_WEEK           = 2338,

                        ITM_LAST_DAY           = 2339,
                        ITM_NEXT_DAY           = 2340,
                        ITM_TO_NUMBER          = 2341,
                        ITM_MONTHS_BETWEEN     = 2342,
                        ITM_YEARWEEK           = 2343,
                        ITM_YEARWEEKD          = 2344,
                        ITM_TO_TIMESTAMP       = 2345,
                        ITM_TO_BINARY          = 2346,
                        ITM_ENCODE_BASE64      = 2347,
                        ITM_DECODE_BASE64      = 2348,
                        ITM_TO_CHAR            = 2349,

                        // misc. functions
                        ITM_NARROW = 2350, // a variant on Cast used for key building
                        ITM_INTERVAL = 2351,
                        ITM_INSTANTIATE_NULL = 2352,
                        ITM_INCREMENT = 2353,
                        ITM_DECREMENT = 2354,
                        ITM_GREATER_OR_GE = 2355,
                        ITM_LESS_OR_LE = 2356,
                        ITM_RANGE_LOOKUP = 2357,
                        ITM_NOT_USED_1 = 2358, // This opertype is available.
                        ITM_HDPHASHCOMB = 2359,
                        ITM_RANDOMNUM = 2360,
                        ITM_PROGDISTRIB = 2361,
                        ITM_HASHCOMB = 2362,
                        ITM_HDPHASH = 2363,
                        ITM_EXEC_COUNT = 2364,
                        ITM_CURR_TRANSID = 2365,
                        ITM_NOTCOVERED = 2366,
                        ITM_BALANCE = 2367,
                        ITM_RAND_SELECTION = 2368,
                        ITM_PROGDISTRIBKEY = 2369,
                        ITM_PAGROUP = 2370,
                        ITM_HASH2_DISTRIB = 2371,
                        ITM_IDENTITY = 2372,
                        ITM_QUERYID_EXTRACT = 2374,
                        ITM_HEADER = 2375,
                        ITM_RANGE_SPEC_FUNC = 2376,

                        // insert into lob column
                        ITM_LOBINSERT = 2377,

                        // select from lob column
                        ITM_LOBSELECT = 2378,

                        // delete lob column
                        ITM_LOBDELETE = 2379,

                        // update lob column
                        ITM_LOBUPDATE = 2380,

                        // convert LOB data to string
                        ITM_LOBCONVERT = 2381,

                        // convert LOB handle stored in the row.
                        ITM_LOBCONVERTHANDLE = 2382,

                        ITM_LOBLOAD = 2383,
                        ITM_LOBEXTRACT = 2384,

                        ITM_GREATEST = 2385,
                        ITM_LEAST = 2386,

			ITM_UNIQUE_EXECUTE_ID = 2391,
			ITM_GET_TRIGGERS_STATUS = 2392,
			ITM_GET_BIT_VALUE_AT = 2393,
			ITM_CURRENTEPOCH = 2394,
			ITM_VSBBROWTYPE = 2395,
			ITM_VSBBROWCOUNT = 2396,
                        ITM_IS_BITWISE_AND_TRUE = 2397,

                        ITM_NCHAR_MP_CHAR = 2398,
                        ITM_NCHAR_MP_CODE_VALUE = 2399,

                        // subqueries
                        ITM_ROW_SUBQUERY = 2400,
                        ITM_IN_SUBQUERY,
                        ITM_IN,
                        ITM_EXISTS,
                        ITM_NOT_EXISTS,
                        ITM_EQUAL_ALL,
                        ITM_EQUAL_ANY,
                        ITM_NOT_EQUAL_ALL,
                        ITM_NOT_EQUAL_ANY,
                        ITM_LESS_ALL,
                        ITM_LESS_ANY,
                        ITM_GREATER_ALL,
                        ITM_GREATER_ANY,
                        ITM_LESS_EQ_ALL,
                        ITM_LESS_EQ_ANY,
                        ITM_GREATER_EQ_ALL,
                        ITM_GREATER_EQ_ANY,

                        ITM_WILDCARD_EQ_NE = 2449,

                        // renaming, conversion, assignment
                        ITM_RENAME_COL = 2450,
                        ITM_CONVERT = 2451,
                        ITM_CAST = 2452,
                        ITM_ASSIGN = 2453,

                        // convert an NA-type to an item expression
                        ITM_NATYPE = 2454,

                        // do a cast but adjust target length based
                        // on operand (used by ODBC)
                        ITM_CAST_CONVERT = 2455,

                        // for OperatorType::match() of ItemExpr::origOpType()
                        ITM_ANY_AGGREGATE = 2456,

                        // to match Cast, Cast_Convert, Instantiate_Null, Narrow
                        ITM_ANY_CAST = 2457,

                        ITM_UNIQUE_ID = 2458,
                        ITM_UNIQUE_ID_SYS_GUID = 2459,

                       // Do not do any conversion. Just interpret source as the specified type.
                        ITM_CAST_TYPE,

                        // item expressions describing constraints
                        ITM_CHECK_CONSTRAINT = 2500,
                        ITM_CARD_CONSTRAINT = 2501,
                        ITM_UNIQUE_CONSTRAINT = 2502,
                        ITM_REF_CONSTRAINT = 2503,
                        ITM_UNIQUE_OPT_CONSTRAINT = 2504,
                        ITM_FUNC_DEPEND_CONSTRAINT = 2505,
                        ITM_CHECK_OPT_CONSTRAINT,
                        ITM_REF_OPT_CONSTRAINT,
                        ITM_COMP_REF_OPT_CONSTRAINT,

                        // lookup a column in a native hbase table being accessed in row format
                        ITM_HBASE_COLUMN_LOOKUP,

                        // display hbase columns being accessed in row format
                        ITM_HBASE_COLUMNS_DISPLAY,

                        ITM_HBASE_COLUMN_CREATE,

                        // generate sequence numbers
                        ITM_SEQUENCE_VALUE,

                        // return number of the row being returned. Starts at 1
                        ITM_ROWNUM,

                        ITM_HBASE_TIMESTAMP,
                        ITM_HBASE_TIMESTAMP_REF,
                        ITM_HBASE_VERSION,
                        ITM_HBASE_VERSION_REF,

                        // list of item expressions
                        ITM_ITEM_LIST = 2550,

                        // leaf nodes of item expressions
                        ITM_CONSTANT = 2555,
                        ITM_REFERENCE = 2556,
                        ITM_BASECOLUMN = 2557,
                        ITM_INDEXCOLUMN = 2558,
                        ITM_HOSTVAR = 2559,
                        ITM_DYN_PARAM = 2560,
                        ITM_SEL_INDEX = 2561,
                        ITM_VALUEIDREF = 2562,
                        ITM_VALUEIDUNION = 2563,
                        ITM_VEG = 2564,
                        ITM_VEG_PREDICATE = 2565,
                        ITM_VEG_REFERENCE = 2566,
                        ITM_DEFAULT_SPECIFICATION = 2567,
                        ITM_SAMPLE_VALUE = 2568,
                        ITM_CACHE_PARAM = 2569,
                        ITM_ROUTINE_PARAM = 2570,
                        ITM_VALUEID_PROXY = 2571,

                        // Item expressions for transactions
                        ITM_SET_TRANS_ISOLATION_LEVEL = 2600,
                        ITM_SET_TRANS_ACCESS_MODE = 2601,
                        ITM_SET_TRANS_DIAGS = 2602,
                        ITM_DECODE = 2603,  // this one alone not related to transactions
                        ITM_SET_TRANS_ROLLBACK_MODE = 2604,
                        ITM_SET_TRANS_AUTOABORT_INTERVAL = 2605,
                        ITM_SET_TRANS_MULTI_COMMIT = 2606,

                        // Item expressions for MySQL/Oracle/DB2/SQL-Server compatibility
                        ITM_ISIPV4 = 2630,
                        ITM_ISIPV6 = 2631,
                        ITM_INET_ATON = 2632,
                        ITM_INET_NTOA = 2633,
                        ITM_CRC32     = 2634,
                        ITM_MD5       = 2635,
                        ITM_SHA1      = 2636,
                        ITM_SHA2_256  = 2637,
                        ITM_SHA2_224  = 2638,
                        ITM_SHA2_384  = 2639,
                        ITM_SHA2_512  = 2640,

                        ITM_AES_ENCRYPT = 2641,
                        ITM_AES_DECRYPT = 2642,

                        // Items for needed for Translating to UCS2 output strings
                        ITM_DATEFMT     = 2990,
                        ITM_CURRNT_USER = 2991,
                        ITM_SESSN_USER  = 2992,
                        ITM_CONVERTTOHX = 2993,
                        ITM_HIVE_HASH = 2994,
                        ITM_HIVE_HASHCOMB = 2995,
                        ITM_NAMED_TYPE_TO_ITEM = 2996,

                        ITM_LAST_ITEM_OP = 2999,
                        DML_LAST_DML_OP  = ITM_LAST_ITEM_OP,

                        // now for the statement ops
                        STM_FIRST_STMT_OP = 3000,
                        STM_QUERY,
                        STM_BEGIN_DECLARE,
                        STM_END_DECLARE,
                        STM_ALLOC_STATIC_DESC,
                        STM_PROCEDURE,
                        STM_MODULE,
                        STM_TIMESTAMP,
                        STM_DECL_STATCURS,
                        STM_DECL_DYNCURS,
                        STM_DECL_XDYNCURS,
                        STM_OPEN,
                        STM_FETCH,
                        STM_CLOSE,
                        STM_PREPARE,
                        STM_EXECUTE,
                        STM_WHENEVER,
                        STM_XACT_CTL,
                        STM_DESCRIBE,
                        STM_DEALLOC_DESC,
                        STM_DEALLOC_STM,
                        STM_EXEC_IMMED,
                        STM_STMT_DIAGS,
                        STM_COND_DIAGS,
                        STM_ALLOC_DESC,
                        STM_GET_DESCCOUNT,
                        STM_GET_DESCITEM,
                        STM_SET_DESCCOUNT,
                        STM_SET_DESCITEM,
                        STM_TRANSACTION,
                        STM_SET_TRANS,
                        STM_LAST_STMT_OP,
                        STM_BEGIN_WORK,
                        STM_COMMIT_WORK,
                        STM_ROLLBACK_WORK,
                        STM_SOURCE_FILE,
                        STM_GET_ROWSETSIZE,
                        STM_SET_ROWSETSIZE,
                        //
                        // DDL statements
                        //
                        DDL_FIRST_DDL_OP = 4000,
                        DDL_FIRST_STMT_OP = DDL_FIRST_DDL_OP,
                        DDL_ALTER_AUDIT_CONFIG,
                        DDL_ALTER_CATALOG,
                        DDL_ALTER_SCHEMA,
                        DDL_ALTER_INDEX_ATTRIBUTE,
                        DDL_ALTER_INDEX_ALTER_HBASE_OPTIONS,
                        DDL_ALTER_LIBRARY,
                        DDL_ALTER_MV,  // MV
                        DDL_ALTER_ROUTINE,
                        DDL_ALTER_SYNONYM,
                        DDL_ALTER_TABLE_ADD_COLUMN,
                        DDL_ALTER_TABLE_ADD_CONSTRAINT_CHECK,
                        DDL_ALTER_TABLE_ADD_CONSTRAINT_PRIMARY_KEY,
                        DDL_ALTER_TABLE_ADD_CONSTRAINT_REFERENTIAL_INTEGRITY,
                        DDL_ALTER_TABLE_ADD_CONSTRAINT_UNIQUE,
                        DDL_ALTER_TABLE_ALTER_COLUMN,
                        DDL_ALTER_TABLE_ALTER_COLUMN_LOGGABLE, //++ MV
                        DDL_ALTER_TABLE_DISABLE_INDEX,
                        DDL_ALTER_TABLE_ENABLE_INDEX,
                        DDL_ALTER_TABLE_ATTRIBUTE,
                        DDL_ALTER_TABLE_DROP_COLUMN,
                        DDL_ALTER_TABLE_DROP_CONSTRAINT,
                        DDL_ALTER_TABLE_COLUMN,
                        DDL_ALTER_TABLE_MOVE,
                        DDL_ALTER_TABLE_PARTITION,
                        DDL_ALTER_TABLE_RENAME,
                        DDL_ALTER_TABLE_STORED_DESC,
                        DDL_ALTER_TABLE_SET_CONSTRAINT,
                        DDL_ALTER_TABLE_DROPPABLE,
                        DDL_ALTER_TABLE_INSERT_ONLY,
                        DDL_ALTER_TABLE_TOGGLE_CONSTRAINT,
                        DDL_ALTER_TABLE_TOGGLE_ONLINE,
                        DDL_ALTER_TABLE_NAMESPACE,
                        DDL_ALTER_TABLE_ALTER_COLUMN_DATATYPE,
                        DDL_ALTER_TABLE_ALTER_COLUMN_RENAME,
                        DDL_ALTER_TABLE_ALTER_COLUMN_DEFAULT_VALUE,
                        DDL_ALTER_TABLE_ALTER_COLUMN_SET_SG_OPTION,
                        DDL_ALTER_TABLE_ALTER_HBASE_OPTIONS,
                        DDL_ALTER_MV_RENAME,
                        DDL_ALTER_MV_REFRESH_GROUP, // MV
                        DDL_ALTER_TRIGGER,
                        DDL_ALTER_USER,
                        DDL_ALTER_VIEW,
                        DDL_ALTER_DATABASE,
                        DDL_ANY_ALTER_INDEX_STMT,
                        DDL_ANY_ALTER_TABLE_ADD_CONSTRAINT_STMT,
                        DDL_ANY_ALTER_TABLE_STMT,
                        DDL_ANY_STMT,
                        DDL_CREATE_SYNONYM,
                        DDL_CREATE_CATALOG,
                        DDL_CREATE_COMPONENT_PRIVILEGE,
                        DDL_CREATE_INDEX,
                        DDL_POPULATE_INDEX,
                        DDL_CREATE_LIBRARY,
                        DDL_CREATE_MV, // MV
                        DDL_CREATE_SCHEMA,
                        DDL_CREATE_ROUTINE,
                        DDL_CREATE_SEQUENCE,
                        DDL_CREATE_TABLE,
                        DDL_CREATE_HBASE_TABLE,
                        DDL_CREATE_MV_REFRESH_GROUP,  // MV  - ( refresh groups )
                        DDL_CREATE_TRIGGER,
                        DDL_CREATE_VIEW,
                        DDL_CREATE_EXCEPTION_TABLE,
                        DDL_DROP_SYNONYM,
                        DDL_DROP_CATALOG,
                        DDL_DROP_COMPONENT_PRIVILEGE,
                        DDL_DROP_INDEX,
                        DDL_DROP_LIBRARY,
                        DDL_DROP_MODULE,
                        DDL_DROP_MV, // MV
                        DDL_DROP_ROUTINE,
                        DDL_DROP_SEQUENCE,
                        DDL_DROP_SCHEMA,
                        DDL_DROP_SQL,
                        DDL_DROP_TABLE,
                        DDL_DROP_HBASE_TABLE,
                        DDL_DROP_MV_REFRESH_GROUP, // MV - ( refresh groups )
                        DDL_DROP_TRIGGER,
                        DDL_DROP_VIEW,
                        DDL_DROP_EXCEPTION_TABLE,
                        DDL_GIVE_ALL,
                        DDL_GIVE_CATALOG,
                        DDL_GIVE_OBJECT,
                        DDL_GIVE_SCHEMA,
                        DDL_GRANT,
                        DDL_GRANT_SCHEMA,
                        DDL_GRANT_ROLE,
                        DDL_GRANT_COMPONENT_PRIVILEGE,
                        DDL_PUBLISH,
                        DDL_INITIALIZE_SQL,
                        DDL_REGISTER_CATALOG,
                        DDL_REGISTER_COMPONENT,
                        DDL_UNREGISTER_CATALOG,
                        DDL_REGISTER_USER,
                        DDL_REG_OR_UNREG_OBJECT,
                        DDL_REINITIALIZE_SQL,
                        DDL_REVOKE,
                        DDL_REVOKE_COMPONENT_PRIVILEGE,
                        DDL_REVOKE_SCHEMA,
                        DDL_REVOKE_ROLE,
                        DDL_CREATE_ROLE,
                        DDL_CLEANUP_OBJECTS,
                        DDL_LAST_STMT_OP,
                        DDL_INITIALIZE_SECURITY,
                        DDL_COMMENT_ON,

                        // ddl operations on hive objects
                        DDL_ON_HIVE_OBJECTS,

                        //
                        // Elements in DDL statements
                        //
                        ELM_FIRST_ELEM_OP = 4200,
                        ELM_ALTER_TABLE_MOVE_ELEM,

                        ELM_ANY_ELEM,
                        ELM_ANY_CONSTRAINT_ELEM,
                        ELM_ANY_CONSTRAINT_ATTR_ELEM,
                        ELM_ANY_FILE_ATTR_ELEM,
                        ELM_ANY_LIKE_ELEM,
                        ELM_ANY_LIKE_OPT_ELEM,
                        ELM_ANY_LOAD_OPT_ELEM,
                        ELM_ANY_PARTITION_BY_ELEM,
                        ELM_ANY_PARTITION_ELEM,
                        ELM_ANY_PRIV_ACT_ELEM,
                        ELM_ANY_PRIV_ACT_WITH_COLUMNS_ELEM,
                        ELM_ANY_REF_ACT_ELEM,
                        ELM_ANY_REF_TRIG_ACT_RULE_ELEM,
                        ELM_ANY_STORE_OPT_ELEM,
                        
                        ELM_CLIENTNAME_ELEM,
                        ELM_CLIENTFILE_ELEM,
                        ELM_PATHNAME_ELEM,

                        ELM_COL_DEF_ELEM,
                        ELM_COL_DEFAULT_ELEM,
                        ELM_COL_HEADING_ELEM,
                        ELM_COL_NAME_ELEM,
                        ELM_COL_NAME_LIST,
                        ELM_COL_NAME_LIST_NODE,
                        ELM_MV_NAME_LIST, // MV
                        ELM_COL_REF_ELEM,
                        ELM_COL_REF_LIST,
                        ELM_COL_VIEW_DEF_ELEM,

                        ELM_CONSTRAINT_ATTR_DEFERRABLE_ELEM,
                        ELM_CONSTRAINT_ATTR_DROPPABLE_ELEM,

                        ELM_CONSTRAINT_CHECK_ELEM,
                        ELM_CONSTRAINT_NAME_LIST,
                        ELM_CONSTRAINT_NOT_NULL_ELEM,
                        ELM_CONSTRAINT_PRIMARY_KEY_COLUMN_ELEM,
                        ELM_CONSTRAINT_PRIMARY_KEY_ELEM,
                        ELM_CONSTRAINT_REFERENTIAL_INTEGRITY_ELEM,
                        ELM_CONSTRAINT_UNIQUE_ELEM,

                        ELM_LOGGABLE,
                        ELM_LOBATTRS,
                        ELM_SEABASE_SERIALIZED,

                        // MV OZ
                        ELM_CREATE_MV_ONE_ATTRIBUTE_TABLE_LIST,

                        ELM_DIVISION_CLAUSE_ELEM,

                        // Note that classes with the prefix ElemDDL
                        // and the suffix List are derived from class
                        // ElemDDLList
                        ELM_ELEM_LIST,

                        ELM_FILE_ATTR_ALLOCATE_ELEM,
                        ELM_FILE_ATTR_AUDIT_ELEM,
                        ELM_FILE_ATTR_AUDIT_COMPRESS_ELEM,
                        ELM_FILE_ATTR_BLOCK_SIZE_ELEM,
                        ELM_FILE_ATTR_BUFFERED_ELEM,

                        // Note that class ElemDDLFileAttrClause
                        // is derived from class ElemDDLNode instead
                        // of class ElemDDLFileAttr
                        ELM_FILE_ATTR_CLAUSE_ELEM,

                        ELM_FILE_ATTR_CLEAR_ON_PURGE_ELEM,
                        ELM_FILE_ATTR_COMPRESSION_ELEM,
                        ELM_FILE_ATTR_D_COMPRESS_ELEM,
                        ELM_FILE_ATTR_DEALLOCATE_ELEM,
                        ELM_FILE_ATTR_I_COMPRESS_ELEM,

                        // Note that class ElemDDLFileAttrList and
                        // ElemDDLPartnAttrList are derived
                        // from class ElemDDLList (instead
                        // of class ElemDDLFileAttr.
                        ELM_FILE_ATTR_LIST,
                        ELM_PARTN_ATTR_LIST,
                        ELM_FILE_ATTR_MAX_SIZE_ELEM,
                        ELM_FILE_ATTR_NO_LABEL_UPDATE_ELEM,
                        ELM_FILE_ATTR_OWNER_ELEM,
                        //++ MV OZ
                        ELM_FILE_ATTR_RANGE_LOG_ELEM,
                        ELM_FILE_ATTR_LOCK_ON_REFRESH_ELEM,
                        ELM_FILE_ATTR_INSERT_LOG_ELEM,
                        ELM_FILE_ATTR_MVS_ALLOWED_ELEM,
                        ELM_FILE_ATTR_MV_COMMIT_EACH_ELEM,
                        ELM_FILE_ATTR_MVAUDIT_ELEM,
                        ELM_MV_FILE_ATTR_CLAUSE_ELEM,
                        //-- MV

                        //POS
                        ELM_FILE_ATTR_POS_NUM_PARTNS_ELEM,
                        ELM_FILE_ATTR_POS_TABLE_SIZE_ELEM,
                        ELM_FILE_ATTR_POS_DISK_POOL_ELEM,
                        ELM_FILE_ATTR_POS_IGNORE_ELEM,

                        //ROW FORMAT
                        ELM_FILE_ATTR_ROW_FORMAT_ELEM,
  
                       // default column family for columns specified in CREATE stmt
                        ELM_FILE_ATTR_COL_FAM_ELEM,

                        ELM_GRANTEE_ELEM,
                        ELM_KEY_VALUE_ELEM,
                        ELM_KEY_VALUE_LIST,
                        ELM_LIKE_CREATE_TABLE_ELEM,
                        ELM_LIKE_OPT_WITHOUT_CONSTRAINTS_ELEM,
                        ELM_LIKE_OPT_WITH_HEADINGS_ELEM,
                        ELM_LIKE_OPT_WITH_HORIZONTAL_PARTITIONS_ELEM,
                        ELM_LIKE_OPT_WITHOUT_SALT_ELEM,
                        ELM_LIKE_OPT_SALT_CLAUSE_ELEM,
                        ELM_LIKE_OPT_WITHOUT_DIVISION_ELEM,
                        ELM_LIKE_OPT_LIMIT_COLUMN_LENGTH,
                        ELM_LIKE_OPT_WITHOUT_ROW_FORMAT_ELEM,
                        ELM_LIKE_OPT_WITHOUT_LOB_COLUMNS,
                        ELM_LOCATION_ELEM,
                        ELM_OPTION_LIST,
                        ELM_PARALLEL_EXEC_ELEM,
                        ELM_PARAM_DEF_ELEM,
                        ELM_PARAM_NAME_ELEM,
                        ELM_PARTITION_BY_COLUMN_LIST_ELEM,

                        // Note that class ElemDDLPartitionClause
                        // is derived from class ElemDDLNode instead
                        // of class ElemDDLPartition
                        ELM_PARTITION_CLAUSE_ELEM,

                        // Note that class ElemDDLPartitionList is
                        // derived from class ElemDDLList instead
                        // of class ElemDDLPartition
                        ELM_PARTITION_LIST,

                        ELM_PARTITION_RANGE_ELEM,
                        ELM_PARTITION_SINGLE_ELEM,
                        ELM_PARTITION_SYSTEM_ELEM,

                        ELM_PASS_THROUGH_PARAM_DEF_ELEM,

                        ELM_PRIVILEGES_ELEM,

                        ELM_PRIV_ACT_DELETE_ELEM,
                        ELM_PRIV_ACT_EXECUTE_ELEM,
                        ELM_PRIV_ACT_INSERT_ELEM,
                        ELM_PRIV_ACT_REFERENCES_ELEM,
                        ELM_PRIV_ACT_SELECT_ELEM,
                        ELM_PRIV_ACT_UPDATE_ELEM,
                        ELM_PRIV_ACT_USAGE_ELEM,

                        // Added for schema privileges
                        ELM_PRIV_ACT_CREATE_ELEM,
                        ELM_PRIV_ACT_CREATE_TABLE_ELEM,
                        ELM_PRIV_ACT_CREATE_VIEW_ELEM,
                        ELM_PRIV_ACT_CREATE_LIBRARY_ELEM,
                        ELM_PRIV_ACT_CREATE_MV_ELEM,
                        ELM_PRIV_ACT_CREATE_MVGROUP_ELEM,
                        ELM_PRIV_ACT_CREATE_PROCEDURE_ELEM,
                        ELM_PRIV_ACT_CREATE_ROUTINE_ELEM,
                        ELM_PRIV_ACT_CREATE_ROUTINE_ACTION_ELEM,
                        ELM_PRIV_ACT_CREATE_TRIGGER_ELEM,
                        ELM_PRIV_ACT_CREATE_SYNONYM_ELEM,
                        ELM_PRIV_ACT_ALTER_ELEM,
                        ELM_PRIV_ACT_ALTER_TABLE_ELEM,
                        ELM_PRIV_ACT_ALTER_VIEW_ELEM,
                        ELM_PRIV_ACT_ALTER_LIBRARY_ELEM,
                        ELM_PRIV_ACT_ALTER_MV_ELEM,
                        ELM_PRIV_ACT_ALTER_MVGROUP_ELEM,
                        ELM_PRIV_ACT_ALTER_TRIGGER_ELEM,
                        ELM_PRIV_ACT_ALTER_ROUTINE_ELEM,
                        ELM_PRIV_ACT_ALTER_ROUTINE_ACTION_ELEM,
                        ELM_PRIV_ACT_ALTER_SYNONYM_ELEM,
                        ELM_PRIV_ACT_DROP_ELEM,
                        ELM_PRIV_ACT_DROP_TABLE_ELEM,
                        ELM_PRIV_ACT_DROP_VIEW_ELEM,
                        ELM_PRIV_ACT_DROP_LIBRARY_ELEM,
                        ELM_PRIV_ACT_DROP_MV_ELEM,
                        ELM_PRIV_ACT_DROP_MVGROUP_ELEM,
                        ELM_PRIV_ACT_DROP_PROCEDURE_ELEM,
                        ELM_PRIV_ACT_DROP_ROUTINE_ELEM,
                        ELM_PRIV_ACT_DROP_ROUTINE_ACTION_ELEM,
                        ELM_PRIV_ACT_DROP_TRIGGER_ELEM,
                        ELM_PRIV_ACT_DROP_SYNONYM_ELEM,
                        ELM_PRIV_ACT_MAINTAIN_ELEM,
                        ELM_PRIV_ACT_REORG_ELEM,
                        ELM_PRIV_ACT_REFRESH_ELEM,
                        ELM_PRIV_ACT_TRANSFORM_ELEM,
                        ELM_PRIV_ACT_UPDATE_STATS_ELEM,
                        ELM_PRIV_ACT_ALL_DDL_ELEM,
                        ELM_PRIV_ACT_ALL_DML_ELEM,
                        ELM_PRIV_ACT_ALL_OTHER_ELEM,
                        ELM_PRIV_ACT_DBA_ELEM,
                        ELM_PRIV_ACT_BUSINESS_CONTINUITY_ELEM,
                        ELM_PRIV_ACT_BACKUP_ELEM,
                        ELM_PRIV_ACT_RESTORE_ELEM,
                        ELM_PRIV_ACT_REPLICATE_ELEM,
                        ELM_PRIV_ACT_ARCHIVE_ELEM,

                        ELM_REFERENCES_ELEM,

                        ELM_REF_ACT_CASCADE_ELEM,
                        ELM_REF_ACT_NO_ACTION_ELEM,
                        ELM_REF_ACT_RESTRICT_ELEM,
                        ELM_REF_ACT_SET_DEFAULT_ELEM,
                        ELM_REF_ACT_SET_NULL_ELEM,

                        ELM_REF_TRIG_ACT_DELETE_RULE_ELEM,
                        ELM_REF_TRIG_ACT_UPDATE_RULE_ELEM,

                        ELM_AUTH_SCHEMA_ELEM,
                        ELM_SCHEMA_NAME_ELEM,

                        ELM_SALT_OPTIONS_ELEM,

                        // Sequence generator options
                        ELM_SG_OPT_DEFAULT_ELEM,
                        ELM_SG_OPT_START_VALUE_ELEM,
                        ELM_SG_OPT_INCREMENT_ELEM,
                        ELM_SG_OPT_MIN_VALUE_ELEM,
                        ELM_SG_OPT_MAX_VALUE_ELEM,
                        ELM_SG_OPT_CYCLE_OPTION_ELEM,
                        ELM_SG_OPT_CACHE_OPTION_ELEM,
                        ELM_SG_OPT_DATATYPE_ELEM,
                        ELM_SG_OPT_RESET_OPTION_ELEM,
                        ELM_SG_OPTS_DEFAULT_ELEM,

                        ELM_STORE_OPT_DEFAULT_ELEM,
                        ELM_STORE_OPT_ENTRY_ORDER_ELEM,
                        ELM_STORE_OPT_KEY_COLUMN_LIST_ELEM,
                        ELM_STORE_OPT_NONDROPPABLE_PRIMARY_KEY_ELEM,

                        ELM_TABLE_FEATURE_ELEM,
                        ELM_HBASE_OPTIONS_ELEM,

                        ELM_UDF_EXECUTION_MODE,
                        ELM_UDF_FINAL_CALL,
                        ELM_UDF_OPTIMIZATION_HINT,
                        ELM_UDF_PARALLELISM,
                        ELM_UDF_STATE_AREA_SIZE,
                        ELM_UDF_VERSION_TAG,
                        ELM_UDR_DETERMINISTIC,
                        ELM_UDR_EXTERNAL_NAME,
                        ELM_UDR_EXTERNAL_PATH,
                        ELM_UDR_ISOLATE,
                        ELM_UDR_LANGUAGE,
                        ELM_UDR_LIBRARY,
                        ELM_UDR_MAX_RESULTS,
                        ELM_UDR_PARAM_STYLE,
                        ELM_UDF_SPECIAL_ATTRIBUTES,
                        ELM_UDR_SQL_ACCESS,
                        ELM_UDR_SURROGATE_LOCATION,
                        ELM_UDR_TRANSACTION_ATTRIBUTES,
                        ELM_UDR_EXTERNAL_SECURITY,
                        ELM_UUDF_PARAM_DEF,

                        //ELM_VERTICAL_PARTITION_LIST,
                        //ELM_VERTICAL_PARTITION_OPTION_ELEM,

                        ELM_WITH_CHECK_OPTION_ELEM,
                        ELM_WITH_GRANT_OPTION_ELEM,
                        ELM_WITH_POPULATE_OPTION_ELEM,
                        ELM_CONSTRAINT_ATTR_ENFORCED_ELEM,

                        ELM_LAST_ELEM_OP,

                        // Unpublished attributes
                        ELM_FILE_ATTR_EXTENT_ELEM,
                        ELM_FILE_ATTR_MAXEXTENTS_ELEM,
                        ELM_FILE_ATTR_UID_ELEM
                      };

#endif // OPERTYPEENUM_H
