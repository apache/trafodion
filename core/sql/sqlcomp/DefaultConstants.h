//******************************************************************************
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
//**********************************************************************/
#ifndef DEFAULTCONSTANTS_H
#define DEFAULTCONSTANTS_H

#undef DEFAULT_CHARSET
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         DefaultConstants.h
 * Description:  Default attribute enum and keyword enum.
 *
 * Created:      7/11/1996
 * Language:     C++
 *
 *
 *
 ***************************************************************************** */

// This enum contains defaults used in Trafodion.
// To add a default, include it in this enum and in DefaultDefaults of
// sqlcomp/NADefaults.cpp.
//
// To change the value at run-time, do either:
// a) Insert the ATTRIBUTE name of the default along with its ATTR_VALUE
//    into the SQL DEFAULTS table.
//    This change take effect the next time you start a compiler session.
// b) Issue a CONTROL QUERY DEFAULT statement.
//    This change takes effect immediately.
//    (Note that some SET statements, e.g. SET CATALOG/SCHEMA,
//    also take effect immediately *if* you are in dynamic compilation mode,
//    e.g., if you are in SQLCI.)

// ***************************************************************************
// ***************************************************************************
// NOTE: DO NOT make this enum non-contiguous.
// Valid attributes must begin at zero and there must be no holes.
// The algorithm to read defaults from the defaults table requires this.
// You must always add a default default in NADefaults.cpp, otherwise
// the code will fail in an assertion.
// ***************************************************************************
// ***************************************************************************


enum DefaultConstants
{
  __INVALID_DEFAULT_ATTRIBUTE = -1,	// negative; not in DefaultDefaults
  ALLOW_INCOMPATIBLE_ASSIGNMENT = 0,
  ALLOW_INCOMPATIBLE_COMPARISON,
  ARKCMP_FAKE_HW,
  AUTOMATIC_RECOMPILATION,

  // -------------------------------------------------------------------------
  // defaults for big memory operators (HJ, HGB, SORT)
  // -------------------------------------------------------------------------
  BMO_MEMORY_USAGE_PERCENT,
  PHY_MEM_CONTINGENCY_MB,
  BMO_CITIZENSHIP_FACTOR,
  // -------------------------------------------------------------------------
  // Costs for common fundamental primitive operations.
  // -------------------------------------------------------------------------

  // Set this to TRUE to enable display cost_detail in explain
  // to support the calibration reports. Default is FALSE.

  EXPLAIN_DETAIL_COST_FOR_CALIBRATION,

  // Used in CostPrimitives::cpuCostForCompare().
  CPUCOST_COMPARE_COMPLEX_DATA_TYPE_OVERHEAD,
  CPUCOST_COMPARE_COMPLEX_DATA_TYPE_PER_BYTE,
  CPUCOST_COMPARE_SIMPLE_DATA_TYPE,

  // Used in both CostPrimitives::cpuCostForCopyRow() and cpuCostForCopySet().
  CPUCOST_COPY_ROW_OVERHEAD,
  CPUCOST_COPY_ROW_PER_BYTE,

  // Used in Exchange Costing
  CPUCOST_EXCHANGE_COST_PER_BYTE,
  CPUCOST_EXCHANGE_INTERNODE_COST_PER_BYTE,
  CPUCOST_EXCHANGE_REMOTENODE_COST_PER_BYTE,

  // Used in CostPrimitives::cpuCostForCopySet().
  CPUCOST_COPY_SIMPLE_DATA_TYPE,

  // Used in CostPrimitives::cpuCostForEvalArithExpr().
  CPUCOST_EVAL_ARITH_OP,

  // Used in CostPrimitives::cpuCostForEvalFunc().
  CPUCOST_EVAL_FUNC_DEFAULT,

  // Used in CostPrimitives::cpuCostForEvalPred().
  CPUCOST_EVAL_SIMPLE_PREDICATE,

  // Used in CostPrimitives::cpuCostForLikeCompare().
  CPUCOST_LIKE_COMPARE_OVERHEAD,
  CPUCOST_LIKE_COMPARE_PER_BYTE,

  // -------------------------------------------------------------------------
  // Used in CostPrimitives::cpuCostForHash().
  //
  // Hash(k1,k2) is done by Hash(k1) + Hash(k2) where the + may be overloaded.
  // Here, CPUCOST_HASH_PER_KEY records the cost for the +. If we have 5 keys
  // to hash, this cost (CPUCOST_HASH_PER_KEY * 5), include the file modulus
  // operation to give the final hash value. The cost of the Hash function
  // itself is assumed to be dependent on the byte count of each key. The per
  // byte cost for that is given as CPUCOST_HASH_PER_BYTE.
  //
  // Assume the overloaded + is half way between simple and complex arithmetic
  // operation. Hash() operates a simple arithmetic on each byte of the key.
  // -------------------------------------------------------------------------
  CPUCOST_HASH_PER_KEY,
  CPUCOST_HASH_PER_BYTE,

  // Used in CostPrimitives::cpuCostForEncode()
  CPUCOST_ENCODE_PER_BYTE,

  // -----------------------------------------------------------------------
  // The following 11 were (semi-blindly) converted from #define's so
  // that calibration would (hopefully) go easier --> i.e., change a
  // value in the Defaults table instead of recompiling.
  //
  // If any of these need additional commenting, the original author
  // (or just someone who understands their use) should do so.
  //
  // The file alongside each reference tells where the original #define
  // existed; although it's currently commented out, many have associated
  // comments (which I (Martin) can't decide if they're vital or not).
  // -----------------------------------------------------------------------
  CPUCOST_ESP_INITIALIZATION,           // Cost.h
  CPUCOST_LOCK_ROW,                     // Cost.h
  CPUCOST_PREDICATE_COMPARISON,         // Cost.h
  CPUCOST_SUBSET_OPEN,                  // Cost.h
  CPUCOST_SUBSET_OPEN_AFTER_FIRST,      // Cost.h
  DP2_MESSAGE_BUFFER_SIZE,              // Cost.h
  LOCAL_MESSAGE_BUFFER_SIZE,			// Cost.h
  OS_MESSAGE_BUFFER_SIZE,               // Cost.h

  // The CPU overhead paid by a data request to DP2:
  CPUCOST_DATARQST_OVHD,

  // There are four types of cache in DP2:
  DP2_CACHE_512_BLOCKS,
  DP2_CACHE_1024_BLOCKS,
  DP2_CACHE_2048_BLOCKS,
  DP2_CACHE_4096_BLOCKS,



  // Constants needed to estimate the cost of communication
  // between DP2 and exeInDp2:
  CPUCOST_SCAN_KEY_LENGTH,  // key encoding/decoding
  CPUCOST_SCAN_OVH_PER_KB, // catch all overhead per KB
  CPUCOST_SCAN_OVH_PER_ROW, // catch all overhead per row
  // Catch all for cost  of transferring data from disk to dp2:
  CPUCOST_SCAN_DSK_TO_DP2_PER_KB,
  CPUCOST_SCAN_DSK_TO_DP2_PER_SEEK,


  // Used in estimating blocks to read per scan access:
  COST_PROBE_DENSITY_THRESHOLD,

  // Cost factors used in IO:
  // The read ahead penalty is a fraction of the KB reads.
  // As the scan selectivity increases, reading the same
  // ammount of data costs more because the scan is busier
  // putting together buffers to send which makes it miss
  // prefetch cycles. Thus it reads more data. The best case
  // (i.e. the case when the scan reads less data) happens when
  // the scan is not selecting any rows. The worst case is when
  // the scan selects all the rows. the prefetch penalty determines
  // how much more data the scan needs to read (as a fraction of the
  // absolute data) because of prefetch misses. The absolute data
  // is the blocks that need to be read in a key sequenced table
  // in a particular single subset access.
  // This number must be between 0 and 1.
  IO_TRANSFER_COST_PREFETCH_MISSES_FRACTION,

  DEF_CHUNK_SIZE,
  DEF_CPU_ARCHITECTURE,
  DEF_DISCS_ON_CLUSTER,
  DEF_INSTRUCTIONS_SECOND,
  DEF_LOCAL_CLUSTER_NUMBER,
  DEF_LOCAL_SMP_NODE_NUMBER,
  DEF_MAX_HISTORY_ROWS,//will be obeseleted
  DEF_NUM_BM_CHUNKS,
  DEF_NUM_NODES_IN_ACTIVE_CLUSTERS,
  DEF_NUM_SMP_CPUS,
  DEF_PAGE_SIZE,
  DEF_PHYSICAL_MEMORY_AVAILABLE,
  DEF_TOTAL_MEMORY_AVAILABLE,
  DEF_VIRTUAL_MEMORY_AVAILABLE,

  // to generate detailed executor statitics
  DETAILED_STATISTICS,


  // Used in Exchange costing.
  DP2_END_OF_BUFFER_HEADER_SIZE,
  DP2_EXCHANGE_REQUEST_SIZE,
  DP2_MAX_READ_PER_ACCESS_IN_KB,
  DP2_MESSAGE_HEADER_SIZE,
  DP2_MESSAGE_HEADER_SIZE_BYTES,
  DP2_MINIMUM_FILE_SIZE_FOR_SEEK_IN_BLOCKS,

  MAX_DEPTH_TO_CHECK_FOR_CYCLIC_PLAN,

  MAX_LONG_VARCHAR_DEFAULT_SIZE,
  MIN_LONG_VARCHAR_DEFAULT_SIZE,
  MAX_LONG_WVARCHAR_DEFAULT_SIZE,
  MIN_LONG_WVARCHAR_DEFAULT_SIZE,

  // --------------------------------------------------------------------------
  // Basic Executor Operations.
  // --------------------------------------------------------------------------
  EX_OP_ALLOCATE_BUFFER_POOL,
  EX_OP_ALLOCATE_BUFFER,
  EX_OP_ALLOCATE_TUPLE,
  EX_OP_COPY_ATP,


  FIND_COMMON_SUBEXPRS_IN_OR,

  FLOAT_ESP_RANDOM_NUM_SEED,

  // Special handling for forcing Bushy CQS, for shapes generated by the wizard
  FORCE_BUSHY_CQS,

  // CQD to enable or disable explain
  GENERATE_EXPLAIN,

  // --------------------------------------------------------------------------
  // Operator defaults used by the Generator:
  // buffer size, number of buffers, and queue sizes (down, up).
  // --------------------------------------------------------------------------
  // DDL.
  GEN_DDL_BUFFER_SIZE,
  GEN_DDL_NUM_BUFFERS,
  GEN_DDL_SIZE_DOWN,
  GEN_DDL_SIZE_UP,
  // Describe.
  GEN_DESC_BUFFER_SIZE,
  GEN_DESC_NUM_BUFFERS,
  GEN_DESC_SIZE_DOWN,
  GEN_DESC_SIZE_UP,
  // DP2 Insert.
  GEN_DP2I_BUFFER_SIZE,
  GEN_DP2I_NUM_BUFFERS,
  GEN_DP2I_SIZE_DOWN,
  GEN_DP2I_SIZE_UP,
  // DP2 Subset Operator.
  GEN_DPSO_BUFFER_SIZE,
  GEN_DPSO_SIZE_DOWN,
  GEN_DPSO_SIZE_UP,
  // DP2 Unique Operator.
  GEN_DPUO_NUM_BUFFERS,
  // Hash GroupBy.
  GEN_HGBY_BUFFER_SIZE,
  GEN_HGBY_NUM_BUFFERS,
  GEN_HGBY_SIZE_DOWN,
  GEN_HGBY_SIZE_UP,
  // Hash Join.
  GEN_HSHJ_BUFFER_SIZE,
  GEN_HSHJ_NUM_BUFFERS,
  GEN_HSHJ_SIZE_DOWN,
  GEN_HSHJ_SIZE_UP,
  // Memory pressure Threshold
  GEN_MEM_PRESSURE_THRESHOLD,
  // Merge Join.
  GEN_MJ_BUFFER_SIZE,
  GEN_MJ_NUM_BUFFERS,
  GEN_MJ_SIZE_DOWN,
  GEN_MJ_SIZE_UP,
  // Ordered NLJ.
  GEN_ONLJ_BUFFER_SIZE,
  GEN_ONLJ_NUM_BUFFERS,
  GEN_ONLJ_SIZE_DOWN,
  GEN_ONLJ_SIZE_UP,
  // Partition Access.
  GEN_PA_BUFFER_SIZE,
  GEN_PA_NUM_BUFFERS,
   // Sample node
  GEN_SAMPLE_SIZE_DOWN,
  GEN_SAMPLE_SIZE_UP,
  // Scan.
  // Sequence Functions
  GEN_SEQFUNC_BUFFER_SIZE,
  GEN_SEQFUNC_NUM_BUFFERS,
  GEN_SEQFUNC_SIZE_DOWN,
  GEN_SEQFUNC_SIZE_UP,
  // Sort GroupBy.
  GEN_SGBY_BUFFER_SIZE,
  GEN_SGBY_NUM_BUFFERS,
  GEN_SGBY_SIZE_DOWN,
  GEN_SGBY_SIZE_UP,
  // Send Bottom.
  GEN_SNDB_NUM_BUFFERS,
  GEN_SNDB_SIZE_DOWN,
  GEN_SNDB_SIZE_UP,
  // Send Top.
  GEN_SNDT_BUFFER_SIZE_DOWN,
  GEN_SNDT_BUFFER_SIZE_UP,
  GEN_SNDT_NUM_BUFFERS,
  GEN_SNDT_RESTRICT_SEND_BUFFERS,
  GEN_SNDT_SIZE_DOWN,
  GEN_SNDT_SIZE_UP,
  // Sort.
  GEN_SORT_MAX_BUFFER_SIZE,
  GEN_SORT_NUM_BUFFERS,
  GEN_SORT_SIZE_DOWN,
  GEN_SORT_SIZE_UP,
  // Split Bottom.
  // Split Top.
  GEN_SPLT_SIZE_UP,
  // Tuple Flow.
  GEN_TFLO_BUFFER_SIZE,
  GEN_TFLO_NUM_BUFFERS,
  GEN_TFLO_SIZE_DOWN,
  GEN_TFLO_SIZE_UP,

   // Set Table Timeout.
  GEN_TIMEOUT_BUFFER_SIZE,
  GEN_TIMEOUT_NUM_BUFFERS,
  GEN_TIMEOUT_SIZE_DOWN,
  GEN_TIMEOUT_SIZE_UP,


  // Transaction.
  GEN_TRAN_BUFFER_SIZE,
  GEN_TRAN_NUM_BUFFERS,
  GEN_TRAN_SIZE_DOWN,
  GEN_TRAN_SIZE_UP,
  // Transpose.
  GEN_TRSP_BUFFER_SIZE,
  GEN_TRSP_NUM_BUFFERS,
  GEN_TRSP_SIZE_DOWN,
  GEN_TRSP_SIZE_UP,
  // Tuple.
  GEN_TUPL_BUFFER_SIZE,
  GEN_TUPL_NUM_BUFFERS,
  GEN_TUPL_SIZE_DOWN,
  GEN_TUPL_SIZE_UP,
  // Union.
  GEN_UN_BUFFER_SIZE,
  GEN_UN_NUM_BUFFERS,
  GEN_UN_SIZE_DOWN,
  GEN_UN_SIZE_UP,
  // Compound Stmt.
  GEN_CS_BUFFER_SIZE,
  GEN_CS_NUM_BUFFERS,
  GEN_CS_SIZE_DOWN,
  GEN_CS_SIZE_UP,
  // User-Defined Routines
  GEN_UDR_BUFFER_SIZE,
  GEN_UDR_NUM_BUFFERS,
  GEN_UDR_SIZE_DOWN,
  GEN_UDR_SIZE_UP,

  // -------------------------------------------------------------------------
  // Parameters used by the executor when dynamically resizing queues
  // (are stored in each TDB that is generated)
  // -------------------------------------------------------------------------
  DYN_QUEUE_RESIZE_FACTOR,
  DYN_QUEUE_RESIZE_INIT_DOWN,
  DYN_QUEUE_RESIZE_INIT_UP,
  DYN_QUEUE_RESIZE_LIMIT,
  DYN_QUEUE_RESIZE_OVERRIDE,
  // -------------------------------------------------------------------------
  // Enable 'ON' or disable 'OFF' considering hash joins of any form
  // -------------------------------------------------------------------------
  HASH_JOINS,

  // -------------------------------------------------------------------------
  // For HashGroupBy.
  // -------------------------------------------------------------------------
  HGB_BITMUX,
  HGB_CPUCOST_INITIALIZE,
  HGB_DP2_MEMORY_LIMIT,        // Memory limit if operator executes in DP2.
  HGB_GROUPING_FACTOR_FOR_SPILLED_CLUSTERS,

  // -------------------------------------------------------------------------
  // Histogram fudge factors
  //
  // Since these values are all pretty arbitrary (having to do with
  // default statistics -- i.e., the case when no statistics exist, or the
  // predicate, for whatever reason, cannot be reasonably evaluated by
  // histograms), we might as well let the user fiddle with them.
  //
  // The default selectivity values are used in
  //
  //   /optimizer/ColStatDesc.cpp
  //   /optimizer/ItemExpr.cpp
  //   /optimizer/OptItemExpr.cpp
  //
  // The "no stats" rowcount & uec values are used in
  //
  //   /ustat/hs_read.cpp
  //
  // These are the values we use if no stats information exists for a
  // particular table.
  //
  // NO_STATS_ROWCOUNT_MINIMUM is the absolutely fewest number of rows
  // we'll put into a "fake histogram" -- i.e., in the case where our file
  // label says the histogram has only 5 rows, we say fooey, for the
  // purposes of generating a reasonable plan, there are at least
  // ROWCOUNT_MINIMUM rows.
  //
  // ROWCOUNT_REQUIRING_STATS determines whether we fire off WARNING
  // 6008 or not -- so if the user really doesn't want to update stats, he
  // can set this to an extremely large value in order to never get a
  // "6008".  (6008 is a friendly reminder to the user that better plans
  // come from tables whose statistics have been updated.)
  //
  // HIST_SAME_TABLE_PRED_REDUCTION is a kludge to account for cases where
  // we apply multiple predicates to the same table, and multiplying the
  // resulting reductions together results in too large of an overall
  // reduction.  This reduction is applied to every reduction after the
  // first for a table that has 2+ predicates applied to it.  This value
  // should be between 1 and 2 in general; initial value is 1.4 .
  //
  // HIST_MAX_NUMBER_OF_INTERVALS is a way to limit the number of
  // histogram intervals that are synthesized & maintained in the
  // histogram code.  This upper bound is used to reduce compile-time for
  // queries involving large numbers of joins, where the user has updated
  // statistics to have more intervals than s/he actually needs.  In
  // general, this number should be set high (100 or so); in practice, we may
  // want to play with this value to see the trade-offs between compile time &
  // estimation accuracy.
  // -------------------------------------------------------------------------
  HIST_DEFAULT_SEL_FOR_BOOLEAN,          // col_a is true | false
  HIST_DEFAULT_SEL_FOR_IS_NULL,          // col_a is null | unknown
  HIST_DEFAULT_SEL_FOR_JOIN_EQUAL,       // col_a = col_b
  HIST_DEFAULT_SEL_FOR_JOIN_RANGE,       // col_a < col_b
  HIST_DEFAULT_SEL_FOR_LIKE_NO_WILDCARD, // col_a like "blah"
  HIST_DEFAULT_SEL_FOR_LIKE_WILDCARD,    // col_a like "%blah"
  HIST_DEFAULT_SEL_FOR_PRED_EQUAL,       // col_a = 5
  HIST_DEFAULT_SEL_FOR_PRED_RANGE,       // col_a < 5

  HIST_MAX_NUMBER_OF_INTERVALS,          // 1 ... 1000 (1000 by default: i.e., no limit)
  HIST_MC_STATS_NEEDED,                  //T: use MC histograms and issue 6007 message

  HIST_NO_STATS_ROWCOUNT,
  HIST_NO_STATS_UEC,
  HIST_NO_STATS_UEC_CHAR1,
  // This default is used to define the sample size for the quick stats
  HIST_ON_DEMAND_STATS_SIZE,
  HIST_REMOVE_TRAILING_BLANKS,

  HIST_ROWCOUNT_REQUIRING_STATS,


  //jbbsubset that specifies the root of the tree
  //for which we want to check the histograms
  HIST_ROOT_NODE,

  // -------------------------------------------------------------------------
  // For Histogram Intervals reduction and histogram prefetching
  // -------------------------------------------------------------------------
  // HIST_BASE_REDUCTION controls if reduction of the number of hist ints is
  // ON / OFF for histograms read from disk using ustat method FetchStatistics.
  // Valid values are 'ON' / 'OFF'.
  // HIST_INTERMEDIATE_REDUCTION controls if reduction of the number of hist ints
  // is ON / OFF for histograms generated after relational operations like join etc.
  // Valid values are 'ON' / 'OFF'.
  // HIST_BASE_REDUCTION_FUDGE_FACTOR controls the fudge factor used in the comparison
  // when deciding to merge two adjacent hist ints for reducing the number of hist ints
  // in histograms read from disk using ustat method FetchStatistics.
  // Valid values are positive ratios like 0.12 representing 12%
  // HIST_INTERMEDIATE_REDUCTION_FUDGE_FACTOR controls the fudge factor used in the comparison
  // when deciding to merge two adjacent hist ints for reducing the number of hist ints
  // in histograms generated after relational operations like join etc.
  // Valid values are positive ratios like 0.12 representing 12%
  // HIST_CONSTANT_ALPHA is used in the comparison of two adjacent intervals.
  // Valid values are numbers between 0 - 1.
  // HIST_PREFETCH controls if histograms are prefetched by ustat method FetchStatistics.
  // Valid values are 'ON' / 'OFF'
  DYNAMIC_HISTOGRAM_COMPRESSION,
  HIST_INTERMEDIATE_REDUCTION,
  HIST_BASE_REDUCTION_FUDGE_FACTOR,
  HIST_INTERMEDIATE_REDUCTION_FUDGE_FACTOR,
  HIST_CONSTANT_ALPHA,
  HIST_PREFETCH,


  // --------------------------------------------------------------------------
  // For Update Statistics.
  // --------------------------------------------------------------------------
  USTAT_ADD_SALTED_KEY_PREFIXES_FOR_MC,  // Generate MCs for primary key prefixes as well as full key of
                                         //   salted table when ON EVERY KEY or ON EVERY COLUMN is specified.
  USTAT_ATTEMPT_ESP_PARALLELISM,  // use parallel plans for reading columns to form histograms
  USTAT_AUTOMATIC_LOGGING,     // If ON, gives same semantics as UPDATE STATISTICS LOG SYSTEM
  USTAT_COMPACT_VARCHARS,      // For internal sort, store only the actual # chars used in each value
  USTAT_DELETE_NO_ROLLBACK,    // If ON, use DELETE WITH NO ROLLBACK in incremental stats when updating sample table
  USTAT_ESTIMATE_HBASE_ROW_COUNT,  // If ON, estimate row count of HBase table instead of count(*), subject
                                   //     to USTAT_MIN_ESTIMATE_FOR_ROWCOUNT setting)
  USTAT_FORCE_TEMP,            // Force temporary table to be used
  USTAT_HBASE_SAMPLE_RETURN_INTERVAL, // When sampling in HBase, adjust sampling rate to return once
                                      //   on average once per this many rows
  USTAT_JIT_LOGGING,           // Use just-in-time logging when error occurs
                               //   avoid deadlock for concurrent Update Stats on same table
  USTAT_LOG,                   // Generate USTAT log and write to file specified
  USTAT_MIN_ESTIMATE_FOR_ROWCOUNT, // Minimum estimated rowcount to be accepted
                                   // as actual rowcount
  USTAT_MIN_ROWCOUNT_FOR_SAMPLE,     // Smallest table for which sampling used
  USTAT_MIN_ROWCOUNT_FOR_LOW_SAMPLE, // Smallest table for which lowest sampling rate used
  USTAT_MODIFY_DEFAULT_UEC,    // Modify the default UEC
  USTAT_MULTI_COLUMN_LIMIT,    // Limit on number of columns in a multi-column histogram
  USTAT_UEC_HI_RATIO,          // When the estimated UEC/ROWCOUNT ratio is
  USTAT_UEC_LOW_RATIO,         // between HI_RATIO and LOW_RATIO, we will avoid
                               // calling xValue() to find the root value.
  USTAT_USE_BACKING_SAMPLE,    // Use Hive persistent sample table instead of reading from full table. @ZXtemp
  USTAT_USE_SLIDING_SAMPLE_RATIO,  // Vary sampling rate according to table size
  USTAT_AUTOMATION_INTERVAL,   // set the ustat automation interval
  USTAT_MAX_READ_AGE_IN_MIN,   // criterion for 'recently read histogram'
  USTAT_COLLECT_FILE_STATS,       // do we collect file stats
  USTAT_USE_IS_WHEN_NO_STATS,  // use IS when no histograms exist for the column
  USTAT_CQDS_ALLOWED_FOR_SPAWNED_COMPILERS, // list of CQDs that can be pushed to seconday compilers
                                            // CQDs are delimited by ","
  USTAT_MIN_CHAR_UEC_FOR_IS,     // minimum UEC for char type to use internal sort
  USTAT_MIN_DEC_BIN_UEC_FOR_IS,  // minimum UEC for binary types to use internal sort
  USTAT_YOULL_LIKELY_BE_SORRY, // minimum row count where explicit NO SAMPLE clause is required


  // -------------------------------------------------------------------------
  // For Hybrid Hash Operators (shared by HashJoin and HashGroupBy).
  // -------------------------------------------------------------------------
  HH_OP_ALLOCATE_HASH_TABLE,
  HH_OP_PROBE_HASH_TABLE,      // ref head of hash chain given a hash value
  HH_OP_INSERT_ROW_TO_CHAIN,   // insert a row in a hash chain
  HH_OP_HASHED_ROW_OVERHEAD,   // no of bytes to store hash value and pointer

  // -------------------------------------------------------------------------
  // For HashJoin.
  // -------------------------------------------------------------------------
  HJ_CPUCOST_INITIALIZE,
  HJ_INITIAL_BUCKETS_PER_CLUSTER,
  HJ_SCAN_TO_NJ_PROBE_SPEED_RATIO,
  HJ_TYPE,

  // -------------------------------------------------------------------------
  // If on, indicates that conversion from/to sql/application-hostvars are to
  // be done using the CAST rules, even if the datatypes are incompatible.
  // For example, conversion from DATE column to CHAR user hostvar is not
  // allowed according to ANSI as these are incompatible datatypes even
  // though this conversion could be done using cast function.
  // This default will enable all conversions that are allowed if a CAST
  // function is used.
  // -------------------------------------------------------------------------
  IMPLICIT_HOSTVAR_CONVERSION,

  //--------------------------------------------------------------------------
  // Index elimination level allows us to control the heuristics that will be
  // applied to reduce improbable indexes early. Default can take three values:
  // MINIMUM - No or little heuristic is applied
  // MEDIUM - Probably useless indexes are eliminated and no histogram statistics
  //									required
  // MAXIMUM - Histogram statistics required and aggressive approach to reducing
  // indexes. Indexes tagged for single subset access will not be used with MDAM
  //--------------------------------------------------------------------------
  INDEX_ELIMINATION_LEVEL,

  // -------------------------------------------------------------------------
  // Enable 'ON' or disable 'OFF' the heuristic to limit # of merge joins
  // -------------------------------------------------------------------------
  MERGE_JOIN_CONTROL,
  // -------------------------------------------------------------------------
  // Enable 'ON' or disable 'OFF' considering merge joins of any form
  // -------------------------------------------------------------------------
  MERGE_JOINS,

  //--------------------------------------------------------------------------
  // Enable 'ON' or disable 'OFF' for min-max optimization
  //--------------------------------------------------------------------------
  MIN_MAX_OPTIMIZATION,

  //--------------------------------------------------------------------------
  // Minimum ESP parallelism. If the user does not specifies this value then
  // the number of segments will be used as CQD value. If user sets this value
  // it should be integer >=1 AND <= totalNumberOfCPUs
  //--------------------------------------------------------------------------
  MINIMUM_ESP_PARALLELISM,

  // -------------------------------------------------------------------------
  // For MergeJoin.
  // -------------------------------------------------------------------------
  MJ_CPUCOST_INITIALIZE,
  MJ_CPUCOST_INSERT_ROW_TO_LIST,
  MJ_CPUCOST_REWIND_LIST,
  MJ_CPUCOST_GET_NEXT_ROW_FROM_LIST,
  MJ_CPUCOST_CLEAR_LIST,
  MJ_LIST_NODE_SIZE,

  // -------------------------------------------------------------------------
  // Factors used in converting resource usage to Elapsed Time (ET)
  // (Resource-to-time multipliers are environment dependant)
  // -------------------------------------------------------------------------
  MSCF_ET_CPU,
  MSCF_ET_NUM_IO_SEEKS,        // count of Random I/Os
  MSCF_ET_IO_TRANSFER,         // KB transfer
  MSCF_ET_NUM_LOCAL_MSGS,
  MSCF_ET_LOCAL_MSG_TRANSFER,  // KB transfer
  MSCF_ET_NUM_REMOTE_MSGS,
  MSCF_ET_REMOTE_MSG_TRANSFER, // KB transfer

  // turn on and off olt query optimizations. Default is ON.
  OLT_QUERY_OPT,

  // -----------------------------------------------------------------------
  // Factors used for recalibrating the resource-to-time multipliers
  // -----------------------------------------------------------------------
  // Reference calibration machine environment defaults
  REFERENCE_CPU_FREQUENCY, // In MHZ
  REFERENCE_IO_SEQ_READ_RATE, // In KB/Second
  REFERENCE_IO_SEEK_TIME, // In milliseconds
  REFERENCE_MSG_LOCAL_RATE, // In Mb/Second
  REFERENCE_MSG_LOCAL_TIME, // In seconds
  REFERENCE_MSG_REMOTE_RATE, // In Mb/Second
  REFERENCE_MSG_REMOTE_TIME, // In seconds
  REFERENCE_CODE, // ['DEBUG' | 'RELEASE']

  // End user machine environment defaults
  TARGET_CPU_FREQUENCY, // In MHZ
  TARGET_IO_SEQ_READ_RATE, // In KB/Second
  TARGET_IO_SEEK_TIME, // In milliseconds
  TARGET_MSG_LOCAL_RATE, // In Mb/Second
  TARGET_MSG_LOCAL_TIME, // In seconds
  TARGET_MSG_REMOTE_RATE, // In Mb/Second
  TARGET_MSG_REMOTE_TIME, // In seconds
  TARGET_CODE, // ['DEBUG' | 'RELEASE']

  // Default to account for speed up (or slowdown) of runnin in release
  // or default
  MSCF_DEBUG_TO_RELEASE_MULTIPLIER,

  // -------------------------------------------------------------------------
  // Factors used for estimating overlappability of I/O and messaging used
  // in the calculation for overlapped addition
  // -------------------------------------------------------------------------
  MSCF_OV_IO,
  MSCF_OV_MSG,


  // -----------------------------------------------------------------------
  // Big memory usage
  // -----------------------------------------------------------------------
  MEMORY_UNITS_SIZE,
  NUMBER_OF_USERS,

  // -------------------------------------------------------------------------
  // For MergeUnion.
  // -------------------------------------------------------------------------
  MU_CPUCOST_INITIALIZE,          // Per probe initialization cost.
  MU_INITIAL_BUFFER_COUNT,        // Initial no of buffers allocated.
  MU_INITIAL_BUFFER_SIZE,         // Size of executor buffer it is using.

  // -------------------------------------------------------------------------
  //++ MV OZ
  // For Materialized Views:
  MV_AGE,			  // timestamp default is 0
  MV_ALLOW_SELECT_SYSTEM_ADDED_COLUMNS, // ON/OFF default is OFF
  MV_AS_ROW_TRIGGER,		  // YES/NO default is NO
  MV_ENABLE_INTERNAL_REFRESH_SHOWPLAN, // internal only - enable showplan for IR
  MV_LOG_PUSH_DOWN_DP2_INSERT, // push down mv logging tp dp2 for insert
  MV_LOG_PUSH_DOWN_DP2_DELETE, // push down mv logging tp dp2 for delete
  MV_LOG_PUSH_DOWN_DP2_UPDATE, // push down mv logging tp dp2 for update


  MV_REFRESH_MAX_PARALLELISM,	  // unsigned integer - initially set to 1.
  MV_REFRESH_MAX_PIPELINING,	  // unsigned integer - initially set to 1.
  MV_TRACE_INCONSISTENCY,	  // YES/NO default is NO

  // For MV Query Rewrite:
  // -------------------------------------------------------------------------
  MVQR_ALL_JBBS_IN_QD,            // add JBBs to query descriptor even if criteria
                                  //   not met. Used for testing descriptors.
  MVQR_ENABLE_LOGGING,            // Use log files. Temporary until Log4CXX.
  MVQR_LOG_QUERY_DESCRIPTORS,     // whether or not to log query descriptors
  MVQR_PARAMETERIZE_EQ_PRED,      // whether or not to parameterize equality 
  MVQR_PRIVATE_QMS_INIT,          // how to init private qms: SMD, XML, NONE (default)
  MVQR_PUBLISH_TO,                // PRIVATE(default) qms, PUBLIC qms, BOTH
  MVQR_REWRITE_CANDIDATES,        // list of MVs by MV qualified names
  MVQR_REWRITE_ENABLED_OPTION,    // establishes default for MVs that don't state
                                  //   whether or not they are enabled for rewrite
  MVQR_REWRITE_LEVEL,             // 0 to 4, default 0 (disabled)
  MVQR_REWRITE_SINGLE_TABLE_QUERIES, // whether or not to rewrite queries on single tables
  MVQR_USE_EXTRA_HUB_TABLES,      // Use extra-hub tables in descriptors.
  MVQR_USE_RI_FOR_EXTRA_HUB_TABLES, // Use RI constraints for extra-hub tables in descriptors.
  MVQR_MAX_EXPR_SIZE,             // maximum size of an expr for which mvqr can produce a descriptor
  MVQR_MAX_EXPR_DEPTH,            // maximum depth of an expr for which mvqr can produce a descriptor
  MVQR_WORKLOAD_ANALYSIS_MV_NAME, // Name of MV descriptor to be generated from next query.

  // -------------------------------------------------------------------------
  // Enable 'ON' or disable 'OFF' considering nested joins
  // except when they are the only choice.
  // -------------------------------------------------------------------------
  NESTED_JOINS,
  KEYLESS_NESTED_JOINS,
  NESTED_JOINS_PLAN0, // turn off PLAN0 of nested joins


  // -------------------------------------------------------------------------
  // For NestedJoin.
  // -------------------------------------------------------------------------
  NJ_CPUCOST_INITIALIZE,          // Per probe initialization cost.
  NJ_CPUCOST_PASS_ROW,          // Cost for passing a row to right child
  NJ_INITIAL_BUFFER_COUNT,        // Initial no of buffers allocated.
  NJ_INITIAL_BUFFER_SIZE,         // Size of executor buffer it is using.


  // Calibration Constants for NESTEDJOIN INORDER created 11/21/00

  NJ_INC_UPTOLIMIT,	  // cost for seeks upto the blocks in innertable
  NJ_INC_AFTERLIMIT,	  // cost for seeks once the probes > innerblocks
  NJ_INC_MOVEROWS,	  // cost for moving the final result rows
  NJ_MAX_SEEK_DISTANCE,	  // Average distance for a seek

  // ----------------------------------------------------------------------
  // If enabled (ON), indicates that query processing is to be done
  // following ODBC rules. This affects (currently) type synthesis of
  // params and some unicode support.
  // ----------------------------------------------------------------------
  ODBC_PROCESS,

  // -------------------------------------------------------------------------
  // Tells what the optimization goal is (for instance, first row, last row).
  // -------------------------------------------------------------------------
  OPTIMIZATION_GOAL,

  // -----------------------------------------------------------------------
  // monitor the compile time using time.h system calls and print the result
  // summary
  // -----------------------------------------------------------------------
  COMPILE_TIME_MONITOR,
  COMPILE_TIME_MONITOR_OUTPUT_FILE,

  //------------------------------------------------------------------------
  // If CACHE_HISTOGRAMS is 'ON', we cache histograms that have been read by a
  // previous statement compilation and use the histogram for a compilation
  // on the same table
  //------------------------------------------------------------------------
  CACHE_HISTOGRAMS,
  CACHE_HISTOGRAMS_CHECK_FOR_LEAKS,
  CACHE_HISTOGRAMS_IN_KB,
  CACHE_HISTOGRAMS_MONITOR_HIST_DETAIL,
  CACHE_HISTOGRAMS_MONITOR_MEM_DETAIL,
  CACHE_HISTOGRAMS_MONITOR_OUTPUT_FILE,
  CACHE_HISTOGRAMS_TRACE_OUTPUT_FILE,

  //------------------------------------------------------------------------
  //NATable Cache Size in MBs
  //------------------------------------------------------------------------
  METADATA_CACHE_SIZE,

  //NARoutine and NARoutineDesc cache size in MBs
  ROUTINE_CACHE_SIZE,

  // -----------------------------------------------------------------------
  // Specifies if simple vector cost has to be reused to compute cost objects
  // -----------------------------------------------------------------------
  REUSE_BASIC_COST,

  // -----------------------------------------------------------------------
  // These are switches and variables to use for compiler debugging
  // -----------------------------------------------------------------------
  COMP_BOOL_1,
  COMP_BOOL_2,
  COMP_BOOL_3,
  COMP_BOOL_4,
  COMP_BOOL_5,
  COMP_BOOL_7,
  COMP_BOOL_8,
  COMP_BOOL_9,

  COMP_INT_2,
  COMP_INT_3,
  COMP_INT_4,
  COMP_INT_5,

  COMP_FLOAT_1,
  COMP_FLOAT_2,
  COMP_FLOAT_3,

  COMP_STRING_1,
  COMP_STRING_2,
  //------------------------------------------------------------------------
  // Do the query analysis phase, building JBBs, TCG, QGs, and ASM.
  //------------------------------------------------------------------------
  QUERY_ANALYSIS,

  //------------------------------------------------------------------------
  // Dump the data flow graph into a file and invokes DOTTY to view graph.
  //------------------------------------------------------------------------
  DISPLAY_DATA_FLOW_GRAPH,

  //------------------------------------------------------------------------
  // Specifies the threshold at MultiJoin Rewrite step occur.
  //------------------------------------------------------------------------
  MULTI_JOIN_THRESHOLD,

  //------------------------------------------------------------------------
  // Force the user specified join order to enumerate among orders enumrated
  //------------------------------------------------------------------------
  MULTI_JOIN_CONSIDER_INITIAL_JOIN_ORDER,

  MULTI_JOIN_PROBE_HASH_TABLE,

  //------------------------------------------------------------------------
  // Specifies if ASM cache should be available.
  //------------------------------------------------------------------------
  ASM_ALLOWED,

  //------------------------------------------------------------------------
  // Specifies if pre-computation of statistics should be done in ASM
  //------------------------------------------------------------------------
  ASM_PRECOMPUTE,

  // -------------------------------------------------------------------------
  // Specifies the level of optimization desired.
  // 0 = minimum level, 1 = intermediate level, 2 = maximum level (default)
  // constant1 and constant2 are used by optimization level 1.
  // -------------------------------------------------------------------------
  OPTIMIZATION_LEVEL,
  OPTIMIZATION_LEVEL_1_CONSTANT_1,
  OPTIMIZATION_LEVEL_1_CONSTANT_2,
  OPTIMIZATION_LEVEL_1_THRESHOLD,
  OPTIMIZATION_LEVEL_1_SAFETY_NET,
  OPTIMIZATION_LEVEL_1_SAFETY_NET_MULTIPLE,

  // Specifies the type of graceful termination to use
  OPTIMIZER_GRACEFUL_TERMINATION,
  
  // -------------------------------------------------------------------------
  // Temp. optimizer heuristics. Used for testing only, The heuristic is
  // renamed after proven worthy
  // -------------------------------------------------------------------------
  OPTIMIZER_HEURISTIC_1,
  OPTIMIZER_HEURISTIC_2,
  OPTIMIZER_HEURISTIC_3,
  // #4 & #5 are heuristics for elimination of plans with relatively
  // high data flow wrt other logical plans in the group
  OPTIMIZER_HEURISTIC_4,
  OPTIMIZER_HEURISTIC_5,

  // Optimizer heuristics to prevent unnecessary parallelism
  // for "small" non-partitioned groups
  PARALLEL_HEURISTIC_1,
  PARALLEL_HEURISTIC_2,
  PARALLEL_HEURISTIC_3,
  PARALLEL_HEURISTIC_4,

  // if this one is "ON" it overwrites optimizer heuristics 4 & 5 as "ON"
  // if its "OFF" then the defaults of the two heuristics will be used
  DATA_FLOW_OPTIMIZATION,

  PARTITIONING_SCHEME_SHARING,

  // -------------------------------------------------------------------------
  // Optimization heuristic to control # of cross-products in the plan
  // -------------------------------------------------------------------------
  CROSS_PRODUCT_CONTROL,

  // -------------------------------------------------------------------------
  // Force the join order given by the user
  // -------------------------------------------------------------------------
  JOIN_ORDER_BY_USER,

  // -------------------------------------------------------------------------
  // Optimization heuristic to control # of nested joins considered in a plan
  // -------------------------------------------------------------------------
  NESTED_JOIN_CONTROL,

  // -------------------------------------------------------------------------
  // Allow nested join for cross products
  // -------------------------------------------------------------------------
  NESTED_JOIN_FOR_CROSS_PRODUCTS,

  // -------------------------------------------------------------------------
  // Threshold above which OCR is enabled for nested joins
  // -------------------------------------------------------------------------
  NESTED_JOINS_OCR_MAXOPEN_THRESHOLD,

  // -------------------------------------------------------------------------
  // Controls the number of blocks read per DP2 access
  // -------------------------------------------------------------------------
  NUM_OF_BLOCKS_PER_ACCESS,

  // -------------------------------------------------------------------------
  // Controls the complexity threshold to skip pass1 optimization
  // -------------------------------------------------------------------------
  SHORT_OPTIMIZATION_PASS_THRESHOLD,

  // -------------------------------------------------------------------------
  // Controls whether plan stealing is allowed or not
  // -------------------------------------------------------------------------
  PLAN_STEALING,

  // -------------------------------------------------------------------------
  // Use info from right child to require order on left child of NJ
  // -------------------------------------------------------------------------
  PREFERRED_PROBING_ORDER_FOR_NESTED_JOIN,

  // -------------------------------------------------------------------------
  // Heuristic to reduce the number of double exchanges (OFF enables it)
  // -------------------------------------------------------------------------
  TRY_DP2_REPARTITION_ALWAYS,

  // -------------------------------------------------------------------------
  // Specifies the max. number of tasks the optimizer is allowed to do.
  // If exceeded and beyond the first pass, aggressive pruning will
  // finalize the optimization process.
  // -------------------------------------------------------------------------
  OPTIMIZATION_TASKS_LIMIT,

  // -----------------------------------------------------------------------
  // Disables/enables pruning.
  // -----------------------------------------------------------------------
  OPTIMIZER_PRUNING,
  OPTIMIZER_PRUNING_FIX_1,

  // -----------------------------------------------------------------------
  // Optimizer pruning heuristics.
  // -----------------------------------------------------------------------
  OPH_EXITHJCRCONTCHILOOP,
  OPH_EXITMJCRCONTCHILOOP,
  OPH_EXITNJCRCONTCHILOOP,
  OPH_PRUNING_COMPLEXITY_THRESHOLD,
  OPH_PRUNING_PASS2_COST_LIMIT,
  OPH_PRUNE_WHEN_COST_LIMIT_EXCEEDED,
  OPH_USE_NICE_CONTEXT,
  OPH_REDUCE_COST_LIMIT_FROM_CANDIDATES,
  OPH_REDUCE_COST_LIMIT_FROM_PASS1_SOLUTION,
  OPH_REUSE_FAILED_PLAN,
  OPH_REUSE_OPERATOR_COST,
  OPH_SKIP_OGT_FOR_SHARED_GC_FAILED_CL,
  OPH_USE_CACHED_ELAPSED_TIME,
  OPH_USE_CANDIDATE_PLANS,
  OPH_USE_COMPARE_COST_THRESHOLD,
  OPH_USE_CONSERVATIVE_COST_LIMIT,
  OPH_USE_ENFORCER_PLAN_PROMOTION,
  OPH_USE_FAILED_PLAN_COST,
  OPH_USE_ORDERED_MJ_PRED,
  OPH_USE_PWS_FLAG_FOR_CONTEXT,


  // -----------------------------------------------------------------------
  // PCODE
  // -----------------------------------------------------------------------
  PCODE_OPT_LEVEL,

  //--------------------------------------------------------------------------
  // The group by reduction for pushing a partial group by past the
  //  right side of the TSJ must be at least this much.  If 0.0, then
  //  pushing it will always be tried.
  //--------------------------------------------------------------------------
  REDUCTION_TO_PUSH_GB_PAST_TSJ,

  // ------------------------------------------------------------------------
  // Ordered Hash Join
  // ------------------------------------------------------------------------
  ORDERED_HASH_JOIN_CONTROL,
  OHJ_BMO_REUSE_SORTED_BMOFACTOR_LIMIT,
  OHJ_BMO_REUSE_SORTED_UECRATIO_UPPERLIMIT,
  OHJ_BMO_REUSE_UNSORTED_UECRATIO_UPPERLIMIT,
  OHJ_VBMOLIMIT,

  //--------------------------------------------------------------------------
  // OR-optimization (using multiple indexes for OR predicates)
  //--------------------------------------------------------------------------
  OR_OPTIMIZATION,

  // -------------------------------------------------------------------------
  // Makes the optimizer issue a warning message with its internal
  // counters
  // -------------------------------------------------------------------------
  OPTIMIZER_PRINT_INTERNAL_COUNTERS,

  // -------------------------------------------------------------------------
  // Makes the optimizer prints costing information.
  // -------------------------------------------------------------------------
  OPTIMIZER_PRINT_COST,

  //--------------------------------------------------------------------------
  // Push down certain operators down dp2 for TPC-C performance.
  // 0 = do not push (default), 1= do push (any nonzero pushes operator)
  //--------------------------------------------------------------------------
  OPTS_PUSH_DOWN_DAM,

  //--------------------------------------------------------------------------
  // Partition Overlay Support (POS) options
  //--------------------------------------------------------------------------
  POS_NUM_OF_PARTNS,

  //--------------------------------------------------------------------------
  // Query Caching options
  //--------------------------------------------------------------------------
  QUERY_CACHE,
  QUERY_CACHE_AVERAGE_PLAN_SIZE,
  QUERY_CACHE_SELECTIVITY_TOLERANCE,
  QUERY_CACHE_MAX_CHAR_LEN,
  QUERY_CACHE_MAX_EXPRS,
  QUERY_CACHE_MAX_VICTIMS,
  QUERY_CACHE_REQUIRED_PREFIX_KEYS,
  QUERY_CACHE_STATEMENT_PINNING,
  QUERY_CACHE_STATISTICS,
  QUERY_CACHE_STATISTICS_FILE,
  QUERY_CACHE_USE_CONVDOIT_FOR_BACKPATCH,


  // -------------------------------------------------------------------------
  // If set to TRUE, this indicates that a select query will not be updated
  // or deleted using an 'upd/del where current of cursor' statement.
  // According to ANSI, all SELECTs could be upd/del by default.
  // The default is FALSE.
  // ------------------------------------------------------------------------
  READONLY_CURSOR,

  // multiplicative factor used to inflate cost of risky operators.
  RISK_PREMIUM_MJ,
  RISK_PREMIUM_NJ,
  RISK_PREMIUM_SERIAL,

  ROBUST_HJ_TO_NJ_FUDGE_FACTOR,
  ROBUST_QUERY_OPTIMIZATION,
  ROBUST_SORTGROUPBY,

  // -------------------------------------------------------------------------
  // For Scratch files
  // -------------------------------------------------------------------------

  SCRATCH_DIRS,
  

  // -------------------------------------------------------------------------
  // For SortGroupBy.
  // -------------------------------------------------------------------------
  SGB_CPUCOST_INITIALIZE,
  SGB_INITIAL_BUFFER_COUNT,
  SGB_INITIAL_BUFFER_SIZE,

  // -------------------------------------------------------------------------
  // For Sort.
  // -------------------------------------------------------------------------
  SORT_CPUCOST_INITIALIZE,        // Per probe initialization cost.
  SORT_QS_FACTOR,                 // Constant factor for Quick Sort.
  SORT_RS_FACTOR,                 // Constant factor for Replacement Sel.
  SORT_IO_BUFFER_SIZE,            // I/O buffer size.
  SORT_EX_BUFFER_SIZE,            // Executor buffer size.
  SORT_TREE_NODE_SIZE,            // Internal node size of Tournament Tree.
  // -------------------------------------------------------------------------
  // For Node Maps
  // -------------------------------------------------------------------------
  FAKE_VOLUME_ASSIGNMENTS, // Force num of DP2 vols = num of partitions? ON/OFF
  FAKE_VOLUME_NUM_VOLUMES,  // Number of partitions per volume

  // -------------------------------------------------------------------------
  // For Exchange and parallel execution
  // -------------------------------------------------------------------------
  ATTEMPT_ASYNCHRONOUS_ACCESS, // Nowait I/O allowed? ON/OFF
  ATTEMPT_ESP_PARALLELISM, // Generate and cost ESP parallelism plans?
                           // ON or OFF or SYSTEM
                           // SYSTEM = optimizer decides, based on whether
                           // operator is a BMO, uses lots of CPU, etc.
  BASE_NUM_PAS_ON_ACTIVE_PARTS, // limit # of PAs to # of active parts?
  MAX_ESPS_PER_CPU_PER_OP, // max number of ESPs per cpu for a given operator
  MAX_ACCESS_NODES_PER_ESP, // max number of access nodes per ESP
  NUMBER_OF_PARTITIONS_DEVIATION, // % deviation allowed for # of parts/ESPs
  NUMBER_OF_ROWS_PARALLEL_THRESHOLD, // # of rows where parallelism is a good idea
  NUM_OF_PARTS_DEVIATION_TYPE2_JOINS, // % deviation allowed for type2 joins
  PARALLEL_ESP_NODEMASK, // nodes to be used for ESPs
  PARALLEL_NUM_ESPS, // max # of ESPs an operator can use
  PARTITION_ACCESS_NODES_PER_ESP, // optimal number of PAs per ESP
  UPDATED_BYTES_PER_ESP,          // ratio of updated bytes/ESPs in updates

  // ------------------------------------------------------------------------
  // For allowing / hiding indexes
  // ------------------------------------------------------------------------
  HIDE_INDEXES,

  // -------------------------------------------------------------------------
  // For Insert/update/delete
  // -------------------------------------------------------------------------
  UPD_ORDERED,                    // require ins/upd/del rows to be ordered
  IUD_NONAUDITED_INDEX_MAINT,     // is index maint. allowed on nonaud. tables?
  INSERT_VSBB,                    // insert VSBB options
  VSBB_TEST_MODE,                 // Bypass heuristics for VSBB selection.
  DISABLE_BUFFERED_INSERTS,       // Use only single row inserts.

  // for IUD operations, this default indicates whether a transaction needs
  // to be rolled back on error(ON), not rolled back(OFF), or let the
  // system decide. See BindRelExpr.cpp and GenPreCode.cpp and look for
  // setRollbackOnError() methods for how this is set.
  //  ROLLBACK_ON_ERROR,   : obsolete

  // -------------------------------------------------------------------------
  // For constraint droppable default option.
  // -------------------------------------------------------------------------
  NOT_NULL_CONSTRAINT_DROPPABLE_OPTION,
  PRIMARY_KEY_CONSTRAINT_DROPPABLE_OPTION,

  CATALOG,
  SCHEMA,

  // --------------------------------------------------------------------------
  // These values are taken from Ganesh Hariharan's document,
  // Mapping the SQL/MP defines and control in SQL/ARK.
  // --------------------------------------------------------------------------
  ISOLATION_LEVEL,
  IF_LOCKED,
  MDAM_CPUCOST_NET_OVH,
  MDAM_CPUCOST_NET_PER_PRED,
  MDAM_SCAN_METHOD,
  //---------------------------------------------------------------------------
  //Used to control agressiveness of executor predicate replication minimization
  ALLOW_INPUT_PRED_REPLICATION_REDUCTION,

  //---------------------------------------------------------------------------
  //Following three defaults were added in conjunction to index elimination project.

  //We calculate the promise of index join using the initial inputEstLogProp.
  //In FileScanRule::nextSubstitute() we need to ensure that context inputEstLogProp
  //cardinality is not less than acceptable error * cardinality of initial
  // estlogprop
  //----------------------------------------------------------------------------
  ACCEPTABLE_INPUTESTLOGPROP_ERROR,

  // if mdamSkips exceeds indexBlocks * selection default then index probably
  // not good for mdam. Look at IndexDesc.cpp for more detail.
  MDAM_SELECTION_DEFAULT,

  //How many blocks do we read ahead for a sequential scan of a file.
  READ_AHEAD_MAX_BLOCKS,

  TABLELOCK,
  TIMEOUT,
  STREAM_TIMEOUT,		// BertBert VV
  UNAVAILABLE_PARTITION,
  //	ACCESS_OPTION,
  //	ACCESS_PATH,
  //	AGGREGATE_METHOD,
  //	GROUP_METHOD,
  //	JOIN_METHOD_HASH,
  //	JOIN_METHOD_MERGE,
  //	JOIN_METHOD_NESTED,
  //	OPEN_PARTITION,
  //	PARALLEL_PLAN,
  //	SEQUENTIAL,
  //	SEQUENTIAL_BLOCKSPLIT,
  //	TRANSACTION_MODE,

  ZIG_ZAG_TREES, // Enable 'ON' or disable 'OFF' considering zig-zag trees
  ZIG_ZAG_TREES_CONTROL, // A heuristic to reduce # of non-promising z-zag nodes

  RECOMPILATION_WARNINGS,

  SHOWCONTROL_SHOW_ALL,
  NAMETYPE,


  NATIONAL_CHARSET,
  TERMINAL_CHARSET,


  //--------------------------------------------------------------------------
  // Enable character set inference for ODBC 2.X.
  // OFF = do not infer (default), ON = do infer
  //--------------------------------------------------------------------------
  INFER_CHARSET,

  // --------------------------------------------------------------------------
  // NSK DEBUG defaults.
  // --------------------------------------------------------------------------
  NSK_DBG,
  NSK_DBG_LOG_FILE,
  //
  NSK_DBG_SHOW_TREE_AFTER_PARSING,
  NSK_DBG_SHOW_TREE_AFTER_BINDING,
  NSK_DBG_SHOW_TREE_AFTER_TRANSFORMATION,
  NSK_DBG_SHOW_TREE_AFTER_NORMALIZATION,
  NSK_DBG_SHOW_TREE_AFTER_ANALYSIS,
  NSK_DBG_SHOW_PASS1_PLAN,
  NSK_DBG_SHOW_PASS2_PLAN,
  NSK_DBG_SHOW_PLAN_LOG,
  NSK_DBG_SHOW_TREE_AFTER_PRE_CODEGEN,
  NSK_DBG_SHOW_TREE_AFTER_CODEGEN,

  //
  NSK_DBG_PRINT_COST,
  NSK_DBG_PRINT_LOG_PROP,
  NSK_DBG_PRINT_PHYS_PROP,
  NSK_DBG_PRINT_CHAR_INPUT,
  NSK_DBG_PRINT_CHAR_OUTPUT,
  NSK_DBG_PRINT_CONSTRAINT,
  NSK_DBG_PRINT_CONTEXT,
  NSK_DBG_PRINT_CONTEXT_POINTER,
  NSK_DBG_PRINT_ITEM_EXPR,
  NSK_DBG_PRINT_TASK,
  NSK_DBG_PRINT_TASK_STACK,

  // -----------------------------------------------------------------------
  // for IUD operations, this default, if set, indicates that the Xn needs
  // to be rolled back on error.
  // -----------------------------------------------------------------------
  UPD_ABORT_ON_ERROR,

  // -------------------------------------------------------------------
  // For certain IUD operations, this default, if set, returns an error
  // and not aborts the transaction. This simulates sql/mp error.
  // -------------------------------------------------------------------
  UPD_PARTIAL_ON_ERROR,

  // -------------------------------------------------------------------
  // this default, if set, indicates that dp2 savepoint will be used
  // and stmt will be rolled back to the start of that savepoint.
  // -------------------------------------------------------------------
  UPD_SAVEPOINT_ON_ERROR,


  // ----------------------------------------------------------------------
  // User-defined routines
  //
  // UDF_METADATA_SCHEMA: Schema containing user metadata tables for
  // UDFs. This setting will not be needed once we support UDF
  // metadata in the SQL catalog.
  //
  // UDR_JAVA_OPTIONS: JVM startup options in the UDR server. Can be
  // OFF, ANYTHING, or a series of JVM options separated by the
  // UDR_JAVA_OPTION_DELIMITER string. The default is OFF.
  //
  // UDR_JAVA_OPTION_DELIMITERS: Characters in this string delimit JVM
  // options. The default is a single space character.
  //
  // ----------------------------------------------------------------------
  UDF_METADATA_SCHEMA,
  UDR_JAVA_OPTIONS,
  UDR_JAVA_OPTION_DELIMITERS,

  FSO_RUN_TESTS,
  FSO_TO_USE,

  OPTIMIZER_SYNTH_FUNC_DEPENDENCIES,

  COMP_BOOL_11,
  COMP_BOOL_12,
  COMP_BOOL_13,
  COMP_BOOL_14,
  COMP_BOOL_15,
  COMP_BOOL_18,
  COMP_BOOL_19,
  COMP_BOOL_20,
  COMP_BOOL_21, // used by generator
  COMP_BOOL_22,
  COMP_BOOL_23,
  COMP_BOOL_24, // used by generator
  COMP_BOOL_25, // used by generator
  COMP_BOOL_27,
  COMP_BOOL_28,
  COMP_BOOL_29,
  COMP_BOOL_30,
  COMP_BOOL_31,
  COMP_BOOL_32,
  COMP_BOOL_33,
  COMP_BOOL_34,
  COMP_BOOL_35,
  COMP_BOOL_36,
  COMP_BOOL_37,
  COMP_BOOL_38,
  COMP_BOOL_39,
  COMP_BOOL_40,
  COMP_BOOL_41,
  COMP_BOOL_42,
  COMP_BOOL_44,
  COMP_BOOL_45,
  COMP_BOOL_46,
  COMP_BOOL_47,
  COMP_BOOL_48,
  COMP_BOOL_49,
  COMP_BOOL_51,
  COMP_BOOL_52,
  COMP_BOOL_53,
  COMP_BOOL_54,
  COMP_BOOL_55,
  COMP_BOOL_56,
  COMP_BOOL_57,
  COMP_BOOL_59,
  COMP_BOOL_60,
  COMP_BOOL_61,
  COMP_BOOL_62,
  COMP_BOOL_63,
  COMP_BOOL_64,
  COMP_BOOL_65,
  COMP_BOOL_66,
  COMP_BOOL_67,
  COMP_BOOL_68,
  COMP_BOOL_69,
  COMP_BOOL_70,
  COMP_BOOL_71,
  COMP_BOOL_72,
  COMP_BOOL_73,
  COMP_BOOL_74,
  COMP_BOOL_75,
  COMP_BOOL_76,
  COMP_BOOL_77,
  COMP_BOOL_78,
  COMP_BOOL_79,
  COMP_BOOL_80,
  COMP_BOOL_81,
  COMP_BOOL_82,
  COMP_BOOL_83,
  COMP_BOOL_84,
  COMP_BOOL_85,
  COMP_BOOL_86,
  COMP_BOOL_87,
  COMP_BOOL_88,
  COMP_BOOL_90,
  COMP_BOOL_91, // used by generator
  COMP_BOOL_92,
  COMP_BOOL_93,
  COMP_BOOL_94,
  COMP_BOOL_95,
  COMP_BOOL_96,
  COMP_BOOL_97,
  COMP_BOOL_98,
  COMP_BOOL_99,

  COMP_INT_0,
  COMP_INT_7,
  COMP_INT_8,
  COMP_INT_9,

  COMP_FLOAT_0,
  COMP_FLOAT_4,
  COMP_FLOAT_5,
  COMP_FLOAT_6,
  COMP_FLOAT_7,
  COMP_FLOAT_8,
  COMP_FLOAT_9,
  COMP_STRING_5,

  COSTING_SHORTCUT_GROUPBY_FIX,

  VARCHAR_PARAM_DEFAULT_SIZE,

  // ----------------------------------------------------------------------
  // If enabled (ON), indicates that query processing is to be done
  // following JDBC rules. This covers all of stuff done for ODBC_PROCESS
  // plus more.
  // ----------------------------------------------------------------------
  JDBC_PROCESS,

  OPTIMIZATION_LEVEL_1_IMMUNITY_LIMIT,
  OPTIMIZATION_LEVEL_1_MJENUM_LIMIT,

  NSK_DBG_MJRULES_TRACKING,

  NEW_MDAM,

  MDAM_COSTING_REWRITE,

  MDAM_SUBSET_FACTOR,

  MDAM_FSO_SIMPLE_RULE,

  // -------------------------------------------------------------------------
  // Makes NO ACTION referential action behave like RESTRICT.
  // -------------------------------------------------------------------------
  REF_CONSTRAINT_NO_ACTION_LIKE_RESTRICT,


  GEN_SORT_MAX_NUM_BUFFERS,
  GEN_SORT_TOPN,

  SORT_ALGO,            // Sort algorithm choice


  // -------------------------------------------------------------------------
  // Specifies what percentage of smaller table in join, should be used as
  // a lower bound for join cardinality
  // -------------------------------------------------------------------------
  HIST_JOIN_CARD_LOWBOUND,

  // allow any/first N in subqueries.
  ALLOW_FIRSTN_IN_SUBQUERIES,

  // ------------------------------------------------------------------------
  // This default will use a new type of an ASSERT, CCMPASSERT as a CMPASSERT
  // when ON, else use that as a DCMPASSERT.
  // -------------------------------------------------------------------------
  USE_CCMPASSERT_AS_CMPASSERT,


  NEW_OPT_DRIVER,
  FORCE_PASS_ONE,
  FORCE_PASS_TWO,
  TRY_PASS_ONE_IF_PASS_TWO_FAILS,
  TEST_PASS_ONE_ASSERT_TASK_NUMBER,
  TEST_PASS_TWO_ASSERT_TASK_NUMBER,
  SHOWWARN_OPT,

  // ------------------------------------------------------------------------
  // This default will dictate which $volume the temporary sample table will
  // be located.
  // ------------------------------------------------------------------------
  HIST_SCRATCH_VOL,

  PRESERVE_MIN_SCALE,

  // -------------------------------------------------------------------------
  // If on, indicates that conversion from sql to application-hostvars are to
  // be done using the CAST rules, even if the datatypes are incompatible.
  // For example, conversion from CHAR column to DATE user hostvar is not
  // allowed according to ANSI as these are incompatible datatypes even
  // though this conversion could be done using cast function.
  // This default will enable all conversions that are allowed as if a CAST
  // function is used.
  // -------------------------------------------------------------------------
  IMPLICIT_DATETIME_INTERVAL_HOSTVAR_CONVERSION,

  //------------------------------------------------------------------------
  // If on, compiler considers generation of a plan that does synchronous
  //  access in reverse order
  //------------------------------------------------------------------------
  ATTEMPT_REVERSE_SYNCHRONOUS_ORDER,

  //--------------------------------------------------------------------------
  // The foll indicates the percentage of the free space on a particular disk
  // after which the scratch overflow code will not use that disk any more.
  // By default this is set at 20%. If the user wants to change this to 0%
  //  then the scratch logic will use all the space on the disk
  //----------------------------------------------------------------------------
  SCRATCH_FREESPACE_THRESHOLD_PERCENT,

  //--------------------------------------------------------------------------
  // rand() function in sql is disabled unless this CQD is turned on
  //--------------------------------------------------------------------------
  ALLOW_RAND_FUNCTION,

  // This defaults control whether we should use MC UEC while computing cardinalities
  // for Join on non-key columns. By default we shall not use those
  //
  HIST_SKIP_MC_FOR_NONKEY_JOIN_COLUMNS,

  //--------------------------------------------------------------------------
  // If set, then view validation(view existence, security checks) at runtime
  // is only done at fixup(open) time instead of on each execution.
  // This is non-ANSI behavior.
  //--------------------------------------------------------------------------
  VALIDATE_VIEWS_AT_OPEN_TIME,

  //10-040621-7139-begin
  INTERACTIVE_ACCESS,
  //10-040621-7139-end

  
  //Support transactions for DDL operations.
  DDL_TRANSACTIONS,
  
  // controls various optimizations, see enum QueryOptimizationOptions.
  QUERY_OPTIMIZATION_OPTIONS,


  // force use of Method-of-Moments UEC estimator in update stats
  USTAT_FORCE_MOM_ESTIMATOR,

  // set the maximum weight for the Shlosser UEC estimator
  USTAT_DSHMAX,

  // turn on and off olt query optimizations. Default is ON.
  EID_SPACE_USAGE_OPT,


  // partition threshold to control partition size when update statistics
  // temporary sample table is created
  HIST_SCRATCH_VOL_THRESHOLD,


  // Allows users to specify SYSKEY value. In other words
  // the system does not generate one.
  OVERRIDE_SYSKEY ,

  //--------------------------------------------------------------------------
  //External sort involves both READ and WRITE operation, where as the DISK
  // IO rate is only for READ. We need to multiply the elapsed time by this
  // factor.
  //--------------------------------------------------------------------------
  SORT_RW_FACTOR,

  HIST_MISSING_STATS_WARNING_LEVEL,

  HIST_DEFAULT_BASE_SEL_FOR_LIKE_WILDCARD,

  DP2_SEQ_READS_WITHOUT_SEEKS,

  DIMENSIONAL_QUERY_OPTIMIZATION,
  // turn post-parser & post-binder query caching ON or OFF
  QUERY_TEMPLATE_CACHE,
  // turn pre-parser query caching ON or OFF
  QUERY_TEXT_CACHE,

  // allows users to specify a limit on the number of nonfatal errors that will be
  // tolerated for a non-atomic statement.
  NOT_ATOMIC_FAILURE_LIMIT,

  // allows user to turn ON/OFF the ROWSET_ROW_COUNT feature.
  ROWSET_ROW_COUNT,


  // POS
  POS,

  // how many maximum groups dp2 hash group-by operator can handle?
  MAX_DP2_HASHBY_GROUPS,

  // HASH_header entries per hash table used in group-by costing
  MAX_HEADER_ENTREIS_PER_HASH_TABLE,

  HGB_MAX_TABLE_SIZE_FOR_CLUSTERS,
  HGB_MEMORY_AVAILABLE_FOR_CLUSTERS,


  //Allows user to specify default SAMPLE limits that should be used during fetchcount
  HIST_DEFAULT_SAMPLE_MIN,
  HIST_DEFAULT_SAMPLE_MAX,
  HIST_DEFAULT_SAMPLE_RATIO,


  // EMS events
  USER_EXPERIENCE_LEVEL,


  // Starting size of Partition Access's parent queues.
  DYN_PA_QUEUE_RESIZE_INIT_DOWN,
  DYN_PA_QUEUE_RESIZE_INIT_UP,

  // if this default is set to ON, then the max precision of a numeric
  // expression(arithmetic, aggregate) is limited to MAX_NUMERIC_PRECISION
  // (= 18). If this is set to OFF, the default value, then the max precision
  // is computed based on the operands and the operation which could make the
  // result a software datatype(BIGNUM). Software datatypes give better
  // precision but degraded performance.
  LIMIT_MAX_NUMERIC_PRECISION,

  CONSTANT_FOLDING,

  QUERY_CACHE_TABLENAME,

  QUERY_CACHE_RUNTIME,

  SKIP_UNAVAILABLE_PARTITION,

  ISOLATION_LEVEL_FOR_UPDATES,

  //HISTOGRAMS: If the UEC for a column is <= HIST_LOW_UEC_THRESHOLD, then
  //the histogram generated will have 1 interval per unique value.
  HIST_LOW_UEC_THRESHOLD,

  DESTROY_ORDER_AFTER_REPARTITIONING,

  MERGE_JOIN_WITH_POSSIBLE_DEADLOCK,

  ALLOW_RANGE_PARTITIONING,


  SORT_MAX_HEAP_SIZE_MB,   //Heap memory for SORT operator in megabytes.


// Optimization to expand any "short" rows found in tables with added columns
  EXPAND_DP2_SHORT_ROWS,

  MVGROUP_AUTOMATIC_CREATION,


  // Allow RequireApproximatelyNPartitions::realize() to use one of three
  // hash partitioning schemes: 0 - HashPartitioningFunction,
  // 1 - HashDistPartitioningFunction, 2 - Hash2PartitioningFunction.
  SOFT_REQ_HASH_TYPE,

  // Use conservative hash2 grouping
  OLD_HASH2_GROUPING,


  COMP_INT_10,
  COMP_INT_11,
  COMP_INT_14,
  COMP_INT_15,
  COMP_INT_16,
  COMP_INT_17,
  COMP_INT_18,
  COMP_INT_19,
  COMP_INT_20,
  COMP_INT_21,
  COMP_INT_22,
  COMP_INT_23,
  COMP_INT_24,
  COMP_INT_26,
  COMP_INT_30,
  COMP_INT_31,
  COMP_INT_32,
  COMP_INT_34,
  COMP_INT_35,
  COMP_INT_36,
  COMP_INT_38,
  COMP_INT_39,
  COMP_INT_40,
  COMP_INT_43,
  COMP_INT_44,
  COMP_INT_45,
  COMP_INT_46,
  COMP_INT_47,
  COMP_INT_48,
  COMP_INT_50,
  COMP_INT_51,
  COMP_INT_54,
  COMP_INT_60,
  COMP_INT_61,
  COMP_INT_62,
  COMP_INT_63,
  COMP_INT_66,
  COMP_INT_67,
  COMP_INT_70,
  COMP_INT_71,
  COMP_INT_73,
  COMP_INT_74,
  COMP_INT_77,
  COMP_INT_79,
  COMP_INT_80,
  COMP_INT_89,
  COMP_INT_90,
  COMP_INT_95,
  COMP_INT_98,
  COMP_INT_99,

  COMP_BOOL_100,
  COMP_BOOL_101,
  COMP_BOOL_102,
  COMP_BOOL_103,
  COMP_BOOL_104,
  COMP_BOOL_106,
  COMP_BOOL_107,
  COMP_BOOL_108,
  COMP_BOOL_110,
  COMP_BOOL_111,
  COMP_BOOL_112,
  COMP_BOOL_113,
  COMP_BOOL_114,
  COMP_BOOL_115,
  COMP_BOOL_116,
  COMP_BOOL_117,
  COMP_BOOL_118,
  COMP_BOOL_119,
  COMP_BOOL_120,
  COMP_BOOL_122,
  COMP_BOOL_123,
  COMP_BOOL_124,
  COMP_BOOL_125,
  COMP_BOOL_126,
  COMP_BOOL_127,
  COMP_BOOL_128,
  COMP_BOOL_129,
  COMP_BOOL_130,
  COMP_BOOL_131,
  COMP_BOOL_132,
  COMP_BOOL_133,
  COMP_BOOL_134,
  COMP_BOOL_135,
  COMP_BOOL_136,
  COMP_BOOL_137,
  COMP_BOOL_138,
  COMP_BOOL_139,
  COMP_BOOL_140,
  COMP_BOOL_141,
  COMP_BOOL_144,
  COMP_BOOL_145,
  COMP_BOOL_147,
  COMP_BOOL_148,
  COMP_BOOL_149,
  COMP_BOOL_150,
  COMP_BOOL_151,
  COMP_BOOL_152,
  COMP_BOOL_153,
  COMP_BOOL_154,
  COMP_BOOL_155,
  COMP_BOOL_156,
  COMP_BOOL_158,
  COMP_BOOL_159,
  COMP_BOOL_160,
  COMP_BOOL_161,
  COMP_BOOL_162,
  COMP_BOOL_163,
  COMP_BOOL_164,
  COMP_BOOL_165,
  COMP_BOOL_166,
  COMP_BOOL_167,
  COMP_BOOL_168,
  COMP_BOOL_169,
  COMP_BOOL_171,
  COMP_BOOL_172,
  COMP_BOOL_173,
  COMP_BOOL_174,
  COMP_BOOL_175,
  COMP_BOOL_176,
  COMP_BOOL_177,
  COMP_BOOL_178,
  COMP_BOOL_183,
  COMP_BOOL_184,
  COMP_BOOL_185,
  COMP_BOOL_186,
  COMP_BOOL_187,
  COMP_BOOL_188,
  COMP_BOOL_189,
  COMP_BOOL_190,
  COMP_BOOL_191,
  COMP_BOOL_192,
  COMP_BOOL_193,
  COMP_BOOL_194,  // Can be used to turn off fix to JIRA Trafodion 3325
  COMP_BOOL_196,
  COMP_BOOL_197,
  COMP_BOOL_198,
  COMP_BOOL_199,
  COMP_BOOL_200,
  COMP_BOOL_201,
  COMP_BOOL_202,
  COMP_BOOL_203,

  COMP_BOOL_206,
  COMP_BOOL_207,
  COMP_BOOL_210,
  COMP_BOOL_211,
  COMP_BOOL_217,
  COMP_BOOL_219,

  HIST_OPTIMISTIC_CARD_OPTIMIZATION,


  // System IDentity buffer specification
  GEN_SID_BUFFER_SIZE,
  GEN_SID_NUM_BUFFERS,

  // Partial Hash GroupBy.
  GEN_HGBY_PARTIAL_GROUP_FLUSH_THRESHOLD,
  GEN_HGBY_PARTIAL_GROUP_ROWS_PER_CLUSTER,


  ALLOW_DP2_ROW_SAMPLING,

  INCORPORATE_SKEW_IN_COSTING,
  TOTAL_RESOURCE_COSTING,



  USE_PARALLEL_FOR_NUM_PARTITIONS,

  EXE_DIAGNOSTIC_EVENTS,    // Sends various executor diagnostics to EMS

  SHARE_TEMPLATE_CACHED_PLANS,


  // to enable special mode 1. Currently enabled for  release.
  MODE_SPECIAL_1,

  SESSION_ID,

  SESSION_IN_USE,

  VOLATILE_SCHEMA_IN_USE,

  VOLATILE_CATALOG,

  LAST0_MODE,

  OUTPUT_DATE_FORMAT,

  // implicit upd stats on volatile table is only done if the number
  // of inserted rows exceed this threshold.
  IMPLICIT_UPD_STATS_THRESHOLD,

  // POS attributes for temporary/volatile tables
  POS_NUM_OF_TEMP_TABLE_PARTNS,
  POS_TEMP_TABLE_SIZE,

  HIST_DEFAULT_NUMBER_OF_INTERVALS,


 SESSION_USERNAME,

  // For DBLimits project
  DP2_CACHE_8K_BLOCKS ,
  DP2_CACHE_16K_BLOCKS,
  DP2_CACHE_32K_BLOCKS,

  // Enable DBLimits functionality
  CAT_LARGE_BLOCKS_LARGE_KEYS,

  GEN_DBLIMITS_LARGER_BUFSIZE,



  FORCE_PARALLEL_INSERT_SELECT,

    // For stored procedure result sets
  GEN_UDRRS_BUFFER_SIZE,
  GEN_UDRRS_NUM_BUFFERS,
  GEN_UDRRS_SIZE_DOWN  ,
  GEN_UDRRS_SIZE_UP    ,

  ALLOW_UNEXTERNALIZED_MAINTAIN_OPTIONS,

  EXE_MEMORY_AVAILABLE_IN_MB, // Mem size (MB) in ESP available for BMOs

  EXE_MEMORY_FOR_PARTIALHGB_IN_MB, // Mem size (MB) in ESP available
                                   // for partial hash groupby.
  SSD_BMO_MAX_MEM_THRESHOLD_IN_MB,

  NAR_DEPOBJ_ENABLE ,

  // For Adaptive Segmentation (aka Virtual Segmentation, Adaptive
  // parallelism control)

  AFFINITY_VALUE,
  CYCLIC_ESP_PLACEMENT,
  DEFAULT_DEGREE_OF_PARALLELISM,
  USE_OPERATOR_MAX_FOR_DOP,
  NSK_DBG_GENERIC,
  MEMORY_UNIT_ESP,
  WORK_UNIT_ESP,
  WORK_UNIT_ESP_DATA_COPY_COST,




  MV_REFRESH_MDELTA_MAX_DELTAS_THRESHOLD,
  MV_REFRESH_MDELTA_MAX_JOIN_SIZE_FOR_SINGLE_PHASE,
  MV_REFRESH_MDELTA_MIN_JOIN_SIZE_FOR_SINGLE_PRODUCT_PHASE,
  MV_REFRESH_MDELTA_PHASE_SIZE_FOR_MID_RANGE,

  // Enable 'ON' or disable 'OFF' merge join overflow
  MJ_OVERFLOW,

  SKEW_EXPLAIN,            // embed skew-buster plan details in EXPLAIN output
  SKEW_ROWCOUNT_THRESHOLD, // minimal row count for skew-buster to be active

  // Percentage of BMO memory quota "equal share" used by merge join.
  MJ_BMO_QUOTA_PERCENT,

  SKEW_SENSITIVITY_THRESHOLD, // skew threshold value

  // Enable use of internal sorting/grouping for Update Stats instead of doing
  // it in a query.
  USTAT_INTERNAL_SORT,

  // Control percentage of available physical memory to be used by Update
  // Statistics for internal sort.
  USTAT_IS_MEMORY_FRACTION,
  // For Simple Cost Model
  SIMPLE_COST_MODEL,

  // Enable creation of intervals by Update Stats representing significant
  // gaps between values.
  USTAT_PROCESS_GAPS,

  // Multiply this value by the average gap area to get the gap areas size threshold
  // for creating a gap interval.
  USTAT_GAP_SIZE_MULTIPLIER,

  // Percentage of intervals to be used for representing gaps.
  USTAT_GAP_PERCENT,

  // Create a separate interval in a column's histogram for any distinct value
  // that has a frequency exceeding this percentage of the total row count for
  // the table.
  USTAT_FREQ_SIZE_PERCENT,

  MAX_SKEW_VALUES_DETECTED,

  CREATE_OBJECTS_IN_METADATA_ONLY,


  // ON/OFF flag to invoke ghost objects
  ALLOW_GHOST_OBJECTS,

 //Sort to use memory quota system.
  SORT_MEMORY_QUOTA_SYSTEM,

  // New charsets
  INPUT_CHARSET,
  ISO_MAPPING,
  DEFAULT_CHARSET,

  // controls rounding of how arith and cast results.
  // 0: truncate(no rounding)
  // 1: round half up (values .5 and up are rounded to nearest high digit).
  // 2: round half even (values > .5 rounded up, .5 rounded to nearest even
  //                     digit)
  ROUNDING_MODE,

  // if set, tables are not closed at the end of a query. This allows
  // the same open to be reused for the next query which accesses that
  // table.
  // If the table is shared opened by multiple openers from the same
  // process, then the share count is decremented until it reaches 1.
  // At that time, the last open is preserved so it could be reused.
  // Tables are closed if user id changes.
  REUSE_OPENS,


  // Self-referencing updates.
  BLOCK_TO_PREVENT_HALLOWEEN,
  R2_HALLOWEEN_SUPPORT,
  // Adding for RangeSpec transformation of selection predicates.
  RANGESPEC_TRANSFORMATION,
  HIST_ASSUME_INDEPENDENT_REDUCTION,
  HIST_USE_SAMPLE_FOR_CARDINALITY_ESTIMATION ,

  EXE_PARALLEL_DDL,

  ALLOW_MULTIDEGREE_SUBQ_IN_SELECTLIST,
  ALLOW_SUBQ_IN_SET,
  ALLOW_RENAME_OF_MVF_OR_SUBQ,


  NSK_DBG_SHOW_TREE_AFTER_SEMANTIC_QUERY_OPTIMIZATION,
  SUBQUERY_UNNESTING,
  SUBQUERY_UNNESTING_P2,

  // Defaults for UDF routine costing
  // The CPU cost values will be taken as milliseconds
  // The IO cost values are used as the count of IOs per row
  // The MSG cost values are used as the count of msgs per row
  // The FANOUT cost value is 1 for a Scalar Routine and higher for
  //   a TableValued UDF. It should be thought of reflecting how many
  //   rows the routine can return for each row passed in (maximum).
  // The UEC value is an average UEC for a routine. For scalarUDFs it
  //   is usually 1.
 
  INITIAL_UDF_CPU_COST,
  INITIAL_UDF_IO_COST,
  INITIAL_UDF_MSG_COST,
  NORMAL_UDF_CPU_COST,
  NORMAL_UDF_IO_COST,
  NORMAL_UDF_MSG_COST,
  UDF_FANOUT,
  ROUTINE_DEFAULT_UEC,

  // TableMappingUDF Defaults
  TMUDF_CARDINALITY_FACTOR,
  TMUDF_LEAF_CARDINALITY,

  UDF_SUBQ_IN_AGGS_AND_GBYS,
  USTAT_FETCHCOUNT_ACTIVE,

  SEMIJOIN_TO_INNERJOIN_INNER_ALLOWANCE,
  SEMIJOIN_TO_INNERJOIN_REDUCTION_RATIO,
  SEMIJOIN_TO_INNERJOIN_TRANSFORMATION,

  POS_NUM_DISK_POOLS,
  POS_DISKS_IN_SEGMENT,

  // query complexity threshold for query caching to match constants
  // in parameterization of equality predicates
  MATCH_CONSTANTS_OF_EQUALITY_PREDICATES,
  // Memory limit per Esp for BMOs
  BMO_MEMORY_SIZE,


  // Compress plan fragments downloaded to ESPs (ESP and DP2 frags)
  FRAG_COMPRESSION_THRESHOLD,


  // Display only externalized information in SHOWDDL if set to EXTERNAL
  SHOWDDL_DISPLAY_FORMAT,

  // Generate leaner expressions in all fragments
  GEN_LEANER_EXPRESSIONS,



  POS_DISK_POOL,

  //
  //   Internal options for BMOs
  //
  EXE_BMO_SET_BUFFERED_WRITES,
  // Disallow overflow (in HJ, SORT) -- thus Virtual Memory is used up to its
  // limits (may involve costly swapping)
  EXE_BMO_DISABLE_OVERFLOW,

  // Disable the mechanism for triggering overflow early based on hints
  // given by the compiler (currently used only by SORT and (non unique) HJ).
  EXE_BMO_DISABLE_CMP_HINTS_OVERFLOW_HASH,   // for the Hash Operators
  EXE_BMO_DISABLE_CMP_HINTS_OVERFLOW_SORT,   // for Sort

  // Minimal memory size of a BMO before checking for system memory pressure
  // and compiler hints for early overflow (to avoid "thrashing" at low mem)
  EXE_BMO_MIN_SIZE_BEFORE_PRESSURE_CHECK_IN_MB,

  // The following 3 are only for testing overflow
  EXE_TEST_FORCE_CLUSTER_SPLIT_AFTER_MB,
  EXE_TEST_FORCE_HASH_LOOP_AFTER_NUM_BUFFERS,
  EXE_TEST_HASH_FORCE_OVERFLOW_EVERY, 

  // Force HJ to use at least that many clusters
  EXE_HJ_MIN_NUM_CLUSTERS,
  // Override the memory quota system; set a memory limit per each BMO
  // (including cases of a single BMO in a fragment, and BMOs in the master).
  // Units are in MegaBytes; zero (default) means that this CQD is OFF !!
  EXE_MEM_LIMIT_PER_BMO_IN_MB,

  // Disables use of ProbeCache
  NESTED_JOIN_CACHE,

  // Buffer, cache, and queue sizing for the ProbeCache operator.
  GEN_PROBE_CACHE_NUM_INNER,
  GEN_PROBE_CACHE_NUM_ENTRIES,
  GEN_PROBE_CACHE_SIZE_DOWN,
  GEN_PROBE_CACHE_SIZE_UP,

  // SQL/MX Compiler/Optimizer Memory Monitor.
  MEMORY_MONITOR,
  MEMORY_MONITOR_IN_DETAIL,
  MEMORY_MONITOR_LOGFILE,
  MEMORY_MONITOR_LOG_INSTANTLY,
  MEMORY_MONITOR_AFTER_TASKS,
  MEMORY_MONITOR_TASK_INTERVAL,
  MEMORY_USAGE_SAFETY_NET,
  MEMORY_USAGE_OPT_PASS_FACTOR,
  MEMORY_USAGE_NICE_CONTEXT_FACTOR,

  NAR_DEPOBJ_ENABLE2,

  // Allows update of clustering key or unique index column.
  UPDATE_CLUSTERING_OR_UNIQUE_INDEX_KEY,

  // Cost Adjustment factors for obtain a robust parallel groupby
  // plan i.e. a partial grouping plan
  ROBUST_PAR_GRPBY_EXCHANGE_FCTR,
  ROBUST_PAR_GRPBY_LEAF_FCTR,

  // Histogram boundary value maximum length.
  USTAT_MAX_CHAR_BOUNDARY_LEN,

  // When update statistics computes UECs on character columns or
  // creates a sample table, it limits them to this size in bytes.
  USTAT_MAX_CHAR_COL_LENGTH_IN_BYTES,

  // Maximum number of MC histograms to create for any one MC key.
  USTAT_NUM_MC_GROUPS_FOR_KEYS,

  // maximum precision that can be specified in a NUMERIC datatype.
  // Syntax: NUMERIC(p,s), 'p' is the precision, max value defined
  // by this default. Enforced in parser.
  // Same thing for max precision of a number literal.
  MAX_NUMERIC_PRECISION_ALLOWED,

  // If set to ON, input and output of data from/to user applications
  // will be done in bignum format.
  // If set to OFF, input/output will be done with max precision of 18 (same as
  // from pre-bignum days).
  // An overflow error will be returned if data would not fit within the
  // max precision of 18.
  // If set of SYSTEM (the default), then bignum IO will be done if the input
  // or output value is a bignum and was derived from a real bignum value
  // entered by user(bignum literal, bignum column, param...).
  BIGNUM_IO,

  IS_DB_TRANSPORTER,

  // Allow for the setting of the row count in a long running operation
  MULTI_COMMIT_SIZE,

  // caller is sqlci
  IS_SQLCI,

  // caller is nvci (scripts) process
  NVCI_PROCESS,

  // if this is set, then find a suitable key among all the columns of
  // a volatile table.
  // If this is not set, and there is no user specified primary key or
  // store by clause, then make the first column of the volatile table
  // to be the clustering key.
  VOLATILE_TABLE_FIND_SUITABLE_KEY,


// Allows pcodes for varchars
  VARCHAR_PCODE,


  // if set to ON, then isolation level (read committed, etc) could be
  // specified in a regular CREATE VIEW (not a create MV) statement.
  ALLOW_ISOLATION_LEVEL_IN_CREATE_VIEW,

  // For ESP exchanges introduced in the generator.
  UNOPTIMIZED_ESP_BUFFER_SIZE_DOWN,
  UNOPTIMIZED_ESP_BUFFER_SIZE_UP,

  // For special initial queue sizings for ONLJ
  GEN_ONLJ_LEFT_CHILD_QUEUE_UP,
  GEN_ONLJ_LEFT_CHILD_QUEUE_DOWN,
  GEN_ONLJ_RIGHT_SIDE_QUEUE_UP,
  GEN_ONLJ_RIGHT_SIDE_QUEUE_DOWN,
  GEN_ONLJ_SET_QUEUE_RIGHT,
  GEN_ONLJ_SET_QUEUE_LEFT,

  
  SORT_REC_THRESHOLD,
  SORT_MERGE_BUFFER_UNIT_56KB,


 PARTIAL_SORT_ADJST_FCTR,
  SCRATCH_MAX_OPENS_HASH,
  SCRATCH_MAX_OPENS_SORT,
  SCRATCH_DISK_LOGGING,
  SCRATCH_MGMT_OPTION,
  SCRATCH_PREALLOCATE_EXTENTS,
  SCRATCH_IO_BLOCKSIZE_SORT_MAX,
  SCRATCH_IO_VECTOR_SIZE_HASH,
  SCRATCH_IO_VECTOR_SIZE_SORT,

  OVERRIDE_GENERATED_IDENTITY_VALUES,

  ELIMINATE_REDUNDANT_JOINS,
  MULTI_PASS_JOIN_ELIM_LIMIT,
// Used to modify the timeout for the internal maintain control
  // information table

  MAINTAIN_CONTROL_TABLE_TIMEOUT,

  //OCB Cost adjustment factor
  OCB_COST_ADJSTFCTR,


  // Flags for enabling/disabling optimizations within advanced PCODE
  // optimization framework
  PCODE_OPT_FLAGS,

  USTAT_SAMPLE_PERCENT_DIFF,   // percentage diff allowed to use old sample in FetchCount().
  USTAT_DEBUG_TEST,            // settings for testing ustat, normally empty.
 
  // Disallow/Allow left joins in MultiJoin framework
  LEFT_JOINS_SPOIL_JBB,

  // Disallow/Allow semi and anti-semi joins in MultiJoin framework
  SEMI_JOINS_SPOIL_JBB,

  // Disallow/Allow full outer joins in MultiJoin framework
  FULL_OUTER_JOINS_SPOIL_JBB,

  // Disallow/Allow TSJs in MultiJoin framework
  TSJS_SPOIL_JBB,

  // Disallow/Allow routine joins in MultiJoin framework
  ROUTINE_JOINS_SPOIL_JBB,
  




  // Specify the node
  UNIQUE_HASH_JOINS,
  UNIQUE_HASH_JOIN_MAX_INNER_SIZE,
  UNIQUE_HASH_JOIN_MAX_INNER_SIZE_PER_INSTANCE,
  UNIQUE_HASH_JOIN_MAX_INNER_TABLES,

  // catalog where maintain database is maintained
  MAINTAIN_CATALOG,



  // turn on and off lean olt query optimizations. Default is OFF.
  OLT_QUERY_OPT_LEAN,

 // Specify the file that logs certain MXCMP errors.
  CMP_ERR_LOG_FILE,


  // override the source schema with the target schema
  // SOURCE_SCHEMA:TARGET_SCHEMA
  OVERRIDE_SCHEMA,

  // To handle dynamic resizable hash-table
  EXE_HGB_INITIAL_HT_SIZE,

  EXE_NUM_CONCURRENT_SCRATCH_IOS,

  // in case of an error during query processing, retry the query if
  // that error is set as a retryable error.
  // A retry (AQR) would prepare, execute and fetch, depending on which
  // state the query was in.
  AUTO_QUERY_RETRY,

  // return warnings about aqr.
  // If OFF, no warning is returned.
  // If ON, warnings are returned for retries due to external events.
  // If ALL, warnings are returned due to both external and
  // internal (query cache recompile) events.
  AUTO_QUERY_RETRY_WARNINGS,


  DISABLE_READ_ONLY,

  // perform substring transformation (ICBC PoC)
  SUBSTRING_TRANSFORMATION,


  // by default, a primary key or unique constraint must be non-nullable.
  // This default, if set, allows them to be nullable.
  // The default value is OFF.
  ALLOW_NULLABLE_UNIQUE_KEY_CONSTRAINT,

  // if set to ON, then ORDER BY could be 
  // specified in a regular CREATE VIEW (not a create MV) statement.
  ALLOW_ORDER_BY_IN_CREATE_VIEW,


  EXE_LOG_RETRY_IPC,

  ALLOW_IMPLICIT_CHAR_CASTING,
  
  IN_MEMORY_OBJECT_DEFN,

  // Update statistics command updates the histogram tables of the schema
  // where the table is created. 
  // 
  // This default points to the schema whose histogram tables, instead of
  // the original table's histogram tables, are to be updated during an 
  // 'update statistics' command. 
  HISTOGRAMS_SCHEMA,

  // specifying a pre-existing file with sampled rows for update stats
  USTAT_SAMPLE_TABLE_NAME,

  // Can be used to specify the name of the persistent sample table created
  // by ustat. Used for testing only, to provide a known name that allows the
  // sample table to be accessed by name after creation in a test script.
  // Must take care to change this value (or restore default), before issuing
  // a statement that creates a sample for another table, or the name will be
  // duplicated. The value used must be an UNQUALIFIED table name.
  USTAT_SAMPLE_TABLE_NAME_CREATE,

  // By default (ON), distribute the metadata tables across all
  // segments.  If set to LOCAL_NODE, distribute metadata tables
  // across volumes in the local segment when the first schema is
  // created.
  // If set to OFF, place all metadata tables on the same volume.
  CAT_DISTRIBUTE_METADATA,



  // Pubsub holdable cursor is not closed on rollback by default
  // But, ANSI standard expects the holdable cursor to be closed on rollback 
  PSHOLD_CLOSE_ON_ROLLBACK,
  // To ensure the Pubsub holdable cursor is fetched after commit in case of 
  // positioned update/deletes. Pubsub holdable cursor is not checked for this
  // condition by default
  PSHOLD_UPDATE_BEFORE_FETCH,



  // Update statistics automation CQDs to allow for automation settings.
  USTAT_AUTO_READTIME_UPDATE_INTERVAL,
  USTAT_AUTO_FOR_VOLATILE_TABLES,

  // Use a compressed data format internally rather than current
  // SQLARK_EXPLODED_FORMAT
  COMPRESSED_INTERNAL_FORMAT,


  // Enables possibility of performing a bulk move of nullable column values
  // and variable length column values.
  BULK_MOVE_NULL_VARCHAR,


  // display detailed MV information. Default is OFF
  MV_DUMP_DEBUG_INFO,

  // tracking compilers specific defaults
  COMPILER_TRACKING_INTERVAL,
  COMPILER_TRACKING_LOGFILE,
  COMPILER_TRACKING_LOGTABLE,

  // allow the frequency of the 1st and the 2nd mostly occurred values to 
  // be used in max cardinality and local predicate evaluation.
  HIST_USE_HIGH_FREQUENCY_INFO,
  USTAT_SHOW_MFV_INFO,

  // These CQDs are reserved for NCM. These are mostly used for 
  // internal testing, turning on/off features for debugging, and for tuning. 
  // In normal situations, these will not be externalized in keeping 
  // with the very few CQDs philosophy of NCM. 
  // These are applicable only in conjunction with SIMPLE_COST_MODEL 'on'.
  NCM_CACHE_SIZE_IN_BLOCKS,
  NCM_COSTLIMIT_FACTOR,
  NCM_ESP_FIXUP_WEIGHT,
  NCM_ESP_STARTUP_FIX,
  NCM_EXCH_MERGE_FACTOR,
  NCM_EXCH_NDCS_FIX,
  NCM_HGB_OVERFLOW_COSTING,
  NCM_HJ_OVERFLOW_COSTING,
  NCM_MAP_CPU_FACTOR,
  NCM_MAP_MSG_FACTOR,
  NCM_MAP_RANDIO_FACTOR,
  NCM_MAP_SEQIO_FACTOR,
  NCM_MJ_TO_HJ_FACTOR,
  NCM_NJ_PC_THRESHOLD,
  // -------------------------------------------------------------------------
  // Threshold above which NJ partial order probe is treated as random order
  // -------------------------------------------------------------------------
  NCM_NJ_PROBES_MAXCARD_FACTOR,
  NCM_NJ_SEQIO_FIX,
  NCM_NUM_SORT_RUNS,
  NCM_PAR_GRPBY_ADJ,
  NCM_PRINT_ROWSIZE,
  NCM_RAND_IO_ROWSIZE_FACTOR,
  NCM_RAND_IO_WEIGHT,
  NCM_SEQ_IO_ROWSIZE_FACTOR,
  NCM_SEQ_IO_WEIGHT,
  NCM_SERIAL_NJ_FACTOR,
  NCM_SGB_TO_HGB_FACTOR,
  NCM_SORT_OVERFLOW_COSTING,
  NCM_TUPLES_ROWSIZE_FACTOR,

  // These CQDs are for Multi-Fragment ESPs
  ESP_MULTI_FRAGMENTS,
  ESP_NUM_FRAGMENTS,
  ESP_MULTI_FRAGMENT_QUOTA_VM,
  EXE_SINGLE_BMO_QUOTA, // Quota system applys to fragments with single BMO

  // Fudge factor for tupleList under NJ cost estimation
  CPUCOST_NJ_TUPLST_FF,

  ////////////////////////////////////////////////////////////////////////
  // this cqd returns showddl output for validate or create phase of 
  // a replicate command.
  // For validate, output is returned without any generated or other names
  // that can vary from one system to another (like constraint name, or
  // location clause, or partition name). 
  // For create, output is returned for 'some' generated names, like that
  // of system created implicit indices or constraints.
  // Used by bulk replicator to validate that the ddl on source 
  // system is exactly the same as that on the target system. This
  // validation is done by matching showddl output of the table on the source 
  // and target systems.
  // Also used by bulk replicator to create the source ddl on the target 
  // system.
  // 
  // Value of 0: this cqd not specified
  // Value of 1: return object showddl(base table, internal system indices)
  //             in 'validate' format.
  // Value of 2: return object showddl(base table, internal system indices)
  //             in 'create' format.
  // Value of 3: return object showddl(base table, all dependent indices)
  //             in 'validate' format.
  // Value of 4: return object showddl(base table, all dependent indices)
  //             in 'create' format.
  // Value of 5: return all dependent objects in 'original' format.
  SHOWDDL_FOR_REPLICATE,



  REPLICATE_ALLOW_ROLES,


  COMP_BOOL_226, // enable UNLOAD feature

  OR_PRED_TO_JUMPTABLE,
  OR_PRED_TO_SEMIJOIN,
  OR_PRED_TO_SEMIJOIN_TABLE_MIN_SIZE,
  OR_PRED_TO_SEMIJOIN_PROBES_MAX_RATIO,


  // Total mem size (MB) for a query
  BMO_MEMORY_LIMIT_PER_NODE_IN_MB, 

  // lower-bound memory limit for BMOs 
  BMO_MEMORY_LIMIT_LOWER_BOUND_HASHJOIN,
  EXE_MEMORY_LIMIT_LOWER_BOUND_MERGEJOIN,
  BMO_MEMORY_LIMIT_LOWER_BOUND_HASHGROUPBY ,
  BMO_MEMORY_LIMIT_LOWER_BOUND_SORT ,

  // lower-bound memory limit for nBMOs 
  EXE_MEMORY_LIMIT_LOWER_BOUND_SEQUENCE ,
  EXE_MEMORY_LIMIT_LOWER_BOUND_EXCHANGE ,

  // Limit CPU time a query can use in master, ESP, or DP2.  Unit is seconds.
  QUERY_LIMIT_SQL_PROCESS_CPU, 
  QUERY_LIMIT_SQL_PROCESS_CPU_DEBUG,
  QUERY_LIMIT_SQL_PROCESS_CPU_XPROD,

  MV_LOG_CLEANUP_SAFETY_FACTOR,
  MV_LOG_CLEANUP_USE_MULTI_COMMIT,
  COMP_BOOL_221,
  OVERFLOW_MODE,


  POS_TEST_NUM_NODES,

  // for default/public schema settings
  PUBLIC_SCHEMA_NAME,
  DEFAULT_SCHEMA_NAMETYPE,
  LDAP_USERNAME,

  // for schema access control
  DEFAULT_SCHEMA_ACCESS_ONLY,



  // To control ESP exchange memory usage
  GEN_EXCHANGE_MAX_MEM_IN_KB,
  GEN_EXCHANGE_MSG_COUNT,

  // Enable PA-DP2 affinity

  // enable explain for ddl and ctas
  DDL_EXPLAIN,


  // the number of ESPs that will be dealing with skew in OCR nested joins
  NESTED_JOINS_ANTISKEW_ESPS,

  //ANSI NULL SEMANTICS CQD
  // hash anti semi join optimization project
  NOT_IN_ANSI_NULL_SEMANTICS,
  NOT_IN_OPTIMIZATION,
  NOT_IN_OUTER_OPTIMIZATION,
  NOT_IN_SKEW_BUSTER_OPTIMIZATION,

  // QUERY STRATEGIZER RELATED
  QUERY_STRATEGIZER,
  EXPLAIN_STRATEGIZER_PARAMETERS,

  CASCADED_GROUPBY_TRANSFORMATION,

  // Use large queues on RHS of Flow/Nested Join when appropriate.
  USE_LARGE_QUEUES ,

  //OLAP CQDs
  //GEN_SEQFUNC_OLAP_BUFFER_SIZE,
  OLAP_BUFFER_SIZE,
  OLAP_MAX_NUMBER_OF_BUFFERS,
  OLAP_CAN_INVERSE_ORDER,
  OLAP_MAX_FIXED_WINDOW_FRAME,
  //used mainly for testing, ignored when set to 0
  //specifies the maximum number of rows per olap buffer
  OLAP_MAX_ROWS_IN_OLAP_BUFFER,
  //aplies for fixed window-- number of additional oplap buffers
  //to allocate on top of the minumum numbers 
  OLAP_MAX_FIXED_WINDOW_EXTRA_BUFFERS,

  // create frequent value list for tuple list if the elements in the list are
  // below this threshold
  HIST_TUPLE_FREQVAL_LIST_THRESHOLD,

// Gen Sol:10-100408-9393
  ALLOW_RISKY_UPDATE_WITH_NO_ROLLBACK,

  // used for controlling multi-join transformation level.
  ASYMMETRIC_JOIN_TRANSFORMATION,

  // CLI caller to redrive CTAS(create table as) for child query monitoring
  REDRIVE_CTAS,

  // type a CASE expression or ValueIdUnion as varchar if its leaves 
  // are of type CHAR of unequal length
  TYPE_UNIONED_CHAR_AS_VARCHAR,

  // enable special features to be used for compatability with certain vendors.
  MODE_SPECIAL_4,

  // toggle display system grants for SHOWDDL
  SHOWDDL_DISPLAY_PRIVILEGE_GRANTS,

  // ***** END REGULAR DEFAULTS *********************************
  
  // Add regular default values (CQDs) before this line.
  // Add session defaults below this line.

  // these defaults are set using SET SESSION DEFAULT statement.
  // They cannot be set using a CQD and are not looked at by mxcmp.
  // But they can be inserted into
  // the defaults table which is read by executor at runtime.
  // ****** BEGIN SET SESSION DEFAULTs ***********************************
  ALTPRI_ESP,
  ALTPRI_MASTER,
  AQR_ENTRIES,
  ESP_ASSIGN_DEPTH,
  ESP_FIXUP_PRIORITY_DELTA,
  ESP_PRIORITY,
  ESP_PRIORITY_DELTA,
  ESP_IDLE_TIMEOUT,
  MASTER_PRIORITY,
  MASTER_PRIORITY_DELTA,
  MXCMP_PRIORITY,
  MXCMP_PRIORITY_DELTA,
  // ****** END SET SESSION DEFAULTs ***********************************

  USTAT_AUTO_EMPTYHIST_TWO_TRANS,
  USTAT_AUTO_MISSING_STATS_LEVEL,
  MAX_EXPRS_USED_FOR_CONST_FOLDING,
  HIST_FETCHCOUNT_SCRATCH_VOL_THRESHOLD,

  EXPLAIN_DISPLAY_FORMAT,
  HIST_NUM_ADDITIONAL_DAYS_TO_EXTRAPOLATE,
  NCM_PAR_ADJ_FACTOR,


  SAP_KEY_NJ_TABLE_SIZE_THRESHOLD,
  SAP_PREFER_KEY_NESTED_JOIN,
  SAP_TUPLELIST_SIZE_THRESHOLD,

  SHOWCONTROL_SHOW_SUPPORT,
  HIST_FREQ_VALS_NULL_FIX,
  HIST_MERGE_FREQ_VALS_FIX,
  HIST_SKEW_COST_ADJUSTMENT,
  USTAT_AUTO_MC_MAX_WIDTH, 
  USTAT_USE_INTERNAL_SORT_FOR_MC,
  USTAT_IS_IGNORE_UEC_FOR_MC,


  NSK_DBG_QUERY_LOGGING_ONLY,
  RISK_PREMIUM_SERIAL_SCALEBACK_MAXCARD_THRESHOLD,

  CAT_DEFAULT_COMPRESSION,
  COMPRESSION_TYPE,

  OCR_FOR_SIDETREE_INSERT,
  UNC_PROCESS,
  USTAT_RETRY_DELAY,
  USTAT_RETRY_LIMIT,
  USTAT_RETRY_NEC_COLS_LIMIT,
  REPLICATE_IO_VERSION,
  COMPRESSED_INTERNAL_FORMAT_ROOT_DOES_CONVERSION,
  COMPRESSED_INTERNAL_FORMAT_BULK_MOVE,

  OR_PRED_ADD_BLOCK_TO_IN_LIST,
  OR_PRED_KEEP_CAST_VC_UCS2,

  // SeaMonster messaging
  SEAMONSTER,
  
  CAT_LIBRARY_PATH_RELATIVE,
  
  MDAM_UNDER_NJ_PROBES_THRESHOLD,

  MDOP_CPUS_PENALTY,
  MDOP_CPUS_SOFT_LIMIT,
  MDOP_MEMORY_PENALTY,
  SORT_INTERMEDIATE_SCRATCH_CLEANUP,

  //CQDs to control CIF for HJ,HGB,Sort and Exchange operators
  COMPRESSED_INTERNAL_FORMAT_BMO,
  DISPLAY_DIVISION_BY_COLUMNS,
  MTD_GENERATE_CC_PREDS,
  GEN_HSHJ_MIN_MAX_OPT,

  COMPRESSED_INTERNAL_FORMAT_MIN_ROW_SIZE,
  COMPRESSED_INTERNAL_FORMAT_ROW_SIZE_ADJ,
  COMPRESSED_INTERNAL_FORMAT_BMO_AFFINITY,
  COMPRESSED_INTERNAL_FORMAT_EXPLAIN,
  COMPRESSED_INTERNAL_FORMAT_DEFRAG_RATIO,


  // Enable the query invalidation processing in catman
  CAT_ENABLE_QUERY_INVALIDATION,
  
  
  MC_SKEW_SENSITIVITY_THRESHOLD, // multi-column skew threshold value

  MTD_MDAM_NJ_UEC_THRESHOLD, // the threshold of the UEC of predicate-less 
                             // leading key columns of a MTD table, above
                             // which the MTD table can be the inner table of 
                             // NJ.
  // this should be used for testing only. DML should not be executed on
  // non-audited tables
  ALLOW_DML_ON_NONAUDITED_TABLE,

  CAT_ALLOW_NEW_FEATUREX,  // to turn on and off features

  DOP_REDUCTION_ROWCOUNT_THRESHOLD, // the threshold below which dop can 
                                    // be reduced

  MC_SKEW_INNER_BROADCAST_THRESHOLD, // MC-SB inner side broadcast threshold
                                     // in bytes

  NESTED_JOIN_CACHE_PREDS,
  //Temporary fix to bypass volatile schema name checking for non-table objects - ALM Case#4764
  BYPASS_CHECK_FOR_VOLATILE_SCHEMA_NAME, 

  //--------------------------------------------------------------------------
  // Query Invalidation - Debug/Regression test CQDs
  //--------------------------------------------------------------------------
  QI_PATH,
  QI_PRIV,

  // The switch to control whether N2J (O(n^2) open) nested joins should
  // be generated. 
  NESTED_JOINS_NO_NSQUARE_OPENS,

  NESTED_JOINS_OCR_GROUPING,   // Hans' fix to check mapped left and right part func. 

  NESTED_JOINS_KEYLESS_INNERJOINS,  // When ON, optimistically allow potential keyless joins 
                                    // when the inner contains joins

  MERGE_JOIN_ACCEPT_MULTIPLE_NJ_PROBES, // merge join accepts multiple NJ probes


  // queries coming in from mariaDB interface
  MARIAQUEST_PROCESS,

  //fast extract related
  FAST_EXTRACT_DIAGS,
  FAST_EXTRACT_IO_BUFFERS,
  FAST_EXTRACT_IO_TIMEOUT_SEC,

  
  REPLICATE_COMPRESSION_TYPE, // Compression type to be used when replicating the DDL
  // only reorg if ReorgCheck indicates that a reorg is needed.
  // explore fully covered NJ plan
  NESTED_JOINS_FULL_INNER_KEY,
  NESTED_JOINS_CHECK_LEADING_KEY_SKEW,
  NESTED_JOINS_LEADING_KEY_SKEW_THRESHOLD,

  // This CQD controls the display of detailed interval information for MC histograms 
  // using showstats output
  USTAT_SHOW_MC_INTERVAL_INFO,

  // query must be this old to be canceled if only PID is specified
  CANCEL_MINIMUM_BLOCKING_INTERVAL,

  // propagate child available Btree indexes from child for arity 1 ops
  GA_PROP_INDEXES_ARITY_1, 
  // FAst extract Routines
  GEN_FE_BUFFER_SIZE,
  GEN_FE_NUM_BUFFERS,
  GEN_FE_SIZE_DOWN,
  GEN_FE_SIZE_UP,
  // WMS query monitoring
  WMS_QUERY_MONITORING,
 //WMS child query monitoring,
  WMS_CHILD_QUERY_MONITORING,

  // size in Mb of the file containing a blob/clob column.
  // size is for all rows for a particular column.
  // Size is expressed in Mbs
  LOB_MAX_SIZE,
  //Max memory used to tranfer data/perform I/O to lob data file. 
  LOB_MAX_CHUNK_MEM_SIZE,
  LOB_STORAGE_TYPE,
  LOB_STORAGE_FILE_DIR,

  LOB_HDFS_SERVER,
  LOB_HDFS_PORT,
  LOB_GC_LIMIT_SIZE,

  LOB_INPUT_LIMIT_FOR_BATCH,
  LOB_LOCKING,
  // Should the DISK POOL be turned on when replicating the DDL using COPY DDL
  REPLICATE_DISK_POOL,

  NESTED_JOINS_PLAN3_TRY_SORT, // try sort when plan2 produces a non-sort plan

  HJ_NEW_MCSB_PLAN, // control the new MCSB feature
  
  USTAT_INCREMENTAL_UPDATE_STATISTICS, // main control switch for 
                                      // incremental update stats (IUS): on / off

  USTAT_INCREMENTAL_FALSE_PROBABILITY,

  USTAT_IUS_INTERVAL_ROWCOUNT_CHANGE_THRESHOLD,
  USTAT_IUS_TOTAL_ROWCOUNT_CHANGE_THRESHOLD,

  USTAT_IUS_INTERVAL_UEC_CHANGE_THRESHOLD,
  USTAT_IUS_TOTAL_UEC_CHANGE_THRESHOLD,

  USTAT_IUS_MAX_NUM_HASH_FUNCS,

  USTAT_IUS_PERSISTENT_CBF_PATH,

  USTAT_IUS_USE_PERIODIC_SAMPLING,

  USTAT_IUS_MAX_TRANSACTION_DURATION,

  USTAT_IUS_MAX_PERSISTENT_DATA_IN_MB,

  USTAT_IUS_MAX_PERSISTENT_DATA_IN_PERCENTAGE,
 
  USTAT_COLLECT_MC_SKEW_VALUES,   // controls MC skew values

  HIST_MIN_MAX_OPTIMIZATION,

  // Maximum number of PCODE Branch Instructions in an Expr
  // for which we will attempt PCODE optimizations.
  PCODE_MAX_OPT_BRANCH_CNT,

  // Maximum number of PCODE Instructions in an Expr
  // for which we will attempt PCODE optimizations.
  PCODE_MAX_OPT_INST_CNT,

  USTAT_USE_GROUPING_FOR_SAMPLING,

  USTAT_IUS_NO_BLOCK,
  
  HIVE_MAX_STRING_LENGTH_IN_BYTES,
  HIVE_USE_FAKE_TABLE_DESC,
  HIVE_LIB_HDFS_PORT_OVERRIDE,
  HIVE_HDFS_STATS_LOG_FILE,
  HIVE_MIN_BYTES_PER_ESP_PARTITION,
  HIVE_NUM_ESPS_PER_DATANODE,
  HIVE_MAX_ESPS,

  // buffer size to do hdfs io. In K bytes. 64M = 65536.
  HDFS_IO_BUFFERSIZE,
  HDFS_USE_CURSOR_MULTI,
  HDFS_PREFETCH,

  HIVE_NUM_ESPS_ROUND_DEVIATION,

  HDFS_REPLICATION,
  HIVE_USE_FAKE_SQ_NODE_NAMES,
  HIVE_LOCALITY_BALANCE_LEVEL,
  HDFS_IO_BUFFERSIZE_BYTES,  // test boundaries.
  HDFS_IO_RANGE_TAIL,
  HIVE_METADATA_REFRESH_INTERVAL,

  HIVE_USE_HASH2_AS_PARTFUNCION,

  MODE_SEAHIVE,
  HIVE_CATALOG,
  HIVE_DEFAULT_SCHEMA,
  HIVE_DEFAULT_CHARSET,

  // size of the DESCRIPTION field of explain output. Default is 3000.
  EXPLAIN_DESCRIPTION_COLUMN_SIZE,
  // size of each row in full explain output. Default is 80.
  EXPLAIN_OUTPUT_ROW_SIZE,

  // to avoid stackoverflow when statement is an insert with a large 
  // tuple list
  EXPLAIN_ROOT_INPUT_VARS_MAX,

  // enable exeutil(explain, reorg, maintain...) output in rwrs mode.
  EXE_UTIL_RWRS,

  MODE_SEABASE,
  SEABASE_CATALOG,
  HBASE_CATALOG,
  //  HBASE_DEFAULT_CHARSET,

  HBASE_SERVER,

  HBASE_MAX_COLUMN_NAME_LENGTH,
  HBASE_MAX_COLUMN_VAL_LENGTH,
  HBASE_MAX_COLUMN_INFO_LENGTH,

  // Hbase silently inserts a duplicate row. 
  // Hbase doesn't tell whether a row got deleted.
  // if set to ON, follow SQL semantics.
  // Return an error when a duplicate row is inserted.
  // Also, return indication whether a row got deleted or not.
  // This requires a check to be made before doing the IUD operation.
  HBASE_SQL_IUD_SEMANTICS,

  HBASE_ROWSET_VSBB_OPT,
  HBASE_UPDEL_CURSOR_OPT,
  HBASE_CHECK_AND_UPDEL_OPT,

  HIVE_SORT_HDFS_HOSTS,
  HBASE_MAX_NUM_SEARCH_KEYS,
  CALL_EMBEDDED_ARKCMP,

  // ON, if blocks should be cached for this scan. This includes subset scans used by Update and Delete 
  HBASE_CACHE_BLOCKS,

  // minimum number of rows to cache for a scan. Default is 100. 
  // This includes subset scans used by Update and Delete
  HBASE_NUM_CACHE_ROWS_MIN,
  // maximum number of rows to cache for a scan. Default is 10000.
  // The actual number of cache rows will be in the range [min, max] and
  // is determined by the cardinality estimate available to the generator.
  // Without stats the size of the cache will be typically min.
  HBASE_NUM_CACHE_ROWS_MAX,
  // if ON, create volatile tables in seabase. Otherwise create them in Seaquest.
  // OFF by default, in closed source. ON by default, in open source.
  SEABASE_VOLATILE_TABLES,

  // During a drop of seabase table or index, the object is first removed from 
  // seabase metadata. If that succeeds, the corresponding hbase object is dropped.
  // if set, this drop of hbase table is done in another worker thread. That speeds up the
  // over drop. If a create of the same table comes in later and an error is returned
  // during create, we delay and retry for a fixed number of times since that table
  // may still be dropped by the worked thread.
  HBASE_ASYNC_DROP_TABLE,

  // if ON, hbase coprocessors could be used, if the query allows it.
  HBASE_COPROCESSORS,

  // if ON, use an HBase coprocessor when estimating row counts; if OFF use
  // client-side code (the latter does not work if HBase encryption is in use)
  HBASE_ESTIMATE_ROW_COUNT_VIA_COPROCESSOR,

  // if OFF or '0' is disabled, ON or '1' is simple pushdown, '2' is for advance pushdown
  // It will depends on the query on which predicates or sub-predicates could be pushed.
  HBASE_FILTER_PREDS,

  // If ON, mark all columns that could be serialized as being declared with
  // SERIALIZED option.
  // This option is currently internal and used for testing without having to change
  // all DDL statements.
  HBASE_SERIALIZATION,


  SQLMX_REGRESS,

  // For PCODE - Native Expressions work
  PCODE_NE_DBG_LEVEL,
   

  HBASE_ZOOKEEPER_PORT,

  // Set to a posotive value to allow mdam scan on table with no stats.
  // Set to 0 to disable the feature.
  //
  // The total number of positions has to be <= the thresold, where
  // The number of positions is computed as
  //
  // #UECs for the first n-1 MDAM columns *
  //    # of ranges for the last MDAM column
  MDAM_NO_STATS_POSITIONS_THRESHOLD,

  HBASE_USE_FAKED_REGIONS,

  // use MC stats to decide on the partitioning function range splits
  HBASE_RANGE_PARTITIONING_MC_SPLIT,

  HBASE_NATIVE_IUD,

  AQR_WNR,
  AQR_WNR_DELETE_NO_ROWCOUNT,
  AQR_WNR_INSERT_CLEANUP,
  AQR_WNR_LOCK_INSERT_TARGET,
  AQR_WNR_EXPLAIN_INSERT,

  MDAM_TRACING,
  NCM_MDAM_COST_ADJ_FACTOR,

  // Include skew info for non-inner join card estimation
  HIST_INCLUDE_SKEW_FOR_NON_INNER_JOIN,

  // By default, the control info table for Maintain operations is not used,
  // but can be enabled via this CQD.
  USE_MAINTAIN_CONTROL_TABLE,


  // if set, then constr validation is skipped during unique, ri or check constr
  // creation. Done if constraints are being created during CREATE TABLE time.
  TRAF_NO_CONSTR_VALIDATION,


  HBASE_MIN_BYTES_PER_ESP_PARTITION,
  HBASE_MAX_ESPS,
  

  // for testing setjmp/longjmp compiler logic
  MEMORY_LIMIT_CMPCTXT_UPPER_KB,
  MEMORY_LIMIT_CMPSTMT_UPPER_KB,
  MEMORY_LIMIT_HISTCACHE_UPPER_KB,
  MEMORY_LIMIT_NATABLECACHE_UPPER_KB,
  MEMORY_LIMIT_QCACHE_UPPER_KB,

  // if set, change blob/clob datatype to varchar. This is needed until blob/clob
  // support is externalized.
  TRAF_BLOB_AS_VARCHAR,
  TRAF_CLOB_AS_VARCHAR,

  // for internal use only.
  // execute a create table statement and create the table with this UID.
  TRAF_CREATE_TABLE_WITH_UID,

  // if set, do not use natable cache. Reload it.
  // Used internally until full Xn support for DDL is in.
  TRAF_RELOAD_NATABLE_CACHE,
  
  TRAF_UPSERT_ADJUST_PARAMS,
  TRAF_UPSERT_WB_SIZE,
  TRAF_UPSERT_WRITE_TO_WAL,
  TRAF_LOAD_PREP_ADJUST_PART_FUNC,
  TRAF_LOAD_PREP_TMP_LOCATION,
  TRAF_LOAD_USE_FOR_INDEXES,
  //log only the overall time of a compilation.
  COMPILE_TIME_MONITOR_LOG_ALLTIME_ONLY,

  TRAF_LOAD_PREP_KEEP_HFILES,

  // if an explicit salt option is not specified in create stmt and this cqd is
  // specified with a value > 0, then that value is used to create the number
  // of salted partitions on primary key columns.
  TRAF_NUM_OF_SALT_PARTNS,
  TRAF_LOAD_PREP_CLEANUP,

  // HDFS root location of backing sample tables.
  TRAF_SAMPLE_TABLE_LOCATION,

  HBASE_SALTED_TABLE_MAX_FILE_SIZE,
  HBASE_SALTED_TABLE_SET_SPLIT_POLICY,

  HBASE_DATA_BLOCK_ENCODING_OPTION,
  HBASE_COMPRESSION_OPTION,
  HBASE_MEMSTORE_FLUSH_SIZE_OPTION,
  HQC_LOG,
  HQC_LOG_FILE, // specify the HQC log file name
  HYBRID_QUERY_CACHE,
  HQC_MAX_VALUES_PER_KEY,
  HQC_CONVDOIT_DISABLE_NUMERIC_CHECK, // TEMPORARY CQD - SHOULD REMOVE

  // if ON limits DoP of Hbase scan to number of partitions
  LIMIT_HBASE_SCAN_DOP,
  TRAF_LOAD_TAKE_SNAPSHOT,
  TRAF_LOAD_PREP_SKIP_DUPLICATES,

  // if not set to UNKNOWN, then this is the charset of a column that is not
  // being declared with an explicit charset.
  TRAF_DEFAULT_COL_CHARSET,

  // if set to ON, then the length specified for a char/varchar datatype is 
  // in characters unless it is explicitly qualified as BYTES or CHARACTERS.
  TRAF_COL_LENGTH_IS_CHAR,

  // if set to on, table will be created in aligned row format.
  // SQ row will be created in aligned format and stored as a single col in hbase.
  TRAF_ALIGNED_ROW_FORMAT,

  // this is used to change cache size of sequence numbers for a session.
  // It overwrites the cache size that was specified during sequence creation.
  TRAF_SEQUENCE_CACHE_SIZE,
 
  // this is used to set the retry time if two concurrent update of sequence
  // conflict, and how many times will retry
  // by default it is 100, when you saw error 1583, you can try to increase
  // this settings
  TRAF_SEQUENCE_RETRY_TIMES,

  TRAF_LOAD_MAX_HFILE_SIZE,


  USTAT_USE_BULK_LOAD,

  // if not null, then add this suffix to the original name and select from that hive table.
  USE_HIVE_SOURCE,

  // if set to ON, then query is not run as part of an enclosing DTM transaction 
  // nor is a transaction started to execute it.
  // It is run using underlying hbase single row transaction consistency.
  TRAF_NO_DTM_XN,

  // if set to ON, then query is run as part of localized region transaction.
  // No external transaction is started to run it.
  TRAF_USE_REGION_XN,

  // HBASE_HASH2_PARTITIONING (ON - OFF for now):
  // - OFF: Treat salted table like a non-salted table
  // - ON: Always use HASH2 for salted table

  // HBASE_RANGE_PARTITIONING (ON):
  // - OFF: Never use HBase ranges for partitioning
  // - ON: Use HBase ranges (stats may override)

  // HBASE_STATS_PARTITIONING (ON):
  // - OFF: Don't use stats split
  // - ON: Use stats split if possible, except for HASH2
  HBASE_HASH2_PARTITIONING,
  HBASE_RANGE_PARTITIONING,
  HBASE_STATS_PARTITIONING,

  // ignore conversion errors when rows are read from hdfs.
  HDFS_READ_CONTINUE_ON_ERROR,

  TRAF_LOAD_USE_FOR_STATS,

  ASG_FEATURE,

  TRAF_UNLOAD_HDFS_COMPRESS,

  NCM_IND_JOIN_COST_ADJ_FACTOR,
  NCM_IND_SCAN_COST_ADJ_FACTOR,
  GROUP_BY_PARTIAL_ROOT_THRESHOLD,
  TRAF_UNLOAD_DEF_DELIMITER,
  TRAF_UNLOAD_DEF_RECORD_SEPARATOR,
  TRAF_LOAD_FORCE_CIF,
  TRAF_ENABLE_ORC_FORMAT,
  TRAF_LOAD_LOG_ERROR_ROWS,
  TRAF_LOAD_MAX_ERROR_ROWS,
  TRAF_LOAD_CONTINUE_ON_ERROR,
  TRAF_LOAD_ERROR_COUNT_ID,
  TRAF_LOAD_ERROR_COUNT_TABLE,
  TRAF_LOAD_ERROR_LOGGING_LOCATION,

  TRAF_TRANS_TYPE, 

  // max size in bytes of a char or varchar column in a trafodion table.
  // Valid values are 0 through MAX_CHAR_COL_LENGTH_IN_BYTES.
  //     (defined in common/ComSmallDefs.h)
  TRAF_MAX_CHARACTER_COL_LENGTH,

  // In special cases, previous default value could be overridden. 
  // Internal use only or use only under trafodion supervision.
  TRAF_MAX_CHARACTER_COL_LENGTH_OVERRIDE,
  // set when metadata definition is to be read from hardcoded structs
  // and not from metadata. 

  // set to limit the number of rows in scanner cache when we have very
  // wide rows. If the rows are too large we may run into an OOM error
  // since weuse HBASE_NUM_CACHE_ROWS_MIN(MAX) CQDs to calculate
  // the number of rows
  TRAF_MAX_ROWSIZE_IN_CACHE,

  TRAF_BOOTSTRAP_MD_MODE,

  UDR_DEBUG_FLAGS,

  // The threshold on the number of rows scanned in an index join scan. 
  // If the estimated value is below the threshold, the index will be 
  // subjected to the elimination heuristics.
  INDEX_ELIMINATION_THRESHOLD,

  NSK_DBG_PRINT_INDEX_ELIMINATION,

  NSK_DBG_COMPILE_INSTANCE,

  NSK_DBG_QUERY_PREFIX,

  EXPLAIN_IN_RMS,
  NCM_HBASE_COSTING,
  NCM_IND_JOIN_SELECTIVITY,
  NCM_IND_SCAN_SELECTIVITY,
  NCM_SKEW_COST_ADJ_FOR_PROBES, 
  PCODE_NE_IN_SHOWPLAN,
  TRAF_TABLE_SNAPSHOT_SCAN,
  TRAF_TABLE_SNAPSHOT_SCAN_TMP_LOCATION,
  TRAF_TABLE_SNAPSHOT_SCAN_SNAP_SUFFIX,
  TRAF_TABLE_SNAPSHOT_SCAN_TIMEOUT,
  HBASE_REGION_SERVER_MAX_HEAP_SIZE,

  TRAF_TABLE_SNAPSHOT_SCAN_TABLE_SIZE_THRESHOLD,

  TRAF_USE_RWRS_FOR_MD_INSERT,


  PCODE_DEBUG_LOGDIR,
  PCODE_EXPR_CACHE_CMP_ONLY,
  PCODE_EXPR_CACHE_DEBUG,
  PCODE_EXPR_CACHE_ENABLED,
  PCODE_NE_ENABLED,
  PCODE_EXPR_CACHE_SIZE,

  // if on, store only the needed explain data. See ExplainTuple::genExplainTupleData.
  EXPLAIN_SPACE_OPT,
  HBASE_ROWSET_VSBB_SIZE,
  // hbase table block size, default is 64KB
  HBASE_BLOCK_SIZE,
  // hbase table index level, when zero, index level will be read from Hfile 
  HBASE_INDEX_LEVEL,

  // enable self referencing foreign key constraints
  TRAF_ALLOW_SELF_REF_CONSTR,
  // enable ESP-RegionServer colocation logic
  TRAF_ALLOW_ESP_COLOCATION,

  // number of hbase versions of rows to be retrieved.
  // -1, get max number of versions.
  // -2, get all versions including those marked for deletion.
  //  N, get N versions. N > 0
  TRAF_NUM_HBASE_VERSIONS,

  // if set, index population step is skipped for external and internal index creates.
  // Should be set as an opt when objects are being
  // created in one session (create table, create index, add constraints, etc).
  // Does a fast check to see if source table is empty to validate.
  TRAF_INDEX_CREATE_OPT, 

  // truncate strings on insert and updates without returning an error.
  TRAF_STRING_AUTO_TRUNCATE,
  // return a warning on truncation.
  TRAF_STRING_AUTO_TRUNCATE_WARNING,
  NCM_UDR_NANOSEC_FACTOR,
  HBASE_ASYNC_OPERATIONS,

  //control lob output size when converting to string/memory 
  LOB_OUTPUT_SIZE,

  TRAF_MULTI_COL_FAM,

  // estimated max naheap memory, used as a limit for update stats utility
  USTAT_NAHEAP_ESTIMATED_MAX,

  EXE_MEMORY_FOR_PROBE_CACHE_IN_MB,
  
  TRAF_INDEX_ALIGNED_ROW_FORMAT,

  UDR_JVM_DEBUG_PORT,
  UDR_JVM_DEBUG_TIMEOUT,
  //enable HBASE Small Scanner, optimizing scans of size below HFile block size
  HBASE_SMALL_SCANNER,

  TRAF_LOAD_ALLOW_RISKY_INDEX_MAINTENANCE,
  HBASE_RANGE_PARTITIONING_PARTIAL_COLS,
  MERGE_WITH_UNIQUE_INDEX,

  USTAT_MAX_CHAR_DATASIZE_FOR_IS,

  // If the next two are 'ON' we use the HBase costing code; if they
  // are 'OFF' we use a stub cost of 1 for Updates and Deletes to
  // Trafodion or HBase tables instead. We'll remove these once the
  // costing code has broader exposure.
  HBASE_DELETE_COSTING,
  HBASE_UPDATE_COSTING,
  TRAF_LOAD_FLUSH_SIZE_IN_KB,

  // if ON, then trafodion views on hive objects are supported.
  HIVE_VIEWS,

  // Specify whic additional restriction check to apply
  //  0: no check
  //  1: apply majority of keys with predicates check
  //  2: apply total UECs on keyless key columns check
  //  3: apply both 1) and 2)
  MDAM_APPLY_RESTRICTION_CHECK,

  // A threshold of total UECs on keyless key columns above which MDAM will not be considered.
  // The threshold is expressed as a percentage of the total RC.
  MDAM_TOTAL_UEC_CHECK_UEC_THRESHOLD,

  // A threshold of minitmal RC above which the above total UEC check will be applied.
  MDAM_TOTAL_UEC_CHECK_MIN_RC_THRESHOLD,

  // A multiplier of cumulative probe cost for MDAM
  MDAM_PROBE_TAX,

  // set to ON to aggressively allocate ESP per core
  AGGRESSIVE_ESP_ALLOCATION_PER_CORE,

  // if ON, use older datetime value constructor in DatetimeValue::DatetimeValue
  // Default is OFF. This cqd is used in case there are problems.
  // It will be removed after testing is complete.
  USE_OLD_DT_CONSTRUCTOR,

  // real charset in the HIVE table
  HIVE_FILE_CHARSET,

  // By default only alter of varchar col length is supported.
  // If this cqd is on, then other alters (name, datatype) are also supported.
  TRAF_ALTER_COL_ATTRS,

  // Controls the behavior of upsert - MERGE, REPLACE, OPTIMAL
  TRAF_UPSERT_MODE,
  // if set, let users create system reserved names. Default is OFF.
  // This cqd should only be used to debug or if system column names are
  // REALLY needed by users.
  // Currently syskey, _salt_, _division_.
  TRAF_ALLOW_RESERVED_COLNAMES,

  // enable support for hbase tables mapped to relational traf tables
  TRAF_HBASE_MAPPED_TABLES,
  TRAF_HBASE_MAPPED_TABLES_IUD,

  //if 0, regular scanner is used. From 0.x to 1.0, percentage of regions that need to be scanned that will be done in parallel.
  //if >= 2, set a fixed number of thread, real DOP. 2.0 2 thread, 3.0 3 thread etc.
  HBASE_DOP_PARALLEL_SCANNER,

  // bitmap to control various special behavior of HIVE_SCAN
  //   // 1 : DOS FORMAT conversion on
  //     // 2 : todo
  HIVE_SCAN_SPECIAL_MODE,

  // if set, data modification check is done at runtime before running
  // a query.
  HIVE_DATA_MOD_CHECK,

  COMPILER_IDLE_TIMEOUT,

  // TINYINT cqds are added until all components can handle this datatype
  // for various actions (listed below).

  // if OFF, tinyint is not supported. It is treated as smallint.
  // if ON,  tinyint is supported as table cols and in cast stmts.
  TRAF_TINYINT_SUPPORT,

  // If ON, tinyint is supported as returned datatype from a select stmt.
  // otherwise returned as smallint.
  TRAF_TINYINT_RETURN_VALUES,

  // if ON, input params are typed as tinyint.
  // Otherwise typed as smallint.
  TRAF_TINYINT_INPUT_PARAMS,

  // if ON, spj input/output params are typed as tinyint.
  // Otherwise typed as smallint.
  TRAF_TINYINT_SPJ_SUPPORT,

  // use info from external table created on this hive table
  HIVE_USE_EXT_TABLE_ATTRS,

  // if 0, datatype error check is not done during inserts into hive tables.
  //       Invalid values may get inserted.
  // if 1, error check done, row is not inserted if conversion error,
  //       and further processing stops.
  // if 2, error check done, row is not inserted if conversion error,
  //       and further processing continues.
  // if 3, null inserted if conversion error, and processing continues.
  HIVE_INSERT_ERROR_MODE,
  
  // If ON, largeint unsigned is supported as returned datatype for a select
  // stmt, and for input params.
  // Otherwise typed as bignum
  TRAF_LARGEINT_UNSIGNED_IO,

  // If ON, boolean is supported as returned datatype for a select
  // stmt, and for input params.
  // Otherwise typed as char
  TRAF_BOOLEAN_IO,

  // if TRUE, create signed numeric literal for both +ve and -ve numbers.
  // if FALSE, create unsigned literal for +ve and signed literal for -ve nums.
  TRAF_CREATE_SIGNED_NUMERIC_LITERAL,


  TRAF_UPSERT_TO_EFF_TREE,

  // if TRUE, create tinyint literal insteadl of smallint.
  TRAF_CREATE_TINYINT_LITERAL,

  // if ON, generate object descriptor during DDL and store it in metadata.
  TRAF_STORE_OBJECT_DESC,

  // if ON, read object descriptor from metadata, if it was stored.
  TRAF_READ_OBJECT_DESC,

  ALLOW_INCOMPATIBLE_OPERATIONS, 

  // if ON: expr and renamed cols allowed in groupby/orderby expr.
  GROUP_OR_ORDER_BY_EXPR,

  // allow ORDER BY in subqueries.
  ALLOW_ORDER_BY_IN_SUBQUERIES,

  // if the schema specified in a create stmt doesn't exist, automatically
  // create it.
  TRAF_AUTO_CREATE_SCHEMA,

  // controls if sim check is to be done and where (root or leaf)
  TRAF_SIMILARITY_CHECK,

  // Common subexpressions in RelExpr trees
  CSE_DEBUG_WARNINGS,
  CSE_FOR_WITH,
  CSE_USE_TEMP,
  CSE_HIVE_TEMP_TABLE,
  CSE_PRINT_DEBUG_INFO,
  CSE_CLEANUP_HIVE_TABLES,
  CSE_CACHE_TEMP_QUERIES,


  // mode for AES_ENCRYPT/AED_DECRYPT
  BLOCK_ENCRYPTION_MODE,

  GROUP_BY_PUSH_TO_BOTH_SIDES_OF_JOIN,

  CSE_TEMP_TABLE_MAX_SIZE,
  CSE_TEMP_TABLE_MAX_MAX_SIZE,
  CSE_COMMON_KEY_PRED_CONTROL,
  CSE_PCT_KEY_COL_PRED_CONTROL,

  TRANSLATE_ERROR,
  TRANSLATE_ERROR_UNICODE_TO_UNICODE,
  INDEX_HINT_WARNINGS,

  // Operations on hive objects also register it in traf OBJECTS metadata table,
  // if not already registered. create external table, grant, upd stats, create
  // views are the current operations that also register hive objects.
  // 
  // This default is used to simulate the 
  // scenario prior to 'hive registration' change. At that time, hive objects
  // were represented by an external table. With this default set, operations
  // on hive could be created without registering them.
  // This default is for internal testing usage only and not externalized.
  HIVE_NO_REGISTER_OBJECTS,
 
  BMO_MEMORY_LIMIT_UPPER_BOUND,
  BMO_MEMORY_ESTIMATE_RATIO_CAP,

  // if OFF, binary/varbinary is not supported. It is treated as char/varchar.
  // if ON,  binary is supported as table cols and in cast stmts.
  // This is needed until binary/varbinary support is fully
  // supported and externalized.
  TRAF_BINARY_SUPPORT,

  // If ON, binary is supported as returned datatype from a select stmt,
  // otherwise returned as char/varchar.
  TRAF_BINARY_OUTPUT,

  // If ON, input params are typed as binary,
  // otherwise typed as char/varchar.
  TRAF_BINARY_INPUT,

  // if ON, spj input/output params are typed as binary.
  // Otherwise typed as char/varchar.
  TRAF_BINARY_SPJ_SUPPORT,

  // if set, cleanse output of explain text by filtering values that
  // may not be deterministic on different systems.
  // Same as explain format: options 'c'
  // Used during dev regressions to cleanse explain output.
  EXPLAIN_OPTION_C,

  // max length of hive binary datatype
  HIVE_MAX_BINARY_LENGTH,

  // Threshold when TOPN sort becomes a regular sort
  GEN_SORT_TOPN_THRESHOLD,

  // Ratio of BMO_MEMORY_LIMIT_PER_NODE_IN_MB that will be divided
  // equally across all BMO operators 
  BMO_MEMORY_EQUAL_QUOTA_SHARE_RATIO,

  EXE_MEMORY_FOR_UNPACK_ROWS_IN_MB,
  MEMORY_LIMIT_ROWSET_IN_MB,

  SUPPRESS_CHAR_LIMIT_CHECK,
 
  // Allow users to grant privileges to roles using the With Grant Option
  ALLOW_WGO_FOR_ROLES,

  BMO_MEMORY_ESTIMATE_OUTLIER_FACTOR,

  // Use the earlier implementation of Hdfs access including LOB via libhdfs
  USE_LIBHDFS,

  // if set, make primary key columns non-nullable. ANSI specification.
  // Default is ON.
  TRAF_MAKE_PKEY_COLUMNS_NOT_NULL,

  // if ON and there are dependent objects on the table, then
  // create unique constraint instead of clustered primary key.
  //
  // Otherwise return error. Users will need to drop dependent objects and
  // then recreate them after adding the primary key.
  // Default is OFF.
  TRAF_ALTER_ADD_PKEY_AS_UNIQUE_CONSTRAINT,

  // if set, do not drop or create hbase objects.
  // Internal cqd. Used during pkey alter/add
  TRAF_NO_HBASE_DROP_CREATE,

  // if set, ddl from Traf interface on Hive objects is supported.
  TRAF_DDL_ON_HIVE_OBJECTS,

  // If set to TRUE, CTAS on Hive object(Create and Insert...select) is processed in Hive.
  // If set to FALSE, Create is done in Hive, and Insert...select is done in Traf.
  // Default is OFF.
  HIVE_CTAS_IN_NATIVE_MODE,

  // Size of byte[] in java when direct byteBuffer can't be used
  // Used to read compressed hdfs text files and to write
  // both compressed and uncompressed hdfs files
  HDFS_IO_INTERIM_BYTEARRAY_SIZE_IN_KB,
  // Use BLOB column in LIBRARIES tables to store libraries.
  USE_LIB_BLOB_STORE,

  // When creating a Trafodion table like a partitioned Hive table
  // use NO NULL attribute for partitin columns
  HIVE_CREATE_TABLE_LIKE_PARTITION_NO_NULL,

  CANCEL_QUERY_ALLOWED,

  // Determines if a transaction needs to be started for select
  // 0 - No transactions for SELECT
  // 1 - Transaction started for SELECT .. FOR UPDATE
  // 2 - Transaction started for SELECT with isolaton level other than skip conflict access 
  BEGIN_TRANSACTION_FOR_SELECT, 
  // This enum constant must be the LAST one in the list; it's a count,
  // not an Attribute (it's not IN DefaultDefaults; it's the SIZE of it)!
  __NUM_DEFAULT_ATTRIBUTES
};


//////////////////////////////////////////////////////////////////////
// options to be used with QUERY_OPTIMIZATION_OPTIONS CQD
//
//////////////////////////////////////////////////////////////////////
enum QueryOptimizationOptions
{
  QO_NO_OPT = 0x0000,
  QO_EXPR_OPT = 0x0001,
  QO_PARAM_NULLABILITY_OPT = 0x0002,
  QO_LEAN_EXEINDP2_FROM_JDBC_OPT = 0x0004,
  QO_LEAN_EXEINDP2_FROM_EMB_OPT = 0x0008,
  QO_LEAN_EXEINMASTER_OPT = 0x0010,
  QO_OLT_WAITED_OPT = 0x0020,
  QO_DO_NT_OLT_OPT = 0x0040
};


// ***************************************************************************
// ***************************************************************************
//
// HAVE YOU READ THE NOTE ABOVE THIS ENUM DECLARATION????
//
// ***************************************************************************
// ***************************************************************************


// Defaults Tokens
// There is a set of keywords which can appear as values of Defaults entries
// in the Defaults Table.  We declare, for each such token, a string (the
// keyword), and an enumeration value.  The string values belong in an
// array, DFkeywords, in sorted order.  The idea is we can use binary
// search in order to obtain the index of a string to the matching
// entry in this sorted array.
//
// If we define the enumerations carefully (pay attention here!), then
// that index we just found (see previous paragraph) is the enum value
// of the token.

// These enums and the strings, below, had *BETTER* be declared in the
// SAME order, and that order *BETTER BE* alphabetical!

// In simple words: this has to be in identical order with enum
// NADefaults::keywords_ in nadefaults.cpp


enum DefaultToken {
 DF_noSuchToken = -1,	// Negative; not in DFkeywords array
 DF_ACCUMULATED,
 DF_ADVANCED,
 DF_AGGRESSIVE,
 DF_ALL,
 DF_ANSI,
 DF_BEGINNER,
 DF_BOTH,
 DF_CLEAR,
 DF_DEBUG,
 DF_DISK,
 DF_DISK_POOL,
 DF_DUMP,
 DF_DUMP_MV,
 DF_EXTERNAL,
 DF_EXTERNAL_DETAILED,
 DF_FIRSTROW,
 DF_HARDWARE,
 DF_HEAP,
 DF_HIGH,
 DF_HYBRID,
 DF_IEEE,
 DF_INDEXES,
 DF_INTERNAL,
 DF_IQS,
 DF_JNI,
 DF_JNI_TRX,
 DF_KEYINDEXES,
 DF_LASTROW,
 DF_LATEST,
 DF_LEAF,
 DF_LOADNODUP,
 DF_LOCAL,
 DF_LOCAL_NODE,
 DF_LOG,
 DF_MAXIMUM,
 DF_MEDIUM,
 DF_MEDIUM_LOW,
 DF_MERGE,
 DF_MINIMUM,
 DF_MMAP,
 DF_MULTI_NODE,
 DF_MVCC,
 DF_NONE,
 DF_OFF,
 DF_ON,
 DF_OPENS_FOR_WRITE,
 DF_OPERATOR,
 DF_OPTIMAL,
 DF_ORDERED,
 DF_PERTABLE,
 DF_PRINT,
 DF_PRIVATE,
 DF_PUBLIC,
 DF_QS,
 DF_READ_COMMITTED,
 DF_READ_UNCOMMITTED,
 DF_RELEASE,
 DF_REMOTE,
 DF_REPEATABLE_READ,
 DF_REPLACE,
 DF_REPSEL,
 DF_RESOURCES,
 DF_RETURN,
 DF_ROOT,
 DF_SAMPLE,
 DF_SERIALIZABLE,
 DF_SHORTANSI,
 DF_SIMPLE,
 DF_SKIP,
 DF_SMD,
 DF_SOFTWARE,
 DF_SOURCE,
 DF_SQLMP,
 DF_SSCC,
 DF_SSD,
 DF_STOP,
 DF_SUFFIX,
 DF_SYSTEM,
 DF_TANDEM,
 DF_THRIFT,
 DF_USER,
 DF_VERTICAL,
 DF_WAIT,
 DF_WARN,
 DF_XML,
 DF_lastToken,
    // Synonyms must NOT be (and are not) present
    // in the DFkeywords array in the .cpp file!
    DF_COMPAQ	= DF_TANDEM,
    DF_DISABLE	= DF_OFF,	// Notice that DISABLE = OFF, but
    DF_ENABLE 	= DF_SYSTEM,	// ENABLE means let the SYSTEM provide the value
    DF_FALSE  	= DF_OFF,
    DF_FULL	= DF_MAXIMUM,
    DF_TRUE   	= DF_ON
};

#endif // DEFAULTCONSTANTS_H
