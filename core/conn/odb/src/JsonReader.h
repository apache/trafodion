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

#ifndef JSONREADER_H
#define JSONREADER_H

#include <stdlib.h>
#include <stdio.h>

#define JSON_PARSER_BUF_LEN 1024
#define JSON_PARSER_ERROR_MESSAGE_LEN 512
#define JSON_PARSER_MAX_FILE_NAME_LEN 256
#define JSON_PARSER_MAX_NESTED_NUM 100

#ifndef bool
#define bool unsigned char
#endif // !_Bool

#ifndef true
#define true 1
#endif // !true

#ifndef false
#define false 0
#endif // !false


enum JsonReaderState_
{
    JSON_STATE_START,
    JSON_STATE_OBJECT_INITIAL,
    JSON_STATE_ARRAY_INITIAL,
    JSON_STATE_MEMBER_KEY,
    JSON_STATE_ELEMENT,
    JSON_STATE_KEY_VALUE_DELIMITER,
    JSON_STATE_ELEMENT_DELIMITER,
    JSON_STATE_ARRAY_FINISH,
    JSON_STATE_MEMBER_VALUE,
    JSON_STATE_MEMBER_DELIMITER,
    JSON_STATE_OBJECT_FINISH,
    JSON_STATE_FINISH
};
typedef enum JsonReaderState_ JsonReaderState;

enum JsonReaderError_
{
    JSON_SUCCESS,
    JSON_CONTINUE,
    JSON_ERROR_DEPTH,
    JSON_ERROR_PARSE_EOF,
    JSON_ERROR_PARSE_UNEXPECTED,
    JSON_ERROR_PARSE_NULL,
    JSON_ERROR_PARSE_BOOLEAN,
    JSON_ERROR_PARSE_NUMBER,
    JSON_ERROR_PARSE_ARRAY,
    JSON_ERROR_PARSE_OBJECT_KEY_NAME,
    JSON_ERROR_PARSE_OBJECT_KEY_SEP,
    JSON_ERROR_PARSE_OBJECT_VALUE_SEP,
    JSON_ERROR_PARSE_STRING,
    JSON_ERROR_PARSE_COMMENT,
    JSON_ERROR_SIZE,
    JSON_ERROR_STATE,
    JSON_ERROR_BAD_FORMAT
};
typedef enum JsonReaderError_ JsonReaderError;

#define IS_NOT_WHITE_SPACE(c) (((c)!=' ') && ((c)!='\t') && ((c)!='\n') && ((c)!='\r'))
// 1 <= len <= 3
#define IN_CHARS(c, cs, len) (((c) == (cs)[0]) || ((len) > 1 && (c) == (cs)[1]) || ((len) > 2 && (c) == (cs)[2]))
#define CASE_VALUE case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':case '-':case 'f':case 't':case 'n':case '"'

struct JsonReader_
{
    bool isBufReady;
    FILE *jsonFile;
    char *tmpSaveBuf; // save parsed contents.
    size_t bufLen;
    size_t nestDepth;
    JsonReaderState state;
    JsonReaderError errorCode;
    char errorMsg[JSON_PARSER_ERROR_MESSAGE_LEN];
    char buf[JSON_PARSER_BUF_LEN];
    char *currentCharPtr;
    char jsonFileName[JSON_PARSER_MAX_FILE_NAME_LEN];
    size_t numberReadBuf;
    size_t lineNum;
    size_t linePos;
    JsonReaderState statesSave[JSON_PARSER_MAX_NESTED_NUM];
};

typedef struct JsonReader_ JsonReader;

/* jsonReaderNew: create a json reader
 *
 * path: path of json file
 * return: pointer to json reader
 */
JsonReader *jsonReaderNew(const char *path);

/* jsonMoveCurrentCharPtr: move current char pointer forward.
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonMoveCurrentCharPtr(JsonReader *pJsonReader);

/* jsonSaveAndToState: save and change parser state
 *
 * pJsonReader: pointer of json reader
 * JsonReaderState : parser state
 * return: json reader error code
 */
JsonReaderError jsonSaveAndToState(JsonReader *pJsonReader, JsonReaderState state);

/* jsonReaderSetTmpbuf: set temp buffer to save parsed contents
 *
 * pJsonReader: pointer of json reader
 * buf: the buf to save contents
 * len: length of buf
 * return: json reader error code
 */
JsonReaderError jsonReaderSetTmpbuf(JsonReader *pJsonReader, char *buf, size_t len);

/* jsonReaderUnsetTmpbuf: unset tempbuf to save parsed contents.
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonReaderUnsetTmpbuf(JsonReader *pJsonReader);

/* jsonParseStateStart: parse json file start
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonParseStateStart(JsonReader *pJsonReader);

/* jsonParseObjectInitial: encuntered '{'
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonParseObjectInitial(JsonReader *pJsonReader);

/* jsonGetStringValue: parse string value
 *
 * pJsonReader: pointer of json reader
 * strValue: pointer to buf to save string
 * len: length of strValue
 * return: strValue
 */
char *jsonGetStringValue(JsonReader *pJsonReader, char *strValue, size_t len);

/* jsonParseMemberKey: parse member key
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonParseMemberKey(JsonReader *pJsonReader);

/* jsonParseKeyValueDelimiter: encountered key value delimiter
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonParseKeyValueDelimiter(JsonReader *pJsonReader);

/* jsonInternalGetValue: common function to get string value
 *
 * pJsonReader: pointer of json reader
 * endChars: encounter these chars function end
 * len: length of end chars
 * return: json reader error code
 */
JsonReaderError jsonInternalGetValue(JsonReader *pJsonReader, char *endChars, size_t len);

/* jsonParseMemberValueEnd: parse member value end
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonParseMemberValueEnd(JsonReader *pJsonReader);

/* jsonParseElementEnd: parse element value end
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonParseElementEnd(JsonReader *pJsonReader);

/* jsonParseMemberValue: parse member value
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonParseMemberValue(JsonReader *pJsonReader);

/* jsonParseMemberDelimiter: parse contents after member delimiter
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonParseMemberDelimiter(JsonReader *pJsonReader);

/* jsonParseObjectFinish: parse contents after object finish
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonParseObjectFinish(JsonReader *pJsonReader);

/* jsonParseArrayInitial: encounter '['
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonParseArrayInitial(JsonReader *pJsonReader);

/* jsonParseElement: parse array element
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonParseElement(JsonReader *pJsonReader);

/* jsonParseElementDelimiter: parse contents after array elements delimiter
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonParseElementDelimiter(JsonReader *pJsonReader);

/* jsonParseArrayFinish: parse contents after array finish
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonParseArrayFinish(JsonReader *pJsonReader);

/* jsonReadKey: parse key
 *
 * pJsonReader: pointer of json reader
 * keyBuf: buf to save key
 * len: keyBuf length
 * return: json reader error code
 */
JsonReaderError jsonReadKey(JsonReader *pJsonReader, char *keyBuf, size_t len);

/* jsonReadMemberValue: parse member value
 *
 * pJsonReader: pointer of json reader
 * memberValbuf: buf to save member value
 * len: memberValbuf length
 * return: json reader error code
 */
JsonReaderError jsonReadMemberValue(JsonReader *pJsonReader, char *memberValbuf, size_t len);

/* jsonReadArrayValue: parse array value
 *
 * pJsonReader: pointer of json reader
 * arrayValbuf: buf to save array value
 * len: arrayValbuf length
 * return: json reader error code
 */
JsonReaderError jsonReadArrayValue(JsonReader *pJsonReader, char *arrayValBuf, size_t len);

/* jsonRead: parse stream to next state
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonRead(JsonReader *pJsonReader);

/* jsonParse: parse stream to end if no error
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
JsonReaderError jsonParse(JsonReader *pJsonReader);

/* jsonReaderFree: free json reader
 *
 * pJsonReader: pointer of json reader
 * return: json reader error code
 */
void jsonReaderFree(JsonReader *pJsonReader);

#endif //JSONREADER_H
