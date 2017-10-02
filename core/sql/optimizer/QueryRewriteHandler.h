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
#ifndef QUERYREWERITEHANDLER_H
#define QUERYREWERITEHANDLER_H
/* -*-C++-*-
 **************************************************************************
 *
 * File:         QueryRewriteHandler.h
 * Description:  MvQueryRewriteHandler class and methods
 * Created:      02/14/2008
 * Language:     C++
 *
 **************************************************************************
 */

#include "Analyzer.h"
#include "XMLUtil.h"

class QRMVDescriptor;
class QRXmlMessageObj;

//============================================================================
class MvQueryRewriteHandler : public NABasicObject
{ 
public:
  MvQueryRewriteHandler(CollHeap* heap)
    : mvName_(heap),
      mvDescriptor_(NULL),
      formatXml_(TRUE),
      warningMessage_(heap)
  {}

  // Methods called by CatMan during CREATE MV
  QRMVDescriptorPtr getMvDescriptor() { return mvDescriptor_; }

  // Methods called by the Analyzer
  void createMvDescriptor(QueryAnalysis* qa, RelExpr* expr, NAString& warningMessage);
  RelExpr* handleMvQueryRewrite(QueryAnalysis* qa, RelExpr* expr, NAString& warningMessage);

  void dumpToFile(const char* fileName, const char* data);
  //void dumpAnalysisToFile(QueryAnalysis* qa, RelExpr* expr);

  /**
   * Determines whether query rewrite is worth pursuing for the current query.
   * This decision is based on the nature and complexity of the query.
   *
   * @param rootExpr The pointer to the query root node.
   * @return \c TRUE iff the query appears likely to benefit from looking for
   *         MVs to use in rewriting it.
   */
  static NABoolean rewriteWorthTrying(RelRoot* rootExpr);

  NABoolean isMvqrSupportedForMV(QueryAnalysis* qa, RelExpr* expr, NAString& warningMessage);
  
  NAString& getWarningMessage() 
  { 
    return warningMessage_; 
  }

protected:
  RelRoot* handleAnalyzeOnlyQuery(RelRoot* rootExpr, NAString* xmlText);
  RelRoot* handleAnalyzeOnlyQuery(RelRoot* rootExpr, const char* excuse);

private:
  // Details about the MV, to be written into the descriptor
  NAString      mvName_;
  NABoolean     isIncremental_;
  NABoolean     isImmediate_;

  QRMVDescriptorPtr mvDescriptor_;
  NABoolean     formatXml_;
  XMLString*    xmlText_;
  NAString      warningMessage_;
};  // MvQueryRewriteHandler

#endif // QUERYREWERITEHANDLER_H
