// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef PTPA_MESSAGE_TEXT_PARAMETERS_H_
#define PTPA_MESSAGE_TEXT_PARAMETERS_H_

#include "MalMessageTextParameters.h"

//Forward Reference
namespace google
{
    namespace protobuf
    {
        class Message;
        class Descriptor;
    }
};


//! The class MalPTPAMessageTextParameters is an abstract base class
//! that provides default message text and wraps the interface to 
//! obtain text substitution parameters
class PTPAMessageTextParameters : public Trinity::MalMessageTextParameters
{
public:
    //! Constructor
    PTPAMessageTextParameters();

    //! Destructor
    virtual ~PTPAMessageTextParameters();

    //! obtains the message text for a given message number, and performs
    //! parameter substitution into it
    //! \return the text with parameters substituted
    virtual const char * defaultMessage();

    //! gets the value for the given parameter if present
    //! \param [in] parameterName is the name of the parameter
    //! \return the value of the parameter if present, empty string otherwise
    virtual std::string getParameterValue(std::string & parameterName);

    //! sets default message text (in case text doesn't exist in catalog)
    //! \param [in] defaultMessage is the default message text
    void setDefaultMessage(const std::string & defaultMessage);

    //! sets the current Google protobuf
    //! \param [in] m is a pointer to the Google protocol buffer
    void setProtobuf(const google::protobuf::Message *msg);

private:
    std::string defaultMessage_;
    const google::protobuf::Message *msg_;
};



#endif

