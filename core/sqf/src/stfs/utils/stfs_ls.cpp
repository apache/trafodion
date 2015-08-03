//////////////////////////////////////////////////////////////////             
///
/// \file     stfs_ls.cpp
/// \brief    STFS_ls utility
///              
//////////////////////////////////////////////////////////////////             

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

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>

#include "stfs/stfslib.h"

#define  VERSION 1

void PrintHelp();
void PrintVersion();
int  ProcessFilename(char *pp_FileName, struct InputOpts pp_InputOpts);
int  ProcessFiles(struct InputOpts pp_InputOpts);

struct InputOpts { 
  bool lv_i, 
       lv_h, 
       lv_k, 
       lv_l, 
       lv_t, 
       lv_S; 
};

int main(int argc, char *argv[])
{
  char *lp_FileName = new char[256];
  int long_opt_index = 0;

  struct option long_options[] = { 
      { "help", 0, NULL, 'p' },  
      { "version", 0, NULL, 'v' }, 
      { 0, 0, 0, 0 }   
  };

  struct InputOpts lv_InputOpts = {false, false, false, false, false, false};  
  char c;

  while (( c = getopt_long(argc, argv, "ihkltS", long_options, &long_opt_index)) != -1)
    switch (c)
    {
      case 'i':
        lv_InputOpts.lv_i = true;
        break;
      case 'h':
        lv_InputOpts.lv_h = true;
        break;
      case 'k':
        lv_InputOpts.lv_k = true;
        break;
      case 'l':
        lv_InputOpts.lv_l = true;
        break;
      case 't':
        lv_InputOpts.lv_t = true;
        break;
      case 'S':
        lv_InputOpts.lv_S = true;
        break;
      case 'p':
        PrintHelp();
        break;
      case 'v':
        PrintVersion();
        break;
      case '?':
        printf("Invalid option entered\n");
        return 1;
      default:
        printf("Error parsing options\n");
        return 1;
     }

  if (optind < argc) {  //There IS a file name input
    while (optind < argc) 
    {
      if (ProcessFilename(lp_FileName, lv_InputOpts) < 0)
      {
        return -1;
      }
    }
    return 0;
  }

  if(ProcessFiles(lv_InputOpts) < 0)
  {
    return -1;
  }
  return 0;
}
  
void PrintHelp()
{
  printf("Usage: stfs_ls [OPTION]... [FILE]...\nList information about STFS files.\n\n\
-h         Report sizes in human readable format, e.g. 1K, 200M, 1.2G.\n\
-i         Display file fragments of each STFS file reported.\n\
-k         Use 1024-byte units, instead byte units, when writing space figures.\n\
-l         Display using long listing format.\n\
-r         Reverse the order of the sort to get reverse collating sequence or oldest first.\n\
-t         Sort by modification time.\n\
-S         Sort by file size.\n\
-1         List one file per line.\n\
--help     Displays this help and exit.\n\
--version  Display version information and exit.\n");
}

void PrintVersion()
{
  int lv_version = VERSION;
  printf("STFS_ls Version: %d\n", lv_version);
}

int  ProcessFilename(char *pp_FileName, struct InputOpts pp_InputOpts) 
{
  bool lv_first = true;
  long * lp_PrevIndex = new long(0);
  stfs_stat *lp_stfsStat = new stfs_stat();
  char *lp_FileName = new char[256];
  
  int lv_Retval = 0;
  do 
  {
    lv_Retval = STFS_stat(-1,pp_FileName, lp_PrevIndex, 077777, lp_stfsStat);
    if(lv_Retval != 0) {
      break;
    }

    if(pp_InputOpts.lv_l) {
      if(lv_first) {
        printf("%-7s   %-7s   %-7s   %-7s   %-10s %-13s %-s\n", "NODE ID","USER ID", 
               "OPENERS", "# FRAG", "SIZE", "LAST MOD", "FILE NAME");
        lv_first = false;
      }
      char timebuf[13]; 
      strftime(timebuf, sizeof(timebuf), "%3b %2d %5R", localtime(&lp_stfsStat->mtime));
      printf("%-7d   %-7d   %-7d   %-7d   %-10ld %-13s %-s\n", lp_stfsStat->nid, lp_stfsStat->uid,
             lp_stfsStat->opens, lp_stfsStat->nfrag, lp_stfsStat->size, timebuf, lp_FileName);
    }
    else {
      if(lv_first) {
        printf("%-6s  %-10s  %-s\n", "# FRAG", "SIZE", "FILE NAME");
        lv_first = false;
      }
      printf("%-6d  %-10ld  %-s\n", lp_stfsStat->nfrag, lp_stfsStat->size, lp_FileName);
    }
    
  } while(lv_Retval == 0);

  if(lv_Retval < 0) {
    printf("Error occurred while obtaining file statistics\n");
    return -1;
  }  

  return 0;
}

int  ProcessFiles( struct InputOpts pp_InputOpts) 
{
  int lv_Retval = 0;
  bool lv_first = true;
  long * lp_PrevIndex = new long(0);
  stfs_stat *lp_stfsStat = new stfs_stat();
  char *lp_FileName = new char[256];
  do 
  {
    lv_Retval = STFS_stat(1,lp_FileName, lp_PrevIndex, 077777, lp_stfsStat);
    if(lv_Retval != 0) {
      break;
    }

    if(pp_InputOpts.lv_l) {
      if(lv_first) {
        printf("%-7s   %-7s   %-7s   %-7s   %-10s %-13s %-s\n", "NODE ID","USER ID", 
               "OPENERS", "# FRAG", "SIZE", "LAST MOD", "FILE NAME");
        lv_first = false;
      }
      char timebuf[13]; 
      strftime(timebuf, sizeof(timebuf), "%3b %2d %5R", localtime(&lp_stfsStat->mtime));
      printf("%-7d   %-7d   %-7d   %-7d   %-10ld %-13s %-s\n", lp_stfsStat->nid, lp_stfsStat->uid,
             lp_stfsStat->opens, lp_stfsStat->nfrag, lp_stfsStat->size, timebuf, lp_FileName);
    }
    else {
      if(lv_first) {
        printf("%-6s  %-10s  %-s\n", "# FRAG", "SIZE", "FILE NAME");
        lv_first = false;
      }
      printf("%-6d  %-10ld  %-s\n", lp_stfsStat->nfrag, lp_stfsStat->size, lp_FileName);
    }
    
  } while(lv_Retval == 0);

  if(lv_Retval < 0) {
    printf("Error occurred while obtaining file statistics\n");
    return 1;
  }  

  return 0;
}
