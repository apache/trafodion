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
 * File:         ComUnits.h
 * Description:
 *
 *   This file contains units that can be used when looking at DDL
 *   requests requiring MAXSIZE and ALLOCATE attributes.  This file
 *   will eventually be included as part of ComSmallDefs.h currently
 *   found in CAT subvolume.
 *
 * Created:      10/18/95
 * Modified:     
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#ifndef COMUNITS_H
#define COMUNITS_H

enum ComSizes { COM_PAGE_SIZE_IN_BYTES = 2*1024
              , COM_PAGE_SIZE_IN_MEG = 512
              , COM_ONE_GIG = 1*1024*1024*1024
              , COM_ONE_MEG = 1024*1024
              , COM_ONE_KB  = 1024
              , COM_ONE_BYTE = 1
	      , COM_MAX_PART_SIZE_IN_BYTES = COM_ONE_GIG + (COM_ONE_GIG -1)
	      , COM_MIN_PART_SIZE_IN_BYTES = 20*1024*1024   // 20 Mbytes
              , COM_MAX_PART_SIZE_FOR_FORMAT2_IN_PAGES = 536870400
	      };

// COM_MAX_PART_SIZE_FOR_FORMAT2_IN_PAGES is calculated using this formula: 
//(COM_ONE_MEG*COM_ONE_MEG - COM_ONE_MEG) / COM_PAGE_SIZE_IN_BYTES = 1099510579200 in bytes

enum ComUnits  { COM_UNKNOWN_UNIT
               , COM_BYTES 
               , COM_KBYTES
               , COM_MBYTES
               , COM_GBYTES
               };

#define COM_UNKNOWN_UNIT_LIT  " "
#define COM_BYTES_LIT         " "
#define COM_KBYTES_LIT        "K"
#define COM_MBYTES_LIT        "M"
#define COM_GBYTES_LIT        "G"

enum ComExt { COM_PRI_EXTENT = 16
       	      , COM_SEC_EXTENT = 64
	      , COM_MAX_EXTENT = 160
	      , COM_MAX_MAXEXTENTS = 768
	    };

#endif // COMUNITS_H
