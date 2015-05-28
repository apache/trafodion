// **********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2014 Hewlett-Packard Development Company, L.P.
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
// **********************************************************************

#include <ctype.h>
#include "XMLUtil.h"
#include <ctype.h>

//
// XMLDocument
//

XMLDocument::XMLDocument(NAHeap *heap, XMLElementMapper &elementMapper)
  : documentElementMapper_(elementMapper),
    documentElement_(NULL),
    currElemPtr_(NULL),
    parser(XML_ParserCreate(NULL))
{
  XML_UseParserAsHandlerArg(parser);
  XML_SetElementHandler(parser, startElementHandler, endElementHandler);
  XML_SetCharacterDataHandler(parser, charDataHandler);

  // The heap used while parsing the XML document is accessed through the
  // XML_GetNAHeap/XML_SetNAHeap functions we added to expat.
  XML_SetNAHeap(parser, heap);

  // Put ptr to XMLDocument in parser's user data. If a shared pointer is used
  // for the XMLDocument this is still ok, because when the reference count
  // for the shared pointer gets to 0 and the XMLDocument gets deleted, the
  // then-dangling conventional pointer stored in the parser data structure will
  // not be accessible because the parser is private to XMLDocument.
  XML_SetUserData(parser, this);
}

XMLElementPtr XMLDocument::parse(const char *buf, Int32 len, Int32 done)
{
  if (documentElement_)
    throw XMLException("Already contains a document; use clear() first");
  if (!XML_Parse(parser, buf, len, done))
    throw XMLException("%s at column %d of line %d",
                       XML_ErrorString(XML_GetErrorCode(parser)),
                       XML_GetCurrentColumnNumber(parser),
                       XML_GetCurrentLineNumber(parser));
  if (done)
    documentElement_ = getCurrentElement(parser);
  return documentElement_;
}

void XMLDocument::serialize(XMLString &xml)
{
  if (documentElement_)
    documentElement_->toXML(xml);
  else
    throw XMLException("This XMLDocument does not currently contain a document");
}

const XMLElementPtr& XMLDocument::getDocumentElement() const
{
  return documentElement_;
}

void XMLDocument::startElementHandler(void *parser,
                                        const char *elemName,
                                        const char **atts)
{
  XMLElementPtr containingElement = getCurrentElement(parser);
  if (containingElement)
    containingElement->startElement(parser, elemName, atts);
  else
    {
      containingElement = ((XMLDocument*)(XML_GetUserData(parser)))->documentElementMapper_(parser, (char*)elemName, atts);
      if (!containingElement)
        throw XMLException("Unknown top-level element: %s", elemName);
      else
        setCurrentElement(parser, containingElement);
    }
}

void XMLDocument::endElementHandler(void *parser, const char *name)
{
  getCurrentElement(parser)->endElement(parser, name);
//  setCurrentElement(parser, getCurrentElement(parser)->getParent());
}

void XMLDocument::charDataHandler(void *parser, const char *data, Int32 len)
{
  // @ZX
  XMLElementPtr ptr = getCurrentElement(parser);
  ptr->charData(parser, data, len);
  //getCurrentElement(parser)->charData(parser, data, len);
}


//
// XMLElement
//

void XMLElement::toXML(XMLString& xml, NABoolean sameLine, NABoolean noEndTag)
{
  // Indent to the current level and write start tag and attributes.
  xml.indent();
  xml.append('<').append(getElementName()).append(' ');
  serializeAttrs(xml);
  if (xml[xml.length()-1] == ' ')
    xml.remove(xml.length()-1);

  // If caller requests abbreviated form for element with no content, make sure
  // the element is actually empty. Note the early return if the abbreviated
  // form is used.
  if (noEndTag)
    {
      size_t len = xml.length();
      serializeBody(xml);
      assertLogAndThrow(CAT_SQL_COMP_QR_COMMON, LL_MVQR_FAIL,
                        len == xml.length(), QRLogicException, 
		        "Tried to omit closing tag, but element has content");
      xml.append("/>");
      xml.endLine();
      return;
    }
  else
    xml.append('>');

  // If content is to appear on same line, serialize the body without any
  // newlines or indentation.
  if (sameLine)
    serializeBody(xml);
  else
    {
      xml.endLine();
      xml.incrementLevel();
      serializeBody(xml);
      xml.decrementLevel();
      xml.indent();
    }

  // Write the end tag and end the line.
  xml.append("</").append(getElementName()).append('>');
  xml.endLine();
}

void XMLElement::stripWhitespace(const char*& data, Int32& len)
{
  const char* endPtr = data + len - 1;
  while (data <= endPtr && isspace(*data))
    data++;
  while (data <= endPtr && isspace(*endPtr))
    endPtr--;
  len = (endPtr - data) + 1;
}

NABoolean XMLElement::isspace(char c)
{
  switch (c)
  {
  case ' ':
  case '\t':
  case '\r':
  case '\n':
    return TRUE;

  default:
    return FALSE;
  }
}

//
// XMLBitmap
//

char XMLBitmap::HexChars_[] = 
  { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

void XMLBitmap::toXML(XMLString& xml) const
{
  Int32 nWords = getWordSize();
  Int32 nBytes = nWords * sizeof(WordAsBits);
  NAString hexString((NASize_T)(nBytes*2));
  for (Int32 i=0; i<nWords; i++)
  {
    // Get the word.
    WordAsBits thisWord = word(i);

    for (Int32 j=0; j<sizeof(WordAsBits)*2; j++)
    {
      // Get the half byte.
      Int32 offset = j*4;
      WordAsBits mask = 0x000F << offset;
      unsigned char thisHalfByte = (unsigned char)((thisWord & mask) >> offset);

      char thisChar = HexChars_[thisHalfByte];
      hexString += thisChar;
    }
  }

  xml.append(hexString);
}  // toXML()

void XMLBitmap::initFromHexString(const char* hexChar, UInt32 nBits)
{
  resize((nBits + BitsPerWord) / BitsPerWord);

  Int32 nWords = getWordSize();
  CollIndex wordStartBit;

  // The outer loop is on 32 bit words.
  for (Int32 wordIndex=0; wordIndex<nWords; wordIndex++)
  {
    wordStartBit = wordIndex * BitsPerWord;

    // Next loop is on 4 bit nibbles (each is 1 hex digit)
    // Note that the index of this loop must be a signed int, because it goes
    // negative for the termination condition.
    for (Int32 nibbleBitOffsetInWord = (sizeof(WordAsBits)*2-1)*4;
             nibbleBitOffsetInWord >= 0;
             nibbleBitOffsetInWord -= 4)
    {
      char thisDigit = *(hexChar++);
      if (thisDigit == '0')
	continue;

      // Finally loop on the bits within the nibble.
      unsigned char thisBits = hexDigitToInt(thisDigit);
      for (Int32 bitInNibble=0; bitInNibble<4; bitInNibble++)
      {
        if ((thisBits >> (3 - bitInNibble)) & BIT0)
          addElementFast(wordStartBit + nibbleBitOffsetInWord + bitInNibble);
      }
    }
  }
}  // initFromHexString()

unsigned char XMLBitmap::hexDigitToInt(char c)
{
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_MVQR_FAIL,
                     (c>='0' && c<='9') || (c>='A' && c<='F'), QRLogicException, 
		      "input to hexDigitToInt() must be a hex digit, not '%c'.", c);
  if (c <= '9')
    return c-'0';
  else
    return 10 + (c-'A');
}

