/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         AllStmtDDLAlterTable.h
 * Description:  a header file that includes all header files
 *               defining classes relating to Alter Table statements.
 *               
 *               
 * Created:      9/22/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "StmtDDLAlterTable.h"

#include "StmtDDLAddConstraint.h"
#include "StmtDDLAddConstraintArray.h"
#include "StmtDDLAddConstraintCheck.h"
#include "StmtDDLAddConstraintCheckArray.h"
#include "StmtDDLAddConstraintPK.h"
#include "StmtDDLAddConstraintRI.h"
#include "StmtDDLAddConstraintRIArray.h"
#include "StmtDDLAddConstraintUnique.h"
#include "StmtDDLAddConstraintUniqueArray.h"
#include "StmtDDLDropConstraint.h"
#include "StmtDDLAlterTableAddColumn.h"
#include "StmtDDLAlterTableDropColumn.h"
#include "StmtDDLAlterTableAlterColumnLoggable.h" //++ MV
#include "StmtDDLAlterTableDisableIndex.h"
#include "StmtDDLAlterTableEnableIndex.h"
// #include "StmtDDLAlterTableAlterColumn.h"
#include "StmtDDLAlterTableAttribute.h"
#include "StmtDDLAlterTableColumn.h"
#include "StmtDDLAlterTableMove.h"
#include "StmtDDLAlterTablePartition.h"
#include "StmtDDLAlterTableRename.h"
#include "StmtDDLAlterTableNamespace.h"
#include "StmtDDLAlterTableSetConstraint.h"
#include "StmtDDLAlterTableToggleConstraint.h"
#include "StmtDDLAlterTableAlterColumnDefaultValue.h"
#include "StmtDDLAlterTableAlterColumnSetSGOption.h"


//
// End of File
//
