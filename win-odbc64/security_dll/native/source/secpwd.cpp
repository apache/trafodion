/*************************************************************************
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
 **************************************************************************/
//
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "security.h"
#include "secpwd.h"
#include "utils.h"
#include "drvrglobal.h"


#ifdef _WINDOWS
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif

const int BUFSIZE = 1024;
    static void copyFile(char* source, char* target)
throw (SecurityException)
{
    FILE *in = fopen(source, "r");
    if ( in == NULL )
        throw SecurityException(ERR_READ_CERT_FILE, source);

    FILE *out = fopen(target, "w");
    if ( out == NULL )
        throw SecurityException(ERR_WRITE_CERT_FILE, target);

    char buf[BUFSIZE];
    while ( !feof(in) )
    {
        int bread = fread(buf, 1, BUFSIZE, in);
        if (bread == 0) break;
        int bwrite = fwrite(buf, 1, bread, out);
        if ( bread != bwrite )
        {
            fclose(in);
            fclose(out);
            throw SecurityException(ERR_WRITE_CERT_FILE, target);
        }
    }
    fclose(in);
    fclose(out);
}

static int compareFile(char* source, char* target)
{
    FILE *in = fopen(source, "r");
    FILE *out = fopen(target, "r");

    if ( in == NULL || out == NULL ) return 1;

    int retVal = 0;
    char inContent[BUFSIZE], outContent[BUFSIZE];
    while ( !feof(in) )
    {
        int bread_in = fread(inContent, 1, BUFSIZE, in);
        int bread_out = fread(outContent, 1, BUFSIZE, out);

        if ( bread_in != bread_out ||
                memcmp(inContent, outContent, bread_in) )
        {
            retVal = 1;
            break;
        }
    }
    fclose(in);
    fclose(out);
    return retVal;
}

static void TrimRight(char* str)
{
    // this function trims any unwanted spaces at the end of a string.

    char *buffer = str;

    int len = (int)strlen(buffer);
    int index = 0;

    for (index = len - 1; index >= 0; index--) {
        if ((index == 0) && (buffer[index] == ' ')) {
            buffer[index] = '\0';
            break;
        }
        if (buffer[index] != ' ') {
            buffer[index+1] = '\0';
            break;
        }
    }
}

static void TrimLeft(char *str)
{

    // this function trims any unwanted spaces at the beginning of a string.
    int len = strlen(str);
    char *buffer = new char[len + 1];
    strcpy(buffer, str);

    int index = 0;

    for (index = 0; index < len; index++) {
        if ((index == len - 1) && (buffer[index] == ' ')) {
            buffer[0] = '\0';
            strcpy(str ,buffer);
            break;
        }
        if (buffer[index]!= ' ') {
            strcpy(str,buffer+index);
            break;
        }
    }

    delete buffer;
}

#define CER ".cer"
#define ACTIVE_CER ".cer"

static char* buildName(const char* dir, const char* fileName, const char* serverName, const char* suffix)
{
    char *fname = NULL;
    int srvrNmLen = strlen(serverName);
    int sfxLen = strlen(suffix);
    int dirLen = strlen(dir);

    int len = dirLen + strlen(SEPARATOR);
    int fNmLen = srvrNmLen + sfxLen + 1;

    fname = new char[fNmLen];   // Runtime determine the buffer length of the cert file name

    if ( fileName == NULL )
    {
        strcpy_s(fname, fNmLen, serverName);
        strcat_s(fname, fNmLen, suffix);
        fileName = fname;
        len += strlen(fname) + 1;
    }
    else len += strlen(fileName) + 1;

    char* path = new char[len];
    sprintf(path, "%s%s%s\0", dir, SEPARATOR, fileName);
    delete[] fname;

    return path;
}

SecPwd::SecPwd(const char *dir, const char* fileName,
        const char* activeFileName,
        const char* serverName,
        int serverNameLength) throw (SecurityException)
: m_sec(NULL), certFile(NULL), activeCertFile(NULL)
{
    char* dir_from_env = NULL;
    char  certDir[MAX_SQL_IDENTIFIER_LEN + 1];
    char* serverNameGBKToUtf8 = NULL;
    LCID lcid = GetSystemDefaultLCID();
    int  translen = 0;
    char transError[128] = { 0 };

    if (serverName == NULL)
        throw SecurityException (INPUT_PARAMETER_IS_NULL, " - serverName.");

    dir = (dir != NULL) ? dir : getenv("HOME");

    if (dir == NULL)
    {
        //Check HOMEDRIVE (or HOMEDRIVE + HOMEPATH ) for interix
        char *hmdrive=getenv("HOMEDRIVE");
        char *hmpath=getenv("HOMEPATH");
        if (hmdrive != NULL && hmpath != NULL)
        {
            // Fix solution #10-090803-8661
            TrimLeft(hmdrive);
            TrimRight(hmdrive);
            TrimLeft(hmpath);
            TrimRight(hmpath);
            dir_from_env = new char[strlen(hmdrive) + strlen(hmpath) + 1];
            sprintf((char *)dir_from_env, "%s%s\0", hmdrive, hmpath);
        }
        else
            throw SecurityException (HOME_ENVIRONMENT_VAR_IS_NULL, NULL);
    }
    else
    {
        TrimLeft((char *)dir);
        TrimRight((char *)dir);
    }
    if(dir != NULL)
        strncpy(certDir, dir, sizeof(certDir));
    else
    {
        strncpy(certDir, dir_from_env, sizeof(certDir));
        certDir[sizeof(certDir) -1] = '\0';
        delete[] dir_from_env;
    }

    struct stat st;
    if(stat(certDir,&st) != 0)
        throw SecurityException(DIR_NOTFOUND, (char *)certDir);

    if (lcid == 0x804) // if local charset is gbk
    {
        serverNameGBKToUtf8 = (char *)malloc(MAX_SQL_IDENTIFIER_LEN + 1);
        if (TranslateUTF8(TRUE, serverName, serverNameLength,
            serverNameGBKToUtf8, MAX_SQL_IDENTIFIER_LEN, &translen, transError) != SQL_SUCCESS)
        {
            free(serverNameGBKToUtf8);
            serverNameGBKToUtf8 = NULL;
            throw SecurityException(INPUT_PARAMETER_IS_NULL, " - serverName.");
        }
        certFile = buildName(certDir, fileName, serverNameGBKToUtf8, CER);
        activeCertFile = buildName(certDir, activeFileName, serverNameGBKToUtf8, ACTIVE_CER);

        free(serverNameGBKToUtf8);
        serverNameGBKToUtf8 = NULL;
    }
    else
    {
        certFile = buildName(certDir, fileName, serverName, CER);
        activeCertFile = buildName(certDir, activeFileName, serverName, ACTIVE_CER);
    }

    // If activeCertFile does not exist and certFile exist, create activeCertFile
    // by from copying certFile to activeCertFile
    struct stat certStats;
    struct stat aCertStats;

    if ( (stat(certFile, &certStats) == 0 ) &&
            (stat(activeCertFile, &aCertStats) != 0 ))
        copyFile(certFile, activeCertFile);
}

void SecPwd::openCertificate() throw (SecurityException)
{
    if(m_sec != NULL)
    {
        delete m_sec;
        m_sec = NULL;
    }
    m_sec = new Security(activeCertFile);
}

SecPwd::~SecPwd()
{
    delete certFile;
    delete activeCertFile;
    if(m_sec != NULL)
    {
        delete m_sec;
        m_sec = NULL;
    }
}
void SecPwd::encryptPwd(const char *pwd, const int pwdLen,
        const char *rolename, const int rolenameLen,
        const char *procInfo, const int procInfoLen,
        char *pwdkey, int *pwdkeyLen)
throw (SecurityException)
{
    // rolename is optional so can be NULL
    if (!pwd)
        throw SecurityException(INPUT_PARAMETER_IS_NULL, " - pwd.");
    if (!pwdkey)
        throw SecurityException(INPUT_PARAMETER_IS_NULL, " - pwdkey.");
    if (!procInfo)
        throw SecurityException(INPUT_PARAMETER_IS_NULL, " - procInfo.");
    if (*pwdkeyLen != m_sec->getPwdEBufferLen())
        throw SecurityException(BAD_PWDKEY_LENGTH, NULL);

    // If Little Endian, swap the 2 cpu bytes and the 2 pin bytes of the PprocInfo
    if (!m_sec->getLittleEndian())
    {
        ProcInfo *pI = (ProcInfo *) procInfo;
        ProcInfo *pInfo = new ProcInfo;

        pInfo->cpu = swapByteOrderOfS((unsigned int)pI->cpu);
        pInfo->pin = swapByteOrderOfS((unsigned int)pI->pin);
        //	  memcpy((char*)pInfo->seg_name, (char *)pI->seg_name, sizeof(pInfo->seg_name));
        pInfo->timestamp = (long long)swapByteOrderOfLL((unsigned long long)pI->timestamp);
        m_sec->encryptPwd(pwd, pwdLen, rolename, rolenameLen,
                (char *)pInfo, procInfoLen, pwdkey, pwdkeyLen);
        delete pInfo;
    }
    else
        m_sec->encryptPwd(pwd, pwdLen, rolename, rolenameLen,
                procInfo, procInfoLen, pwdkey, pwdkeyLen);
}


int SecPwd::getPwdEBufferLen() throw (SecurityException)
{
    return (m_sec->getPwdEBufferLen());
}

unsigned char* SecPwd::getCertExpDate()
{
    return m_sec->getCertExpDate();
}


    void SecPwd::switchCertificate(char* content, int len)
throw (SecurityException)
{
    FILE *out = fopen(activeCertFile, "w");
    if ( out == NULL )
        throw SecurityException(ERR_WRITE_CERT_FILE, activeCertFile);

    int bwrite = fwrite(content, 1, len, out);

    fclose(out);
    if ( bwrite != len )
        throw SecurityException(ERR_WRITE_CERT_FILE, activeCertFile);

    if(m_sec != NULL)
    {
        delete m_sec;
        m_sec = NULL;
    }
    m_sec = new Security(activeCertFile);
}

int SecPwd::switchCertificate() throw (SecurityException)
{
    // If same name, then no point trying to switch.
    if (strcmp(certFile, activeCertFile) == 0) return 1;

    struct stat inFileStats;
    struct stat outFileStats;
    int retVal = 0;

    if ( stat(certFile, &inFileStats) == 0 )
    {
        if ( stat(activeCertFile, &outFileStats) == 0 )
        {
            // if both files exist and they differ, then copy
            if ( inFileStats.st_size != outFileStats.st_size ||
                    compareFile(activeCertFile, certFile)   )
            {
                copyFile(certFile, activeCertFile);
            }
            else
            {
                // file with same content, don't switch.
                retVal = 1;
            }
        }
        else
        {
            // activeCertFile doesn't exist, copy from certFile
            copyFile(certFile, activeCertFile);
        }
    }
    else retVal = 1;

    if ( retVal == 0 )
    {
        delete m_sec;
        m_sec = new Security(activeCertFile);
    }
    return retVal;
}

char* SecPwd::getCertFile()
{
    return certFile;
}

char* SecPwd::getActiveCertFile()
{
    return activeCertFile;
}

