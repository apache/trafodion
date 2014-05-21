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

#include "MalTrinityException.h"
#include "sqevlog/evl_sqlog_writer.h"   // for SQ_LOG_ERR etc. constants

using namespace Trinity;


//extern MalEnvironment & getMalEnv();
//extern void tfds(const char* file,uint32_t line);

void Trinity::malAssertFailure(const char* exprText, const char * file, uint32_t line)
{
    MalTrinityException e(file , line);
    MalTrinityErrorStack* errorStack = e.getMalTrinityErrorStack();
    errorStack->addError(MAL_ASSERTION_FAILURE, SQ_LOG_ERR, exprText, 0);
    throw e;
}

MalTrinityException::MalTrinityException()
{
    triError = new MalTrinityErrorStack();
    sourceFile = "";
    lineNumber = 0;
    deleteStack = true;
}

MalTrinityException::MalTrinityException(string filename,uint32_t line)
{
    triError = new MalTrinityErrorStack();
    sourceFile = filename;
    lineNumber = line;
    deleteStack = true;
}

MalTrinityException::MalTrinityException(MalTrinityErrorStack *t_err)
{
    triError = t_err;
    deleteStack = false;
}

MalTrinityException::MalTrinityException(const MalTrinityException &mte)
{
    triError = new MalTrinityErrorStack(mte.getMalTrinityErrorStackRef());
    sourceFile = mte.getFile();
    lineNumber = mte.getLine();
    deleteStack = true;
}

MalTrinityErrorStack *MalTrinityException::getMalTrinityErrorStack()
{
    return triError;
}

const MalTrinityErrorStack &MalTrinityException::getMalTrinityErrorStackRef() const
{
    return (*triError);
}

MalTrinityException::~MalTrinityException(void)
{
    if(deleteStack && triError != NULL)
    {
        delete triError;
    }
}

void MalTrinityException::setLine(uint32_t line)
{
    lineNumber = line;
}

void MalTrinityException::setFile(string file)
{
    sourceFile = file;
}

uint32_t MalTrinityException::getLine() const
{
    return lineNumber;
}

string MalTrinityException::getFile() const
{
    return sourceFile;
}

void MalTrinityException::setMalTrinityErrorStack(MalTrinityErrorStack *t_err)
{
    if(deleteStack && triError != NULL)
    {
        delete triError;
    }
    triError = t_err;
}

/*
string MalTrinityException::toString()
{
    string info,errorMessage,test;
    stringstream itos;
    itos << lineNumber;
    //need to at some point decide where to push this error up to
    ErrorCode returnError = OK;
    info = "File:" + sourceFile + " Line:" + itos.str();
    for(uint16_t i= 0;i<triError->errorCount();i++)
    {
        //itos.clear();
        itos.str(""); // clear buffer
        errorMessage = getErrorTranslation(triError->getErrorCode(i),returnError);
        itos << i;
        test = itos.str();
        info += " Error" + itos.str() + ": " + errorMessage;
        for(uint16_t j=0;j<4;j++)
        {
            if(triError->getErrorStr(i,j) != "")
            {
                info += ", " + triError->getErrorStr(i,j);
            }
        }
    }
    return info;
}

string MalTrinityException::getErrorTranslation(ErrorCode err,ErrorCode &returnError)
{
    return getErrorTranslation(err,catalog,returnError);
}

string MalTrinityException::getErrorTranslation(ErrorCode err,string cat,ErrorCode &returnError)
{
    ErrorCode   retCode = returnError;

    if(!isCatalogSet)
    {
        //store error
        retCode = ERR_CATALOG_NOT_FOUND;
    }
    string filename;
    string line,errCode;
    uint32_t temp,temp2;

    if (catalogLocation == ".")
    {
        filename = cat;
    }
    else
    {
        filename = catalogLocation + cat;
    }
    ifstream fin(filename.c_str());
    if(!fin)
    {
        //store error
        retCode = FILE_ERR;
    }
    while(getline(fin,line))
    {
        errCode = line.substr(0,line.find_first_of(" "));
        istringstream buffer(errCode);
        buffer >> temp;
        temp2 = err;
        if(temp == temp2)
        {
            return line.substr(line.find_first_of(" ")+1,(line.find_first_of("\n")-line.find_first_of(" ")));
        }
    }
    fin.close();
    //store error
    returnError = retCode;
    return "";
}

bool MalTrinityException::initializeCatalog()
{
    catalogLocation = "";//clear catalogLocation
    catalog = "";//clear catalog

#ifdef _TANDEM_ARCH_
#ifdef _GUARDIAN_TARGET
    string filename = "CATPROP";  // on Guardian, assume local subvolume
#else
    string filename = "../conf/catalog.properties";  // on OSS, look in the conf directory, assuming that the MddDLL is in the bin directory (i.e., this path is relative to the location of the MddDLL)
#endif // _GUARDIAN_TARGET
#else
    string filename = "../conf/catalog.properties"; // on Windows or Linux
#endif // _TANDEM_ARCH_
    string line,value;

    ifstream fin(filename.c_str());
    if(!fin)
    {
        return false;
    }
    while(getline(fin,line))
    {
        if(line.substr(0,1) == "#")
            continue;

        value = line.substr(0,line.find_first_of(" "));
        if(value == "cataloglocation:")
        {
            catalogLocation = line.substr(line.find_first_of(" ")+1,(line.find_first_of("\n")-(line.find_first_of(" "))));
        }
        if(value == "catalog:")
        {
            catalog = line.substr(line.find_first_of(" ")+1,(line.find_first_of("\n")-line.find_first_of(" ")));
        }
    }
    fin.close();
    //check to see if catalogLocation and catalog were found on catalog.properties
    if (catalog == "")
    {
       return false;
    }
    if (catalogLocation == "")
    {
       return false;
    }
    return true;
}
*/

