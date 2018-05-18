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
******************************************************************************
*
* File:         ExprNode.C
* Description:  Expression nodes (relational nodes and item expression nodes)
*
* Created:      5/13/94 (Friday, 13, keep that in mind when reading the file)
* Language:     C++
*
*
*
******************************************************************************
*/


#include "ExprNode.h"

// -----------------------------------------------------------------------
// method for class OperatorType
// -----------------------------------------------------------------------
NABoolean OperatorType::match(OperatorTypeEnum wildcard) const
{
  if (op_ == wildcard)
    return TRUE; // that was easy!
  else
    {
      #ifndef NDEBUG
      // the method assumes that only "wildcard" can contain a wildcard!!
      assert (!(OperatorType(op_).isWildcard()));
      #endif

      // now the actual logic to match wildcards 
      switch (wildcard)
	{
	case ITM_ANY_AGGREGATE:
	  switch (op_)
	    {
	    case ITM_AVG:
	    case ITM_MAX:
	    case ITM_MIN:
	    case ITM_MAX_ORDERED:
	    case ITM_MIN_ORDERED:
	    case ITM_SUM:
	    case ITM_COUNT:
	    case ITM_COUNT_NONULL:
	    case ITM_STDDEV:
	    case ITM_VARIANCE:
	    //##? Do we want to add Austin group's Sequence-Functions here
	    //##? (moving and running sums, counts, averages, etc) ?
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case ITM_ANY_CAST:
	  switch (op_)
	    {
	    case ITM_CAST:
	    case ITM_CAST_CONVERT:
	    case ITM_INSTANTIATE_NULL:
	    case ITM_NARROW:
	    case ITM_CAST_TYPE:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case ITM_WILDCARD_EQ_NE:
	  switch (op_)
	    {
	    case ITM_EQUAL:
	    case ITM_NOT_EQUAL:
	    case ITM_EQUAL_ALL:
	    case ITM_EQUAL_ANY:
	    case ITM_NOT_EQUAL_ALL:
	    case ITM_NOT_EQUAL_ANY:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_LEAF_OP:
	  switch (op_)
	    {
	    case REL_SCAN:
	    case REL_EXPLAIN:
	    case REL_TUPLE:
	    case REL_TUPLE_LIST:
	    case REL_LEAF_INSERT:
	    case REL_LEAF_UPDATE:
	    case REL_LEAF_DELETE:
	    case REL_ISOLATED_SCALAR_UDF:
	    case REL_LEAF_TABLE_MAPPING_UDF:
	    case REL_HIVE_INSERT:
	    case REL_HBASE_INSERT:
	    case REL_SP_PROXY:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_FORCE_ANY_SCAN:
	  switch (op_)
	    {
	    case REL_SCAN:
	    case REL_FILE_SCAN:
	    case REL_HBASE_ACCESS:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_UNARY_OP:
	  switch (op_)
	    {
	    case REL_UNARY_INSERT:
	    case REL_UNARY_UPDATE:
	    case REL_UNARY_DELETE:
	    case REL_GROUPBY:
	    case REL_RENAME_TABLE:
	    case REL_FILTER:
	    case REL_ROOT:
	    case REL_SORT:
	    case REL_EXCHANGE:
	    case REL_MAP_VALUEIDS:
	    case REL_ORDERED_GROUPBY:
	    case REL_HASHED_GROUPBY:
	    case REL_INSERT_CURSOR:
	    case REL_DELETE_CURSOR:
	    case REL_UPDATE_CURSOR:
	    case REL_MATERIALIZE_SIMPLE:
	    case REL_MATERIALIZE_HASHED:
	    case REL_MATERIALIZE_ORDERED:

	    case REL_UNPACKROWS:
	    case REL_PACK:
	    case REL_TRANSPOSE:

	    case REL_SEQUENCE:
	    case REL_SAMPLE:
	    case REL_UNARY_TABLE_MAPPING_UDF:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_ROUTINE:
	  switch (op_)
	    {
	    case REL_TABLE_VALUED_FUNCTION:
	    case REL_BUILTIN_TABLE_VALUED_FUNCTION:
	    case REL_TABLE_VALUED_UDF:
	    case REL_ISOLATED_NON_TABLE_UDR:
	    case REL_ISOLATED_SCALAR_UDF:
	    case REL_EXPLAIN:
	    case REL_STATISTICS:
	    case REL_CALLSP:
	    case REL_LEAF_TABLE_MAPPING_UDF:
	    case REL_UNARY_TABLE_MAPPING_UDF:
	    case REL_BINARY_TABLE_MAPPING_UDF:
	    case REL_TABLE_MAPPING_BUILTIN_LOG_READER:
	    case REL_TABLE_MAPPING_BUILTIN_SERIES:
	    case REL_TABLE_MAPPING_BUILTIN_TIMESERIES:
	    case REL_TABLE_MAPPING_BUILTIN_JDBC:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_SCALAR_UDF_ROUTINE:
	case REL_FORCE_ANY_SCALAR_UDF:
	  switch (op_)
	    {
	    case REL_ISOLATED_NON_TABLE_UDR:
	    case REL_ISOLATED_SCALAR_UDF:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_GEN_UPDATE:
	  switch (op_)
	    {
	    case REL_UNARY_INSERT:
	    case REL_UNARY_UPDATE:
	    case REL_UNARY_DELETE:
	    case REL_LEAF_INSERT:
	    case REL_LEAF_UPDATE:
	    case REL_LEAF_DELETE:
	    case REL_INSERT_CURSOR:
	    case REL_DELETE_CURSOR:
	    case REL_UPDATE_CURSOR:
	    case REL_HIVE_INSERT:
	    case REL_HBASE_INSERT:
	    case REL_HBASE_DELETE:
	    case REL_HBASE_UPDATE:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_LEAF_GEN_UPDATE:
	  switch (op_)
	    {	    
	    case REL_LEAF_INSERT:
	    case REL_LEAF_UPDATE:
	    case REL_LEAF_DELETE:
	    case REL_HIVE_INSERT:
	    case REL_HBASE_INSERT:
	    case REL_HBASE_DELETE:
	    case REL_HBASE_UPDATE:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_UNARY_GEN_UPDATE:
	  switch (op_)
	    {
	    case REL_UNARY_INSERT:
	    case REL_UNARY_UPDATE:
	    case REL_UNARY_DELETE:
	    case REL_INSERT_CURSOR:
	    case REL_DELETE_CURSOR:
	    case REL_UPDATE_CURSOR:
	    case REL_HBASE_INSERT:
	    case REL_HBASE_UPDATE:
	    case REL_HBASE_DELETE:
	    case REL_CALLSP:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_UPDATE_DELETE:
	  switch (op_)
	    {
	    case REL_UNARY_UPDATE:
	    case REL_UNARY_DELETE:
	    case REL_LEAF_UPDATE:
	    case REL_LEAF_DELETE:
	    case REL_DELETE_CURSOR:
	    case REL_UPDATE_CURSOR:
	    case REL_HBASE_UPDATE:
	    case REL_HBASE_DELETE:
	      return TRUE;
	    default:
	      return FALSE;
	    }


	case REL_ANY_DELETE:
	  switch (op_)
	    {
	    case REL_UNARY_DELETE:
	    case REL_LEAF_DELETE:
	    case REL_DELETE_CURSOR:
	    case REL_HBASE_DELETE:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_HBASE:
	  {
	    switch (op_)
	      {
	      case REL_HBASE_INSERT:
	      case REL_HBASE_DELETE:
	      case REL_HBASE_UPDATE:
	      case REL_HBASE_ACCESS:
		return TRUE;
	      default:
		return FALSE;
	      }
	  }

	case REL_ANY_HBASE_GEN_UPDATE:
	  {
	    switch (op_)
	      {
	      case REL_HBASE_INSERT:
	      case REL_HBASE_DELETE:
	      case REL_HBASE_UPDATE:
		return TRUE;
	      default:
		return FALSE;
	      }
	  }

	case REL_ANY_GROUP:
	  switch (op_)
	    {
	    case REL_GROUPBY:
	    case REL_AGGREGATE:
	    case REL_ORDERED_GROUPBY:
	    case REL_HASHED_GROUPBY:
	    case REL_SHORTCUT_GROUPBY:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_FORCE_EXCHANGE:
	  switch (op_)
	    {
	    case REL_EXCHANGE:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_BINARY_OP:
	  switch (op_)
	    {
	    case REL_JOIN:
	    case REL_TSJ:
	    case REL_ROUTINE_JOIN:
	    case REL_INDEX_JOIN:
	    case REL_SEMIJOIN:
	    case REL_SEMITSJ:
	    case REL_ANTI_SEMIJOIN:
	    case REL_ANTI_SEMITSJ:
	    case REL_LEFT_JOIN:
	    case REL_LEFT_TSJ:
	    case REL_UNION:
	    case REL_NESTED_JOIN:
	    case REL_LEFT_NESTED_JOIN:
	    case REL_NESTED_SEMIJOIN:
	    case REL_NESTED_ANTI_SEMIJOIN:
	    case REL_MERGE_JOIN:
	    case REL_LEFT_MERGE_JOIN:
	    case REL_MERGE_SEMIJOIN:
	    case REL_MERGE_ANTI_SEMIJOIN:
	    case REL_HASH_JOIN:
	    case REL_LEFT_HASH_JOIN:
	    case REL_HASH_SEMIJOIN:
	    case REL_HASH_ANTI_SEMIJOIN:
	    case REL_ORDERED_HASH_JOIN:
	    case REL_LEFT_ORDERED_HASH_JOIN:
	    case REL_ORDERED_HASH_SEMIJOIN:
	    case REL_ORDERED_HASH_ANTI_SEMIJOIN:
	    case REL_HYBRID_HASH_JOIN:
	    case REL_LEFT_HYBRID_HASH_JOIN:
	    case REL_FULL_HYBRID_HASH_JOIN:
	    case REL_HYBRID_HASH_SEMIJOIN:
	    case REL_HYBRID_HASH_ANTI_SEMIJOIN:
	    case REL_MERGE_UNION:
            case REL_INTERSECT:
            case REL_EXCEPT:
	    case REL_BINARY_TABLE_MAPPING_UDF:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_JOIN:
	case REL_FORCE_JOIN:
	  switch (op_)
	    {
	    case REL_JOIN:
	    case REL_TSJ:
	    case REL_ROUTINE_JOIN:
	    case REL_INDEX_JOIN:
	    case REL_SEMIJOIN:
	    case REL_SEMITSJ:
	    case REL_ANTI_SEMIJOIN:
	    case REL_ANTI_SEMITSJ:
	    case REL_LEFT_JOIN:
	    case REL_LEFT_TSJ:
	    case REL_RIGHT_JOIN:
	    case REL_NESTED_JOIN:
	    case REL_LEFT_NESTED_JOIN:
	    case REL_NESTED_SEMIJOIN:
	    case REL_NESTED_ANTI_SEMIJOIN:
	    case REL_MERGE_JOIN:
	    case REL_LEFT_MERGE_JOIN:
	    case REL_MERGE_SEMIJOIN:
	    case REL_MERGE_ANTI_SEMIJOIN:
	    case REL_HASH_JOIN:
	    case REL_LEFT_HASH_JOIN:
	    case REL_HASH_SEMIJOIN:
	    case REL_HASH_ANTI_SEMIJOIN:
	    case REL_ORDERED_HASH_JOIN:
	    case REL_LEFT_ORDERED_HASH_JOIN:
	    case REL_ORDERED_HASH_SEMIJOIN:
	    case REL_ORDERED_HASH_ANTI_SEMIJOIN:
	    case REL_HYBRID_HASH_JOIN:
	    case REL_LEFT_HYBRID_HASH_JOIN:
	    case REL_FULL_HYBRID_HASH_JOIN:
	    case REL_HYBRID_HASH_SEMIJOIN:
	    case REL_HYBRID_HASH_ANTI_SEMIJOIN:
	    case REL_FULL_JOIN:
            case REL_INTERSECT:
            case REL_EXCEPT:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_INNER_JOIN:
	  switch (op_)
	    {
	    case REL_JOIN:
	    case REL_TSJ:
	    case REL_ROUTINE_JOIN:
	    case REL_INDEX_JOIN:
	    case REL_SEMIJOIN:
	    case REL_SEMITSJ:
	    case REL_NESTED_JOIN:
	    case REL_NESTED_SEMIJOIN:
	    case REL_MERGE_JOIN:
	    case REL_MERGE_SEMIJOIN:
	    case REL_HASH_JOIN:
	    case REL_HASH_SEMIJOIN:
	    case REL_ORDERED_HASH_JOIN:
	    case REL_ORDERED_HASH_SEMIJOIN:
	    case REL_HYBRID_HASH_JOIN:
	    case REL_HYBRID_HASH_SEMIJOIN:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_NON_TS_INNER_JOIN:
	  switch (op_)
	    {
	    case REL_JOIN:
	    case REL_INDEX_JOIN:
	    case REL_SEMIJOIN:
	    case REL_MERGE_JOIN:
	    case REL_MERGE_SEMIJOIN:
	    case REL_HASH_JOIN:
	    case REL_HASH_SEMIJOIN:
	    case REL_ORDERED_HASH_JOIN:
	    case REL_ORDERED_HASH_SEMIJOIN:
	    case REL_HYBRID_HASH_JOIN:
	    case REL_HYBRID_HASH_SEMIJOIN:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_NON_TSJ_JOIN:
	  switch (op_)
	    {
	    case REL_JOIN:
	    case REL_INDEX_JOIN:
	    case REL_SEMIJOIN:
	    case REL_ANTI_SEMIJOIN:
	    case REL_LEFT_JOIN:

	    case REL_MERGE_JOIN:
	    case REL_MERGE_SEMIJOIN:
	    case REL_LEFT_MERGE_JOIN:
	    case REL_MERGE_ANTI_SEMIJOIN:

	    case REL_HASH_JOIN:
	    case REL_LEFT_HASH_JOIN:
	    case REL_HASH_SEMIJOIN:
	    case REL_HASH_ANTI_SEMIJOIN:
	    case REL_ORDERED_HASH_JOIN:
	    case REL_LEFT_ORDERED_HASH_JOIN:
	    case REL_ORDERED_HASH_SEMIJOIN:
	    case REL_ORDERED_HASH_ANTI_SEMIJOIN:
	    case REL_HYBRID_HASH_JOIN:
	    case REL_LEFT_HYBRID_HASH_JOIN:
	    case REL_FULL_HYBRID_HASH_JOIN:
	    case REL_HYBRID_HASH_SEMIJOIN:
	    case REL_HYBRID_HASH_ANTI_SEMIJOIN:
	      return TRUE;
	    default:
	      return FALSE;
	    }


	case REL_ANY_TSJ:
	case REL_FORCE_NESTED_JOIN:
	  switch (op_)
	    {
	    //case REL_JOIN:  // Needed for fbcqs.
	    case REL_TSJ:
	    case REL_TSJ_FLOW:
	    case REL_ROUTINE_JOIN:
	    case REL_LEFT_TSJ:
	    case REL_SEMITSJ:
	    case REL_ANTI_SEMITSJ:
	    case REL_NESTED_JOIN:
	    case REL_NESTED_SEMIJOIN:
	    case REL_NESTED_ANTI_SEMIJOIN:
	    case REL_LEFT_NESTED_JOIN:
	    case REL_NESTED_JOIN_FLOW:
	      return TRUE;
	    default:
	      return FALSE;
	    }
	case REL_ANY_FULL_JOIN:
	  switch (op_)
	    {
	    case REL_FULL_JOIN:
	    case REL_FULL_HYBRID_HASH_JOIN:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_LEFT_JOIN:
	  switch (op_)
	    {
	    case REL_LEFT_JOIN:
	    case REL_LEFT_TSJ:
	    case REL_LEFT_NESTED_JOIN:
	    case REL_LEFT_MERGE_JOIN:
	    case REL_LEFT_HASH_JOIN:
	    case REL_LEFT_ORDERED_HASH_JOIN:
	    case REL_LEFT_HYBRID_HASH_JOIN:
	    case REL_FULL_HYBRID_HASH_JOIN:
	    case REL_FULL_JOIN:     // is left join as well
	      return TRUE;
	    default:
	      return FALSE;
	    }
	case REL_ANY_RIGHT_JOIN:
	  switch (op_)
	    {
	    case REL_RIGHT_JOIN:
	    case REL_FULL_JOIN:     // is right join as well
	    case REL_FULL_HYBRID_HASH_JOIN: 
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_SEMIJOIN:
	  switch (op_)
	    {
	    case REL_SEMIJOIN:
	    case REL_SEMITSJ:
	    case REL_NESTED_SEMIJOIN:
	    case REL_MERGE_SEMIJOIN:
	    case REL_HASH_SEMIJOIN:
	    case REL_HYBRID_HASH_SEMIJOIN:
	    case REL_ORDERED_HASH_SEMIJOIN:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_ANTI_SEMIJOIN:
	  switch (op_)
	    {
	    case REL_ANTI_SEMIJOIN:
	    case REL_ANTI_SEMITSJ:
	    case REL_NESTED_ANTI_SEMIJOIN:
	    case REL_MERGE_ANTI_SEMIJOIN:
	    case REL_HASH_ANTI_SEMIJOIN:
	    case REL_HYBRID_HASH_ANTI_SEMIJOIN:
	    case REL_ORDERED_HASH_ANTI_SEMIJOIN:
            case REL_EXCEPT:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_SEMITSJ:
	  switch (op_)
	    {
	    case REL_SEMITSJ:
	    case REL_NESTED_SEMIJOIN:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_ANTI_SEMITSJ:
	  switch (op_)
	    {
	    case REL_ANTI_SEMITSJ:
	    case REL_NESTED_ANTI_SEMIJOIN:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_NESTED_JOIN :
	  switch (op_)
	    {
	    case REL_NESTED_JOIN:
	    case REL_NESTED_SEMIJOIN:
	    case REL_NESTED_ANTI_SEMIJOIN:
	    case REL_LEFT_NESTED_JOIN:
	    case REL_NESTED_JOIN_FLOW:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case REL_ANY_HASH_JOIN:
	case REL_FORCE_HASH_JOIN:
	  switch (op_)
	    {
	    
	    case REL_HASH_JOIN:
	    case REL_LEFT_HASH_JOIN:	    
	    case REL_HASH_SEMIJOIN:
	    case REL_HASH_ANTI_SEMIJOIN:

	    case REL_HYBRID_HASH_JOIN:
	    case REL_LEFT_HYBRID_HASH_JOIN:
	    case REL_FULL_HYBRID_HASH_JOIN:
	    case REL_HYBRID_HASH_SEMIJOIN:
	    case REL_HYBRID_HASH_ANTI_SEMIJOIN:
	    
	    case REL_ORDERED_HASH_JOIN:
	    case REL_LEFT_ORDERED_HASH_JOIN:
	    case REL_ORDERED_HASH_SEMIJOIN:
	    case REL_ORDERED_HASH_ANTI_SEMIJOIN:

	      return TRUE;
	    default:
	      return FALSE;
	    }

	  case REL_FORCE_HYBRID_HASH_JOIN:
	  switch (op_)	
	    {
	    case REL_HASH_JOIN:
	    case REL_HYBRID_HASH_JOIN:
	    case REL_LEFT_HYBRID_HASH_JOIN:
	    case REL_FULL_HYBRID_HASH_JOIN:
	    case REL_HYBRID_HASH_SEMIJOIN:
	    case REL_HYBRID_HASH_ANTI_SEMIJOIN:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	  case REL_FORCE_ORDERED_CROSS_PRODUCT:
	  switch (op_)	
	    {
	    case REL_HASH_JOIN:
	    case REL_HYBRID_HASH_JOIN:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	  case REL_FORCE_ORDERED_HASH_JOIN:
	  switch (op_)
	    {
	    case REL_HASH_JOIN:
	    case REL_ORDERED_HASH_JOIN:
	    case REL_LEFT_ORDERED_HASH_JOIN:
	    case REL_ORDERED_HASH_SEMIJOIN:
	    case REL_ORDERED_HASH_ANTI_SEMIJOIN:
	      return TRUE;
	    default:
	      return FALSE;
	    }


	case REL_ANY_MERGE_JOIN:
	case REL_FORCE_MERGE_JOIN:
	  switch (op_)
	    {
	    case REL_MERGE_JOIN:
	    case REL_LEFT_MERGE_JOIN:
	    case REL_MERGE_SEMIJOIN:
	    case REL_MERGE_ANTI_SEMIJOIN:
	      return TRUE;
	    default:
	      return FALSE;
	    }

        case REL_ANY_TABLE_MAPPING_UDF:
	  switch (op_)
	    {
	    case REL_LEAF_TABLE_MAPPING_UDF:
	    case REL_UNARY_TABLE_MAPPING_UDF:
	    case REL_BINARY_TABLE_MAPPING_UDF:
	    case REL_TABLE_MAPPING_BUILTIN_LOG_READER:
	    case REL_TABLE_MAPPING_BUILTIN_SERIES:
	    case REL_TABLE_MAPPING_BUILTIN_TIMESERIES:
	    case REL_TABLE_MAPPING_BUILTIN_JDBC:
	      return TRUE;
	    default:
	      return FALSE;
	    }

        case REL_ANY_LEAF_TABLE_MAPPING_UDF:
	  switch (op_)
	    {
	    case REL_LEAF_TABLE_MAPPING_UDF:
	    case REL_TABLE_MAPPING_BUILTIN_LOG_READER:
            case REL_TABLE_MAPPING_BUILTIN_JDBC:
	      return TRUE;
	    default:
	      return FALSE;
	    }

        case REL_ANY_UNARY_TABLE_MAPPING_UDF:
	  switch (op_)
	    {
	    case REL_UNARY_TABLE_MAPPING_UDF:
            case REL_TABLE_MAPPING_BUILTIN_TIMESERIES:
	      return TRUE;
	    default:
	      return FALSE;
	    }

        case REL_ANY_BINARY_TABLE_MAPPING_UDF:
	  switch (op_)
	    {
	    case REL_BINARY_TABLE_MAPPING_UDF:
	      return TRUE;
	    default:
	      return FALSE;
	    }

        case REL_ANY_EXTRACT:
	  switch (op_)
	    {
	    case REL_FAST_EXTRACT:
	    case REL_HIVE_INSERT:
	      return TRUE;
	    default:
	      return FALSE;
	    }

	case ANY_REL_OR_ITM_OP:
	  ABORT("internal error in OperatorType::match()");

	default:
	  return FALSE; // operators don't match
	}
    }
}

NABoolean OperatorType::isWildcard() const
{
  switch (op_)
    {
    case ANY_REL_OR_ITM_OP:
    case ITM_ANY_AGGREGATE:
    case ITM_ANY_CAST:
    case ITM_WILDCARD_EQ_NE:
    case REL_ANY_BINARY_OP:
    case REL_ANY_GEN_UPDATE:
    case REL_ANY_GROUP:
    case REL_ANY_INNER_JOIN:
    case REL_ANY_JOIN:
    case REL_ANY_LEAF_GEN_UPDATE:
    case REL_ANY_LEAF_OP:
    case REL_ANY_LEFT_JOIN:
    case REL_ANY_NON_TS_INNER_JOIN:
    case REL_ANY_SEMIJOIN:
    case REL_ANY_SEMITSJ:
    case REL_ANY_ANTI_SEMITSJ:
    case REL_ANY_ANTI_SEMIJOIN:
    case REL_ANY_TSJ:
    case REL_ANY_UNARY_GEN_UPDATE:
    case REL_ANY_UNARY_OP:
    case REL_FORCE_ANY_SCAN:
    case REL_ANY_ROUTINE:
    case REL_ANY_SCALAR_UDF_ROUTINE:
    case REL_ANY_EXTRACT:
      return TRUE;

    default:
      return FALSE;
    }
}

// -----------------------------------------------------------------------
// methods for class ExprNode
// -----------------------------------------------------------------------


// default constructor (assuming the max arity in the system is 2)
ExprNode::ExprNode(OperatorTypeEnum otype )
        : operator_(otype), bound_(FALSE), transformed_(FALSE),
          normalized_(FALSE), preCodeGenned_(FALSE), normalized4Cache_(FALSE),
          cacheable_(MAYBECACHEABLE), semanticQueryOptimized_(FALSE)
{}

ExprNode::ExprNode(const ExprNode& s)
  : NABasicObject(s)
  , operator_(s.operator_)
  , bound_(s.bound_)
  , transformed_(s.transformed_)
  , normalized_(s.normalized_)
  , preCodeGenned_(s.preCodeGenned_)
  , normalized4Cache_(s.normalized4Cache_)
  , cacheable_(s.cacheable_)
  , semanticQueryOptimized_(s.semanticQueryOptimized_)
{}

ExprNode::~ExprNode() {}

ExprNode * ExprNode::getChild(Lng32 )
{
  ABORT("virtual function ExprNode::getChild() must be redefined");
  return NULL;
} // ExprNode::getChild()

const ExprNode * ExprNode::getConstChild(Lng32 ) const
{
  ABORT("virtual function const ExprNode::getConstChild() const must be redefined");
  return NULL;
} // ExprNode::getChild()

void ExprNode::setChild(Lng32 , ExprNode *)
{
  ABORT("virtual function ExprNode::setChild() must be redefined");
} // ExprNode::setChild()

ElemDDLNode * ExprNode::castToElemDDLNode()
{
  return NULL;
}

const ElemDDLNode * ExprNode::castToElemDDLNode() const
{
  return NULL;
}

RelExpr * ExprNode::castToRelExpr()
{
  return NULL;
}

const RelExpr * ExprNode::castToRelExpr() const
{
  return NULL;
}

ItemExpr * ExprNode::castToItemExpr()
{
  return NULL;
}

const ItemExpr * ExprNode::castToItemExpr() const
{
  return NULL;
}

StmtDDLNode * ExprNode::castToStmtDDLNode()
{
  return NULL;
}

const StmtDDLNode * ExprNode::castToStmtDDLNode() const
{
  return NULL;
}

StmtNode * ExprNode::castToStatementExpr()
{
  return NULL;
}
 
const StmtNode * ExprNode::castToStatementExpr() const
{
  return NULL;
}
 
Int32 ExprNode::getArity() const
{
  ABORT("virtual function ExprNode::getArity() must be redefined");
  return 0;
} // ExprNode::getArity()

const NAString ExprNode::getText() const
{
  ABORT("virtual function ExprNode::getText() must be redefined");
  return NAString("unknown??");
} // ExprNode::getText()

// the displayTree() method is defined in file DisplayTree.C
void ExprNode::print(FILE * f,
		     const char * prefix,
		     const char *) const
{
  // print operator text
  fprintf(f,"%sExprNode(%s)\n", prefix, (const char *) getText());
}

void ExprNode::unparse(NAString &,
		       PhaseEnum,
		       UnparseFormatEnum,
		       TableDesc * tabId) const
{
  ABORT("ExprNode::unparse() must be redefined");
}

void ExprNode::addLocalExpr(LIST(ExprNode *) &,
			    LIST(NAString) &) const
{
  // the default implementation is to do nothing
}

