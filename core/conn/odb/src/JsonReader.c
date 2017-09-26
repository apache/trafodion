//------------------------------------------------------------------
//
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

#include "JsonReader.h"
#include <errno.h>
#include <string.h>

JsonReader *jsonReaderNew(const char *path)
{
    JsonReader *pJsonReader = (JsonReader *)calloc(1, sizeof(JsonReader));
    if (!pJsonReader) {
        return NULL;
    }

    pJsonReader->jsonFile = fopen(path, "r");
    if (!pJsonReader->jsonFile) {
        free(pJsonReader);
        return NULL;
    }

    pJsonReader->jsonFileName = (char *)malloc(strlen(path) + 1);
    if (!pJsonReader->jsonFileName) {
        return NULL;
    }

    strcpy(pJsonReader->jsonFileName, path);
    pJsonReader->nestDepth = 0;
    pJsonReader->state = JSON_STATE_START;
    pJsonReader->errorCode = JSON_SUCCESS;
    pJsonReader->currentCharPtr = pJsonReader->buf;

    return pJsonReader;
}

const char *jsonReaderErrorMessage(JsonReader *pJsonReader) {
    switch (pJsonReader->errorCode)
    {
    case JSON_SUCCESS:
        strcpy(pJsonReader->errorMessage, "success");
        break;
    case JSON_ERROR_DEPTH:
        sprintf(pJsonReader->errorMessage, "nested depth exceeding limit of %d in file:%s, line:%lu, pos:%lu",
            JSON_PARSER_MAX_NESTED_NUM, pJsonReader->jsonFileName, pJsonReader->lineNum, pJsonReader->linePos);
        break;
    case JSON_ERROR_PARSE_EOF:
        strcpy(pJsonReader->errorMessage, "parse to end of file");
        break;
    case JSON_ERROR_STATE:
        strcpy(pJsonReader->errorMessage, "error state");
        break;
    case JSON_ERROR_BAD_FORMAT:
        sprintf(pJsonReader->errorMessage, "unexpected character in file:%s, line:%lu, pos:%lu",
            pJsonReader->jsonFileName, pJsonReader->lineNum, pJsonReader->linePos);
        break;
    default:
        strcpy(pJsonReader->errorMessage, "unexpected error code");
        break;
    }
    return pJsonReader->errorMessage;
}

JsonReaderError jsonMoveCurrentCharPtr(JsonReader *pJsonReader)
{
    if (pJsonReader->isBufReady) {
        ++pJsonReader->currentCharPtr;
    }

    if (pJsonReader->currentCharPtr == pJsonReader->buf + pJsonReader->numberReadBuf) {
        if ((pJsonReader->numberReadBuf = fread(pJsonReader->buf, sizeof(char), JSON_PARSER_BUF_LEN, pJsonReader->jsonFile))) {
            pJsonReader->currentCharPtr = pJsonReader->buf;
            pJsonReader->isBufReady = true;
        }
        else {
            return (pJsonReader->errorCode = JSON_ERROR_PARSE_EOF);
        }
    }

    if (*(pJsonReader->currentCharPtr) == '\n') {
        ++pJsonReader->lineNum;
        pJsonReader->linePos = 0;
    }
    else {
        ++pJsonReader->linePos;
    }

    return pJsonReader->errorCode;
}

JsonReaderError jsonSaveAndToState(JsonReader *pJsonReader, JsonReaderState state)
{
    if (pJsonReader->nestDepth < JSON_PARSER_MAX_NESTED_NUM) {
        pJsonReader->statesSave[(pJsonReader->nestDepth)++] = pJsonReader->state;
        pJsonReader->state = state;
    }
    else {
        pJsonReader->errorCode = JSON_ERROR_DEPTH;
    }

    return pJsonReader->errorCode;
}

JsonReaderError jsonReaderSetTmpbuf(JsonReader *pJsonReader, char *buf, size_t len)
{
    pJsonReader->tmpSaveBuf = buf;
    pJsonReader->bufLen = len;
    return pJsonReader->errorCode;
}

JsonReaderError jsonReaderUnsetTmpbuf(JsonReader *pJsonReader)
{
    pJsonReader->tmpSaveBuf = NULL;
    pJsonReader->bufLen = 0;
    return pJsonReader->errorCode;
}

JsonReaderError jsonParseStateStart(JsonReader *pJsonReader)
{
    while (jsonMoveCurrentCharPtr(pJsonReader) == JSON_SUCCESS) {
        if (IS_NOT_WHITE_SPACE(*(pJsonReader->currentCharPtr))) {
            if (*pJsonReader->currentCharPtr == '{') {
                pJsonReader->state = JSON_STATE_OBJECT_INITIAL;
                break;
            }
            else if (*pJsonReader->currentCharPtr == '[') {
                pJsonReader->state = JSON_STATE_ARRAY_INITIAL;
                break;
            }
            else {
                pJsonReader->errorCode = JSON_ERROR_BAD_FORMAT;
                break;
            }
        }
    }

    return pJsonReader->errorCode;
}

JsonReaderError jsonParseObjectInitial(JsonReader *pJsonReader)
{
    while (jsonMoveCurrentCharPtr(pJsonReader) == JSON_SUCCESS) {
        if (IS_NOT_WHITE_SPACE(*(pJsonReader->currentCharPtr))) {
            switch (*pJsonReader->currentCharPtr)
            {
            case '"':
                pJsonReader->state = JSON_STATE_MEMBER_KEY;
                goto ret;
            case '}':
                pJsonReader->state = JSON_STATE_OBJECT_FINISH;
                goto ret;
            default:
                pJsonReader->errorCode = JSON_ERROR_BAD_FORMAT;
                goto ret;
            }
        }
    }

ret:
    return pJsonReader->errorCode;
}

char *jsonGetStringValue(JsonReader *pJsonReader, char *strValue, size_t len)
{
    bool isEscape = 0;
    size_t pos = 0;

    while (jsonMoveCurrentCharPtr(pJsonReader) == JSON_SUCCESS) {
        if (isEscape) {
            if (strValue && pos < len)
                strValue[pos++] = *pJsonReader->currentCharPtr;
            isEscape = false;
        }
        else {
            switch (*pJsonReader->currentCharPtr) {
            case '"':
                if (strValue && pos < len)
                    strValue[pos++] = '\0';
                goto ret;
            case '\\':
                isEscape = true; 
                break;
            default:
                if (strValue && pos < len)
                    strValue[pos++] = *pJsonReader->currentCharPtr;
                break;
            }
        }
    }

ret:
    if (strValue)
        strValue[len - 1] = '\0';
    return strValue;
}

JsonReaderError jsonParseMemberKey(JsonReader *pJsonReader)
{
    jsonGetStringValue(pJsonReader, pJsonReader->tmpSaveBuf, pJsonReader->bufLen);
    while (jsonMoveCurrentCharPtr(pJsonReader) == JSON_SUCCESS) {
        if (IS_NOT_WHITE_SPACE(*(pJsonReader->currentCharPtr))) {
            if (*pJsonReader->currentCharPtr == ':') {
                pJsonReader->state = JSON_STATE_KEY_VALUE_DELIMITER;
                break;
            }
            else {
                pJsonReader->errorCode = JSON_ERROR_BAD_FORMAT;
                break;
            }
        }
    }

    return pJsonReader->errorCode;
}

JsonReaderError jsonParseKeyValueDelimiter(JsonReader *pJsonReader)
{
    while (jsonMoveCurrentCharPtr(pJsonReader) == JSON_SUCCESS) {
        if (IS_NOT_WHITE_SPACE(*(pJsonReader->currentCharPtr))) {
            switch (*(pJsonReader->currentCharPtr)) {
            CASE_VALUE:
                pJsonReader->state = JSON_STATE_MEMBER_VALUE;
                goto ret;
            case '[':
                jsonSaveAndToState(pJsonReader, JSON_STATE_ARRAY_INITIAL);
                goto ret;
            case '{':
                jsonSaveAndToState(pJsonReader, JSON_STATE_OBJECT_INITIAL);
                goto ret;
            default:
                pJsonReader->errorCode = JSON_ERROR_BAD_FORMAT;
                goto ret;
            }
        }
    }
ret:
    return pJsonReader->errorCode;
}

JsonReaderError jsonInternalGetValue(JsonReader *pJsonReader, char *endChars, size_t len)
{
    const char *pBooleanOrNull = NULL;
    // string
    if (*pJsonReader->currentCharPtr == '"' && pJsonReader->tmpSaveBuf) {
        jsonGetStringValue(pJsonReader, pJsonReader->tmpSaveBuf, pJsonReader->bufLen);
    }
    // maybe false,true,null
    else if ((*pJsonReader->currentCharPtr == 'f' ? (pBooleanOrNull = "false") : 0) ||
        (*pJsonReader->currentCharPtr == 't' ? (pBooleanOrNull = "true") : 0) ||
        (*pJsonReader->currentCharPtr == 'n' ? (pBooleanOrNull = "null") : 0)) {
        size_t lenBN = strlen(pBooleanOrNull);
        size_t pos = 1;
        while ((jsonMoveCurrentCharPtr(pJsonReader) == JSON_SUCCESS) && (pos < lenBN)) {
            if (pBooleanOrNull[pos] != *pJsonReader->currentCharPtr) {
                break;
            }
            ++pos;
        }
        if ((pos < lenBN) || (IS_NOT_WHITE_SPACE(*pJsonReader->currentCharPtr) &&
            !IN_CHARS(*pJsonReader->currentCharPtr, endChars, lenBN))) {
            pJsonReader->errorCode = JSON_ERROR_BAD_FORMAT;
        }
        if (pJsonReader->tmpSaveBuf) {
            strncpy(pJsonReader->tmpSaveBuf, pBooleanOrNull, pJsonReader->bufLen);
        }
    }
    // should be number
    else {
        size_t pos = 0;
        do {
            if (IS_NOT_WHITE_SPACE(*pJsonReader->currentCharPtr) &&
                !IN_CHARS(*pJsonReader->currentCharPtr, endChars, len)) {
                if (pJsonReader->tmpSaveBuf && pos < pJsonReader->bufLen) {
                    pJsonReader->tmpSaveBuf[pos++] = *pJsonReader->currentCharPtr;
                }
            }
            else {
                break;
            }
        } while (jsonMoveCurrentCharPtr(pJsonReader) == JSON_SUCCESS);
        if (pJsonReader->tmpSaveBuf) {
            pJsonReader->tmpSaveBuf[pos < len ? pos : len] = '\0';
        }
    }
    return pJsonReader->errorCode;
}

JsonReaderError jsonParseMemberValueEnd(JsonReader *pJsonReader)
{
    do {
        if (IS_NOT_WHITE_SPACE(*pJsonReader->currentCharPtr)) {
            if (*pJsonReader->currentCharPtr == ',') {
                pJsonReader->state = JSON_STATE_MEMBER_DELIMITER;
                break;
            }
            else if (*pJsonReader->currentCharPtr == '}') {
                pJsonReader->state = JSON_STATE_OBJECT_FINISH;
                break;
            }
            else if (*pJsonReader->currentCharPtr != '"') {
                pJsonReader->errorCode = JSON_ERROR_BAD_FORMAT;
                break;
            }
        }
    } while (jsonMoveCurrentCharPtr(pJsonReader) == JSON_SUCCESS);
    return pJsonReader->errorCode;
}

JsonReaderError jsonParseElementEnd(JsonReader *pJsonReader)
{
    do {
        if (IS_NOT_WHITE_SPACE(*pJsonReader->currentCharPtr)) {
            if (*pJsonReader->currentCharPtr == ',') {
                pJsonReader->state = JSON_STATE_ELEMENT_DELIMITER;
                break;
            }
            else if (*pJsonReader->currentCharPtr == ']') {
                pJsonReader->state = JSON_STATE_ARRAY_FINISH;
                break;
            }
            else if (*pJsonReader->currentCharPtr != '"') {
                pJsonReader->errorCode = JSON_ERROR_BAD_FORMAT;
                break;
            }
        }
    } while (jsonMoveCurrentCharPtr(pJsonReader) == JSON_SUCCESS);

    return pJsonReader->errorCode;
}

JsonReaderError jsonParseMemberValue(JsonReader *pJsonReader) {
    
    if (jsonInternalGetValue(pJsonReader, ",}", 2) == JSON_SUCCESS) {
        jsonParseMemberValueEnd(pJsonReader);
    }

    return pJsonReader->errorCode;
}

JsonReaderError jsonParseMemberDelimiter(JsonReader *pJsonReader)
{
    while (jsonMoveCurrentCharPtr(pJsonReader) == JSON_SUCCESS) {
        if (IS_NOT_WHITE_SPACE(*pJsonReader->currentCharPtr)) {
            if (*pJsonReader->currentCharPtr == '"') {
                pJsonReader->state = JSON_STATE_MEMBER_KEY;
                break;
            }
            else {
                pJsonReader->errorCode = JSON_ERROR_BAD_FORMAT;
                break;
            }
        }
    }
    
    return pJsonReader->errorCode;
}

JsonReaderError jsonParseObjectFinish(JsonReader *pJsonReader)
{
    if (pJsonReader->nestDepth == 0) {
        while (jsonMoveCurrentCharPtr(pJsonReader) == JSON_SUCCESS) {
            if (IS_NOT_WHITE_SPACE(*pJsonReader->currentCharPtr)) {
                pJsonReader->errorCode = JSON_ERROR_BAD_FORMAT;
                break;
            }
        }
    }
    else {
        switch (pJsonReader->statesSave[--(pJsonReader->nestDepth)])
        {
        case JSON_STATE_ARRAY_INITIAL:
        case JSON_STATE_ELEMENT_DELIMITER:
            jsonMoveCurrentCharPtr(pJsonReader);
            jsonParseElementEnd(pJsonReader);
            break;
        case JSON_STATE_KEY_VALUE_DELIMITER:
            jsonMoveCurrentCharPtr(pJsonReader);
            jsonParseMemberValueEnd(pJsonReader);
            break;
        default:
            pJsonReader->errorCode = JSON_ERROR_STATE;
            break;
        }
    }
    return pJsonReader->errorCode;
}

JsonReaderError jsonParseArrayInitial(JsonReader *pJsonReader)
{
    while (jsonMoveCurrentCharPtr(pJsonReader) == JSON_SUCCESS) {
        if (IS_NOT_WHITE_SPACE(*pJsonReader->currentCharPtr)) {
            switch (*pJsonReader->currentCharPtr)
            {
            case '{':
                jsonSaveAndToState(pJsonReader, JSON_STATE_OBJECT_INITIAL);
                goto ret;
            case '[':
                jsonSaveAndToState(pJsonReader, JSON_STATE_ARRAY_INITIAL);
                goto ret;
            case ']':
                pJsonReader->state = JSON_STATE_ARRAY_FINISH;
                goto ret;
            CASE_VALUE:
                pJsonReader->state = JSON_STATE_ELEMENT;
                goto ret;
            default:
                pJsonReader->errorCode = JSON_ERROR_BAD_FORMAT;
                goto ret;
            }
        }
    }

ret:
    return pJsonReader->errorCode;
}

JsonReaderError jsonParseElement(JsonReader *pJsonReader)
{
    if (jsonInternalGetValue(pJsonReader, "],", 2) == JSON_SUCCESS) {
        jsonParseElementEnd(pJsonReader);
    }

    return pJsonReader->errorCode;
}

JsonReaderError jsonParseElementDelimiter(JsonReader *pJsonReader)
{
    while (jsonMoveCurrentCharPtr(pJsonReader) == JSON_SUCCESS) {
        if (IS_NOT_WHITE_SPACE(*pJsonReader->currentCharPtr)) {
            switch (*pJsonReader->currentCharPtr)
            {
            CASE_VALUE:
                pJsonReader->state = JSON_STATE_ELEMENT;
                goto ret;
            case '[':
                jsonSaveAndToState(pJsonReader, JSON_STATE_ARRAY_INITIAL);
                goto ret;
            case '{':
                jsonSaveAndToState(pJsonReader, JSON_STATE_OBJECT_INITIAL);
                goto ret;
            default:
                pJsonReader->errorCode = JSON_ERROR_BAD_FORMAT;
                goto ret;
            }
        }
    }

ret:
    return pJsonReader->errorCode;
}

JsonReaderError jsonParseArrayFinish(JsonReader *pJsonReader)
{
    if (pJsonReader->nestDepth == 0) {
        while (jsonMoveCurrentCharPtr(pJsonReader) == JSON_SUCCESS) {
            if (IS_NOT_WHITE_SPACE(*pJsonReader->currentCharPtr)) {
                pJsonReader->errorCode = JSON_ERROR_BAD_FORMAT;
                break;
            }
        }
    }
    else {
        switch (pJsonReader->statesSave[--(pJsonReader->nestDepth)])
        {
        case JSON_STATE_KEY_VALUE_DELIMITER:
            jsonMoveCurrentCharPtr(pJsonReader);
            jsonParseMemberValueEnd(pJsonReader);
            break;
        case JSON_STATE_ELEMENT_DELIMITER:
        case JSON_STATE_ARRAY_INITIAL:
            jsonMoveCurrentCharPtr(pJsonReader);
            jsonParseElementEnd(pJsonReader);
            break;
        default:
            pJsonReader->errorCode = JSON_ERROR_BAD_FORMAT;
            break;
        }
    }

    return pJsonReader->errorCode;
}

JsonReaderError jsonReadKey(JsonReader *pJsonReader, char *keyBuf, size_t len)
{
    if (pJsonReader->state == JSON_STATE_MEMBER_KEY) {
        jsonReaderSetTmpbuf(pJsonReader, keyBuf, len);
        jsonParseMemberKey(pJsonReader);
        jsonReaderUnsetTmpbuf(pJsonReader);
    }
    else {
        pJsonReader->errorCode = JSON_ERROR_STATE;
    }
    return pJsonReader->errorCode;
}

JsonReaderError jsonReadMemberValue(JsonReader *pJsonReader, char *memberValbuf, size_t len)
{
    if (pJsonReader->state == JSON_STATE_MEMBER_VALUE) {
        jsonReaderSetTmpbuf(pJsonReader, memberValbuf, len);
        jsonParseMemberValue(pJsonReader);
        jsonReaderUnsetTmpbuf(pJsonReader);
    }
    else {
        pJsonReader->errorCode = JSON_ERROR_STATE;
    }
    return pJsonReader->errorCode;
}

JsonReaderError jsonReadArrayValue(JsonReader *pJsonReader, char *arrayValBuf, size_t len)
{
    if (pJsonReader->state == JSON_STATE_ELEMENT) {
        jsonReaderSetTmpbuf(pJsonReader, arrayValBuf, len);
        jsonParseElement(pJsonReader);
        jsonReaderUnsetTmpbuf(pJsonReader);
    }
    else {
        pJsonReader->errorCode = JSON_ERROR_STATE;
    }
    return pJsonReader->errorCode;
}

JsonReaderError jsonRead(JsonReader *pJsonReader) {
    switch (pJsonReader->state)
    {
    case JSON_STATE_START:
        jsonParseStateStart(pJsonReader);
        break;
    case JSON_STATE_OBJECT_INITIAL:
        jsonParseObjectInitial(pJsonReader);
        break;
    case JSON_STATE_ARRAY_INITIAL:
        jsonParseArrayInitial(pJsonReader);
        break;
    case JSON_STATE_MEMBER_KEY:
        jsonParseMemberKey(pJsonReader);
        break;
    case JSON_STATE_ELEMENT:
        jsonParseElement(pJsonReader);
        break;
    case JSON_STATE_KEY_VALUE_DELIMITER:
        jsonParseKeyValueDelimiter(pJsonReader);
        break;
    case JSON_STATE_ELEMENT_DELIMITER:
        jsonParseElementDelimiter(pJsonReader);
        break;
    case JSON_STATE_ARRAY_FINISH:
        jsonParseArrayFinish(pJsonReader);
        break;
    case JSON_STATE_MEMBER_VALUE:
        jsonParseMemberValue(pJsonReader);
        break;
    case JSON_STATE_MEMBER_DELIMITER:
        jsonParseMemberDelimiter(pJsonReader);
        break;
    case JSON_STATE_OBJECT_FINISH:
        jsonParseObjectFinish(pJsonReader);
        break;
    default:
        break;
    }
    return pJsonReader->errorCode;
}

JsonReaderError jsonParse(JsonReader *pJsonReader)
{
    while ((pJsonReader->errorCode == JSON_SUCCESS)) {
        jsonRead(pJsonReader);
    }

    return pJsonReader->errorCode;
}

void jsonReaderFree(JsonReader *pJsonReader)
{
    fclose(pJsonReader->jsonFile);
    if (pJsonReader->jsonFileName) {
        free(pJsonReader->jsonFileName);
    }
    free(pJsonReader);
}
