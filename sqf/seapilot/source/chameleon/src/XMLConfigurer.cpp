// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
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

#include <XMLConfigurer.h>
#include <sys/stat.h>

namespace {
    std::string getAsciiXmlCh(const XMLCh* str) {
        char* buf = XMLString::transcode(str);
        std::string res(buf);
        XMLString::release(&buf);
        return res;
    }
}

XMLConfigurer::XMLConfigurer(XMLConfigHandler *cb, std::string const&file):
    initialized_(false),
    file_(file),
    root_(NULL),
    current_(NULL),
    xeh_(this),
    xch_(cb),
    head_(NULL)
{
    if(xch_==NULL)
        return;
    _Action* map = xch_->getActionMap_();
    while(map)
    {
        if(map->key.size()==0)
            break;
        addNode(map->key, map->start, map->end );
        map++;
    }
}

XMLConfigurer::~XMLConfigurer()
{
    delNode(head_);
    try
    {
        delete domParser_;
        domParser_ = NULL;
        XMLPlatformUtils::Terminate();
    }
    catch(XMLException &e)
    {
        std::string msg("XMLPlatformUtils::Terminate(): ");
        msg.append(getAsciiXmlCh(e.getMessage()));
        xch_->handleError(XMLConfigError(XMLConfigError::FATALERROR,std::string(),-1,-1, msg));
    }
}

void XMLConfigurer::initialize()
{
    if(initialized_)
        return;
    try
    {
        XMLPlatformUtils::Initialize();
    }
    catch(XMLException &e)
    {
        std::string msg("XMLPlatformUtils::Initialize(): ");
        msg.append(getAsciiXmlCh(e.getMessage()));
        xch_->handleError( XMLConfigError(XMLConfigError::FATALERROR,std::string(),-1,-1, msg));
        return;
    }
    domParser_ = new XercesDOMParser;
    domParser_->setValidationScheme(XercesDOMParser::Val_Auto);
    domParser_->setDoNamespaces(false);
    domParser_->setDoSchema(false);
    domParser_->setLoadExternalDTD(false);

    domParser_->setErrorHandler(&xeh_);

    currentState_ = -1;
    initialized_ = true;
}

int XMLConfigurer::run()
{
    if(!xch_)
        return 1;
    initialize();
    if(!initialized_)
        return 1;
    struct stat filestatus;
    int ret = stat(file_.c_str(), &filestatus);
    if(ret)
    {
        std::string msg("XML config file #");
        msg.append(file_);
        msg.append(" cannot be opened");
        xch_->handleError(XMLConfigError(XMLConfigError::FATALERROR,std::string(),-1,-1, msg));
        return 1;
    }
    try
    {
        domParser_->parse(file_.c_str() );
        DOMDocument *document = domParser_->getDocument();
        root_ = NULL;
        if(document)
            root_=document->getDocumentElement();
        while(!xch_->isStopped() && buildNextNode())
        {
            if(xch_->isStopped() )
                break;
            XMLConfigurer::ConfigNode *node= findNode(currentKey_);
            if(node)
            {
                currentState_? callEnd(node): callStart(node, currentValue_);
            }
            else
            {
                currentState_? xch_->handleEnd(currentKey_): xch_->handleStart(currentKey_, currentValue_ );
            }
        }

    }
    catch(XMLException &e)
    {
        xch_->handleError(XMLConfigError(XMLConfigError::FATALERROR, std::string(),-1,-1,getAsciiXmlCh(e.getMessage())));
        return 1;
    }
    if(xch_->isStopped())
        return 1;
    return 0;
}

bool XMLConfigurer::buildNextNode()
{
    if(currentState_==1 && current_==root_)
        return false;
    bool foundNext = false;
    if(current_ == NULL )
    {
        current_ = root_;
        foundNext = true;
        currentState_ = 0;
    }
    if(!foundNext && currentState_ == 0)
    {
        DOMNodeList *children = current_->getChildNodes();
        for(size_t index=0; index<children->getLength();index++)
        {
            DOMNode *child = children->item(index);
            if(child->getNodeType() == DOMNode::ELEMENT_NODE)
            {
                current_=child;
                foundNext = true;
                break;
            }
        }
    }
    if(!foundNext && currentState_ ==0)
    {
        foundNext = true;
        currentState_ = 1;
    }
    if(!foundNext)
    {
        while(true )
        {
            if(current_ ->getNextSibling() == NULL)
            {
                current_=current_->getParentNode();
                foundNext = true;
                break;
            }
            current_=current_->getNextSibling();
            if(current_->getNodeType() == DOMNode::ELEMENT_NODE)
            {
                foundNext = true;
                currentState_ = 0;
                break;
            }

        }
    }
    currentKey_.clear();
    currentValue_.clear();
    if(foundNext)
    {
        DOMNode * tempNode = current_;
        while(tempNode != root_)
        {
            currentKey_.insert(0,getAsciiXmlCh(tempNode->getNodeName()));
            currentKey_.insert(0,1,'/');
            tempNode = tempNode->getParentNode();
        }
        currentKey_.insert(0,getAsciiXmlCh(root_->getNodeName()));
        currentKey_.insert(0,1, '/');
        if( currentState_ == 0
                && current_->getChildNodes()->getLength()==1
                && current_->getFirstChild()->getNodeType() == DOMNode::TEXT_NODE )
        {
            currentValue_ = getAsciiXmlCh( ((DOMText*)current_->getFirstChild())->getData() );
        }
    }
    return foundNext;
}

void XMLConfigurer::XMLErrorHandler::warning(SAXParseException const& ex )
{
    handleError(XMLConfigError::WARNING, ex);
}

void XMLConfigurer::XMLErrorHandler::error(SAXParseException const& ex )
{
    handleError(XMLConfigError::ERROR, ex );
}

void XMLConfigurer::XMLErrorHandler::fatalError(SAXParseException const& ex)
{
    handleError(XMLConfigError::FATALERROR, ex);
}

void XMLConfigurer::XMLErrorHandler::resetErrors()
{
}

void XMLConfigurer::XMLErrorHandler::handleError(int level, SAXParseException const & ex)
{
    XMLCh const* XMLSystemId = ex.getSystemId();

    XMLCh const* XMLmessage = ex.getMessage();

    XMLConfigError error(level,getAsciiXmlCh(XMLSystemId), (int32_t)ex.getLineNumber(), (int32_t)ex.getColumnNumber(), getAsciiXmlCh(XMLmessage) );
    xc_->xch_->handleError(error);
}

void XMLConfigurer::delNode(XMLConfigurer::ConfigNode *node)
{
    if(node==NULL)
        return;
    delNode(node->left);
    XMLConfigurer::ConfigNode *right = node->right;
    delete node;
    delNode(right);
}

void XMLConfigurer::addNode(std::string const&key, _HANDLES start, _HANDLEE end )
{
    if(key.size()<2 || key[0] != '/')
        return;
    size_t from=1;
    size_t last = key.find('/', from);
    XMLConfigurer::ConfigNode *tempNode = head_;
    XMLConfigurer::ConfigNode **added = &head_;
    while(last != key.npos)
    {
        if(last == from)
            return;
        if(tempNode == NULL)
        {
            if(from==1)
            {
                if(head_==NULL)
                {
                    *added = new XMLConfigurer::ConfigNode;
                    (*added)->key = key.substr(0,last);
                    from = last+1;
                    last = key.find('/',from);
                    tempNode = (*added) ->left;
                    added = & ((*added)->left);
                }
                else
                    return;
            }
            else
            {
                *added = new XMLConfigurer::ConfigNode;
                (*added)->key = key.substr(0,last);
                from = last+1;
                last = key.find('/', from);
                tempNode = (*added)->left;
                added = & ( (*added)->left );
            }
        }
        else
        {
            if( key.compare(0, last, tempNode->key) == 0 )
            {
                added = & (tempNode->left);
                tempNode = tempNode->left;
                from = last+1;
                last = key.find('/', from);
            }
            else
            {
                added = & (tempNode->right);
                tempNode = tempNode->right;
            }
        }
    }
    if( from == last)
        return;
    while( tempNode!= NULL )
    {
        if(key.compare(tempNode->key) == 0)
        {
            added = &tempNode;
            break;
        }
        added = & (tempNode->right);
        tempNode = tempNode->right;
    }
    *added = new XMLConfigurer::ConfigNode;
    (*added)->key = key;
    (*added) -> start = start;
    (*added) ->end = end;
}

XMLConfigurer::ConfigNode* XMLConfigurer::findNode(std::string const&key)
{
    XMLConfigurer::ConfigNode* result = this->head_;
    while(result!= NULL)
    {
        if(key.compare(0, result->key.size(),result->key) == 0)
        {
            if(key.size() == result->key.size())
                break;
            else
                result = result->left;
        }
        else
            result = result->right;
    }
    return result;
}

void XMLConfigurer::callStart(XMLConfigurer::ConfigNode* node, std::string const& value )
{
    if(node->start)
    {
        (xch_->*(node->start))(node->key, value);
    }
}

void XMLConfigurer::callEnd(XMLConfigurer::ConfigNode* node )
{
    if(node->end)
    {
        (xch_->*(node->end))(node->key);
    }
}

void XMLConfigurer::callStart(std::string const& key, std::string const& value )
{
    XMLConfigurer::ConfigNode * node = findNode(key);
    if(node)
        callStart(node, value);
}

void XMLConfigurer::callEnd(std::string const& key )
{
    XMLConfigurer::ConfigNode * node= findNode(key);
    if(node)
        callEnd(node);
}
