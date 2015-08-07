/**************************************************************************
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

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include "jdbcUtil.h"

#define CMD_INITALIZE			1
#define CMD_STATUS				2
#define CMD_CLEAN				3
#define CMD_BENCHMARK_REPORT	4
#define CMD_EMULATE				5
#define OPTION_NOHEADER			6
#define OPTION_FORCE			7

#define MAX_PARMS	2

bool noHeader = false;

struct {
	int cmd;
	const char *option;
	const char *desc;
} optionStr[] = {
	{ CMD_INITALIZE, "init", "Initialize shared memory" },
	{ CMD_STATUS, "status", "Print status of JDBC/MX processes" },
	{ CMD_CLEAN, "clean", "Clean up dead JDBC/MX processes in process table" },
	{ CMD_BENCHMARK_REPORT, "benchmark_report", "Causes all JDBC/MX processes to create a benchmark report. Params: title dest_dir" },
	{ CMD_EMULATE, "emulate", "Emulate a JDBC driver process for debugging" },
	{ OPTION_NOHEADER, "noheader", "Don't print program header" },
	{ OPTION_FORCE, "force", "Force command operation" },
	{ -1, NULL, NULL }};

int main(int argc, char *argv[])
{
	int i;
	int utilCmd = -1;
	bool force = false;
	int arg_idx=1;
	int rc = 0;
	const char *parameter[MAX_PARMS];
	int total_params = 0;
	char prolog[80];
	char message[80];

	sprintf(prolog,"%s: ",argv[0]);
	message[0] = 0;

	for (i=0; i<MAX_PARMS; i++) parameter[i] = NULL;

	while ((arg_idx<argc) && !rc)
	{
		bool option = (argv[arg_idx][0]=='-');
		if (option)
		{
			int opt_idx = 0;
			bool processed = false;
			while (!processed && !rc && optionStr[opt_idx].option)
			{
				int str_idx = 0;
				while (optionStr[opt_idx].option[str_idx] &&
				       argv[arg_idx][str_idx+1] &&
					   (toupper(argv[arg_idx][str_idx+1])==toupper(optionStr[opt_idx].option[str_idx])))
				   str_idx++;
				if ((optionStr[opt_idx].option[str_idx]==0) &&
					(argv[arg_idx][str_idx+1]==0))
				{
					processed = true;
					switch (optionStr[opt_idx].cmd)
					{
						case OPTION_FORCE:
							force = true;
							break;
						case OPTION_NOHEADER:
							noHeader = true;
							break;
						default:
							if (utilCmd!=-1)
							{
								strcpy(message,"Too many options.");
								rc = 1;
							}
							else
							{
								utilCmd = optionStr[opt_idx].cmd;
							}
					}
				}
				opt_idx++;
			}

		}
		else
		{
			if (total_params>=MAX_PARMS)
			{
				strcpy(message,"Too many parameters.");
				rc = 1;
			}
			else
			{
				parameter[total_params++] = argv[arg_idx];
			}
		}
		arg_idx++;
	}

	if (!rc && (utilCmd==-1))
	{
		strcpy(message,"No command option specified.");
		rc = 1;
	}

	if (!noHeader)
	{
		printf("JDBC/MX Utility Program\n");
		printf("Copyright 2004 Hewlett-Packard Development Company, L.P.\n");
		printf("--------------------------------------------------------\n");
	}

	if (rc)
	{
		if (message[0]) printf("%s %s\n",prolog,message);
		printf("Format: %s [option]\n",argv[0]);
		printf("  Options:\n");
		int opt_idx = 0;
		while (optionStr[opt_idx].option)
		{
			printf("    -%s\n",optionStr[opt_idx].option);
			printf("        %s\n",optionStr[opt_idx].desc);
			opt_idx++;
		}
	}
	else
	{
		JdbcUtil *util = new JdbcUtil();
		switch (utilCmd)
		{
			case CMD_INITALIZE:
				if (total_params!=0)
				{
					printf("%s Memory initialization has no parameters.\n",prolog);
					rc = 1;
				}
				else if (!util->memoryInit())
				{
					printf("%s Error during memory initialization.\n",prolog);
					printf("%s %s\n",prolog,util->getError());
					rc = 1;
				}
				if (rc==0) printf("%s Memory initialized.\n",prolog);

				break;
			case CMD_STATUS:
				if (total_params!=0)
				{
					printf("%s Status has no parameters.\n",prolog);
					rc = 1;
				}
				else if (!util->showStatus())
				{
					printf("%s Error trying to show status.\n",prolog);
					printf("%s %s\n",prolog,util->getError());
					rc = 1;
				}
				break;
			case CMD_CLEAN:
				if (total_params!=0)
				{
					printf("%s Clean has no parameters.\n",prolog);
					rc = 1;
				}
				else if (!util->cleanup())
				{
					printf("%s Error trying to clean up.\n",prolog);
					printf("%s %s\n",prolog,util->getError());
					rc = 1;
				}
				break;
			case CMD_BENCHMARK_REPORT:
				if ((total_params==0) || (total_params>2))
				{
					printf("%s Invalid number of parameters for Report.\n",prolog);
					rc = 1;
				}
				else if (!util->setCmd(JDBCUTIL_CMD_BENCHMARK_REPORT, parameter[0], parameter[1], force))
				{
					printf("%s Error trying to issue benchmark report.\n",prolog);
					printf("%s %s\n",prolog,util->getError());
					rc = 1;
				}
				break;
			case CMD_EMULATE:
				if (total_params!=0)
				{
					printf("%s Emulate has no parameters.\n",prolog);
					rc = 1;
				}
				else
				{
					int i = util->addProcess(getpid());
					if (i<0)
					{
						printf("%s Error trying to add process.\n",prolog);
						printf("%s %s\n",prolog,util->getError());
						rc = 1;
					}
					else
					{
						printf("Waiting for command.\n");
						const struct JdbcUtilProcessTableStruct *cmd;
						while (cmd = util->getCmd())
						{
							 switch (cmd->cmd)
							 {
								 case JDBCUTIL_CMD_NOP:
									 sleep(1);
									 break;
								 case JDBCUTIL_CMD_BENCHMARK_REPORT:
									 printf("Benchmark Report: %s\n",cmd->parameter[0]);
									 break;
								 default:
									 printf("Unknown command %s\n",cmd->cmd);
							 }
						}
						printf("%s Error trying to process commands.\n",prolog);
						printf("%s %s\n",prolog,util->getError());
						rc = 1;
					}
				}
				break;
			default:
				printf("%s Internal Error - Invalid command %s\n",prolog,utilCmd);
				rc = 1;
		}
		delete util;
	}

	if (rc!=0) printf("%s Return Code=%d\n",prolog,rc);
	return rc;
}
