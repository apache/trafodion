/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
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
****************************************************************************
*
* File:         SequenceGeneratorAttributes.cpp
* Description:  The attributes of the sequence generator
* Created:      
* Language:     C++
*
****************************************************************************/

#include "SequenceGeneratorAttributes.h"
#include "ComSpace.h"

void SequenceGeneratorAttributes::genSequenceName(const NAString &catName, const NAString &schName, 
                                                  const NAString &tabName, const NAString &colName,
                                                  NAString &seqName)
{
  seqName = "_" + catName + "_" + schName + "_" + 
    tabName + "_" + colName + "_" ;
}

const void SequenceGeneratorAttributes::display 
(ComSpace *space, NAString * nas, NABoolean noNext) const
{
  char buf[10000];

  if (noNext)
    sprintf(buf, "  START WITH %li", 
            getSGStartValue());
  else
    sprintf(buf, "  START WITH %li /* NEXT AVAILABLE VALUE %li */", 
            getSGStartValue(), getSGNextValue());
    
  if (nas)
    *nas += buf;
  else
    space->allocateAndCopyToAlignedSpace(buf, strlen(buf), sizeof(short));

  sprintf(buf, "  INCREMENT BY %li", 
          getSGIncrement());
  if (nas)
    *nas += buf;
  else
    space->allocateAndCopyToAlignedSpace(buf, strlen(buf), sizeof(short));

  if (getSGMaxValue() == 0)
    sprintf(buf, "  NO MAXVALUE");
  else
    sprintf(buf, "  MAXVALUE %li", 
            getSGMaxValue());
  if (nas)
    *nas += buf;
  else
    space->allocateAndCopyToAlignedSpace(buf, strlen(buf), sizeof(short));
  
  if (getSGMinValue() == 0)
    sprintf(buf, "  NO MINVALUE");
  else
    sprintf(buf, "  MINVALUE %li", 
            getSGMinValue());
  if (nas)
    *nas += buf;
  else
    space->allocateAndCopyToAlignedSpace(buf, strlen(buf), sizeof(short));

  if (getSGCache() == 0)
    sprintf(buf, "  NO CACHE");
  else
    sprintf(buf, "  CACHE %li", 
            getSGCache());
  if (nas)
    *nas += buf;
  else
    space->allocateAndCopyToAlignedSpace(buf, strlen(buf), sizeof(short));

  if (getSGCycleOption())
    sprintf(buf, "  CYCLE");
  else
    sprintf(buf, "  NO CYCLE");
  if (nas)
    *nas += buf;
  else
    space->allocateAndCopyToAlignedSpace(buf, strlen(buf), sizeof(short));

  if (getSGFSDataType() == COM_UNSIGNED_BIN16_FSDT)
      sprintf(buf, "  SMALLINT UNSIGNED ");
  else if (getSGFSDataType() == COM_UNSIGNED_BIN32_FSDT)
      sprintf(buf, "  INT UNSIGNED ");
  else
    sprintf(buf, "  LARGEINT ");
  if (nas)
    *nas += buf;
  else
    space->allocateAndCopyToAlignedSpace(buf, strlen(buf), sizeof(short));
}

