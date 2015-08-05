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
#ifndef ELEMDDLINDEXPOPULATEOPTION_H
#define ELEMDDLINDEXPOPULATEOPTION_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLIndexPopulateoption.h
 * Description:  class for Create Index with Populate and No 
 *               Populate statement. (parser node)
 *
 *
 * Created:      05/29/98
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ElemDDLLoadOptions.h"

class ElemDDLIndexPopulateOption : public ElemDDLLoadOpt
{

public:
      // constructor for the populate option. 
      // The constructor set the option default to be populate, and the check to see if the populate and
      // no populate both exist in one statement. Error will be issue if both populate and no populate 
      // clause exist.
      ElemDDLIndexPopulateOption ()
        :  ElemDDLLoadOpt(ELM_WITH_POPULATE_OPTION_ELEM)
  
      {
	populateCount_ = 0;
	noPopulateCount_= 0;
	populateOption_ = TRUE;
	noPopulateOption_ = FALSE;
      }

      // return 1 if populate is specified. Otherwise, return 0.
      ComBoolean getPopulateOption() { return populateOption_ ;};

      // return 1 if no populate is specified . Otherwise, return 0.
      ComBoolean getNoPopulateOption() { return noPopulateOption_;};

      // chekck to see if the populate clause was specified more than 1.
      Int32 getPopulateOptionCount() { return populateCount_;};

      // check to see if the no populate clause was specified more than 1.
      Int32 getNoPopulateOptionCount() { return noPopulateCount_;} ;

      // set populate option counter, should not be more than 1.
      void setPopulateOptionCount() { populateCount_++;};

      // set no populate option counter, should not be more than 1.
      void setNoPopulateOptionCount() { noPopulateCount_++;};

      // specified the no populate clause.
      void setNoPopulateClause(ComBoolean option) { noPopulateOption_ = option;} ;

      // specified the populate clause.
      void setPopulateClause(ComBoolean option) { populateOption_ = option;} ;

      // cast virtual function.
      virtual ElemDDLIndexPopulateOption * castToElemDDLIndexPopulateOption() { return this;} ;


private:

     ComBoolean populateOption_;
     ComBoolean noPopulateOption_;
     Int32 populateCount_;
     Int32 noPopulateCount_;

};


#endif // ELEMDDLINDEXPOPULATEOPTION_H






