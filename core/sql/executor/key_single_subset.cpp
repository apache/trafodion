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
 * File:         key_single_subset.C
 * Description:  
 *               
 *               
 * Created:      11/11/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "ex_stdh.h"
#include "exp_expr.h"
#include "key_range.h"
#include "key_single_subset.h"
// #include "exp_clause_derived.h"

  

keySingleSubsetEx::keySingleSubsetEx(const keyRangeGen & tdb_key,
				     const short /* in_version */,
				     sql_buffer_pool *pool,
				     ex_globals *g,
				     unsigned short mode,
                                     const ex_tcb *tcb) :
     keyRangeEx(tdb_key,g,1,pool),
     keyReturned_(FALSE)
{
  Space * space = g->getSpace();
  CollHeap *heap = g->getDefaultHeap();

  // mode = 0;  // 0 = PCODE_NONE
  if (bkPred())
    (void) bkPred()->fixup(0, mode, tcb, space, heap, g->computeSpace(), g);
  if (ekPred())
    (void) ekPred()->fixup(0, mode, tcb, space, heap, g->computeSpace(), g);
  if (bkExcludedExpr())
    (void) bkExcludedExpr()->fixup(0, mode, tcb, space, heap, g->computeSpace(), g);
  if (ekExcludedExpr())
    (void) ekExcludedExpr()->fixup(0, mode, tcb, space, heap, g->computeSpace(), g);
};

keySingleSubsetEx::~keySingleSubsetEx()
{ };


ExeErrorCode keySingleSubsetEx::initNextKeyRange(sql_buffer_pool *,atp_struct *)
{
  // note that the parameters are not used by this class
  keyReturned_ = FALSE;
  return (ExeErrorCode)0;
};

keyRangeEx::getNextKeyRangeReturnType keySingleSubsetEx::getNextKeyRange(
     atp_struct *atp0,NABoolean /* fetchRangeHadRows */,
     NABoolean detectNullRange)
{
  getNextKeyRangeReturnType rc = NO_MORE_RANGES;  // assume no more keys
  
  if (!keyReturned_)
    {
      // for capturing errors - initialize assuming success
      ex_expr::exp_return_type rcbk = ex_expr::EXPR_OK;
      ex_expr::exp_return_type rcbkef = ex_expr::EXPR_OK;
      ex_expr::exp_return_type rcek = ex_expr::EXPR_OK;
      ex_expr::exp_return_type rcekef = ex_expr::EXPR_OK;

      // for saving data conversion error flags
      Lng32 bkConvErrorFlag = 0;  // s/b exp_conv_clause::CONV_RESULT_OK
      Lng32 ekConvErrorFlag = 0;  // s/b exp_conv_clause::CONV_RESULT_OK
      
      if(bkPred())
	{
          dataConvErrorFlag_ = 0;  // The zero hard-coded here should be
                                   // ex_conv_clause::CONV_RESULT_OK in
                                   // file exp/exp_clause_derived.h.
	  workAtp_->getTupp(tdbKey_.getKeyValuesAtpIndex()).setDataPointer(
	       ((tdbKey_.getKeytag() > 0) ? bkData_.getDataPointer() + sizeof(short) 
		: bkData_.getDataPointer()));
	  if (tdbKey_.getKeytag() > 0)
	    {
	      unsigned short keytag = tdbKey_.getKeytag();
	      str_cpy_all(bkData_.getDataPointer(), (char*)&keytag, sizeof(short));
	    }

	  if((rcbk = bkPred()->eval(atp0,workAtp_)) == ex_expr::EXPR_ERROR)
                                       return EXPRESSION_ERROR;

          bkConvErrorFlag = dataConvErrorFlag_;  // remember conv error flag
	  
	  if (bkExcludedExpr())
	    {
	      if((rcbkef = bkExcludedExpr()->eval(atp0,workAtp_)) 
                                == ex_expr::EXPR_ERROR) return EXPRESSION_ERROR;
	      bkExcludeFlag_ = (short) getExcludeFlagValue();
	    }
	  else
	    {
	      bkExcludeFlag_ = isBkeyExcluded();
	    }
	};
      
      if(ekPred())
	{
          dataConvErrorFlag_ = 0;  // The zero hard-coded here should be
                                   // ex_conv_clause::CONV_RESULT_OK in
                                   // file exp/exp_clause_derived.h.
	  workAtp_->getTupp(tdbKey_.getKeyValuesAtpIndex()).setDataPointer(
	       ((tdbKey_.getKeytag() > 0) ? ekData_.getDataPointer() + sizeof(short) 
		: ekData_.getDataPointer()));
	  if (tdbKey_.getKeytag() > 0)
	    {
	      unsigned short keytag = tdbKey_.getKeytag();
	      str_cpy_all(ekData_.getDataPointer(), (char*)&keytag, sizeof(short));
	    }

	  if((rcek = ekPred()->eval(atp0,workAtp_)) == ex_expr::EXPR_ERROR)
                                                   return EXPRESSION_ERROR;

	  ekConvErrorFlag = dataConvErrorFlag_;  // remember conv error flag

	  if (ekExcludedExpr())
	    {
	      if((rcekef = ekExcludedExpr()->eval(atp0,workAtp_)) 
                               == ex_expr::EXPR_ERROR) return EXPRESSION_ERROR;
	      ekExcludeFlag_ = (short) getExcludeFlagValue();
	    }
	  else
	    {
	      ekExcludeFlag_ = isEkeyExcluded();
	    }
	};


      // $$$ code here for debugging purposes -- to make it easier
      // to see the begin and end key values

      char *bktarget = bkData_.getDataPointer();
      char *ektarget = ekData_.getDataPointer();

      // $$$ end code

      // check for data conversion errors and modify exclude flags and rc
      // accordingly

      rc = FETCH_RANGE;  // assume we got a range

      if (ekPred()) // if there is both a begin key and an end key
        {
          switch (bkConvErrorFlag)
            {
              case -1:  // s/b ex_conv_clause::CONV_RESULT_ROUNDED_DOWN
              case -2:  // s/b ex_conv_clause::CONV_RESULT_ROUNDED_DOWN_TO_MAX
                {
                  // the begin key value was rounded down, so we must
                  // exclude it from the key range
                  bkExcludeFlag_ = 1;
                  break;
                }
              case 1:  // s/b ex_conv_clause::CONV_RESULT_ROUNDED_UP
              case 2:  // s/b ex_conv_clause::CONV_RESULT_ROUNDED_UP_TO_MIN
                {
                  // the begin key value was rounded up, so we must
                  // include it in the key range
                  bkExcludeFlag_ = 0;
                  break;
                }
              case 3:  // s/b ex_conv_clause::CONV_RESULT_FAILED
                {
                  // the begin key value could not be computed, so no
                  // rows will qualify in the key range
                  rc = NO_MORE_RANGES;
                }
              default:  // must be ex_conv_clause::CONV_RESULT_OK
                {
                  // leave bkExcludeFlag_ as is
                }
            }

          switch (ekConvErrorFlag)
            {
              case -1:  // s/b ex_conv_clause::CONV_RESULT_ROUNDED_DOWN
              case -2:  // s/b ex_conv_clause::CONV_RESULT_ROUNDED_DOWN_TO_MAX
                {
                  // the end key value was rounded down, so we must
                  // include it in the key range
                  ekExcludeFlag_ = 0;
                  break;
                }
              case 1:  // s/b ex_conv_clause::CONV_RESULT_ROUNDED_UP
              case 2:  // s/b ex_conv_clause::CONV_RESULT_ROUNDED_UP_TO_MIN
                {
                  // the end key value was rounded up, so we must 
                  // exclude it from the key range
                  ekExcludeFlag_ = 1;
                  break;
                }
              case 3:  // s/b ex_conv_clause::CONV_RESULT_FAILED
                {
                  // the end key value could not be computed, so no
                  // rows will qualify in the key range
                  rc = NO_MORE_RANGES;
                }
              default:  // must be ex_conv_clause::CONV_RESULT_OK
                {
                  // leave bkExcludeFlag_ as is
                }
            }

	  // if a range was found, then detect if this is a NULL range.
	  // Return NO_MORE_RANGES in this case.
	  if (rc == FETCH_RANGE)
	    {
	      // A null subset is detected if:
	      //   begin key > end key, or
	      //   begin key == end key and either of the excluded
	      //    flags was set.
	      // 
	      // str_cmp returns -1, 0, 1 for <, =, >
	      Int32 cmpCode = str_cmp(bktarget, ektarget, 
				      tdbBeginEndKey().getKeyLength());
	      if (((detectNullRange) &&
		   (cmpCode > 0)) || // begin key > end key
		  ((cmpCode == 0) && 
		   ((bkExcludeFlag_ == 1) ||
		    (ekExcludeFlag_ == 1))))
		{
		  // this is a null range.
		  rc = NO_MORE_RANGES;
		}
	    } // a range was found
        }
      else  // must be only a begin key (which means this is unique access)
        {
        // For unique access, it must be the case that all the predicates
        // are equality. If there was any data conversion error, no rows
        // will qualify. So, treat the error as if there were no more ranges.
        if (bkConvErrorFlag != 0)  // 0 s/b ex_conv_clause::CONV_RESULT_OK
          rc = NO_MORE_RANGES;
        }
      
    };

  // insure that we return at most one key range (per call to
  // initNextKeyRange())
  keyReturned_ = TRUE;

  return rc;
};



