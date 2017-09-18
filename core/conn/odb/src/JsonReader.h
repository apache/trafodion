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

JsonReader *jsonReaderNew(const char *path);

JsonReaderError jsonMoveCurrentCharPtr(JsonReader *pJsonReader);

JsonReaderError jsonSaveAndToState(JsonReader *pJsonReader, JsonReaderState state);

JsonReaderError jsonReaderSetTmpbuf(JsonReader *pJsonReader, char *buf, size_t len);

JsonReaderError jsonReaderUnsetTmpbuf(JsonReader *pJsonReader);

JsonReaderError jsonParseStateStart(JsonReader *pJsonReader);

JsonReaderError jsonParseObjectInitial(JsonReader *pJsonReader);

char *jsonGetStringValue(JsonReader *pJsonReader, char *strValue, size_t len);

JsonReaderError jsonParseMemberKey(JsonReader *pJsonReader);

JsonReaderError jsonParseKeyValueDelimiter(JsonReader *pJsonReader);

JsonReaderError jsonInternalGetValue(JsonReader *pJsonReader, char *endChars, size_t len);

JsonReaderError jsonParseMemberValueEnd(JsonReader *pJsonReader);

JsonReaderError jsonParseElementEnd(JsonReader *pJsonReader);

JsonReaderError jsonParseMemberValue(JsonReader *pJsonReader);

JsonReaderError jsonParseMemberDelimiter(JsonReader *pJsonReader);

JsonReaderError jsonParseObjectFinish(JsonReader *pJsonReader);

JsonReaderError jsonParseArrayInitial(JsonReader *pJsonReader);

JsonReaderError jsonParseElement(JsonReader *pJsonReader);

JsonReaderError jsonParseElementDelimiter(JsonReader *pJsonReader);

JsonReaderError jsonParseArrayFinish(JsonReader *pJsonReader);

JsonReaderError jsonReadKey(JsonReader *pJsonReader, char *keyBuf, size_t len);

JsonReaderError jsonReadMemberValue(JsonReader *pJsonReader, char *memberValbuf, size_t len);

JsonReaderError jsonReadArrayValue(JsonReader *pJsonReader, char *arrayValBuf, size_t len);

JsonReaderError jsonRead(JsonReader *pJsonReader);

JsonReaderError jsonParse(JsonReader *pJsonReader);

void jsonReaderFree(JsonReader *pJsonReader);

#endif //JSONREADER_H