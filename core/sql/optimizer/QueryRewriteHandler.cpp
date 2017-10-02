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
 **************************************************************************
 *
 * File:         QueryRewriteHandler.cpp
 * Description:  MvQueryRewriteHandler methods
 * Created:      02/14/2008
 * Language:     C++
 *
 **************************************************************************
 */

#include "QueryRewriteHandler.h"
#include "Analyzer.h"
#include "NormWA.h"
#include "QRDescGenerator.h"
#include "MVCandidates.h"
#include "QueryRewriteServer.h"

// -----------------------------------------------------------------------
void MvQueryRewriteHandler::createMvDescriptor(QueryAnalysis* qa, RelExpr* expr, NAString& warningMessage)
{
  CollHeap *heap = CmpCommon::statementHeap();
  QRDescGenerator descGenerator(formatXml_, heap); // Work on the statement heap.
  mvDescriptor_ = NULL;

  if (isMvqrSupportedForMV(qa, expr, warningMessage) == FALSE)
    return;
  
  try
  {
    mvDescriptor_ = descGenerator.createMvDescriptor(qa, expr);
  }
  catch (QRDescriptorException& ex)
  {
    QRLogger::log(CAT_SQL_COMP_QR_HANDLER, LL_MVQR_FAIL,
      "A QRDescriptorException occurred while generating MV descriptor: %s", ex.getMessage());
    warningMessage = "Internal error.";
  }
  catch (...)
  {
    QRLogger::log(CAT_SQL_COMP_QR_HANDLER, LL_MVQR_FAIL,
      "An Unknown exception occurred while generating MV descriptor.");
    warningMessage = "Unknown internal error.";
  }
}

// -----------------------------------------------------------------------
NABoolean MvQueryRewriteHandler::isMvqrSupportedForMV(QueryAnalysis* qa, RelExpr* expr, NAString& warningMessage)
{
  if (qa->getJBBs().entries() > 1)
  {
    warningMessage = "Only single JBB queries are supported.";
    return FALSE;
  }

  if (qa->skippedSomeJoins())
  {
  	warningMessage = "Some joins are not supported.";
    return FALSE;
  }

 	if (expr->child(0)->getOperatorType()           == REL_GROUPBY &&
 		  expr->child(0)->child(0)->getOperatorType() == REL_GROUPBY)
  {
  	warningMessage = "Cascaded GROUP BY is not supported.";
    return FALSE;
 	} 	
  
  return TRUE;
}

/**
 * Parses an XML document, which should be one of the three Query Rewrite
 * descriptor types, and builds a hierarchy of classes representing the
 * elements of the document.
 *
 * @param xmlText Text of the descriptor to be parsed.
 * @param xmlLen  Length of the descriptor text.
 * @param[out] descriptor Pointer to class instance representing the document
 *                        element of the parsed document.
 * @return Status indicator.
 */
static QRRequestResult parseXML(char* xmlText, Int32 xmlLen,
                                XMLElementPtr &descriptor)
{
  try
    {
      QRElementMapper em;
      XMLDocument doc = XMLDocument(STMTHEAP, em);
      descriptor = doc.parse(xmlText, xmlLen, TRUE);

      if (!descriptor)
        {
          QRLogger::log(CAT_SQL_COMP_QR_HANDLER, LL_MVQR_FAIL,
            "XMLDocument.parse() returned NULL.");
          return XMLParseError;
        }
      else
        ;//debugMessage("Parsed XML document successfully.");
    }
  catch (XMLException& ex)
    {
      QRLogger::log(CAT_SQL_COMP_QR_HANDLER, LL_MVQR_FAIL,
        "An XMLException occurred: %s", ex.getMessage());
      return XMLParseError;
    }
  catch (QRDescriptorException& ex)
    {
      QRLogger::log(CAT_SQL_COMP_QR_HANDLER, LL_MVQR_FAIL,
        "A QRDescriptorException occurred: %s", ex.getMessage());
      return XMLParseError;
    }
  catch (...)
    {
      QRLogger::log(CAT_SQL_COMP_QR_HANDLER, LL_MVQR_FAIL,
        "An Unknown exception occurred");
      return InternalError;
    }

  return Success;
}

NABoolean MvQueryRewriteHandler::rewriteWorthTrying(RelRoot* rootExpr)
{
  QueryAnalysis* qa = QueryAnalysis::Instance();

  // Don't bother if this is an INTERNAL REFRESH.
  if (rootExpr->isRootOfInternalRefresh())
    return FALSE;

  // @ZX: For now, just check for null result from getLargestTable() until
  //      OptDefaults instance is computed earlier.
  if (!qa->getLargestTable())
    return FALSE;
  //if (CURRSTMT_OPTDEFAULTS->getNumTables() == 0)
  //  return FALSE;

  if (CURRSTMT_OPTDEFAULTS->optLevel() < OptDefaults::MEDIUM)
    {
      QRLogger::log(CAT_SQL_COMP_QR_HANDLER, LL_INFO,
        "Query rewrite skipped due to optimization level");
      return FALSE;
    }

  // We also need some sort of complexity check, to avoid the overhead of query
  // rewrite for simple or inexpensive queries. It has been suggested to use
  // the resource consumption estimates used for Adaptive Segmentation, but
  // those are calculated at the beginning of the optmization phase, after the
  // end of query analysis when QR is invoked.

  return TRUE;
}

RelExpr* MvQueryRewriteHandler::handleMvQueryRewrite(QueryAnalysis* qa,
                                                     RelExpr* expr,
                                                     NAString& warningMessage)
{
  // Allocate a temporary heap for MV query rewrite
  // This heap will self-destruct when it goes out of scope at the end of this method.
  NAHeap mvqrHeap("Heap for MV Query Rewrite",
  		  NAMemory::DERIVED_FROM_SYS_HEAP,
		  (Lng32)32768);

  CMPASSERT(expr->getOperatorType() == REL_ROOT);
  RelRoot* rootExpr = static_cast<RelRoot*>(expr);

  // For AnalyzeOnly queries, force the XML to be formatted.
  NABoolean formatXml = formatXml_;
  if (rootExpr->isAnalyzeOnly())
    formatXml = true;
    
  // Do step 0 checks here.
  // ============================
  // If there does not appear to be any likely benefit from applying query
  // rewrite, just return the original RelExpr.
  if (!rewriteWorthTrying(rootExpr))
  {
    warningMessage = "Query too simple.";
    if (rootExpr->isAnalyzeOnly())
      rootExpr = handleAnalyzeOnlyQuery(rootExpr, warningMessage);
    return rootExpr;
  }
  
  NAString MVName = "";
  CmpCommon::getDefault(MVQR_WORKLOAD_ANALYSIS_MV_NAME, MVName);
  if (MVName != "")
  {
  	// Add a log marker with query/MV name.
    QRLogger::log(CAT_SQL_COMP_QR_DESC_GEN, LL_INFO, "Log marker for query: %s", MVName.data());
  }

  // Create the query descriptor
  // (or an MV descriptor if we are in workload analysis mode).
  // ============================
  xmlText_ = NULL;
  QRDescriptorPtr requestDescriptor = NULL;
  QRDescGenerator descGenerator(formatXml, &mvqrHeap);
  if (rootExpr->isAnalyzeOnly() &&
      CmpCommon::getDefault(MVQR_LOG_QUERY_DESCRIPTORS) == DF_DUMP_MV)
  {
    // Used for generating MV descriptors for queries in workload analysis mode.
    descGenerator.setDumpMvMode();
  }
    
  try
  {
    if (descGenerator.isDumpMvMode())
    {
      QRMVDescriptorPtr mvDesc = descGenerator.createMvDescriptor(qa, rootExpr);
      if (mvDesc)
      {
        requestDescriptor = mvDesc;
        mvDesc->setMvName(MVName, &mvqrHeap);
        mvDesc->getMisc()->setFromQuery(TRUE);
      }
    }
    else
    {
      requestDescriptor = descGenerator.createQueryDescriptor(qa, rootExpr);
    }
    xmlText_ = (requestDescriptor
                   ? descGenerator.createXmlText(requestDescriptor)
                   : NULL);
  }
  catch(QRDescriptorException e)
  {
    // Just ignore it and leave xmlText_ as NULL to skip the rest of this method.
    QRLogger::log(CAT_SQL_COMP_QR_HANDLER, LL_MVQR_FAIL,
      "DescriptorException thrown: %s, %s.", e.getMessage(), MVName.data());
    if (rootExpr->isAnalyzeOnly())
      rootExpr = handleAnalyzeOnlyQuery(rootExpr, e.getMessage());
    warningMessage = e.getMessage();
    return rootExpr;
  }
  catch(...)
  {
    // This exception was not yet logged.
    // Log it and then skip.
    QRLogger::log(CAT_SQL_COMP_QR_HANDLER, LL_MVQR_FAIL,
      "Unknown exception thrown during descriptor generation, %s.", MVName.data());
    if (rootExpr->isAnalyzeOnly())
      rootExpr = handleAnalyzeOnlyQuery(rootExpr, "Unknown exception thrown");
    warningMessage = "Internal error";
    return rootExpr;
  }

  if (xmlText_ == NULL)
  {
    if (rootExpr->isAnalyzeOnly())
      rootExpr = handleAnalyzeOnlyQuery(rootExpr, "No descriptor generated");
  }
  else
  {
    if (CmpCommon::getDefault(MVQR_LOG_QUERY_DESCRIPTORS) == DF_LOG)
    {
      // Dump the query descriptor to a file.
      //NAString queryDescFileName = fileNamePrefix_ + ".qd.xml";
      //dumpToFile(queryDescFileName.data(), xmlText_->data()); 
    }

    if (rootExpr->isAnalyzeOnly())
    {
      // If this is an AnalyzeOnly query - handle that now.
      // ====================================================
      qa->cleanup(rootExpr);
      RelExpr* result = handleAnalyzeOnlyQuery(rootExpr, xmlText_);    	
      delete xmlText_;
      return result;
    }
    else
    {
      try
      {
        // For R2.4 delivery only, don't go looking for a QMS process
        // unless we are going to publish to it as well.
        DefaultToken publishDest = CmpCommon::getDefault(MVQR_PUBLISH_TO);
        if (publishDest != DF_NONE)
        {
          // Do QMM ALLOCATE protocol here.
          // For now, each mxcmp uses its own private qms.
          // ============================
          // Create server process if it doesn't already exist.
          IpcServer* qms = MvQueryRewriteServer::getQmsServer(publishDest);
          if (!qms)
          {
            QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_MVQR_FAIL,
              "Match failed due to inability to connect to QMS.");
            if (rootExpr->isAnalyzeOnly())
              rootExpr = handleAnalyzeOnlyQuery(rootExpr, "Can't connect to QMS");
            return rootExpr;  // Can't get a QMS
          }
	    
          // Do QMS MATCH protocol here.
          // ============================
          QRLogger::log(CAT_SQL_COMP_XML, LL_DEBUG, "MATCH REQUEST sent:");
          QRLogger::log1(CAT_SQL_COMP_XML, LL_DEBUG, xmlText_->data());
          QRXmlMessageObj* xmlResponse = 
                  MvQueryRewriteServer::sendMatchMessage(qms, xmlText_, STMTHEAP);
	
          // Handle MATCH response here.
          // ============================
          // For non-success result, do nothing (no rewrite). Problem will have
          // already been logged.
          XMLElementPtr responseDescriptor = NULL;
          QRRequestResult result;
          if (xmlResponse)
            {
              result = parseXML(xmlResponse->getData(),
                                xmlResponse->getLength(),
                                responseDescriptor);
              QRLogger::log(CAT_SQL_COMP_XML, LL_DEBUG, "MATCH RESPONSE received:");
              QRLogger::log1(CAT_SQL_COMP_XML, LL_DEBUG, xmlResponse->getData());
              xmlResponse->decrRefCount();
            }
          else
            result = ProtocolError;

          if (result == Success)
          {
            // Make sure the parsed document was a result descriptor.
            if (responseDescriptor->getElementType() !=  ET_ResultDescriptor)
            {
              QRLogger::log(CAT_SQL_COMP_QR_HANDLER, LL_MVQR_FAIL,
                          "Response to MATCH request was an XML document with "
                          "document element <%s> instead of <%s>",
                          responseDescriptor->getElementName(), QRResultDescriptor::elemName);
            }
            else
            {
              MVCandidates mvCandidates(rootExpr, (QRQueryDescriptorPtr)requestDescriptor, descGenerator);
              QRResultDescriptorPtr resultDescriptor =
                      static_cast<QRResultDescriptorPtr>(responseDescriptor);
              mvCandidates.analyzeResultDescriptor(resultDescriptor);
            }
          }
        }
      }
      catch(QRDescriptorException e)
      {
        // Exception has generated mx event, but not mvqr-logged. 
        QRLogger::log(CAT_SQL_COMP_QR_HANDLER, LL_MVQR_FAIL, e.getMessage());
        delete xmlText_;
        if (rootExpr->isAnalyzeOnly())
          rootExpr = handleAnalyzeOnlyQuery(rootExpr, e.getMessage());
        return rootExpr;
      }
      catch(MVCandidateException e)
      {
        // Exception has already been logged. 
        delete xmlText_;
        if (rootExpr->isAnalyzeOnly())
          rootExpr = handleAnalyzeOnlyQuery(rootExpr, e.getMessage());
        return rootExpr;
      }
      catch(...)
      {
        // This exception was not yet logged.
        // Log it and then skip.
        QRLogger::log(CAT_SQL_COMP_QR_HANDLER, LL_MVQR_FAIL,
          "Unknown exception thrown during descriptor generation.");
        delete xmlText_;
        if (rootExpr->isAnalyzeOnly())
          rootExpr = handleAnalyzeOnlyQuery(rootExpr, "Unknown exception thrown");
        return rootExpr;
      }
    }
  
    delete xmlText_;
    xmlText_ = NULL;
  }

  return rootExpr;
}  // handleMvQueryRewrite()

// -----------------------------------------------------------------------
void MvQueryRewriteHandler::dumpToFile(const char* fileName, const char* data)
{
  FILE *mvqr_fd = fopen(fileName, "w+");
    
  if (mvqr_fd) 
  {
    fprintf(mvqr_fd, "%s", data);
    fflush(mvqr_fd);
    fclose(mvqr_fd);
  }
}  // dumpToFile()

// -----------------------------------------------------------------------
#if 0
void MvQueryRewriteHandler::dumpAnalysisToFile(QueryAnalysis* qa, RelExpr* expr)
{
  // Dump the QueryAnalysis data to a file.
  NAString analysisFileName = fileNamePrefix_ + ".analysis";
  NAString str;
  expr->unparse(str, OPTIMIZER_PHASE, MVINFO_FORMAT);
  str += "\n";
  str += qa->getText();

  // Add in some stuff to look at join predicates for the JBBCs.
  str += "Join Predicates\n";
  str += "===============";
  char buffer[20];
  ARRAY(JBB*) jbbs = qa->getJBBs();
  for (CollIndex jbbInx = 0; jbbInx < jbbs.entries(); jbbInx++)
    {
      JBB* jbb = jbbs[jbbInx];
      str_itoa(jbbInx, buffer);
      ((str += "\nJBB #") += NAString(buffer)) += ":\n";
      CANodeIdSet jbbcs = jbb->getJBBCs();
      for (CANodeId jbbcId=jbbcs.init();  jbbcs.next(jbbcId); jbbcs.advance(jbbcId) )
      {
        str_itoa(jbbcId, buffer);
        ((str += "\nJBBC with CANodeId ") += NAString(buffer)) += ":\n";
        ValueIdSet joinPreds = jbbcId.getNodeAnalysis()->getJBBC()->getJoinPreds();
        str += valueIdSetGetText(joinPreds);
        if (joinPreds.entries() > 0)
          {
            str.append("\n(value ids of predicates are ");
            NABoolean first = true;
            for (ValueId jpVid=joinPreds.init(); joinPreds.next(jpVid); joinPreds.advance(jpVid))
              {
                if (first)
                  first = FALSE;
                else
                  str.append(", ");
                str_itoa(jpVid, buffer);
                str.append(buffer);
              }
            str.append(")\n");
          }
      }
      str += '\n';
    }

  dumpToFile(analysisFileName.data(), str.data());
}  // dumpAnalysisToFile()
#endif

/**
 * Replace the query with its query descriptor text.
 * The MVQR_LOG_QUERY_DESCRIPTORS CQD was set to 'DUMP', so instead of running
 * this query, we are going to replace it with something that, when executed,
 * will produce the XML text of the query descriptor as its output. This
 * 'something' is a TupleList node that has the text lines of the descriptor 
 * as its tuples.
 * 
 * @param rootExpr The RelExpr tree of the existing query
 * @param xmlText The XML text of the query descriptor
 */
RelRoot* MvQueryRewriteHandler::handleAnalyzeOnlyQuery(RelRoot* rootExpr, NAString* xmlText)
{
  static NAString empty("No descriptor generated.");
  if (xmlText==NULL)
    xmlText = &empty;
    
  CollHeap *heap = CmpCommon::statementHeap();
  Int32 maxLen=xmlText->length();
  Int32 pos=0;
  Int32 lastPos=-1;
  ItemExpr* tupleExpr = NULL;  
  
  while (pos<maxLen-1)
  {
    // Find the next CR character
    pos = xmlText->index("\n", 1, pos+1, NAString::exact);
    if (pos==-1) // If this is the last line with no CR.
      pos=maxLen;

    // The next line if from the last CR to this one.
    NASubString line=(*xmlText)(lastPos+1, pos-lastPos-1);
    lastPos=pos;

    // Make a constant character string from it.
    ItemExpr* tuple = new (heap) 
      Convert(new(heap) SystemLiteral(line));
    
    // Collect the tuples in a list
    if (tupleExpr==NULL)
      tupleExpr = tuple;
    else
      tupleExpr = new(heap) ItemList(tuple, tupleExpr);    
  }

  // Construct the TupleList node
  TupleList* tupleListNode = new(heap) TupleList(tupleExpr->reverseTree(), heap); 
  RelRoot* newRoot = new(heap) RelRoot(tupleListNode);
  
  // A RenameTable above it to give a name to the column of text.
  ItemExpr *renExpr = new(heap) 
    RenameCol(NULL, new(heap) ColRefName("Query Descriptor", heap));
  NAString tableName("Descriptor table");
  RelExpr* renameNode = new(heap) 
    RenameTable(TRUE, newRoot, tableName, renExpr, heap);
  
  // And a final RelRoot on top with a SELECT *.
  ItemExpr *starExpr = new(heap) ColReference(new(heap) ColRefName(1));
  RelRoot* topRoot = new(heap) RelRoot(renameNode, REL_ROOT, starExpr);
  topRoot->setRootFlag(TRUE);
  
  // Now bind the new query tree
  BindWA* bindWA = rootExpr->getRETDesc()->getBindWA();
  RelExpr* boundRoot = topRoot->bindNode(bindWA);

  // Transform it. This will eliminate the Rename node and the extraneous root.
  NormWA normWA(CmpCommon::context());
  ExprGroupId eg(boundRoot);
  boundRoot->transformNode(normWA, eg);

  // And Normalize it.
  RelRoot *normRoot = (RelRoot *)eg.getPtr();
  normRoot->normalizeNode(normWA);
  
  return normRoot;
}
	
RelRoot* MvQueryRewriteHandler::handleAnalyzeOnlyQuery(RelRoot* rootExpr, const char* excuse)
{
  NAString reason("Aborted: ");
  reason += excuse;
  return handleAnalyzeOnlyQuery(rootExpr, &reason);
}
