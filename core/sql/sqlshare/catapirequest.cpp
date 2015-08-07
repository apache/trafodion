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
 * File:         CatApiRequest.C
 * Description:  See CatApiRequest.h for details.
 *
 * Created:      5/2/97
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */


#include "catapirequest.h"
#include <stdio.h>
#include <assert.h>
#include <ctype.h>


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// ======================================================================
//         Class CatApiParam non inline methods:
// ======================================================================

// -----------------------------------------------------------------------
// Constructor:
// -----------------------------------------------------------------------
CatApiParam::CatApiParam( char * param )
: nextParam_ (NULL)
, param_ (param)
{
  paramLengthForMsg_ = (Lng32)strlen(param);
}

// -----------------------------------------------------------------------
// Constructor:
// -----------------------------------------------------------------------
CatApiParam::CatApiParam( char * param, Lng32 lengthForMsg )
: nextParam_ (NULL)
, param_ (param)
, paramLengthForMsg_ (lengthForMsg)
{
}

// -----------------------------------------------------------------------
// Destructor:
// -----------------------------------------------------------------------
CatApiParam::~CatApiParam(void)
{
  if (param_)
  {
    delete [] param_;
    param_ = NULL;
  }
}




// -----------------------------------------------------------------------
// Definition of the overloaded iostream insertion operator.
// -----------------------------------------------------------------------
ostream& operator<< (ostream& s, const CatApiParam &p)
{
  s << "       parameter value = " << *(p.getParamValue()) << endl;
  s << "       next parameter  = " << p.getNextParam() << endl;
  s << endl;
  return s;
}


// =======================================================================
//          class CatApiRequest non inline methods
// =======================================================================

// -----------------------------------------------------------------------
// Constructors:
// -----------------------------------------------------------------------
CatApiRequest::CatApiRequest( void )
  : requestType_  (UNKNOWN_REQUEST_TYPE),
    numParams_    (0),
    firstParam_   (NULL)
{
#pragma nowarn(1506)   // warning elimination 
  Int32 length = strlen(APIVERSION);
#pragma warn(1506)  // warning elimination 
  versionInfo_ = new char [length+1];
  strcpy(versionInfo_, APIVERSION);
  versionInfo_[length] = '\0';
}

CatApiRequest::CatApiRequest( CatApiRequestType requestType )
  : requestType_  (requestType),
    numParams_    (0),
    firstParam_   (NULL)
{
#pragma nowarn(1506)   // warning elimination 
  Int32 length = strlen(APIVERSION);
#pragma warn(1506)  // warning elimination 
  versionInfo_ = new char [length+1];
  strcpy(versionInfo_, APIVERSION);
  versionInfo_[length] = '\0';
}

// -----------------------------------------------------------------------
// Destructors:
// -----------------------------------------------------------------------
CatApiRequest::~CatApiRequest (void)
{
  // free up space for each CatApiParam
  CatApiParam *currentParam = firstParam_;
  CatApiParam *nextParam = NULL;
  while (currentParam)
  {
    nextParam = currentParam->nextParam_;
    delete currentParam;
    currentParam = nextParam;
  }

  // free up space for version info
  delete [] versionInfo_;
}

// ---------------------------------------------------------------------------
// getParamEnd:
//
// Parses the current api string to get the next parameter.  Returns pointer
// in string where end position resides; returns NULL otherwise
//
// Parameters can be 
//   regular identifiers (a-z, A-Z, 0-9, _)
//   delimited identifiers ("<values>")
//   first key values ( <value1>, ... , <valuen> )
//   value(1-n) ( <valid data type representation> ) - all strings '<value>'
//
// Parameters can be delimited by either of the following:
//   '<' and '>'  (used only by test code)
//   '@' and '@'
//
// Example of parameter list this function could parse:
//    @cat.sch."t10c == '""."""@ @0@ @'s',10,10,'d''a'@ @$data2@
//
// This routine assumes you can't have a delimited identifier and a quoted 
// string in the same parameter value.
// ---------------------------------------------------------------------------
char *
CatApiRequest::getParamEnd(size_t startPos, const char *text)
{
  BOOL foundEnd = FALSE;
  BOOL inSingleQuote = FALSE;
  BOOL inDoubleQuote = FALSE;
  BOOL inQuote = FALSE;
  size_t currentPos = startPos;
  while (!foundEnd)
  {
    switch (text[currentPos])
    {
      // This should only occur as end position or in a quoted string
      case '@':
        if (!inQuote)
          foundEnd = 1;
        break;

      // This should only occur as end position, in a quoted string, or in
      // in a delimited identifier
      case '>':
        if (!inQuote)
          foundEnd = 1;
        break;

      // This could be a delimiter for a delimited identifier, part of the
      // delimited string or part of a quoted string.
      case '\'':
        if (!inDoubleQuote)
        {
          if (inSingleQuote)
          {
            if (text[currentPos+1] == '\'')
              currentPos++;
            else
            {
              inSingleQuote = FALSE;
              inQuote = FALSE;
            }
          }
          else
          {
            inSingleQuote = TRUE;
            inQuote = TRUE;
          }
        }
        break;

      // This could be part of a delimited identifier, part of a quoted string,
      // or delimiters for a quoted string.
      case '"':
        if (!inSingleQuote)
        {
          if (inDoubleQuote)
          {
            if (text[currentPos+1] == '"')
              currentPos++;
            else
            {
              inDoubleQuote = FALSE;
              inQuote = FALSE;
            }
          }
          else
          {
            inDoubleQuote = TRUE;
            inQuote = TRUE;
          }
        }
        break;

      default:
        break;
    }
    currentPos++;
  }
  char *endPos = NULL;
  if (foundEnd)
    endPos = (char *)text + (currentPos-1);
  return endPos;
}

// ----------------------------------------------------------------------------
// versionSupported
//
// Verifies the the version specified by the request matches a support version.
// Version supported are: 1 and 2
// ----------------------------------------------------------------------------
short
CatApiRequest::versionSupported (void)
{
  Int32 versionList[2] = {1, 2};
  Lng32 currentVersion = atol (versionInfo_);
  Lng32 numVersions = sizeof (versionList) / sizeof (Int32);
  for (Lng32 i = 0; i < numVersions; i++)
  {
    if (currentVersion == versionList[i])
      return TRUE;
  }
  return FALSE;
}

// ----------------------------------------------------------------------------
// appendParam
//
// Appends the CatApiParam to the end of the list of params.
// ----------------------------------------------------------------------------
void
CatApiRequest::appendParam (CatApiParam *newParam)
{
  CatApiParam *currentParam = firstParam_;
  if (currentParam == NULL)
    firstParam_ = newParam;
  else
  {
    while (currentParam->getNextParam() != NULL)
      currentParam = (CatApiParam *)currentParam->getNextParam();
    currentParam->updateNextParam( newParam );
  }
  incrNumParams();
}

// ----------------------------------------------------------------------------
// convertTextToCatApiRequest:
//
// returns TRUE if successful, FALSE otherwise
// ----------------------------------------------------------------------------
short 
CatApiRequest::convertTextToCatApiRequest(const char *text)
{
  size_t startPos = 0;
  short status = convertTextToCatApiRequest( text, startPos );
  return status;
}

// ----------------------------------------------------------------------------
// convertTextToCatApiRequest - sends and returns current string position
//
// returns TRUE if successful, FALSE otherwise
// ----------------------------------------------------------------------------
short 
CatApiRequest::convertTextToCatApiRequest( const char *text, size_t &startPos )
{
  // allocate a buffer of MAXPARAMSIZE bytes.  
  char buf[MAXPARAMSIZE];
  size_t len = 0;
  Int32 scanCount;
  Int32 reqType = 0;  // UNKNOWN_REQUEST_TYPE, see enum CatApiRequestType

  // The first item in the string is the operation type, skip over.
  char * endPos = (char *) strchr( &text[startPos], '&' );
  if (endPos == NULL)
    return 0;  // failure
  size_t strPos = endPos - text;
  len = strPos - startPos;
  strncpy (buf, &text[startPos], len);
  buf[len] = '\0';
  if (memcmp( buf, CATAPI, len ) != 0)
    return 0;
  startPos = ++strPos;

  // Get the version control information
  endPos = (char *) strchr( &text[startPos], ' ');
  if (endPos == NULL)
    return 0;  // failure
  strPos = endPos - text;
  if (strPos < startPos)
    return 0; // failure
  len = (strPos - startPos);
  delete [] versionInfo_;
  versionInfo_ = new char[len+1];
  strncpy (versionInfo_, &text[startPos], len);
  versionInfo_[len] = '\0';
  if (!versionSupported())
    return 0;

  // Get the request type
  startPos = ++strPos;
  endPos = (char *) strchr(&text[startPos], ' ');
  if (endPos == NULL)
    return 0;  // failure
  strPos = endPos - text;
  if (strPos < startPos)
    return 0; // failure
  len = strPos - startPos;
  strncpy (buf, &text[startPos], len);
  buf[len] = '\0';
  scanCount = sscanf (buf, "%d", &reqType);
  setCatApiRequestType((CatApiRequestType)reqType);
  if (scanCount != 1)
    return 0;

  // Get number parameters
  startPos = ++strPos;
  endPos = (char *) strchr(&text[startPos], ' ');
  if (!endPos)
   {
	// Fixup metadata views catapi request has no parameters.
	// So the request text for metadata views fixup is like
    // CREATE TANDEM_CAT_REQUEST&1 47 0;. The code below
    // handles this condition.
    // Checks if the parameters passed are zero.
    // If yes set numParams_ to zero and return success
    endPos = (char *) strchr(&text[startPos], ';');
    if (endPos != NULL)   
    {
      strPos = endPos - text;
      if (strPos < startPos)
        return 0; // failure
      len = strPos - startPos;
      strncpy (buf, &text[startPos], len);
      buf[len] = '\0';
      scanCount = sscanf (buf, "%d", &numParams_);
      if (scanCount != 1 || numParams_ != 0)
	  {
        return 0;
	  }
	  else
      {
        startPos--;
        return 1;
      }
    }
    else
    {      
      return 0;
    }
  }

  strPos = endPos - text;
  if (strPos < startPos)
    return 0; // failure
  len = strPos - startPos;
  strncpy (buf, &text[startPos], len);
  buf[len] = '\0';
  scanCount = sscanf (buf, "%d", &numParams_);
  if (scanCount != 1)
    return 0;

  // If parameters exist, process params
  startPos = ++strPos;
  if (numParams_ > 0)
  {
    if (strcmp( versionInfo_, APIVERSION ) == 0)
      startPos += getParams (&text[startPos]);
    else
      startPos += getPreviousVersionParams (&text[startPos]);
  }

  // startPos is pointing to the next request, update to point
  // to position just before the next request.
  startPos--;

  return 1; // success
}

// ----------------------------------------------------------------------------
// getParams
//
// get all the parameters from a text request for the current version.
// Parameters have the following format:  'len value'
// 
// The first n bytes contains the length following by a space and followed
//   by the actual value.
//
// If the parameter is of 0 length, then the format is just the length.
// ----------------------------------------------------------------------------
size_t
CatApiRequest::getParams (const char *text)
{
  // initialize variables
  char buf[1000];
  Int32 scanCount;
  size_t len = 0;
  Int32 paramLength = 0; 
  size_t strPos = 0;
  size_t startPos = 0;
  char *endPos = (char *)text;
  CatApiParam *currentParam = NULL;

  // find each parameter and save in a CatApiParam class
  for (Lng32 i = 0; i < numParams_; i++)
  {
    // Get string length, save in paramLength
    endPos = (char *) strchr(&text[startPos], ' ');
    assert (endPos);
    strPos = endPos - text;
    len = strPos - startPos;

    strncpy (buf, &text[startPos], len);
    buf[len] = '\0';
    scanCount = sscanf (buf, "%d", &paramLength);
    assert (scanCount == 1);

    // Get string value, save in param
    startPos = ++strPos; // get past space (expect only 1 trailing space)
    char * param = NULL;
    if (paramLength > 0)
    {
      param = new char [paramLength+1];
      strncpy (param, &text[startPos], paramLength);
      param[paramLength] = '\0';
      startPos += paramLength + 1; // point to next param
      while (isspace((unsigned char)text[startPos])) // allow more than 1 trailing space so  // For VS2003
        startPos++;                   // w:/regress/catman/test103 can pass
                                      // on both Windows and NSK platforms
    }

    // zero length strings set up an empty string
    // startPos is already pointing to the next param
    else
    {
      param = new char[1];
      param[0] = '\0';
    }

    // Convert string into CatApiParam and add to list
    CatApiParam *nextParam = new CatApiParam( param );
    if (i == 0) // first param
    {
      firstParam_ = nextParam;
      currentParam = firstParam_;
    }
    else
    {
      currentParam->updateNextParam( nextParam );
      currentParam = nextParam;
    }
  }

  return startPos;
}

// ----------------------------------------------------------------------------
// getPreviousVersionParams:
//
// Sets up a CatApiParam list for version 1 strings.
// Version one strings are delimited by either @ ... @ or < ... >.  If the
// string contains embedded delimiter values, the text must be qualified with 
// single or double quotes.  If encoded in quotes, any of the same quotes that
// are found in the string must be doubled.
// ----------------------------------------------------------------------------
size_t
CatApiRequest::getPreviousVersionParams (const char *text)
{
  // initialize values
  size_t len = 0;
  size_t strPos = 0;
  size_t startPos = 1; // location 1 is the delimiter, get first text item

  char *endPos = (char *)text; 
  // char endValue = (text[strPos] == '@') ? '@' : '>';

  // Process each parameter.
  CatApiParam *currentParam = NULL;
  for (Lng32 i = 0; i < numParams_; i++)
  {
    // get string length, save in len
    endPos = getParamEnd(startPos, text);
    assert (endPos);
    strPos = endPos - text;
    len = strPos - startPos;

    // get string value, save in paramValue
    char * paramValue = new char [len + 1];  // for null byte
    strncpy (paramValue, &text[startPos], len);
    paramValue[len] = '\0';

    // Add param to CatApiParam list
    CatApiParam *nextParam = new CatApiParam( paramValue );
    if (i == 0) // first param
    {
      firstParam_ = nextParam;
      currentParam = firstParam_;
    }
    else
    {
      currentParam->updateNextParam( nextParam );
      currentParam = nextParam;
    }
    startPos = strPos + 3; // space and delimiter 
  }
  return strPos+1; // get past space (if list request)
}
	  
// -----------------------------------------------------------------------
// getText:
//
// returns TRUE is successful, FALSE otherwise
// -----------------------------------------------------------------------
short
CatApiRequest::getText( char *text, const short includeSemicolon ) const
{
  size_t strPos = 0;
  text[0] = '\0';

  // Add statement header
  strcat( text, CATAPI);
  strPos = strPos + strlen(CATAPI);

  // Add version information. Always version 2; the code below
  // cannot produce version 1 parameters.
  strcat( text, APIVERSION );
  strPos = strPos + strlen(APIVERSION);
  text[strPos++] = ' ';

  // add request type
  size_t length = sprintf( &text[strPos], "%0d", getCatApiRequestType() );
  strPos = strPos + length;
  text[strPos++] = ' ';

  // Add number parameters
  length = sprintf( &text[strPos], "%0d", getNumParams() );
  strPos = strPos + length;
  text[strPos++] = ' ';

  // Add each parameter
  //  Parameters are of the form:  length space value
  const CatApiParam *ptrParam = getFirstParam();
  for (Int32 i = 0; i < getNumParams(); i++)
  {
    if (!ptrParam)
      return 0;

    // add length + space
    length = sprintf( &text[strPos], "%0d", ptrParam->getParamLengthForMsg() );
    strPos += length;
    text[strPos++] = ' ';

    // add parameter + space
    strcpy( &text[strPos], ptrParam->getParamValue() );
    strPos += ptrParam->getParamLength();
    if (ptrParam->getParamLength() > 0)
      text[strPos++] = ' ';
    ptrParam = ptrParam->getNextParam();
  }
 
  if (includeSemicolon)
    text[strPos++] = ';';
  text[strPos] = '\0';

  return 1;
}

// -----------------------------------------------------------------------
// getTextLength:
// -----------------------------------------------------------------------
const Lng32 
CatApiRequest::getTextLength( void ) const
{
#pragma nowarn(1506)   // warning elimination 
  Lng32 length = ( strlen( CATAPI ) + strlen( APIVERSION ) +1 /*space*/);
#pragma warn(1506)  // warning elimination 
  char buf[100];
  length = length + (sprintf( buf, "%d", LAST_API_REQUEST));
  length++; // space
  length = length + (sprintf( buf, "%d", getNumParams() ));
  length++; //space
  const CatApiParam *nextParam = getFirstParam();
  for (Lng32 i = 0; i < getNumParams(); i++)
  {
    assert (nextParam);
    length += nextParam->getParamLength();
    if (strcmp( versionInfo_, APIVERSION ) == 0)
      length += sizeof(MAXPARAMSIZE) + 1; // length bytes + 1 for space
    else
      length += 3; // begin/end delimiter, space
    nextParam = nextParam->getNextParam();
  }
  length = length + 2;  // ; plus null terminator
  return length;
}

// -----------------------------------------------------------------------
// Constant texts and stuff for the internal SpCatApiRequest built-in function
// -----------------------------------------------------------------------
const char selectTextWrapper1 [] = "select * from table (tdmisp ('SpCatApiRequest',_utf8'";
const char selectTextWrapper2 [] = "'))";
const Lng32 selectTextWrapperLength = sizeof(selectTextWrapper1) + sizeof(selectTextWrapper2) - 2;

// -----------------------------------------------------------------------
// getSelectTextLength:
// -----------------------------------------------------------------------
const Lng32 
CatApiRequest::getSelectTextLength( void ) const
{
  return getTextLength() + selectTextWrapperLength;
}

// -----------------------------------------------------------------------
// getSelectText:
//
// returns TRUE is successful, FALSE otherwise
// -----------------------------------------------------------------------
short
CatApiRequest::getSelectText( char *text, const short includeSemicolon ) const
{
  // Allocate end wrapper with room for semicolon.
  char endWrapper [sizeof(selectTextWrapper2)+1];

  // Copy the initial wrapper.
  strcpy (text, selectTextWrapper1);

  // Append the actual CatApiRequest text.
  // Return FALSE if getText fails. Don't include ';' inside of the select text.
  if (!getText (&text[sizeof(selectTextWrapper1)-1], FALSE))
    return FALSE;

  // Create the end wrapper with or without semicolon, then append it to the select text.
  strcpy(endWrapper, selectTextWrapper2);
  if (includeSemicolon)
  {
    endWrapper [sizeof(selectTextWrapper2)-1] = ';';
    endWrapper [sizeof(selectTextWrapper2)] = '\0';
  }
  strcat (text, endWrapper);

  // Done ...
  return TRUE;
}

// -------------------------------------------------------------------
// addParam(s):
// -------------------------------------------------------------------

// Add a long parameter:

void
CatApiRequest::addConstParam (const Lng32 value)
{
  char asciiValue[16];
  Int32 length = sprintf( &asciiValue[0], "%d", value );
  addParam (&asciiValue[0], length);
}

void
CatApiRequest::addParam (Lng32 value)
{
  addConstParam( (const Lng32) value );
}

// Add a 64-bit binary parameter:
void 
CatApiRequest::addConstInt64Param (const Int64 value)
{
  char asciiValue[24];
  Int32 length = sprintf(&asciiValue[0], PF64, value);
  addParam(&asciiValue[0], length);
}

void 
CatApiRequest::addInt64Param(Int64 value)
{
  addConstInt64Param((const Int64)value);
}

// Add a null terminated character string
void 
CatApiRequest::addConstParam (const char * value)
{
#pragma nowarn(1026)   // warning elimination 
//SQ_LINUX (from (const) to (const Lng32)
  addConstParam (value, (const Lng32) strlen(value));
#pragma warn(1026)  // warning elimination 
}

void
CatApiRequest::addParam (char * value)
{
#pragma nowarn(1026)   // warning elimination 
//SQ_LINUX (from (const) to (const Lng32)
  addConstParam ((const char *) value, (const Lng32) strlen(value));
#pragma warn(1026)  // warning elimination 
}

// Add a character string with a specified length
void 
CatApiRequest::addConstParam (const char * value, const Lng32 length)
{
  // Allocate space for parameter and api structure
  char *param = new char [length + 1];  // 1 for null byte
  strncpy(param, value, length);
  param[length] = '\0';
  CatApiParam *newParam = new CatApiParam( param );

  // append to end of list
  appendParam(newParam);
}

// Add a character string that has single-quotes that have been doubled in
// preparation for a prepare query and so the length to be used
// in the request length of the string before the single-quotes were doubled.
void 
CatApiRequest::addConstParam (const Lng32 lengthForMsg, const char * value)
{
  size_t realLen = strlen(value);
  // Allocate space for parameter and api structure
  char *param = new char [realLen + 1];  // 1 for null byte
  strncpy(param, value, realLen);
  param[realLen] = '\0';
  CatApiParam *newParam = new CatApiParam( param, lengthForMsg );

  // append param to end of list of params
  appendParam(newParam);
}

void
CatApiRequest::addParam (char * value, Lng32 length)
{
  addConstParam( (const char *) value, (const Lng32) length );
}

// -----------------------------------------------------------------------
// Definition of the overloaded iostream insertion operator.
// -----------------------------------------------------------------------
ostream& operator<< (ostream& s, CatApiRequest &p)
{
  char * value = new char[p.getTextLength()+1]; // 1 for null byte
  short result = p.getText( value );
  if (result)
  {
    s << "  API Request  " << endl;
    s << "    Text: " << value << endl;
  }
  else
    s << "Could not generate text" << endl;

  const CatApiParam * nextParam = p.getFirstParam();
  while (nextParam != NULL)
    {
      s << *nextParam << endl;

    }
  delete [] value;
  return s;
}

// -----------------------------------------------------------------------
// CatApiRequest::operator[]() -- const version
// -----------------------------------------------------------------------
const CatApiParam *
CatApiRequest::operator[] (Lng32 index) const
{
  if ((index < 0) || (index > getNumParams()-1))
    return NULL;
  const CatApiParam *nextParam = getFirstParam();
    
  for (Lng32 i = 0; i < getNumParams(); i++)
  {
    if (i == index)
      return nextParam;
    if (nextParam == NULL)
      return NULL;
    nextParam = nextParam->getNextParam();
  }
  return NULL;
}

// =======================================================================
//          class CatApiRequestList non inline methods
// =======================================================================

// -----------------------------------------------------------------------
// Constructors:
// -----------------------------------------------------------------------
CatApiRequestList::CatApiRequestList (void)
:  numRequests_   (0)
{
#pragma nowarn(1506)   // warning elimination 
  Int32 length = strlen(APIVERSION);
#pragma warn(1506)  // warning elimination 
  versionInfo_ = new char [length+1];
  strcpy(versionInfo_, APIVERSION);
  versionInfo_[length] = '\0';
  for (Lng32 i = 0; i < MAXNUMREQUESTS; i++)
    requestList_[i] = NULL;
}

// -----------------------------------------------------------------------
// Destructors:
// -----------------------------------------------------------------------
CatApiRequestList::~CatApiRequestList (void)
{
  // free up space for each CatApiRequest
  const CatApiRequest *currentRequest = NULL;
  for (Lng32 i = 0; i < numRequests_; i++)
  {
    currentRequest = requestList_[i];
    if (currentRequest)
    {
      delete currentRequest;
      requestList_[i] = NULL;
    }
  }

  // free up space for version info
  if (versionInfo_)
  {
    delete [] versionInfo_;
    versionInfo_ = NULL;
  }
}

// -----------------------------------------------------------------------
// Operator[]:
//
// returns NULL if not found
// -----------------------------------------------------------------------
const CatApiRequest *  
CatApiRequestList::operator[] (Lng32 index) const
{
  if (index <= numRequests_)
    return requestList_[index];
  else
    return NULL;
}

// -----------------------------------------------------------------------
// getTextLength:
//
// returns length of text, terminating null character not included.
// -----------------------------------------------------------------------
const Lng32
CatApiRequestList::getTextLength(void) const
{
  // List header length
#pragma nowarn(1506)   // warning elimination 
  Lng32 length = APIHEADERLEN;
#pragma warn(1506)  // warning elimination 

  // length for maximum requests
#pragma nowarn(1506)   // warning elimination 
  length = length + sizeof (MAXNUMREQUESTS) + 1 /*space*/;
#pragma warn(1506)  // warning elimination 

  // Lengths of each request
  const CatApiRequest *request = NULL;
  for (Lng32 i = 0; i < numRequests_; i++)
  {
    request = requestList_[i];
    if (request)
    {
      length += request->getTextLength();
      length++; // space
    }
  }

  length++;  // final semicolon
  return length;
}

// -----------------------------------------------------------------------
// getText:
//
// returns TRUE if successful, FALSE otherwise
// terminating null character is included
// -----------------------------------------------------------------------
short 
CatApiRequestList::getText(char *text)
{
  // Get request header
  text[0] = '\0';
  strcat(text, APIHEADER);
  size_t strPos = APIHEADERLEN;
  text[strPos] = '\0';

  // Get number of commands sent
  Lng32 length = sprintf( &text[strPos], "%0d", numRequests_ );
  strPos = strPos + length;
  text[strPos++] = ' ';
  text[strPos] = '\0'; // null terminate the string for strcat performed below

  // Get each command
  CatApiRequest *request = NULL;
  for (Lng32 i = 0; i < numRequests_; i++)
  {
    request = requestList_[i];
    if (request == NULL)
      return 0;
    Lng32 length = request->getTextLength();
    char *requestText = new char [length+1];
    if (!request->getText(requestText, 0 /* do not include semicolon*/ ))
      return 0;
    strcat(text, requestText );
    strPos = strPos + strlen(requestText);
    delete [] requestText;
    text[strPos] = '\0'; // space separator already added
  }

  // Add final semicolon and null byte
  text[strPos++] = ';';
  text[strPos] = '\0';
  return 1;
}
 
// -------------------------------------------------------------------
// addRequest:
//
// return TRUE if successful, FALSE otherwise
// -------------------------------------------------------------------
short 
CatApiRequestList::addRequest(CatApiRequest *newRequest)
{
  // This adds a request at the end of the list
  short foundEntry = 0;
  for (Lng32 i = 0; i < MAXNUMREQUESTS; i++)
  {
    if (! requestList_[i])
    {
      requestList_[i] = newRequest;
      foundEntry = 1;
      numRequests_ = i+1;
      break;
    }
  }
  return foundEntry;
}

// -------------------------------------------------------------------
// convertTextToApiRequest:
//
// return TRUE is successful, FALSE otherwise
// -------------------------------------------------------------------
short 
CatApiRequestList::convertTextToApiRequest (const char *text
                                            // , size_t textLen
                                            // , short /*SQLCHARSET_CODE*/ ddlTextCharSet
                                            )
{
  // The first item in the string is the operation type.  Just verify
  // that the correct verbage exists.
  size_t startPos = 0;
  char buf[MAXPARAMSIZE];
  Lng32 scanCount;
  char * endPos = (char *) strchr( text, '&' );
  size_t strPos = endPos - text;
  if (strPos < startPos)
    return 0; // failure
  size_t len = strPos - startPos;
  strncpy (buf, &text[startPos], len);
  buf[strPos] = '\0';
  startPos = ++strPos;
  
  // Get the version control information
  endPos = (char *) strchr( &text[startPos], ' ');
  strPos = endPos - text;
  if (strPos < startPos)
    return 0; // failure
  len = (strPos - startPos);
  delete [] versionInfo_;
  versionInfo_ = new char[len+1];
  strncpy (versionInfo_, &text[startPos], len);
  versionInfo_[len] = '\0';
  if (strcmp(versionInfo_, APIVERSION) != 0)
    return 0;

  // Get list type
  startPos = ++strPos;
  endPos = (char *) strchr(&text[startPos], ' ');
  strPos = (endPos - text);
  if (strPos < startPos)
    return 0; // failure
  len = (strPos - startPos);
  strncpy (buf, &text[startPos], len);
  buf[len] = '\0';
  if (strcmp(buf, APILIST) != 0)
    return 0;

  // Get number requests
  startPos = ++strPos;
  endPos = (char *) strchr(&text[startPos], ' ');
  strPos = endPos - text;
  if (strPos < startPos)
    return 0; // failure
  len = strPos - startPos;
  strncpy (buf, &text[startPos], len);
  buf[len] = '\0';
  Lng32 numRequests;
  scanCount = sscanf (buf, "%d", &numRequests);
  if (scanCount != 1)
    return 0;

  // Get each request
  if (numRequests > MAXNUMREQUESTS)
    return 0;
  for (Lng32 i = 0; i < numRequests; i++)
  {
    startPos = ++strPos;   
    CatApiRequest *request = new CatApiRequest();
    if (!request->convertTextToCatApiRequest(text, startPos))
      return 0;
    addRequest( request );
    strPos = startPos;
  }
  return 1;
}
