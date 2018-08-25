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
 * File:         ExpSeqGen.cpp
 * Description:  
 *               
 *               
 * Created:      7/20/2014
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "SQLCLIdev.h"
#include "ExpSeqGen.h"

//**************************************************************************
// class SeqGenEntry
//**************************************************************************
SeqGenEntry::SeqGenEntry(Int64 sgUID, CollHeap * heap)
  : heap_(heap),
    sgUID_(sgUID)
{
  fetchNewRange_ = TRUE;
  cliInterfaceArr_ = NULL;
  retryNum_ = 100; //default retry times
}

short SeqGenEntry::fetchNewRange(SequenceGeneratorAttributes &inSGA)
{
  Lng32 cliRC = 0;

  // fetch new range from Seq Generator database
  SequenceGeneratorAttributes sga;
  sga = inSGA;
  if (sga.getSGCache() == 0)
    sga.setSGCache(1); 

  sga.setSGRetryNum(getRetryNum());
  cliRC = SQL_EXEC_SeqGenCliInterface(&cliInterfaceArr_, &sga);
  if (cliRC < 0)
    return (short)cliRC;

  cachedStartValue_ = sga.getSGNextValue();
  cachedCurrValue_ = cachedStartValue_;
  cachedEndValue_ = sga.getSGEndValue();

  if (cachedStartValue_ > sga.getSGMaxValue())
    {
      return -1579; // max reached
    }

  fetchNewRange_ = FALSE;

  return 0;
}

short SeqGenEntry::getNextSeqVal(SequenceGeneratorAttributes &sga, Int64 &seqVal)
{
  short rc = 0;

  if (NOT fetchNewRange_)
    {
      cachedCurrValue_ += sga.getSGIncrement();
      if (cachedCurrValue_ > cachedEndValue_)
	fetchNewRange_ = TRUE;
    }

  if (fetchNewRange_)
    {
      rc = fetchNewRange(sga);
      if (rc)
	return rc;
    }

  seqVal = cachedCurrValue_;

  return 0;
}

short SeqGenEntry::getCurrSeqVal(SequenceGeneratorAttributes &sga, Int64 &seqVal)
{
  short rc = 0;

  if (fetchNewRange_)
    {
      rc = fetchNewRange(sga);
      if (rc)
	return rc;
    }

  seqVal = cachedCurrValue_;
  
  return 0;
}

SequenceValueGenerator::SequenceValueGenerator(CollHeap * heap)
  : heap_(heap)
{
  sgQueue_ = new(heap_) HashQueue(heap);
}

SeqGenEntry * SequenceValueGenerator::getEntry(SequenceGeneratorAttributes &sga)
{
  Int64 hashVal = sga.getSGObjectUID().get_value();

  sgQueue()->position((char*)&hashVal, sizeof(hashVal));

  SeqGenEntry * sge = NULL;
  while ((sge = (SeqGenEntry *)sgQueue()->getNext()) != NULL)
    {
      if (sge->getSGObjectUID() == hashVal)
	break;
    }

  if (! sge)
    {
      sge = new(getHeap()) SeqGenEntry(hashVal, getHeap());
      sgQueue()->insert((char*)&hashVal, sizeof(hashVal), sge);
    }

  sge->setRetryNum(getRetryNum());

  return sge;
}

short SequenceValueGenerator::getNextSeqVal(SequenceGeneratorAttributes &sga,
					    Int64 &seqVal)
{

  SeqGenEntry * sge = getEntry(sga);
  return sge->getNextSeqVal(sga, seqVal);
}

short SequenceValueGenerator::getCurrSeqVal(SequenceGeneratorAttributes &sga,
					    Int64 &seqVal)
{

  SeqGenEntry * sge = getEntry(sga);
  return sge->getCurrSeqVal(sga, seqVal);
}

