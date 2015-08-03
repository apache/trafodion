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
* File:         Computations.C
* RCS:          $Id: Computations.cpp,v 1.4.6.2 1998/07/08 21:46:58  Exp $
*                               
* Description:  This file contains the implementation of all member functions
*               of the class Computations. This class is used by sort for 
*               all computations required. Examples of such computations 
*               include : the optimal run size as calculated using Knuth's
*               Square root formula, the optimal merge order, or the optimal
*               scratch block size when memory limitations exist..
*               
* Created:	    07/29/96
* Modified:     $ $Date: 1998/07/08 21:46:58 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
******************************************************************************
*/


// ***************This file is Obsolete ***************



//Update: 10/07/2009
//Below knuth formula is no more valid since run size is not fixed anymore.
//Run sizes vary based on run time memory availability. Also, the Io block 
//size if fixed at 56Kb.

/*------------------------------------------------------------------------
 Using Knuth's square root formula for determining the optimal memory.
 Knuth's Square root formula can be used to determine the minimum amount of
 memory required to perform the sorting and merging using Replacement
 selection with at most one pass. This means that the following two
 inequalities should hold good:


 Run Generation Phase :
 ---------------------
    P * (RS+TS)       <= M      ...................(i)

 Merge Phase :
 -------------
    NumRuns * (TS+SS) <= M      ...................(ii)


 Where 

   M  = <optimalMemory_> = Optimal Memory.
   P  = <runsize_>       = Sort Order/Number of Initial Tree Nodes.
   TS = <treeNodeSize_>  = Tree Node Size.
   RS = Record Size.
   SS = <scrBlockSize_>  = Scratch Buffer Size.

 Also,
   NumRuns = Number of runs generated 
           = Input FileSize/Average RunSize
           = FileSize/2*P*RecSize

 So (ii) can be rewritten as 
   
   FileSize * (TS+SS) / 2*P*RecSize <= M  ....................(iii)


 Multiplying (i) with (iii)

   FileSize*(TS+SS)*(RS+TS)
   ------------------------         <= M*M
         2 

    FileSize*(TS+SS)*(1+TS/RS)      
    --------------------------      <= M*M
         2

   
    SQRT(FileSize*(TS+RS)/2) * SQRT(1+TS/RS) <= M

 RS-->infinity TS/RS --> 0 Therefor use 1.3 as a safe estimate 

  ------------------------------------------
 |   1.3 * SQRT(FileSize*(TS+SS)/2)   <= M  |
  ------------------------------------------

 Conversely,
 
     ------------------------------------------
   |   M >= 1.3 * SQRT(FileSize*(TS+SS)/2)    |
    ------------------------------------------

--------------------------------------------------------------------------*/

//----------------------------------------------------------------------
// Name         : estimateMinMergeMemory
// 
// Parameters   : 
//
// Description  : 
//   
// Return Value :
//   
//----------------------------------------------------------------------


//----------------------------------------------------------------------
// Name         : estimateMemoryToAvoidIntermediateMerge
// 
// Parameters   : 
//
// Description  : 
//   
// Return Value :
//   
//----------------------------------------------------------------------

