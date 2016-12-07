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

#include "common/Global.h"
#include "test/varchar32k/LargeVarcharCol.h"

int main(int argc, char * argv[])
{
	if (argc != 7)
	{
		std::cout << "Usage: " << argv[0] << " -d <datasource> -u <userid> -p <password>" << endl;
		return -1;
	}
	char inputArgs[6][100];
	for (int i = 1; i < argc; i++)
		strcpy(inputArgs[i - 1], argv[i]);

	if ((strcmp(inputArgs[0], "-d") != 0) || (strcmp(inputArgs[2], "-u") != 0) || (strcmp(inputArgs[4], "-p") != 0))
	{
		std::cout << "Usage: " << argv[0] << " -d <datasource> -u <userid> -p <password>" << endl;
		return -1;
	}

	char chDsn[100];
	char chUID[100];
	char chPwd[100];
	strcpy(chDsn, inputArgs[1]);
	strcpy(chUID, inputArgs[3]);
	strcpy(chPwd, inputArgs[5]);

	CLargeVarcharCol * pTestItem = new CLargeVarcharCol(chDsn, chUID, chPwd);
	pTestItem->Run();

	delete pTestItem;
	return 0;
}

