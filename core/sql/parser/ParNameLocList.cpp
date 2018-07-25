/* -*-C++-*-
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
 *****************************************************************************
 *
 * File:         ParNameLocList.cpp
 * Description:  definitions of non-inline methods for classes
 *               ParNameLoc and ParNameLocList
 *
 *               Also contains the definition of global (file
 *               scope) functions relating to ParNameLocList
 *               or ParNameLocListPtr (defined in SqlParser.y).
 *
 * Created:      5/30/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#ifndef   SQLPARSERGLOBALS_NAMES_AND_TOKENS
#define   SQLPARSERGLOBALS_NAMES_AND_TOKENS
#endif
#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#define   SQLPARSERGLOBALS_NADEFAULTS
#include "SqlParserGlobals.h"

#include "ComOperators.h"
#include "ComSmallDefs.h"
#include "CmpContext.h"
#include "ElemDDLConstraintCheck.h"
#include "StmtDDLCreateView.h"
#include "StmtDDLCreateTrigger.h"
#include "StmtDDLCreateMV.h" 
#include "StmtDDLCreateTable.h"

// -----------------------------------------------------------------------
// definition of global (file scope) function ParInsertNameLoc()
// -----------------------------------------------------------------------

void ParInsertNameLoc(const StringPos namePos, const size_t nameLen)
{
  if (ParNameLocListPtr NEQ NULL)
  {
    ParNameLocListPtr->insert(ParNameLoc(namePos, nameLen));
  }
}

// -----------------------------------------------------------------------
// definition of global (file scope) function ParInsertNameLocInOrder()
// -----------------------------------------------------------------------

void ParInsertNameLocInOrder(const StringPos namePos, const size_t nameLen)
{
  if (ParNameLocListPtr NEQ NULL)
  {
    ParNameLocListPtr->insertInOrder(ParNameLoc(namePos, nameLen));
  }
}

// -----------------------------------------------------------------------
// definition of global (file scope) function ParInsertNameLocForStar()
// -----------------------------------------------------------------------

void ParInsertNameLocForStar(ColRefName * pColRefName)
{
  if (ParNameLocListPtr EQU NULL)
  {
    return;
  }

  const char *pInputStr = ParNameLocListPtr->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);
  
  //
  // Gets the position of the star (asterisk)
  // from the ParScannedTokenQueue. Note that
  // the parser may look ahead one token.
  // So the most recently scanned token (with
  // 0th index) may not be the star.  If the
  // 0th indexed token is not, then the -1th
  // indexed token (the token scanned before
  // the most-recently-scanned token) is.
  //

  StringPos starPos;
  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
    = ParScannedTokens->getScannedTokenInfo(0);
  if (tokInfo.tokenStrLen EQU (size_t)1 AND
      NAString(&pInputStr[tokInfo.tokenStrPos], (size_t)1) EQU "*")
  {
    starPos = tokInfo.tokenStrPos;
  }
  else
  {
    const ParScannedTokenQueue::scannedTokenInfo &prevTokInfo
      = ParScannedTokens->getScannedTokenInfo(-1);
    ComASSERT(prevTokInfo.tokenStrLen EQU (size_t)1 AND
              NAString(&pInputStr[prevTokInfo.tokenStrPos], (size_t)1) EQU "*");
    starPos = prevTokInfo.tokenStrPos;
  }

  pColRefName->setNamePosition(starPos, FALSE);
  ComASSERT(!pColRefName->isDelimited());
  
  ParNameLocListPtr->insert(ParNameLoc(starPos, 1/*star length*/));
}

// -----------------------------------------------------------------------
// definitions of global (file scope) functions ParSetTextEndPos()
// -----------------------------------------------------------------------

NABoolean ParSetTextEndPos(ElemDDLConstraintCheck * pCkCnstrntNode)
{
  ParNameLocList &nameLocList = pCkCnstrntNode->getNameLocList();
  const char *pInputStr = nameLocList.getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);
  
  const ParScannedTokenQueue::scannedTokenInfo *tokInfo
    = ParScannedTokens->getScannedTokenInfoPtr(0);
  NAString tokStr(&pInputStr[tokInfo->tokenStrPos], tokInfo->tokenStrLen);
  
  if (IsNAStringSpace(tokStr))
    {
      tokInfo = ParScannedTokens->getScannedTokenInfoPtr(-1);
      tokStr = NAString(&pInputStr[tokInfo->tokenStrPos], 
                         tokInfo->tokenStrLen);
    }
  TrimNAStringSpace(tokStr);
  if (tokStr NEQ ")") return TRUE;	// syntax error
  pCkCnstrntNode->setEndPosition(tokInfo->tokenStrPos +
                                 tokInfo->tokenStrLen - 1);
  return FALSE;				// no error
}

NABoolean ParSetTextEndPos(ElemDDLDivisionClause * pDivClauseParseNode)
{
  if (pDivClauseParseNode == NULL)
    return TRUE; // error

  ParNameLocList &nameLocList = pDivClauseParseNode->getNameLocList();
  const char *pInputStr = nameLocList.getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);
  
  // Case #1:
  //   TOK_DIVISION TOK_BY '(' sort_by_value_expression_list ')' ...current_position...
  //
  // Case #2:
  //   TOK_DIVISION TOK_BY '(' TOK_USING '(' sort_by_value_expression_list ')' ...current_position...
  
  Int32 tokIndex = 0;
  const ParScannedTokenQueue::scannedTokenInfo * pTokInfo
    = ParScannedTokens->getScannedTokenInfoPtr(tokIndex);
  NAString tokStr((size_t)50); // pre-allocate 50-byte memory space
  tokStr.append(&pInputStr[pTokInfo->tokenStrPos], pTokInfo->tokenStrLen);
  TrimNAStringSpace(tokStr);

  while (ParScannedTokens->isQueueIndexWithinRange(tokIndex) AND tokStr NEQ ")")
    {
    tokIndex--;
    if (ParScannedTokens->isQueueIndexOutOfRange(tokIndex))
      break;
    // Go backward and skip over tokens until a right-parenthesis token found
    pTokInfo = ParScannedTokens->getScannedTokenInfoPtr(tokIndex);
    tokStr.replace(0, tokStr.length(), &pInputStr[pTokInfo->tokenStrPos], pTokInfo->tokenStrLen);
    TrimNAStringSpace(tokStr);
    }
  if (ParScannedTokens->isQueueIndexOutOfRange(tokIndex) OR tokStr NEQ ")")
    return TRUE; // syntax error
  pDivClauseParseNode->setEndPosition(pTokInfo->tokenStrPos +
                                      pTokInfo->tokenStrLen - 1);
  return FALSE; // no error
}

NABoolean ParSetTextEndPos(StmtDDLCreateView * pCreateViewParseNode)
{
  ParNameLocList &nameLocList = pCreateViewParseNode->getNameLocList();
  const char *pInputStr = nameLocList.getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);
  
  // the token trailing a stand-alone Create View statement
  // should be either a semicolon or a null token (the end-
  // of-input-string character).
  //
  // For CREATE VIEW statement appearing within a Create
  // Schema statement, the terminating token can also be
  // the token Alter, Create, or Grant.
  //
  // If the Create View statement contains the optional
  // With ... Check Option clause, the view_definition
  // production may be reduced before the terminating
  // token (for Create View statement) is scanned.

  const ParScannedTokenQueue::scannedTokenInfo *tokInfo
    = ParScannedTokens->getScannedTokenInfoPtr(0);
  NAString tokStr(&pInputStr[tokInfo->tokenStrPos], tokInfo->tokenStrInputLen);
  size_t finalTokenPos = tokInfo->tokenStrPos;
  Int32 isoOffset = 0;
  tokStr.toUpper();
  NABoolean isEndOfInputStr = tokStr.isNull();
  if (NOT isEndOfInputStr AND
      tokStr NEQ ";"      AND
      tokStr NEQ "ALTER"  AND
      tokStr NEQ "CREATE" AND
      tokStr NEQ "GRANT"  AND
      tokStr NEQ "OPTION" ) 
    return TRUE;		// syntax error

  //
  // The following logic locates the last (non-blank) token
  // in the Create View statement.
  //
  // Note that when the WITH [ LOCAL | CASCADED ] CHECK OPTION
  // clause appears in the create view statement, the parser
  // (currently) stops the scanning right at the keyword OPTION;
  // therefore, the most-recently-scanned token is OPTION, the
  // last token in the Create View statement.  Of course, this
  // may change if someone changes the grammar productions
  // relating to view definitions in the future, and the logic
  // in this routine will need to be updated accordingly.
  //
  
  if (isEndOfInputStr OR tokStr NEQ "OPTION")
  {
    // 1. The most-recently-scanned token is a null character OR
    // 2. The most-recently-scanned token is either a semicolon
    //    or the keyword Alter, Create, or Grant.
    //
    // Sets tokInfo and tokStr to the previous scanned token
    // which can be either white space(s) or the last token
    // in the Create View statement
    //
    tokInfo = ParScannedTokens->getScannedTokenInfoPtr(-1);
	// must allow for the offsets of string qualifiers if _ISO88591 or _UCS2 or ... was specified 
	NAString isoStr(&pInputStr[tokInfo->tokenStrPos], finalTokenPos-tokInfo->tokenStrPos);
	isoStr.resize(9);	  // only check the first 9 characters in case these are in the string
	isoStr.toUpper();
	if(isoStr.contains("_ISO88591"))
		isoOffset = 11;
	else if(isoStr.contains("_UCS2") || isoStr.contains("_UTF8") || isoStr.contains("_SJIS"))
		isoOffset = 7;
	tokStr = NAString(&pInputStr[tokInfo->tokenStrPos],
		tokInfo->tokenStrInputLen+isoOffset);
	
    //
    // if the last token in create view statement is white
    // space(s), excludes it from the statement.  Note that
    // there might not be any white space(s) between the
    // Create View statement and the terminating semicolon.
    //
   if (IsNAStringSpace(tokStr))
    {
      // Sets tokInfo to token preceding the white space(s).
      // This token should be the last token in the create view
      // statement.
      tokInfo = ParScannedTokens->getScannedTokenInfoPtr(-2);
    }
  }
  
  pCreateViewParseNode->setEndPosition(tokInfo->tokenStrPos +
				       tokInfo->tokenStrInputLen - 1 + isoOffset);
  return FALSE;			// no error
}


NABoolean ParSetTextEndPos(StmtDDLCreateTrigger * pCreateTriggerParseNode)
{
  ParNameLocList &nameLocList = pCreateTriggerParseNode->getNameLocList();
  const char *pInputStr = nameLocList.getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);
  
  const ParScannedTokenQueue::scannedTokenInfo *tokInfo
    = ParScannedTokens->getScannedTokenInfoPtr(0);
  NAString tokStr(&pInputStr[tokInfo->tokenStrPos], tokInfo->tokenStrLen);
  tokStr.toUpper();
  NABoolean isEndOfInputStr = tokStr.isNull();

  // If the most-recently-scanned token is a semicolon,
  // set tokInfo and tokStr to the previous scanned token
  // which can be either white space(s) or the last token
  // in the create trigger statement. Note that this if 
  // statement only fails if the action of the trigger
  // is a SIGNAL statement. For a SIGNAL action, The 
  // parser (currently) stops the scanning right at 
  // the token ")" of the SIGNAL action. 

  TrimNAStringSpace(tokStr);
  if (tokStr EQU ";")
  {
     tokInfo = ParScannedTokens->getScannedTokenInfoPtr(-1);
     tokStr = NAString(&pInputStr[tokInfo->tokenStrPos], tokInfo->tokenStrLen);
     tokStr.toUpper();
  }

  pCreateTriggerParseNode->setEndPosition(tokInfo->tokenStrPos +
					  tokInfo->tokenStrLen - 1);
  return FALSE;			// no error
}

// -----------------------------------------------------------------------
// definition of global (file scope) function
// ParSetTextStartPosForCreateTrigger()
// -----------------------------------------------------------------------

void ParSetTextStartPosForCreateTrigger(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList EQU ParNameLocListPtr AND
            pNameLocList NEQ NULL);

  const char *pInputStr = pNameLocList->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);

  //
  // locates the TRIGGER token information record
  // from ParScannedTokens circular queue
  //
  
  Int32 triggerTokIndex = 0 /*index of most-recently-scanned token*/;
  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
    = ParScannedTokens->getScannedTokenInfo(triggerTokIndex);
  NAString tokStr(&pInputStr[tokInfo.tokenStrPos], tokInfo.tokenStrLen);
  tokStr.toUpper();

  if (tokStr NEQ "TRIGGER")  // If not on "TRIGGER", try one step back
  {
    triggerTokIndex = -1;
    ComASSERT(ParScannedTokens->isQueueIndexWithinRange(triggerTokIndex));
    const ParScannedTokenQueue::scannedTokenInfo &triggerTokInfo
      = ParScannedTokens->getScannedTokenInfo(triggerTokIndex);
    tokStr = NAString(&pInputStr[triggerTokInfo.tokenStrPos],
                      triggerTokInfo.tokenStrLen);
    tokStr.toUpper();
    ComASSERT(tokStr EQU "TRIGGER");
  }

  //
  // the token before the "TRIGGER" token should be white space(s)
  //
  
  Int32 prevTokIndex = triggerTokIndex - 1;
  ComASSERT(ParScannedTokens->isQueueIndexWithinRange(prevTokIndex));
  const ParScannedTokenQueue::scannedTokenInfo &prevTokInfo
    = ParScannedTokens->getScannedTokenInfo(prevTokIndex);
  NAString prevTokStr(&pInputStr[prevTokInfo.tokenStrPos],
                       prevTokInfo.tokenStrLen);
  ComASSERT(IsNAStringSpace(prevTokStr));

  //
  // locates the CREATE token information record
  // from ParScannedTokens circular queue
  //
  
  Int32 createTokIndex = triggerTokIndex - 2;
  ComASSERT(ParScannedTokens->isQueueIndexWithinRange(createTokIndex));
  const ParScannedTokenQueue::scannedTokenInfo &createTokInfo
    = ParScannedTokens->getScannedTokenInfo(createTokIndex);
  NAString createTokStr(&pInputStr[createTokInfo.tokenStrPos],
                        createTokInfo.tokenStrLen);
  createTokStr.toUpper();
  ComASSERT(createTokStr EQU "CREATE");

  //
  // the starting position of the CREATE token is also the
  // starting position of the CREATE TRIGGER statement.
  //

  pNameLocList->setTextStartPosition(createTokInfo.tokenStrPos);

} // ParSetTextStartPosForCreateTrigger


// Keep (in a global var) the position of the closing parenthesis
// of the optional view column list
void ParSetEndOfOptionalColumnListPos(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList EQU ParNameLocListPtr AND pNameLocList NEQ NULL);

  const char *pInputStr = pNameLocList->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);

  // locates the ')' token
  Int32 MVTokIndex = 0;       /* index of most-recently-scanned token */
  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
    = ParScannedTokens->getScannedTokenInfo(MVTokIndex);
  NAString tokStr(&pInputStr[tokInfo.tokenStrPos], tokInfo.tokenStrLen);
  tokStr.toUpper();

  TrimNAStringSpace(tokStr);
  if (tokStr NEQ ")" ) // Just in case: If not on ")"  --->>> try one step back
  {
    MVTokIndex = -1;
    ComASSERT(ParScannedTokens->isQueueIndexWithinRange(MVTokIndex));
    const ParScannedTokenQueue::scannedTokenInfo &MVTokInfo
      = ParScannedTokens->getScannedTokenInfo(MVTokIndex);
    tokStr = NAString(&pInputStr[MVTokInfo.tokenStrPos],
                      MVTokInfo.tokenStrLen);
    tokStr.toUpper();
    ComASSERT(tokStr EQU ")");
    // Store position of ')' in global variable 
    ParEndOfOptionalColumnListPos = MVTokInfo.tokenStrPos;
    return;
  }
  // Store position of ')' in global variable 
  ParEndOfOptionalColumnListPos = tokInfo.tokenStrPos;
}

// ---------------------------------------------------------------------
// Keep (in a global var) the position of the begining of the optional
// file-options list.  (Called in the "enable/disable rewrite" section;
// adjusts the position forward if this section was not empty.)
void ParSetBeginingOfFileOptionsListPos(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList EQU ParNameLocListPtr AND pNameLocList NEQ NULL);

  const char *pInputStr = pNameLocList->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);

  Int32 MVTokIndex = 0;       /* index of most-recently-scanned token */
  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
    = ParScannedTokens->getScannedTokenInfo(MVTokIndex);
  NAString tokStr(&pInputStr[tokInfo.tokenStrPos], tokInfo.tokenStrLen);
  tokStr.toUpper();

  // Store current position in global variable 
  ParBeginingOfFileOptionsListPos = tokInfo.tokenStrPos;

  NAString rewriteStr = NAString("REWRITE");
  
  if ( tokStr EQU rewriteStr ) {  // "enable/disable rewrite" was specified
    ParBeginingOfFileOptionsListPos += rewriteStr.length() ; // skip this token
  }
}



//----------------------------------------------------------------------------
// Keep (in a global var) the position of the beginning of the MV query text
void ParSetBeginOfMVQueryPos(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList  EQU ParNameLocListPtr AND 
	    NULL	  NEQ pNameLocList );

  const char *pInputStr = pNameLocList->getInputStringPtr();
  
  ComASSERT(pInputStr NEQ NULL);

  Int32 MVTokIndex = 0;     // index of most-recently-scanned token 

  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
			  = ParScannedTokens->getScannedTokenInfo(MVTokIndex);

  NAString tokStr(&pInputStr[tokInfo.tokenStrPos], tokInfo.tokenStrLen);
  
  tokStr.toUpper();

  ComASSERT("AS" == tokStr);

  Int32 nextWhiteSpacePos = tokInfo.tokenStrPos + sizeof("AS") - 1;

  NAString space(&pInputStr[nextWhiteSpacePos], 1);
  
  ComASSERT(IsNAStringSpace(space));

  ParBeginingOfMVQueryPos = nextWhiteSpacePos;
} // ParSetBeginOfMVQueryPos


//----------------------------------------------------------------------------
// Keep (in a global var) the position of the end of the list of columns
// of the select clause of the create MV stmt
void ParSetEndOfSelectColumnListPos(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList EQU ParNameLocListPtr AND pNameLocList NEQ NULL);

  const char *pInputStr = pNameLocList->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);

  // current token should be 'FROM'
  Int32 MVTokIndex = 0;       /* index of most-recently-scanned token */
  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
    = ParScannedTokens->getScannedTokenInfo(MVTokIndex);
  NAString tokStr(&pInputStr[tokInfo.tokenStrPos], tokInfo.tokenStrLen);
  tokStr.toUpper();

  if (tokStr NEQ "FROM" ) return;  // can't be valid ....

  // must step one token back; need to point to white space
  // that follows the last item in the select list.
  // Need to handle the special case - Ex: ... AS SELECT *FROM
  MVTokIndex = -1;
  ComASSERT(ParScannedTokens->isQueueIndexWithinRange(MVTokIndex));
  const ParScannedTokenQueue::scannedTokenInfo &MVTokInfo
    = ParScannedTokens->getScannedTokenInfo(MVTokIndex);
  tokStr = NAString(&pInputStr[MVTokInfo.tokenStrPos],
		    MVTokInfo.tokenStrLen);

  if (IsNAStringSpace(tokStr))
  {
  // Store position in the global variable 
  ParEndOfSelectColumnListPos = MVTokInfo.tokenStrPos;
  }
  else
  { // Special cases:
    // Ex #1: CREATE MV ... AS SELECT *FROM ...
    // Ex #2: CREATE MV ... AS SELECT COUNT(*)FROM ...

    // Store position in the global variable 
    ParEndOfSelectColumnListPos = tokInfo.tokenStrPos;
  }
}

// ---------------------------------------------------------------------
// Keep (in a global var) the position of the end of the file-options list.
void ParSetEndOfFileOptionsListPos(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList EQU ParNameLocListPtr AND pNameLocList NEQ NULL);

  const char *pInputStr = pNameLocList->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);

  Int32 MVTokIndex = 0;       /* index of most-recently-scanned token */
  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
    = ParScannedTokens->getScannedTokenInfo(MVTokIndex);
  NAString tokStr(&pInputStr[tokInfo.tokenStrPos], tokInfo.tokenStrLen);
  tokStr.toUpper();   // tokStr is Only for debugging
  // ComASSERT(tokStr EQU "AS");

  // Store the current (end) position in the global variable 
  ParEndOfFileOptionsListPos = tokInfo.tokenStrPos;

  ComASSERT( ParEndOfFileOptionsListPos > ParBeginingOfFileOptionsListPos );
  ComASSERT( ParBeginingOfFileOptionsListPos > 0 );
}

// ---------------------------------------------------------------------
// Store the end-of-text position in the StmtDDLCreateMV node.
// Also copy into the the StmtDDLCreateMV node the other tree globaly
// kept positions: End of columns list, begining+end of file options.
NABoolean ParSetTextEndPos(StmtDDLCreateMV * pCreateMVParseNode)
{
  ParNameLocList &nameLocList = pCreateMVParseNode->getNameLocList();
  const char *pInputStr = nameLocList.getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);
  
  const ParScannedTokenQueue::scannedTokenInfo *tokInfo
    = ParScannedTokens->getScannedTokenInfoPtr(0);
  NAString tokStr(&pInputStr[tokInfo->tokenStrPos], tokInfo->tokenStrLen);
  tokStr.toUpper();
  NABoolean isEndOfInputStr = tokStr.isNull();

  pCreateMVParseNode->setEndPosition(tokInfo->tokenStrPos +
					  tokInfo->tokenStrLen - 1);

  // Now the node exists; copy the rest of the globals into this node 
  pCreateMVParseNode->
    setEndOptionalColumnListPosition(ParEndOfOptionalColumnListPos);
  pCreateMVParseNode->setFileOptionsPositions(ParBeginingOfFileOptionsListPos,
					      ParEndOfFileOptionsListPos);
  pCreateMVParseNode->setStartOfMVQueryPosition(ParBeginingOfMVQueryPos);
  pCreateMVParseNode->
    setEndSelectColumnListPosition(ParEndOfSelectColumnListPos);
  return FALSE;			// no error
}

// ---------------------------------------------------------------------------
// definition of global (file scope) function  ParSetTextStartPosForCreateMV()
// ---------------------------------------------------------------------------

void ParSetTextStartPosForCreateMV(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList EQU ParNameLocListPtr AND pNameLocList NEQ NULL);

  const char *pInputStr = pNameLocList->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);

  //
  // locates the MV token information record
  // from ParScannedTokens circular queue
  //
  
  Int32 MVTokIndex = 0;              /* index of most-recently-scanned token */
  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
    = ParScannedTokens->getScannedTokenInfo(MVTokIndex);
  NAString tokStr(&pInputStr[tokInfo.tokenStrPos], tokInfo.tokenStrLen);
  tokStr.toUpper();

  if (tokStr NEQ "MV" &&  // If not on "MV"  --->>> try one step back
      tokStr NEQ "VIEW" ) // and if not on "MATERIALIZED VIEW"
  {
    MVTokIndex = -1;
    ComASSERT(ParScannedTokens->isQueueIndexWithinRange(MVTokIndex));
    const ParScannedTokenQueue::scannedTokenInfo &MVTokInfo
      = ParScannedTokens->getScannedTokenInfo(MVTokIndex);
    tokStr = NAString(&pInputStr[MVTokInfo.tokenStrPos],
                      MVTokInfo.tokenStrLen);
    tokStr.toUpper();
    ComASSERT(tokStr EQU "MV" || tokStr EQU "VIEW" );

  }

  // In  case of the two-word  "MATERIALIZED VIEW"  token
  if ( tokStr EQU "VIEW" ) { // step back behind the "MATERIALIZED" token
    MVTokIndex -= 2 ;  // need 2 to skip white space token as well
    ComASSERT(ParScannedTokens->isQueueIndexWithinRange(MVTokIndex));
    const ParScannedTokenQueue::scannedTokenInfo &MVTokInfo
      = ParScannedTokens->getScannedTokenInfo(MVTokIndex);
    tokStr = NAString(&pInputStr[MVTokInfo.tokenStrPos],
		      MVTokInfo.tokenStrLen);
    tokStr.toUpper();
    ComASSERT( tokStr EQU "MATERIALIZED" );
  }

  //
  // the token before the "MV" token should be white space(s)
  //
  
  Int32 prevTokIndex = MVTokIndex - 1;
  ComASSERT(ParScannedTokens->isQueueIndexWithinRange(prevTokIndex));
  const ParScannedTokenQueue::scannedTokenInfo &prevTokInfo
    = ParScannedTokens->getScannedTokenInfo(prevTokIndex);
  NAString prevTokStr(&pInputStr[prevTokInfo.tokenStrPos],
                       prevTokInfo.tokenStrLen);
  ComASSERT(IsNAStringSpace(prevTokStr));

  //
  // Locate the "CREATE" token. There may be an intervening "GHOST" token.
  //
  
  Int32 createTokIndex;
  Int32 ghostTokIndex = MVTokIndex - 2;
  ComASSERT(ParScannedTokens->isQueueIndexWithinRange(ghostTokIndex));
  const ParScannedTokenQueue::scannedTokenInfo &ghostTokInfo
    = ParScannedTokens->getScannedTokenInfo(ghostTokIndex);
  NAString ghostTokStr(&pInputStr[ghostTokInfo.tokenStrPos],
                        ghostTokInfo.tokenStrLen);
  ghostTokStr.toUpper();

  // Check for "GHOST" token
  if ( ghostTokStr EQU "GHOST" ) {
    //
    // The token before "GHOST" should be white space(s)
    //
    prevTokIndex = ghostTokIndex - 1;
    ComASSERT(ParScannedTokens->isQueueIndexWithinRange(prevTokIndex));
    const ParScannedTokenQueue::scannedTokenInfo &prevTokInfo
      = ParScannedTokens->getScannedTokenInfo(prevTokIndex);
    prevTokStr = NAString(&pInputStr[prevTokInfo.tokenStrPos],
		      prevTokInfo.tokenStrLen);
    ComASSERT(IsNAStringSpace(prevTokStr));
    //
    // The token before the white space should be "CREATE".
    //
    createTokIndex = prevTokIndex - 1;
  }
  else
    createTokIndex = ghostTokIndex;

  //
  // Now we must have the "CREATE" token.
  //
  
  ComASSERT(ParScannedTokens->isQueueIndexWithinRange(createTokIndex));
  const ParScannedTokenQueue::scannedTokenInfo &createTokInfo
    = ParScannedTokens->getScannedTokenInfo(createTokIndex);
  NAString createTokStr(&pInputStr[createTokInfo.tokenStrPos],
                        createTokInfo.tokenStrLen);
  createTokStr.toUpper();

  ComASSERT(createTokStr EQU "CREATE");

  //
  // the starting position of the CREATE token is also the
  // starting position of the CREATE MV statement.
  //

  pNameLocList->setTextStartPosition(createTokInfo.tokenStrPos);

} // ParSetTextStartPosForCreateMV

//----------------------------------------------------------------------------
// Keep (in a global var) the position of the beginning of the MV query text
void ParSetBeginOfCreateTableAsQueryPos(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList  EQU ParNameCTLocListPtr AND 
	    NULL	  NEQ pNameLocList );

  const char *pInputStr = pNameLocList->getInputStringPtr();
  
  ComASSERT(pInputStr NEQ NULL);

  Int32 CTTokIndex = 0;     // index of most-recently-scanned token 

  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
			  = ParScannedTokens->getScannedTokenInfo(CTTokIndex);

  NAString tokStr(&pInputStr[tokInfo.tokenStrPos], tokInfo.tokenStrLen);
  
  tokStr.toUpper();

  ComASSERT("AS" == tokStr);

  Int32 nextWhiteSpacePos = tokInfo.tokenStrPos + sizeof("AS") - 1;

  NAString space(&pInputStr[nextWhiteSpacePos], 1);
  
  ComASSERT(IsNAStringSpace(space));

  ParBeginingOfCreateTableQueryPos = nextWhiteSpacePos;
} // ParSetBeginOfCreateTableAsQueryPos

//----------------------------------------------------------------------------
// Keep (in a global var) the position of the beginning attr list of
// a create table as stmt.
void ParSetBeginOfCreateTableAsAttrList(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList  EQU ParNameCTLocListPtr AND 
	    NULL	  NEQ pNameLocList );

  const char *pInputStr = pNameLocList->getInputStringPtr();
  
  ComASSERT(pInputStr NEQ NULL);

  Int32 CTTokIndex = 0;     // index of most-recently-scanned token 

  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
			  = ParScannedTokens->getScannedTokenInfo(CTTokIndex);

  NAString tokStr(&pInputStr[tokInfo.tokenStrPos], tokInfo.tokenStrLen);
  
  tokStr.toUpper();

  Int32 nextPos = 0;
  if (")" == tokStr)
    {
      nextPos = tokInfo.tokenStrPos + sizeof(")") - 1;
    }
  else
    nextPos = tokInfo.tokenStrPos;
  
  ParBeginingOfCreateTableAsAttrList = nextPos;
} // ParSetBeginOfCreateTableAsAttrList

//----------------------------------------------------------------------------
// Keep (in a global var) the position of the beginning attr list of
// a create table as stmt.
void ParSetEndOfCreateTableAsAttrList(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList  EQU ParNameCTLocListPtr AND 
	    NULL	  NEQ pNameLocList );

  const char *pInputStr = pNameLocList->getInputStringPtr();
  
  ComASSERT(pInputStr NEQ NULL);

  Int32 CTTokIndex = 0;     // index of most-recently-scanned token 

  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
			  = ParScannedTokens->getScannedTokenInfo(CTTokIndex);

  NAString tokStr(&pInputStr[tokInfo.tokenStrPos], tokInfo.tokenStrLen);
  
  tokStr.toUpper();

  Int32 nextPos = 0;
  /*  if ("NO" == tokStr)
    {
      nextPos = tokInfo.tokenStrPos + sizeof("NO") - 1;
    }
  else if ("LOAD" == tokStr)
    {
      nextPos = tokInfo.tokenStrPos + sizeof("LOAD") - 1;
    }
  else*/
  nextPos = tokInfo.tokenStrPos;

  ParEndOfCreateTableAsAttrList = nextPos;
} // ParSetEndOfCreateTableAsAttrList

// ---------------------------------------------------------------------
// Store the start of query position in the StmtDDLCreateTable node.
NABoolean ParSetTextEndPos(StmtDDLCreateTable * pCreateCTParseNode)
{
  pCreateCTParseNode->
    setStartOfCreateTableQueryPosition(ParBeginingOfCreateTableQueryPos);
  pCreateCTParseNode->
    setStartOfCreateTableAsAttrListPosition(ParBeginingOfCreateTableAsAttrList);
  pCreateCTParseNode->
    setEndOfCreateTableAsAttrListPosition(ParEndOfCreateTableAsAttrList);
  pCreateCTParseNode->	
	setCreateTableAsScannedInputCharset((Int32)ParScannedInputCharset);
  pCreateCTParseNode->
	setCreateTableAsIsoMapping((Int32)SqlParser_ISO_MAPPING);
  return FALSE;			// no error
}

// -----------------------------------------------------------------------
// definition of global (file scope) function
// ParSetTextStartPosForCheckConstraint()
// -----------------------------------------------------------------------

void ParSetTextStartPosForCheckConstraint(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList EQU ParNameLocListPtr AND
            pNameLocList NEQ NULL);

  const char *pInputStr = pNameLocList->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);

  //
  // locates the left parenthesis token information record
  // from ParScannedTokens circular queue
  //
  
  Int32 leftParenTokIndex = 0;  // index of most-recently-scanned token
  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
    = ParScannedTokens->getScannedTokenInfo(leftParenTokIndex);
  NAString tokStr(&pInputStr[tokInfo.tokenStrPos], 1);// only looking for open paren
  StringPos leftParenStrPos = tokInfo.tokenStrPos;

  TrimNAStringSpace(tokStr);
  if (tokStr NEQ "(")
  {
    leftParenTokIndex = -1;
    ComASSERT(ParScannedTokens->isQueueIndexWithinRange(leftParenTokIndex));
    const ParScannedTokenQueue::scannedTokenInfo &leftParenTokInfo
      = ParScannedTokens->getScannedTokenInfo(leftParenTokIndex);
    tokStr = "";
    tokStr.append(&pInputStr[leftParenTokInfo.tokenStrPos],
                  leftParenTokInfo.tokenStrLen);
    TrimNAStringSpace(tokStr);
    PARSERASSERT(tokStr EQU "(");
    leftParenStrPos = leftParenTokInfo.tokenStrPos;
  }

  //
  // the position of the left parenthesis token is the starting
  // position of the check constraint search condition.
  //

  pNameLocList->setTextStartPosition(leftParenStrPos);

#ifndef NDEBUG
  //
  // The following code is for sanity checking only.
  // It is not really needed.
  //
  // the tokens before the left parenthesis token should be 
  // optional white space(s) and "CHECK"
  //
  
  Int32 prevTokIndex = leftParenTokIndex - 1;
  ComASSERT(ParScannedTokens->isQueueIndexWithinRange(prevTokIndex));
  const ParScannedTokenQueue::scannedTokenInfo &prevTokInfo
    = ParScannedTokens->getScannedTokenInfo(prevTokIndex);
  NAString prevTokStr(&pInputStr[prevTokInfo.tokenStrPos],
                      prevTokInfo.tokenStrLen);
  prevTokStr.toUpper();
  if (prevTokStr NEQ "CHECK")
    {
      ComASSERT(IsNAStringSpace(prevTokStr));

      Int32 prevTokIndex = leftParenTokIndex - 2;
      ComASSERT(ParScannedTokens->isQueueIndexWithinRange(prevTokIndex));
      const ParScannedTokenQueue::scannedTokenInfo &prevTokInfo
	= ParScannedTokens->getScannedTokenInfo(prevTokIndex);
      NAString prevTokStr(&pInputStr[prevTokInfo.tokenStrPos],
			    prevTokInfo.tokenStrLen);
      prevTokStr.toUpper();
      ComASSERT(prevTokStr EQU "CHECK");
    }
#endif

} // ParSetTextStartPosForCheckConstraint

// -----------------------------------------------------------------------
// definition of global (file scope) function
// ParSetTextStartPosForDivisionClause()
// -----------------------------------------------------------------------

void ParSetTextStartPosForDivisionByClause(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList EQU ParNameDivByLocListPtr AND
            pNameLocList NEQ NULL);

  const char *pInputStr = pNameLocList->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);

  // TOK_DIVISION TOK_BY '(' ...current_position...
  //   or
  // TOK_DIVISION TOK_BY '(' TOK_USING '(' ...current_position...
  //
  //   Example #1:
  //
  //     DIVISION BY ( store_id / 2 )
  //                   ^...current_position...
  //
  //   Example #2:
  //
  //     DIVISION BY (USING (store_id/2) NAMED AS ...
  //                         ^...current_position...
  
  //
  // locates the left parenthesis (the one following the BY or USING keyword)
  // token information record from ParScannedTokens circular queue
  //

  Int32 leftParenTokIndex = 0;
  const ParScannedTokenQueue::scannedTokenInfo * pLeftParenTokInfo = NULL;

  Int32 tokIndex = 0; // index of most-recently-scanned token
  const ParScannedTokenQueue::scannedTokenInfo * pTokInfo
    = ParScannedTokens->getScannedTokenInfoPtr(tokIndex);
  NAString tokStr((size_t)50); // pre-allocate 50 bytes memory space
  tokStr.append(&pInputStr[pTokInfo->tokenStrPos], pTokInfo->tokenStrLen);
  TrimNAStringSpace(tokStr);
  tokStr.toUpper();
  while (ParScannedTokens->isQueueIndexWithinRange(tokIndex)
         AND tokStr NEQ "BY" AND tokStr NEQ "USING")
  {
    if (tokStr EQU "(")
    {
      leftParenTokIndex = tokIndex; // maybe the one :-)
      pLeftParenTokInfo = pTokInfo;
  }
    tokIndex--;  // Going backward one token
    if (ParScannedTokens->isQueueIndexOutOfRange(tokIndex))
      break;
    pTokInfo = ParScannedTokens->getScannedTokenInfoPtr(tokIndex);
    tokStr.replace(0, tokStr.length(), &pInputStr[pTokInfo->tokenStrPos], pTokInfo->tokenStrLen);
    TrimNAStringSpace(tokStr);
    tokStr.toUpper();
  }
  PARSERASSERT(ParScannedTokens->isQueueIndexWithinRange(tokIndex)
               AND (tokStr EQU "BY" OR tokStr EQU "USING")
               AND pLeftParenTokInfo NEQ NULL);

  //
  // the position of the left parenthesis token is the starting
  // position of the division expression.
  //

  pNameLocList->setTextStartPosition(pLeftParenTokInfo->tokenStrPos);

  // We are done ...

#ifndef NDEBUG
  
  // Additional checking in the DEBUG version of the software

  if (tokStr EQU "USING")  //    DIVISION BY ( USING ( ...
  {                        //                  ^...we_are_here...
    // Go backward and skip over the USING token
    tokIndex--;
    if (ParScannedTokens->isQueueIndexOutOfRange(tokIndex))
      return;
    pTokInfo = ParScannedTokens->getScannedTokenInfoPtr(tokIndex);
    tokStr.replace(0, tokStr.length(), &pInputStr[pTokInfo->tokenStrPos], pTokInfo->tokenStrLen);
    if (IsNAStringSpace(tokStr))
    {
      // Go backward and skip over the white space(s) token
      tokIndex--;
      if (ParScannedTokens->isQueueIndexOutOfRange(tokIndex))
        return;
      pTokInfo = ParScannedTokens->getScannedTokenInfoPtr(tokIndex);
      tokStr.remove(0); // set to an empty string
      tokStr.append(&pInputStr[pTokInfo->tokenStrPos], pTokInfo->tokenStrLen);
    }
    TrimNAStringSpace(tokStr);     // DIVISION BY ( USING ( ...
    PARSERASSERT(tokStr EQU "(");  //             ^...we_are_here...

    // Go backward and skip over the left-parenthesis token
    tokIndex--;
    if (ParScannedTokens->isQueueIndexOutOfRange(tokIndex))
      return;
    pTokInfo = ParScannedTokens->getScannedTokenInfoPtr(tokIndex);
    tokStr.replace(0, tokStr.length(), &pInputStr[pTokInfo->tokenStrPos], pTokInfo->tokenStrLen);
    if (IsNAStringSpace(tokStr))
    {
      // Go backward and skip over the white space(s) token
      tokIndex--;
      if (ParScannedTokens->isQueueIndexOutOfRange(tokIndex))
        return;
      pTokInfo = ParScannedTokens->getScannedTokenInfoPtr(tokIndex);
      tokStr.replace(0, tokStr.length(), &pInputStr[pTokInfo->tokenStrPos], pTokInfo->tokenStrLen);
    }
    tokStr.toUpper();               // DIVISION BY ( USING ( ...
    PARSERASSERT(tokStr EQU "BY");  //          ^...we_are_here...
  }

  // Go backward and skip over the BY token
  tokIndex--;
  if (ParScannedTokens->isQueueIndexOutOfRange(tokIndex))
    return;
  pTokInfo = ParScannedTokens->getScannedTokenInfoPtr(tokIndex);
  tokStr.replace(0, tokStr.length(), &pInputStr[pTokInfo->tokenStrPos], pTokInfo->tokenStrLen);
  if (IsNAStringSpace(tokStr))
  {
    // Go backward and skip over the white space(s) token
    tokIndex--;
    if (ParScannedTokens->isQueueIndexOutOfRange(tokIndex))
      return;
    pTokInfo = ParScannedTokens->getScannedTokenInfoPtr(tokIndex);
    tokStr.replace(0, tokStr.length(), &pInputStr[pTokInfo->tokenStrPos], pTokInfo->tokenStrLen);
  }                                     // DIVISION BY ( USING ( ...
  tokStr.toUpper();                     // DIVISION BY ( ...
  PARSERASSERT(tokStr EQU "DIVISION");  // ^...we_are_here...
#endif

} // ParSetTextStartPosForDivisionByClause

// -----------------------------------------------------------------------
// definition of global (file scope) function
// ParSetTextStartPosForCreateView()
// -----------------------------------------------------------------------

void ParSetTextStartPosForCreateView(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList EQU ParNameLocListPtr AND
            pNameLocList NEQ NULL);

  const char *pInputStr = pNameLocList->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);

  //
  // locates the VIEW token information record
  // from ParScannedTokens circular queue
  //
  
  Int32 viewTokIndex = 0 /*index of most-recently-scanned token*/;
  
  // The maximum number of tokens that can exist in this sytax are: 
  // TOK_CREATE<sp><TOK_OR><sp><TOK_REPLACE><sp><TOK_VIEW><sp><TOK_CASCADE>
  //    1       2     3     4   5            6   7         8   9
  // If the purpose of this function is to find the position of the token
  // TOK_CREATE then all we have to do is scan maximum of 8 tokens before 
  // the current most-recently-scanned token and we should find the token
  // for CREATE. 

  ParScannedTokenQueue::scannedTokenInfo tokInfo;
  Int32 maxCount=9;   // This will change when the syntax changes. 
  Int32 toksScanned = 0; // counter for number of tokens scanned.
  NABoolean foundCreate=FALSE;
  NAString tokStr;
  while((foundCreate==FALSE) || (toksScanned < maxCount))
  {
    ComASSERT(ParScannedTokens->isQueueIndexWithinRange(viewTokIndex));
    ComASSERT(toksScanned < maxCount);
    tokInfo = ParScannedTokens->getScannedTokenInfo(viewTokIndex);
    tokStr = NAString(&pInputStr[tokInfo.tokenStrPos], tokInfo.tokenStrLen);
    tokStr.toUpper();

    if (tokStr EQU "CREATE")
    {
      //
      // the starting position of the CREATE token is also the
      // starting position of the CREATE VIEW statement.
      //
      foundCreate = TRUE;
      pNameLocList->setTextStartPosition(tokInfo.tokenStrPos);
      return;
    }
    else
    {
      toksScanned = toksScanned + 1;
      viewTokIndex = viewTokIndex -1; // get the previous token
    }
  }
  return;  // It will never come here unless the maxCount is wrong. 
           // We should assert in the above ComAssert checks. Or we 
           // should return with CREATE.  
} // ParSetTextStartPosForCreateView

void ParSetTextStartPosForDisplayExplain(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList EQU ParNameLocListPtr AND
            pNameLocList NEQ NULL);

  const char *pInputStr = pNameLocList->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);

  //
  // locates the left parenthesis token information record
  // from ParScannedTokens circular queue
  //
  
  Int32 leftParenTokIndex = 0;  // index of most-recently-scanned token
  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
    = ParScannedTokens->getScannedTokenInfo(leftParenTokIndex);
  NAString tokStr(&pInputStr[tokInfo.tokenStrPos], tokInfo.tokenStrLen);
  StringPos qryStrPos = tokInfo.tokenStrPos;

  Int32 c;
  c = tokStr.compareTo("EXPLAIN", NAString::ignoreCase);
  if ((c != 0) &&
      (tokStr.data()[0] NEQ '\''))
  {
    leftParenTokIndex = -2;
    ComASSERT(ParScannedTokens->isQueueIndexWithinRange(leftParenTokIndex));
    const ParScannedTokenQueue::scannedTokenInfo &leftParenTokInfo
      = ParScannedTokens->getScannedTokenInfo(leftParenTokIndex);
    tokStr = NAString(&pInputStr[leftParenTokInfo.tokenStrPos],
                      leftParenTokInfo.tokenStrLen);

    c = tokStr.compareTo("EXPLAIN", NAString::ignoreCase);
    qryStrPos = leftParenTokInfo.tokenStrPos;
  }

  c = tokStr.compareTo("EXPLAIN", NAString::ignoreCase);
  if (c == 0)
    qryStrPos = qryStrPos + strlen("EXPLAIN");
  else
    {
      // At this point, tokStr contains the token that follows the "options"
      // key word for the stmt "explain options '<str>' ..."
      // Skip string that follows the "options" keyword
      char * trailingQ = strchr((char*)tokStr.data()+1, '\'');
      Int32 len = trailingQ - tokStr.data() + 1;
      qryStrPos = qryStrPos + len;
    }

  //
  // the position of the left parenthesis token is the starting
  // position of the check constraint search condition.
  //

  pNameLocList->setTextStartPosition(qryStrPos);

} // ParSetTextStartPosForDisplayExplain

NABoolean ParGetTextStartEndPosForDisplayExplain(
     ParNameLocList * pNameLocList,
     StringPos &start, StringPos &end)
{
  const char *pInputStr = pNameLocList->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);
  
  const ParScannedTokenQueue::scannedTokenInfo *tokInfo
    = ParScannedTokens->getScannedTokenInfoPtr(0);
  NAString tokStr(&pInputStr[tokInfo->tokenStrPos], tokInfo->tokenStrLen);
  
  if ((NOT IsNAStringSpace(tokStr)) &&
      (tokStr NEQ ")") &&
      (tokStr NEQ ";"))
    {
      tokInfo = ParScannedTokens->getScannedTokenInfoPtr(-1);
      tokStr = NAString(&pInputStr[tokInfo->tokenStrPos], 
                         tokInfo->tokenStrLen);

      if ((NOT IsNAStringSpace(tokStr)) &&
	  (tokStr NEQ ")") &&
	  (tokStr NEQ ";"))
	return TRUE;
    }
  start = pNameLocList->getTextStartPosition();
  end   = tokInfo->tokenStrPos;

  return FALSE;				// no error
}

// Used by MULTI-COMMIT Delete to keep track of the start of the
// WHERE clause of the delete statement.
//
void ParSetTextStartPosForMultiCommit(ParNameLocList * pNameLocList)
{
  ComASSERT(pNameLocList EQU ParNameLocListPtr AND
            pNameLocList NEQ NULL);

  const char *pInputStr = pNameLocList->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);

  //
  // locates the start of WHERE clause in the 
  // ParScannedTokens circular queue
  //
  
  // index of most-recently-scanned token
  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
    = ParScannedTokens->getScannedTokenInfo(0);
  
  // Get the string of the most recent token
  NAString tokStr(&pInputStr[tokInfo.tokenStrPos], tokInfo.tokenStrLen);

  // The position of this token in the query text.
  StringPos qryStrPos = tokInfo.tokenStrPos;

  // Check to see if we have the WHERE clause.
  Int32 c;
  c = tokStr.compareTo("WHERE", NAString::ignoreCase);
  if (c == 0)
  {
    //
    // the position of the WHERE token is the starting
    // position of the multi commit search condition.
    //
    pNameLocList->setWhereStartPosition(qryStrPos);
  }

} // ParSetTextStartPosForMultCommit

// Used by MULTI-COMMIT Delete to keep track of the start and end of the
// WHERE clause of the delete statement.
//
// Find the end of the WHERE clause and return the start and the end positions
//
NABoolean ParGetTextStartEndPosForMultiCommit(
     ParNameLocList * pNameLocList,
     StringPos &start, StringPos &end)
{
  const char *pInputStr = pNameLocList->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);
  
  // Get the most recently scanned token.
  const ParScannedTokenQueue::scannedTokenInfo *tokInfo
    = ParScannedTokens->getScannedTokenInfoPtr(0);

  // Get the start position, previously stored.
  start = pNameLocList->getWhereStartPosition();

  // Get the end position of the WHERE clause.
  end   = tokInfo->tokenStrPos;

  return FALSE;				// no error
}


// -----------------------------------------------------------------------
// definition of function ParUpdateNameLocForDotStar()
// -----------------------------------------------------------------------

void ParUpdateNameLocForDotStar(const ColRefName * pColRefName)
{
  if (ParNameLocListPtr EQU NULL)
  {
    return;
  }

  const char *pInputStr = ParNameLocListPtr->getInputStringPtr();
  ComASSERT(pInputStr NEQ NULL);
  
  //
  // Gets the position of the dot-star (period-asterisk)
  // from the ParScannedTokenQueue. Note that the parser
  // may look ahead one token.  So the most recently
  // scanned token (with 0th index) may not be the star.
  // If the 0th indexed token is not, then the -1th
  // indexed token (the token scanned before the most-
  // recently-scanned token) is.
  //
  // Note that the scanner currently does not allow
  // white spaces between the dot (period) and the
  // star (asterisk) as of 6/5/96.
  //
  
  StringPos dotStarPos;
  const ParScannedTokenQueue::scannedTokenInfo &tokInfo
    = ParScannedTokens->getScannedTokenInfo(0/*token index*/);
  if (tokInfo.tokenStrLen EQU (size_t)2 AND
      NAString(&pInputStr[tokInfo.tokenStrPos], (size_t)2) EQU ".*")
  {
    dotStarPos = tokInfo.tokenStrPos;
  }
  else
  {
    const ParScannedTokenQueue::scannedTokenInfo &prevTokInfo
      = ParScannedTokens->getScannedTokenInfo(-1/*token index*/);
    ComASSERT(prevTokInfo.tokenStrLen EQU (size_t)2 AND
              NAString(&pInputStr[prevTokInfo.tokenStrPos], (size_t)2)
              EQU ".*");
    dotStarPos = prevTokInfo.tokenStrPos;
  }

  ParNameLoc *pNameLoc
    = ParNameLocListPtr->getNameLocPtr(pColRefName->getCorrNameObj().
                                       getNamePosition());
  ComASSERT(pNameLoc NEQ NULL);

  //
  // pNameLoc->getNameLength() used to contain the length of the
  // correlation name.  Updates it to include the dot-star symbol.
  //
  pNameLoc->setNameLength(dotStarPos - pNameLoc->getNamePosition() + 2);
}

// -----------------------------------------------------------------------
// definitions of non-inline methods of class ParNameLoc
// -----------------------------------------------------------------------

//
// virtual destructor
//

ParNameLoc::~ParNameLoc()
{
}


//
// operators
//

ParNameLoc &
ParNameLoc::operator=(const ParNameLoc &rhs)
{
  if (this EQU &rhs)
  {
    return *this;
  }
  
  namePos_      = rhs.namePos_;
  nameLen_      = rhs.nameLen_;
  expandedName_ = rhs.expandedName_;

  return *this;
}

const NABoolean
ParNameLoc::operator==(const ParNameLoc &rhs) const
{
  if (this EQU &rhs)
  {
    return TRUE;
  }
  
  return (namePos_      EQU rhs.namePos_ AND
          nameLen_      EQU rhs.nameLen_ AND
          expandedName_ EQU rhs.expandedName_);
}

// -----------------------------------------------------------------------
// definitions of non-inline methods of class ParNameLocList
// -----------------------------------------------------------------------

//
// constructors
//

ParNameLocList::ParNameLocList ( const char * const    pInputString     // default is NULL
                               , CharInfo::CharSet     inputStrCS       // default is CharInfo::ISO88591 or UTF8
                               , const NAWchar * const pInputStrInUTF16 // default is NULL
                               , CollHeap *            heap             // default is PARSERHEAP()
                               )
  : LIST(ParNameLoc *)(heap),
    pInputString_(pInputString),       // shallow copy
    inputStrCharSet_(inputStrCS),
    pwInputString_(pInputStrInUTF16),  // shallow copy
    textStartPos_(0),
    heap_(heap)
{
}

ParNameLocList::ParNameLocList(const ParNameLocList &rhs,
                               CollHeap             *heap)
  : LIST(ParNameLoc *)(heap),
    heap_(heap)
{
  copy(rhs) ;
}

//
// virtual destructor
//

ParNameLocList::~ParNameLocList()
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    NADELETE(&operator[](i), ParNameLoc, heap_);
  }
}

//
// operator
//

ParNameLocList &
ParNameLocList::operator=(const ParNameLocList &rhs)
{
  if (this EQU &rhs) return *this;
  clear();
  copy(rhs);
  return *this;
}

//
// accessor
//

//
// given the position (of the first character) of the name,
// searches the list for a match and then returns the pointer
// to the name location element in the list if found; returns
// the NULL pointer value otherwise.
//
ParNameLoc *
ParNameLocList::getNameLocPtr(const StringPos namePos)
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    if (operator[](i).getNamePosition() EQU namePos)
    {
      return &operator[](i);
    }
  }
  return NULL;
}

//
// mutators
//

void
ParNameLocList::clear()
{
  for (CollIndex i = 0; i < entries(); i++)
  {
//KSKSKS
    delete &operator[](i);
//    NADELETE(&operator[](i), ParNameLoc, heap_);
//KSKSKS
  }
  LIST(ParNameLoc *)::clear();
  pInputString_ = NULL;
  inputStrCharSet_ = CharInfo::UnknownCharSet;
  pwInputString_ = NULL;
  textStartPos_ = 0;
  // leave data member heap_ alone (it's only set once, by the constructor).
}

void
ParNameLocList::copy(const ParNameLocList &rhs)
{
  if (this EQU &rhs)
    return;

  pInputString_ = rhs.pInputString_;            // shallow copy
  inputStrCharSet_ = rhs.inputStrCharSet_;
  pwInputString_   = rhs.pwInputString_;        // shallow copy
  textStartPos_ = rhs.textStartPos_;
  // DO NOT set the heap_ field.
  // It's already been set by the constructor
  // heap_      = rhs.heap_;

  for (CollIndex i = 0; i < rhs.entries(); i++)
  { // deep copy
    insert(rhs[i]);  // insert() allocates space from heap_ 
  }
}

NABoolean
ParNameLocList::insert(const ParNameLoc &nameLoc)
{
  ParNameLoc *pNameLoc = getNameLocPtr(nameLoc.getNamePosition());
  if (pNameLoc EQU NULL)  // not found
  {
    // ok to insert
    pNameLoc = new(heap_) ParNameLoc(nameLoc);
    CMPASSERT(pNameLoc NEQ NULL);
    LIST(ParNameLoc *)::insert(pNameLoc);
    return TRUE;
  }
  return FALSE;
}

// This function allows UDF names to be added to the list in the
// order in which they appear in the query text with other objects 
// (such as tables).
NABoolean
ParNameLocList::insertInOrder(const ParNameLoc &nameLoc)
{
  ParNameLoc *pNameLoc = getNameLocPtr(nameLoc.getNamePosition());
  if (pNameLoc EQU NULL)  // not found
  {
    // ok to insert
    pNameLoc = new(heap_) ParNameLoc(nameLoc);
    CMPASSERT(pNameLoc NEQ NULL);

    CollIndex ci=0;
    while (ci < LIST(ParNameLoc *)::entries() &&
           LIST(ParNameLoc *)::at(ci)->getNamePosition() < nameLoc.getNamePosition())
             ci++;
    LIST(ParNameLoc *)::insertAt(ci, pNameLoc);
    return TRUE;
  }
  return FALSE;
}
//
// End of File
//
