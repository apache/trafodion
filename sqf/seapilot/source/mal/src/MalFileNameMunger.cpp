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

#include <sys/types.h>  // to get pid_t
#include "MalFileNameMunger.h"
#include <unistd.h>  // to get getpid()
#include <sstream>
//#include <iostream> // $$$ temp for testing


using namespace Trinity;

// Constructor
// \param [in] messageCatalogName is the name of the message catalog
MalFileNameMunger::MalFileNameMunger(const std::string & fileNameTemplate,
                                     int32_t sequenceNumber,
                                     bool addPid,
                                     const std::string & optionalParameter) 
{
    // do all the work in the "munge" method (so we can see local symbols in gdb)

    munge(fileNameTemplate,sequenceNumber,addPid,optionalParameter);
}

void MalFileNameMunger::munge(const std::string & fileNameTemplate,
                              int32_t sequenceNumber,
                              bool addPid,
                              const std::string & optionalParameter) 
{
    // capture the directory portion

    size_t lastSlash = fileNameTemplate.rfind('/'); 
    if (std::string::npos == lastSlash)
    {
        directory_ = ".";  // no directory; so make it "."
    }
    else
    {
        directory_ = fileNameTemplate.substr(0,lastSlash); // includes trailing slash
    }

    // construct the file name prefix 

    size_t lastDot = fileNameTemplate.rfind('.'); 
    if ((std::string::npos != lastDot) &&
        (std::string::npos != lastSlash) &&
        (lastDot < lastSlash))
    {
        // last dot is in directory path, not file name
        lastDot = std::string::npos;
    }
    
    size_t startFileNamePart = 0; // assume no directory path
    if (std::string::npos != lastSlash)
    {
        startFileNamePart = lastSlash + 1; // file name part starts after directory       
    }

    size_t length = fileNameTemplate.size() - startFileNamePart;
    if (std::string::npos != lastDot)
    {
        length = lastDot - startFileNamePart;
    }

    prefix_ = fileNameTemplate.substr(startFileNamePart,length);
    if (optionalParameter.size() > 0)
    {
        prefix_ += '.';
        prefix_ += optionalParameter;      
    }     

    // construct the munged file name
    
    std::string portionToInsert;

    if (optionalParameter.size() > 0)
    {
        portionToInsert += '.';
        portionToInsert += optionalParameter;      
    }
    
    if (addPid)
    {
        pid_t pid = getpid();
        std::stringstream pidString;
        pidString << pid;

        portionToInsert += '.';
        portionToInsert += pidString.str();
    }

    if (sequenceNumber >= 0)
    {       
        std::stringstream sequenceNumberString;
        sequenceNumberString << sequenceNumber;

        portionToInsert += '.';
        portionToInsert += sequenceNumberString.str();    
    }

    // create the munged file name by inserting this new portion before
    // the file qualifier (if there is one) or appending (if there isn't)

    mungedFileName_ = fileNameTemplate;
    lastDot = mungedFileName_.rfind('.'); 
    lastSlash = mungedFileName_.rfind('/'); 
    if ((lastDot == std::string::npos) || 
        ((lastSlash != std::string::npos) && (lastSlash > lastDot)))
    {
        // no dot in file name portion, just append  
        mungedFileName_ += portionToInsert;
    }
    else
    {
        // got a dot in file name portion; put stuff ahead of the last qualifier
        mungedFileName_.insert(lastDot,portionToInsert);
    }  
}

// Destructor
MalFileNameMunger::~MalFileNameMunger(void)
{
    // nothing to do
}


std::string MalFileNameMunger::getMungedFileName(void)
{   
    return mungedFileName_;
}

std::string MalFileNameMunger::getDirectory(void)
{   
    return directory_;
}

std::string MalFileNameMunger::getFileNamePrefix(void)
{   
    return prefix_;
}
