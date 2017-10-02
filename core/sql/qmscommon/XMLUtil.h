// **********************************************************************
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
// **********************************************************************

#ifndef _XMLUTIL_H
#define _XMLUTIL_H

#include <stdarg.h>
#include "QRSharedPtr.h"
#include "NAString.h"
#include "NABitVector.h"
#include "expat/xmlparse.h"

/**
 * \file
 * Defines utility classes for use in conjunction with the expat XML parser.
 * Included are classes to represent an XML document that gets deserialized
 * (by parsing its textual form) or serialized, iterate over the attributes
 * specified for an element, represent an XML element of some type, provide a
 * mapping from element names to element objects, and construct the textual
 * form of an XML document. The classes for name-to-object mapping and XML
 * elements (XMLElementMapper and XMLElement) are abstract, intended to be
 * subclassed by an application for some specific XML vocabulary.
 */

class XMLException;
class XMLElement;
class XMLString;
class XMLFormattedString;
class XMLAttributeIterator;
class XMLElementMapper;
class XMLDocument;

enum ElementType {
  ET_INVALID = 0
 ,ET_QueryDescriptor
 ,ET_MVDescriptor
 ,ET_ResultDescriptor
 ,ET_PublishDescriptor
 ,ET_QueryMisc
 ,ET_ForcedMVs
 ,ET_JBB
 ,ET_Info
 ,ET_Hub
 ,ET_ExtraHub
 ,ET_Output
 ,ET_JBBCList
 ,ET_Table
 ,ET_Operator
 ,ET_JoinPred
 ,ET_RangePred
 ,ET_ResidualPred
 ,ET_RangeOperator
 ,ET_OpEQ
 ,ET_OpLS
 ,ET_OpLE
 ,ET_OpGT
 ,ET_OpGE
 ,ET_OpBT
 ,ET_NumericVal
 ,ET_StringVal
 ,ET_WStringVal
 ,ET_FloatVal
 ,ET_NullVal
 ,ET_GroupBy
 ,ET_PrimaryGroupBy
 ,ET_DependentGroupBy
 ,ET_MinimalGroupBy
 ,ET_Column
 ,ET_Expr
 ,ET_BinaryOper
 ,ET_UnaryOper
 ,ET_Function
 ,ET_Parameter
 ,ET_Input
 ,ET_MVColumn
 ,ET_MVMisc
 ,ET_JbbResult
 ,ET_JbbSubset
 ,ET_Candidate
 ,ET_MVName
 ,ET_List
 ,ET_Version
 ,ET_Update
 ,ET_Name
 ,ET_Include
 ,ET_Key
};


#ifdef _MEMSHAREDPTR
typedef QRIntrusiveSharedPtr<XMLElement> XMLElementPtr;
#define CAST_TO_ELEM(x) ((QRElementPtr&)x)
#else
typedef XMLElement* XMLElementPtr;
#define CAST_TO_ELEM(x) (x)
#endif

typedef const char** AttributeList;
typedef UInt32 NumericID;
#define WHITESPACE " \t\n\r\v\f"
#define XMLPARSEHEAP XML_GetNAHeap((XML_Parser)parser)

/**
 * Exception thrown when an error occurs in processing or generating an XML
 * message.
 */
class XMLException
{
  public:
    /**
     * Creates an exception with text consisting of the passed template filled in
     * with the values of the other arguments.
     *
     * @param[in] msgTemplate Template for construction of the full message;
     *                        contains printf-style placeholders for arguments,
     *                        passed as part of a variable argument list.
     * @param[in] ... Variable argument list, consisting of a value for each
     *                placeholder in the message template.
     * @return 
     */
    XMLException(const char *msgTemplate ...)
      {
        va_list args;
        va_start(args, msgTemplate);
        vsprintf(msgBuffer_, msgTemplate, args);
        va_end(args);
      }

    virtual ~XMLException()
      {}

    /**
     * Returns the message constructed when the exception was instantiated.
     * @return The full exception message.
     */
    char* getMessage()
      {
        return msgBuffer_;
      }

  private:
    /** Buffer used to construct the message in. */
    char msgBuffer_[200];
}; //XMLException

/**
 * An \c NAString subclass with special formatting capabilities for XML. This
 * class does not actually make use of these capabilities, but provides default
 * (usually empty) definitions of the virtual functions that do. The
 * XMLFormattedString subclass redefines those functions to provide formatted
 * output. This allows functions with a string parameter to which XML will be
 * written to use the formatting functions without being aware of whether
 * formatted, line-oriented XML needs to be produced, or whether a simple stream
 * of XML text will do. That decision is in the hands of the caller, which
 * passes a reference to either a XMLString or a XMLFormattedString.
 */
class XMLString : public NAString
{
  public:
    /**
     * Constructs an empty string.
     * @param[in] heap Heap to be used for allocating the string's internal buffer.
     */
    XMLString(NAMemory *heap = NASTRING_UNINIT_HEAP_PTR)
      : NAString(heap)
      {}

    virtual ~XMLString()
      {}

    /**
     * This function is effectively a no-op; it is redefined in the subclass
     * used for writing formatted XML.
     *
     * @return 0, since nesting level is not relevant to the writing of an
     *         unformatted string.
     */
    virtual short getLevel() const
      {
        return 0;
      }

    /**
     * This function is effectively a no-op; it is redefined in the subclass
     * used for writing formatted XML.
     *
     * @return Reference to this object.
     */
    virtual XMLString& incrementLevel()
      {
        return *this;
      }

    /**
     * This function is effectively a no-op; it is redefined in the subclass
     * used for writing formatted XML.
     *
     * @return Reference to this object.
     */
    virtual XMLString& decrementLevel()
      {
        return *this;
      }

    /**
     * This function is effectively a no-op; it is redefined in the subclass
     * used for writing formatted XML.
     *
     * @return Reference to this object.
     */
    virtual XMLString& indent()
      {
        return *this;
      }

    /**
     * Appends a newline to the string.
     *
     * @return Reference to this object.
     */
    XMLString& endLine()
      {
        append('\n');
        return *this;
      }

    /**
     * Appends the passed text to this string without any formatting.
     *
     * @param[in] charData Content to be appended to this string.
     * @return Reference to this object.
     */
    virtual XMLString& appendCharData(NAString& charData)
      {
        append(charData);
        return *this;
      }

    /**
     * Appends the passed text to this string without any formatting.
     *
     * @param[in] charData Content to be appended to this string.
     * @return Reference to this object.
     */
    virtual XMLString& appendCharData(char *charData)
      {
        append(charData);
        return *this;
      }
}; // XMLString

/**
 * String class that can be used to produce a formatted XML document. In
 * addition to the standard capabilities inherited from \c NAString, this
 * class contains methods for controlling line termination and indentation.
 * These must be explicitly invoked as the XML document is written -- there
 * is no parsing performed herein to determine the proper indentation, etc.,
 * based on the element structure of the document being written. Code that
 * uses this may look like the following:
 *
 * \verbatim
  XMLFormattedString xml;
  xml.append("<A>");
  xml.endLine();
  xml.incrementLevel();
  xml.indent();
  xml.append("<B>");
  xml.endLine();
  xml.incrementLevel();
  xml.appendCharData("hello");
  xml.decrementLevel();
  xml.indent();
  xml.append("</B>");
  xml.endLine();
  xml.decrementLevel();
  xml.indent();
  xml.append("</A>");
  xml.endLine();
  printf(xml.data());
  \endverbatim
 *
 * which will produce the following output:
 *
  \verbatim
  <A>
    <B>
      hello
    </B>
  </A>
  \endverbatim
 *
 * Although it would be easier to write this particular example by inserting the
 * newlines and indentation directly, the intended use of this class is for code
 * that will generate either formatted or unformatted XML without change. If the
 * preceding code was in the body of a function and \c xml was a parameter declared
 * as a XMLString&, it would work either way depending on whether the argument
 * passed to it was a XMLString or a XMLFormattedString.
 */
class XMLFormattedString : public XMLString
{
  public:
    /**
     * Constructs an empty string using the given heap and indentation increment.
     * @param[in] heap       Heap to be used for allocating the string's internal buffer.
     * @param[in] indentSize Number of spaces to increase indentation by at each level.
     */
    XMLFormattedString(NAMemory *heap = NASTRING_UNINIT_HEAP_PTR, short indentSize = 2)
      : XMLString(heap), level_(0), indentSize_(indentSize)
      {}

    virtual ~XMLFormattedString()
      {}

    /**
     * Returns the nesting level for this string. The level starts at 0. It goes
     * up by one each time #incrementLevel is called, and down by one when
     * #decrementLevel is called.
     *
     * @return Current nesting level for the XML content being written to this
     *         string.
     */
    virtual short getLevel() const
      {
        return level_;
      }

    /**
     * Adds one to the current nesting level of the XML content being written
     * to this string.
     *
     * @return Reference to this object.
     */
    virtual XMLString& incrementLevel()
      {
        level_++;
        return *this;
      }

    /**
     * Subtracts one from the current nesting level of the XML content being written
     * to this string.
     *
     * @return Reference to this object.
     */
    virtual XMLString& decrementLevel()
      {
        if (level_ > 0)
          level_--;
        return *this;
      }

    /**
     * Appends a number of spaces to the string equal to the current nesting
     * level times the indentation increment.
     *
     * @return Reference to this object.
     */
    virtual XMLString& indent()
      {
        append(' ', level_ * indentSize_);
        return *this;
      }

    /**
     * Appends character content to the XML string. #indent is called before the
     * data is appended, and #endLine is called after it. If there is no data,
     * the string is left unchanged.
     *
     * @param[in] charData The text to be added to this string as the character
     *                     content of an element.
     * @return Reference to this object.
     */
    virtual XMLString& appendCharData(NAString &charData)
      {
        if (charData.length() == 0)
          return *this;
        indent();
        append(charData);
        endLine();
        return *this;
      }

    /**
     * Appends character content to the XML string. #indent is called before the
     * data is appended, and #endLine is called after it. If there is no data,
     * the string is left unchanged.
     *
     * @param[in] charData The text to be added to this string as the character
     *                     content of an element.
     * @return Reference to this object.
     */
    virtual XMLString& appendCharData(char *charData)
      {
        if (!charData || !*charData)
          return *this;
        indent();
        append(charData);
        endLine();
        return *this;
      }

  private:
    /** Current nesting level of the XML being written to the string. */
    short level_;

    /** Number of spaces to indent for each level */
    short indentSize_;
}; // XMLFormattedString

/**
 * Iterator class for an element's attribute list returned by expat. The format
 * of the list is a sequence of null-terminated strings laid end-to-end,
 * consisting of each attribute immediately followed by its value. Thus, this
 * iterator may be used for any sequence of string pairs laid out in a similar
 * fashion. 
 */
class XMLAttributeIterator
{
  public:
    /**
     * Instantiates an iterator for the given attribute list.
     *
     * @param atts The attribute list.
     */
    XMLAttributeIterator(AttributeList atts)
      : atts_(atts),
        inx_(-2),
        inRange_(FALSE)
      {}

    /**
     * Indicates whether there are more attribute/value pairs to iterate over.
     * #next should not be called unless this returns \c TRUE.
     *
     * @return \c TRUE if #next can be safely called, \c FALSE if the last
     *         attribute has been seen.
     */
    NABoolean hasNext() const
      {
        return *(atts_+(inx_+2)) != NULL;
      }

    /**
     * Makes available the next attribute/value pair. #hasNext should be used
     * before calling this function to ensure that another pair is available,
     * because an exception is thrown if one is not.
     */
    void next()
      {
        if (hasNext())
          {
            inx_ += 2;
            inRange_ = TRUE;
          }
        else
          throw XMLException("Attribute index out of range");
      }

    /**
     * Returns the name of the attribute this iterator is currently positioned
     * on.
     *
     * @return The name of the attribute.
     */
    const char* getName() const
      {
        if (!inRange_)
          throw XMLException("Attribute index out of range");
        return *(atts_+inx_);
      }

    /**
     * Returns the value of the attribute this iterator is currently positioned
     * on.
     *
     * @return The value of the attribute.
     */
    const char* getValue() const
      {
        if (!inRange_)
          throw XMLException("Attribute index out of range");
        return *(atts_+inx_+1);
      }

  private:
    // Copy construction/assignment not defined.
    XMLAttributeIterator(const XMLAttributeIterator&);
    XMLAttributeIterator& operator=(const XMLAttributeIterator&);

    AttributeList atts_;
    Int32 inx_;
    NABoolean inRange_;
}; // XMLAttributeIterator


/**
 * Abstract functor that is the base class for implementation-specific mappings
 * of element names to element objects. These mappings are necessary only for
 * elements that can serve as the outermost element in a document. For all other
 * elements, the determination of the class to instantiate for a given name is
 * made in the containing element's implementation of XMLElement#startElement.
 * In the case of a document (outermost) element, there is no containing
 * element, so the implementation of a specific XML vocabulary must subclass
 * this class and redefine the virtual #operator()(void*, char*, const char**)
 * to return a pointer to an object representing the named element. For a
 * vocabulary defining a single possible document element, this could be as
 * simple as returning a \c new of the appropriate class.
 */
class XMLElementMapper
{
  public:
    /**
     * Maps the passed element name to a new instance of the class representing
     * that element.
     *
     * @param parser Pointer to the parser being used.
     * @param elementName Element name being mapped.
     * @param atts List of attributes of the element, needed when invoking the
     *             constructor for its class.
     * @return Pointer to an instance of the class representing the named element.
     */
    virtual XMLElementPtr operator()(void *parser,
                                     char *elementName,
                                     AttributeList atts) = 0;
}; // XMLElementMapper

/**
 * Wrapper class for an XML document that is being parsed. This class contains
 * an expat parser object, functions to handle parser callbacks, and a pointer
 * to the document element (the outermost element of the document) of the parsed
 * document.
 * \n\n
 * To parse a document, instantiate this class with an \c NAHeap to be used for
 * the element objects that are created, and call the #parse function one or more
 * times. The document can be parsed in chunks by calling #parse and passing 0
 * to the \c done parameter on the first n-1 of n calls, and passing 1 on the nth
 * call with the final part of XML text. To reuse the same %XMLDocument to parse
 * a subsequent XML document, call #clear before any calls to #parse for the new
 * document.
 * \n\n
 * The parser used is expat, which uses a SAX-like interface, making callbacks
 * to the application in response to events such as parsing a start or end tag,
 * character content of an element, etc. This class uses static member
 * functions as the handlers for these events, and forwards the calls to the
 * current element object (an instance of a subclass of XMLElement) being parsed.
 */
class XMLDocument
{
  public:
    /**
     * Handles an event that occurs when an XML start tag is parsed. The element
     * name is passed along with the attributes that appear in the tag. This
     * function merely calls the XMLElement#startElement function of the object
     * representing the element within which this tag appeared, unless it is a
     * top-level element, in which case an element object is created for it.
     *
     * @param[in] xmlParser Pointer to the instance of the parser being used.
     * @param[in] elemName  Name of the element in the start tag.
     * @param[in] atts      Array of pointers to attribute name/value pairs.
     */
    static void startElementHandler(void *xmlParser,
                                    const char *elemName,
                                    const char **atts);

    /**
     * Handles an event that occurs when an XML end tag is parsed.  This
     * function merely calls the XMLElement#endElement function of the object
     * representing the element within which this tag appeared.
     *
     * @param[in] xmlParser Pointer to the instance of the parser being used.
     * @param[in] name      Element name appearing in the end tag.
     */
    static void endElementHandler(void *xmlParser, const char *name);

    /**
     * Handles an event that occurs when character content of an element is
     * parsed. Forwards the call to the XMLElement#charData function of the
     * object representing the element within which it appears.
     *
     * @param[in] xmlParser Pointer to the instance of the parser being used.
     * @param[in] charData  Text appearing as content of an element.
     * @param[in] len       Length of the text.
     */
    static void charDataHandler(void *xmlParser, const char *charData, Int32 len);

    /**
     * Returns a pointer to the object representing the current element being
     * parsed. This is the element of the most recently seen start tag.
     *
     * @param[in] parser Pointer to the instance of the parser being used.
     * @return Pointer to the current element being parsed.
     */
    static XMLElementPtr& getCurrentElement(void *parser)
      {
        return ((XMLDocument*)XML_GetUserData(parser))->currElemPtr_;
      }

    /**
     * Sets the new current element. This is called when a start tag is parsed
     * and a new object (of a subclass of XMLElement) is created to represent the
     * element.
     *
     * @param[in] parser Pointer to the instance of the parser being used.
     * @param[in] currElem Pointer to the object representing the element just
     *                     entered.
     */
#ifdef _MEMSHAREDPTR
    template <class T>
    static void setCurrentElement(void *parser, T &currElem)
      {
        ((XMLDocument*)XML_GetUserData(parser))->currElemPtr_ = currElem;
      }
#else
    static void setCurrentElement(void *parser, XMLElementPtr currElem)
      {
        ((XMLDocument*)XML_GetUserData(parser))->currElemPtr_ = currElem;
      }
#endif

    /**
     * Creates a XMLDocument object with the specified heap. The XML parser is
     * initialized, and callback functions to handle parsing events are registered.
     *
     * @param[in] heap \c NAHeap to use for objects allocated while parsing a
     *                 document.
     * @param[in] elementMapper Function object that is called to instantiate
     *                          the class representing an element with a given
     *                          name.
     */
    XMLDocument(NAHeap *heap, XMLElementMapper &elementMapper);

    /**
     * Destroys this XMLDocument, releasing its parser.
     */
    virtual ~XMLDocument()
      {
        XML_ParserFree(parser);
      }

    /**
     * Parses the XML text in the buffer. A document can be parsed in multiple
     * calls by passing 0 as the value of \c done in all calls but the last for
     * the document. On this final call, a pointer to the object representing the
     * document (outermost) element of the parsed document is returned.
     * \n\n
     * The %XMLDocument class interacts with the parser during the execution of
     * this function by receiving callbacks in response to events recognized by
     * the parser, such as start and end tags, and character content.
     *
     * @param[in] buf Character buffer containing XML text to be parsed.
     * @param[in] len Length of the buffer.
     * @param[in] done 1 if the text in the buffer includes the end of the
     *                 document, 0 if additional text is to follow in subsequent
     *                 calls.
     * @return If \c done is 0, NULL is returned; if \c done is 1 and the document
     *         has successfully been parsed, a pointer to the document (outermost)
     *         element is returned.
     */
    XMLElementPtr parse(const char *buf, Int32 len, Int32 done);

    /**
     * Converts the hierarchy of objects representing an XML document into its
     * serialized (textual) form. This is done by calling the XMLElement::toXML
     * function of the document element, which recursively writes the text.
     *
     * @param[out] xml String to write the serialization of the XML document to.
     */
    void serialize(XMLString &xml);

    /**
     * Returns a pointer to the outermost element of this document. This is
     * rarely necessary, since #parse returns the same information.
     *
     * @return Pointer to the object representing the outermost element of the
     *         document.
     */
    const XMLElementPtr& getDocumentElement() const;

    /**
     * Reinitializes this %XMLDocument, so that a new XML document can be parsed.
     */
    void clear()
      {
        currElemPtr_ = NULL;
        documentElement_ = NULL;
      }

  private:
    /** Function object used to map an element name to a new instance of the
     *  class that represents it.
     */
    XMLElementMapper &documentElementMapper_;

    /** Pointer to the outermost element of the parsed document. */
    XMLElementPtr documentElement_;
 
    /** Pointer to the current element being parsed. */
    XMLElementPtr currElemPtr_;

    /** Instance of parser being used for the document. */
    XML_Parser parser;
}; // XMLDocument

/**
 * Abstract superclass of all classes representing XML elements. Each instance
 * of an element is represented by an object, and the structure of the XML
 * document is maintained by links to the parent of each object. This class
 * contains virtual functions for handling parser callbacks, serializing the
 * element and its attributes, and retrieving the element name.
 */
class XMLElement : public NAIntrusiveSharedPtrObject
{
    friend class XMLDocument;
  public:
    /**
     * Writes the XML text of this element. The start tag, including any
     * attributes, is generated, followed by the element contents and the end
     * tag. The function is called recursively for any contained elements.
     *
     * @param[out] xml String to which the XML text is written.
     * @param[in] sameLine If TRUE, the content and end tag will follow the start
     *                     tag directly, without added whitespace.
     * @param[in] noEndTag If TRUE, use the concise element notation that omits
     *                     the end tag. Only possible if element has no content
     *                     (i.e., has only attributes).
     */
    virtual void toXML(XMLString& xml,
                       NABoolean sameLine = FALSE,
                       NABoolean noEndTag = FALSE);

    /**
     * Returns the name of this element. Namespaces are not used in the Query
     * Rewrite descriptor elements.
     *
     * @return The element name.
     */
    virtual const char *getElementName() const = 0;

    /**
     * Get the element type as an enum.
     * @return 
     */
    virtual ElementType getElementType() const = 0;

    /**
     * Returns the name of this class for use in logging messages. For
     * convenience, the element name is used instead of the actual class
     * name. They are the same except for a prefix, and the name only
     * appears in internal logs generated by the code that uses shared pointers
     * to track memory management.
     *
     * @return Name to be used for the class in messages logged regarding
     *         memory management.
     */
    virtual const char* className() const
      {
        return getElementName();
      }

  protected:
    /**
     * Constructs an object representing an XML element. If the element is not
     * the outermost element in the document, \c parent should be non-null. If
     * dynamically allocated, the file and line number where the constructor
     * was invoked may be passed, and are used for logging memory management
     * messages, if memory checking is enabled (by conditional compilation).
     *
     * @param[in] heap Heap from which to allocate internal data members.
     * @param[in] parent Pointer to the object for the element containing this one.
     * @param[in] fileName Name of the file from which this constructor was called.
     * @param[in] lineNumber Line number at which this constructor was called.
     */
    XMLElement(XMLElementPtr parent = NULL, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap)),
        parent_(parent)
      {}

    /**
     * Writes this element's attribute list to the designated string in standard
     * XML syntax.
     *
     * @param[out] xml String to write the attribute/value pairs to.
     */
    virtual void serializeAttrs(XMLString& xml) {};

    /**
     * Writes the body of an XML element to the designated string in
     * standard XML syntax. The body consists of everything between the start
     * tag (which also includes the attributes) and the end tag, including
     * character content and any nested elements.
     *
     * @param[out] xml String to write the body of the element to.
     */
    virtual void serializeBody(XMLString& xml)  = 0;

    /**
     * Handles a callback when the parser recognizes a start tag while an instance
     * of this class represents the current element. An object is created to
     * represent the element designated by the start tag, and this new element
     * becomes the current element. The previous current element is linked to the
     * new one as the new one's parent.
     *
     * @param[in] parser      Pointer to the instance of the parser being used.
     * @param[in] elementName Name of the element in the start tag.
     * @param[in] atts        Array of pointers to attribute name/value pairs.
     */
    virtual void startElement(void *parser, const char *elementName, const char **atts) = 0;

    /**
     * Handles a callback when the parser recognizes an end tag while an instance
     * of this class represents the current element. Since the current element is
     * changed after each occurrence of a start tag, the only end tag processed by
     * an object is the one that matches its own start tag.
     * 
     * @param[in] parser      Pointer to the instance of the parser being used.
     * @param[in] elementName Element name appearing in the end tag.
     */
    virtual void endElement(void *parser, const char *elementName)
      {
        //XML_SetUserData(parser, getCurrentElement(parser)->getParent());
        XMLDocument::setCurrentElement(parser, getParent());
        setParent(NULL);  // decrement ref count for shared ptr to parent
      }

    /**
     * Handles a callback when the parser recognizes character data within an
     * element while an instance of this class represents the current element.
     *
     * @param[in] parser Pointer to the instance of the parser being used.
     * @param[in] data   Text appearing as content of an element.
     * @param[in] len    Length of the text.
     */
    virtual void charData(void *parser, const char *data, Int32 len)
      {
        // Throw exception if there is any character date (ignore whitespace-only
        // text). This function is overridden for elements that can have character
        // content.
        if (strspn(data, WHITESPACE) < (size_t)len)
          throw XMLException("Character content not allowed for element %s",
                                      getElementName());
      }

    /**
     * Returns the parent of this element object. The parent of an element is
     * the element that immediately contains it. The outermost element of a
     * document references itself as the parent.
     *
     * @return Pointer to the element object that immediately contains this one.
     */
    XMLElementPtr getParent() const
      {
        return parent_;
      }

    /**
     * Sets the parent of this element object. The parent is the object
     * representing the element that immediately contains the one that this
     * object represents.
     *
     * @param[in] parent Pointer to element object to link to as the parent of this
     *                   object.
     */
    void setParent(XMLElementPtr parent)
      {
        parent_ = parent;
      }

    /**
     * Removes leading and trailing whitespace from the passed string.
     *
     * @param [in,out] data Pointer to char buffer; will be advanced beyond
     *                      initial whitespace.
     * @param [in,out] len  Length of string; leading/trailing whitespace will
     *                      be subtracted.
     */
    void stripWhitespace(const char*& data, Int32& len);

    NABoolean isspace(char c);

  private:
    // Copy construction/assignment not defined.
    XMLElement(const XMLElement&);
    XMLElement& operator=(const XMLElement&);

    /** Pointer to parent of this object. */
    XMLElementPtr parent_;
}; // XMLDocument

/**
 * Add fast serialize and deserialie methods to/from a hexadecimal string, 
 * to the NABitVector (Based on NASubCollection) class.
 * We can consider using ClusteredBitmap instead later on.
 */
class XMLBitmap : public NABitVector
{
public:
  /**
   * Minimal constructor
   * @param *heap Heap from to allocate internal data members.
   */
  XMLBitmap(CollHeap *heap)
    : NABitVector(heap)
  {
    clear();
  }

  /**
   * Constructor initialing the bitmap size.
   * @param initSize Required bitmap size in bits.
   * @param *heap Heap from to allocate internal data members.
   */
  XMLBitmap(size_t initSize, CollHeap *heap)
    : NABitVector(heap)
  {
    clear();
    resize(wordNo(initSize)+1);
  }

  virtual ~XMLBitmap()
  { }

  virtual NABoolean isEmpty() const
  {
    return NABitVector::isEmpty() || getWordSize() == 0;
  }

  /**
   * Serialize the bitmap as a hex string.
   * @param xml [OUT] The target XML string.
   */
  void toXML(XMLString& xml) const;

  /**
   * Initialize the bitmap from a hex string.
   * @param hexString The input hex string.
   * @param nBits Required bitmap size in bits.
   */
  void initFromHexString(const char* hexString, UInt32 nBits);

private:

  static char HexChars_[16];

  unsigned char hexDigitToInt(char c);

}; // class XMLBitmap

#endif  /* _XMLUTIL_H */
