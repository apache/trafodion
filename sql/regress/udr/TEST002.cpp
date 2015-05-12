// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2000-2015 Hewlett-Packard Development Company, L.P.
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

#include <stdio.h>
#include <string.h>
#include "sqludr.h"


using namespace tmudr;

/*
tokenizer[1] (TABLE doc_table, char pattern)
*/

class Tokenizer : public UDR
{
public:

  // override the runtime method
  void processData(UDRInvocationInfo &info,
                   UDRPlanInfo &plan);
};

extern "C" UDR * TOKENIZER()
{
  return new Tokenizer();
}

void Tokenizer::processData(UDRInvocationInfo &info,
                            UDRPlanInfo &plan)
{
  std::string pattern = info.par().getString(0);
  std::string inRow;
  char textBuf[10000];
  char *token;
  std::string fileUDR(".TOKENIZER1");
  const std::string &udrName = info.getUDRName();
  // The behavior of this UDR depends on its SQL name:
  // Generally we read the text to tokenize from the
  // input table. If the name of the UDR is TOKENIZER1,
  // however, the input table is a list of file names
  // to read an tokenize.
  bool readFromFile = (udrName.substr(udrName.size() - fileUDR.size()) == fileUDR);
  FILE *fp = NULL;

  // check UDR info to make sure it matches our expectations
  if (info.getNumTableInputs() != 1)
    throw UDRException(38001, "Expecting a single table-valued input");
  if (info.in().getNumColumns() != 1)
    throw UDRException(38002,
                       "Table Mapping UDF %s expects only one column in the table input",
                       info.getUDRName().c_str());

  // getString eliminates trailing blanks, check
  // for that and put the blank back
  if (pattern.size() == 0)
    pattern = " ";

  while (getNextRow(info))
    {
      inRow = info.in().getString(0);
      bool done = false;

      if (readFromFile)
        {
          // open the file with the name given in the input row
          fp = fopen(inRow.c_str(), "r");
          if (fp == NULL)
            throw UDRException(
                 38003,
                 "The Table Mapping UDF %s encountered an error"
                 "while opening file %s",
                 info.getUDRName().c_str(),
                 inRow.c_str());

          done = (fgets(textBuf, sizeof(textBuf), fp) == NULL);
        }
      else
        {
          // process a single line of text, given in inRow
          strncpy(textBuf, inRow.c_str(), sizeof(textBuf));
        }

      // loop over one or more lines of text
      while (!done)
        {
          int len = strlen(textBuf);

          // get rid of a trailing newline
          if (len > 0 && textBuf[len-1] == '\n')
            textBuf[len-1] = 0;

          // tokenize one line of text and emit rows
          token = strtok(textBuf, pattern.c_str());

          while (token != NULL)
            {
              info.out().setString(0, token);
              emitRow(info);
              token = strtok(NULL, pattern.c_str());
            } // while token

          // get next line from file, if needed
          if (readFromFile)
            done = (fgets(textBuf, sizeof(textBuf), fp) == NULL);
          else
            done = true;

        } // while done
    } // while getNextRow
}
