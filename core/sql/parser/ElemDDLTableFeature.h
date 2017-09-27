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
#ifndef ELEMDDLTABLEFEATURE_H
#define ELEMDDLTABLEFEATURE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLTableFeature.h
 * Description:  Describes table options: [NOT] DROPPABLE & INSERT_ONLY
 *
 *               
 * Created:      04/02/2012
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h"


class ElemDDLTableFeature : public ElemDDLNode
{

public:

	ElemDDLTableFeature( ComTableFeature tableFeature)
	  : ElemDDLNode(ELM_TABLE_FEATURE_ELEM),
	  tableFeature_(tableFeature)
	{

	}


	// virtual destructor
	virtual ~ElemDDLTableFeature(){}

	// cast
	virtual ElemDDLTableFeature* castToElemDDLTableFeature(){return this;}
	virtual ComTableFeature getTableFeature() const {return tableFeature_;}
        const NABoolean isDroppable() const 
           { return ((tableFeature_ == COM_DROPPABLE) ||
                     (tableFeature_ == COM_DROPPABLE_INSERT_ONLY)); }

        const NABoolean isInsertOnly() const
           { return ((tableFeature_ == COM_DROPPABLE_INSERT_ONLY) ||
                     (tableFeature_ == COM_NOT_DROPPABLE_INSERT_ONLY)); }

	// methods for tracing
	virtual const NAString displayLabel1() const;
	virtual const NAString getText() const;


private:

	ComTableFeature tableFeature_;

}; // class ElemDDLTableFeature



#endif // ELEMDDLTABLEFEATURE_H
