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
#ifndef ENUM_H
#define ENUM_H

class catsim {

public:
  enum parsenode_type { 
  		   CREATE_CATALOG_TYPE, CREATE_TABLE_TYPE, ALTER_TABLE_TYPE,
		   DROP_CATALOG_TYPE, DROP_TABLE_TYPE, 
		   CREATE_INDEX_TYPE, DROP_INDEX_TYPE, UPDATE_STATS_TYPE,
		   CREATE_VIEW_TYPE, DROP_VIEW_TYPE,
		   TABLE_TYPE, UPDATE_HIST_TYPE,
		   PARTNS_DEF_TYPE,
		   COLDEF_TYPE, CONSTRNT_TYPE, UNIQUE_CONSTRNT_TYPE,
		   CHECK_CONSTRNT_TYPE, REF_CONSTRNT_TYPE,
		   UNION_TYPE,
		   SELECT_TYPE, UPDATE_TYPE, INSERT_TYPE, DELETE_TYPE,
		   LITERAL_TYPE, MISC_TYPE,
		   COMP_OP_TYPE, COLUMN_TYPE,
		   BOOL_OP_TYPE, ARITH_OP_TYPE,
		   FUNCTION_TYPE, CONV_OP_TYPE,
		   AGGREGATE_TYPE,
		   ALLOCATE_DESC_TYPE, DECLARE_CURSOR_TYPE,
		   MODULE_TYPE, PROCEDURE_TYPE,
		   OPTR_AGGR_TYPE, OPTR_OUTPUT_TYPE,
		   CLEANUP_TYPE
		 };
  enum aggregate_type 	  { SUM_, AVG_, COUNT_, MIN_, MAX_ };
  enum alter_type 	  { ADD_CONSTRAINT, DROP_CONSTRAINT, ADD_COLUMN, 
			    SET_CONSTRAINT };
  enum arith_oper_type 	  { ADD_, SUB_, MUL_, DIV_, EXP_ };
  enum bool_oper_type 	  { AND_, OR_, NOT_ };
  enum comp_oper_type 	  { EQ_, NE_, LT_, GT_, LE_, GE_ };
  enum unique_constr_type { UNIQUE_, PRIMARY_KEY_ };
  enum function_type 	  { UPSHIFT_, DOWNSHIFT_, CONCAT_, SUBSTRING_, TRIM_, 
			    CHAR_LENGTH_, OCTET_LENGTH_, POSITION_ };
  enum match_type 	  { MATCH_FULL, NONE };
  enum referential_action { CASCADE_, SET_NULL, SET_DEFAULT, NO_ACTION };
  enum yes_no_type 	  { YES_, NO_ };
};

#endif
