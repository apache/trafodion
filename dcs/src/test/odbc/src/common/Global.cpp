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

#include "Global.h"

char *rTrim(char *string)
{
	char *strPtr;

	for (strPtr = string + strlen(string) - 1;
		strPtr >= string && (*strPtr == ' ' || *strPtr == '\t');
		*(strPtr--) = '\0');
	return(string);
}

int ConvertCharToCNumeric(SQL_NUMERIC_STRUCT& numericTmp, CHAR* cTmpBuf)
{
	unsigned char localBuf[101];
	char* tempPtr = (char*)localBuf, *tempPtr1;
	int i, j, a, b, current, calc, length;

	SQLCHAR tempPrecision;
	SQLCHAR tempScale;
	SQLCHAR tempSign;
	SQLCHAR tmpVal[101];

	if (strlen(rTrim(cTmpBuf)) >= sizeof(tmpVal))
		return -1;

	memset(tmpVal, 0, sizeof(tmpVal));

	length = strlen(strcpy(tempPtr, cTmpBuf));
	if (tempPtr[length - 1] == '.') tempPtr[length - 1] = '\0';

	tempSign = (*tempPtr == '-') ? 0 : 1;

	if (*tempPtr == '+' || *tempPtr == '-') tempPtr++;

	if ((tempPtr1 = strchr(tempPtr, '.')) == NULL)
	{
		tempPrecision = strlen(tempPtr);
		tempScale = 0;
	}
	else
	{
		tempPrecision = strlen(tempPtr) - 1;
		tempScale = strlen(tempPtr1) - 1;
	}

	if (tempPrecision > ENDIAN_PRECISION_MAX)
		return -1;

	for (length = 0, tempPtr1 = (char*)localBuf; *tempPtr != 0; tempPtr++)
	{
		if (*tempPtr == '.') continue;
		*tempPtr1++ = *tempPtr - '0';
		length++;
	}
	memset(tempPtr1, 0, sizeof(localBuf) - length);

	for (j = 0; j < 2 * sizeof(tmpVal); j++)
	{
		a = b = calc = 0;

		for (i = 0; i < length; i++)
		{
			current = localBuf[i];
			calc = calc * 10 + current;
			a = calc % 16;
			b = calc / 16;

			localBuf[i] = b;
			calc = a;
		}
		switch (j % 2)
		{
		case 0:
			tmpVal[j / 2] = a;
			break;
		case 1:
			tmpVal[j / 2] |= a << 4;
			break;
		}
	}

	for (i = sizeof(tmpVal) - 1; i > SQL_MAX_NUMERIC_LEN - 1; i--)
		if (tmpVal[i] != 0)
			return -1;

	numericTmp.sign = tempSign;
	numericTmp.precision = tempPrecision;
	numericTmp.scale = tempScale;
	memcpy(numericTmp.val, tmpVal, SQL_MAX_NUMERIC_LEN);

	return 0;
}
