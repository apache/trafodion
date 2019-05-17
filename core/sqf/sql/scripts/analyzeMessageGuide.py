# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@
#
#  This script checks the consistency of the Messages Guide
#  against the code. It looks for messages whose text differs
#  between the Messages Guide and code. It looks for messages
#  that are no longer in use. It looks for messages that are
#  in use but not in the Messages Guide. It produces a report
#  of these.
#
#  The report includes detailed information for each message
#  that differs between the Messages Guide and code. It lists
#  the enum used for the message if there is one. It lists
#  the code modules that may have a reference to the message
#  (it uses grep to determine this, so there may be false
#  positives or more rarely false negatives). It lists the
#  regression test expected files that contain examples of the
#  error message.
#
import os
import sys
import subprocess 
import sets
import datetime
import argparse  # requires Python 2.7


# The script does the following:
#
# 1. Parses the Messages Guide chapters into a table containing message number
# + message text.
# 2. Parses the bin/SqlciErrors.txt file into a table containing message number
# + message text. Merges that into the table from item 1.
# 3. Parses the various *.h files with error message enums. Merges that 
# information into the table from item 1.
# 4. Compares the message texts in items 1 and 2. The comparison has to be
# smart; parameters appear as angle-bracketed items in the Messages Guide but
# contain $0-String0 notation in the SqlciErrors.txt text. When there are
# mismatches, this is noted. If one or the other text is missing, this notation
# is marked "false".
# 5. Searches the sql *.cpp files for references to the error message number
# and/or enum, and when found, maintains a list for each, merged into the
# table from item 1. We only do this for mismatched messages as it takes a
# lot of time.
# 6. Searches the regression tests LOG* files for examples of each message
# number, and when found, maintains a list for each, attached to the table
# constructed in 1. We only do this for mismatched messages as it takes a
# lot of time.
# 7. Lists error messages that mismatch between Messages Guide and
# SqlciErrors.txt.
# 8. Lists error messages that are missing from the Messages Guide but present
# in SqlciErrors.txt. Mentions whether these are actually used anywhere (in
# *.cpp files or regression tests).
# 9. Lists error messages that are present in the Messages Guide or
# SqlciErrors.txt but apparently are no longer used (that is, don't appear in
# *.cpp files or regression tests).
#
# The table resulting from items 1 through 6 has the following shape:
# a. Message number (key)
# b. Messages Guide text (if any)
# c. SqlciErrors.txt message text (if any)
# d. Enum (if any)
# e. Enum *.h file name (if any)
# f. List of *.cpp files with references to message number
# g. List of *.cpp files with references to enum name
# h. List of regression tests with messages number examples
# i. Boolean indicating whether SqlciErrors.txt text matches Messages Guide
#
# Item 1 is implemented by a function that parses a single *.adoc file
# producing table entries. It is driven by a loop over the set of *.adoc file
# names. The function updates the table directly.
# Item 2 is implemented by a function that parses a *.txt file, driven by a
# caller supplying the bin/SqlciErrors.txt name. The function updates the table
# directly.
# Item 3 is implemented by a function that parses a *.h file looking for error
# message enums. It is driven by a loop over a hard-coded set of known *.h
# files, perhaps with the name of the enum passed in. The function updates the
# table directly.
# Item 4 is implemented by a function that does a grep of the *.cpp files, then
# processes the results. The function updates the table directly.
# Item 5 is implemented by a function that does a grep of the LOG* files, then
# processes the results. The function updates the table directly.
# Item 6 is implemented by a function that crawls the table, comparing the
# retrieved texts. The function updates the table directly.
# Items 7, 8, 9 are implemented by a function that reads the table and reports
# results.
#
# We model the table as a Python object and implement these functions as
# methods on that object.

class MessagesTable:

    # The structure of self.dict is a dictionary of dictionaries.
    # The key to the top level dictionary is the message number.
    # The value in each top level dictionary is essentially a
    # relation, modeled by a Python dictionary. The key in this
    # value is the attribute name, as follows:
    #
    # 'messageGuideText'
    # 'errorMessageFileText'
    # 'enumSymbol'
    # 'enumFile'
    # 'listOfCodeReferences'
    # 'listOfEnumSymbolReferences'
    # 'listOfTestReferences'
    # 'textsMatch'

    def __init__(self):
        self.dict = {}

    def mergeEntry(self,key,values):
        if key in self.dict:
            self.dict[key].update(values)
        else:
            self.dict[key] = values

    def parseAdoc(self,adocFileName):
        #
        # Message entries look like this:
        #
        # [[SQL-1002]]
        # == SQL 1002
        #
        # ```
        # Catalog <catalog> does not exist.
        # ```
        #
        # Where <catalog> is the ANSI name of the target catalog.
        #
        # *Cause:* The catalog does not exist.
        #
        # *Effect:* The operation fails.
        #
        # *Recovery:* Enter a valid catalog name and resubmit.
        #
        # The stuff we are interested in is the message number and
        # the message text. We use a tiny state machine to figure
        # out what lines to look for.
        try:
            f = open(adocFileName)
            state = 0
            messageNumberStr = None
            messageText = None
            for line in f:
                line = line.rstrip('\n')  # get rid of trailing return character
                if line.startswith('== SQL '):
                    words = line.split()
                    if len(words) == 3:
                        try:
                            messageNumber = int(words[2])
                        except ValueError:
                            messageNumber = 0
                        if messageNumber > 0:
                            messageNumberStr = words[2]
                            state = 1 # look for first backticks
                elif state == 1:
                    if line == '```':
                        state = 2 # start capturing text
                elif state == 2:
                    messageText = line
                    state = 3 # continue capturing text until backticks
                elif state == 3:
                    if line == '```':
                        state = 0 # look for next heading
                        values = { 'messageGuideText': messageText } 
                        self.mergeEntry(messageNumberStr,values)
                    else:
                        messageText = ' '.join([messageText,line])                    
            f.close()
         
        except IOError as detail:
            print "Could not open " + adocFileName
            print detail



    def parseMessagesFile(self,messagesFileName):
        try:
            f = open(messagesFileName)
            for line in f:
                words = str.split(line)
                if len(words) >= 7:
                    try:
                        messageNumber = int(words[0])
                    except ValueError:
                        messageNumber = 0
                    if messageNumber > 0:
                        # filter out "unused" messages
                        if not (words[6].startswith('--') or words[6].startswith('***')):
                            values = { 'errorMessageFileText': ' '.join(words[6:]) } 
                            self.mergeEntry(words[0],values)                    
            f.close()
         
        except IOError as detail:
            print "Could not open " + messagesFileName
            print detail


    def Cscreener(self,line):
        # remove any C or C++ comments from the line, returning the line
        # (not precise; we don't check for C strings for example)
        commentState = 0
        result = ''
        for c in line:
            if commentState == 0:
                if c == '/':
                    commentState = 1  # seen '/'
                else:
                    result = result + c
            elif commentState == 1:
                if c == '/':
                    commentState = 4 # in C++ comment, ignore rest of line
                elif c == '*':
                    commentState = 2 # in C comment, look for '*'
                else:
                    result = result + '/' + c  # false alarm
                    commentState = 0
            elif commentState == 2 and c == '*':
                commentState = 3 # in C comment, look for '/' ending comment
            elif commentState == 3:
                if c == '/':
                    result = result + ' '  # so we don't glue two tokens together
                    commentState = 0
                elif c != '*':
                    commentState = 2
        return result
                       

    def Ctokenize(self,line,screenOutComments):
        # break a line of text into a list of C-like tokens (not 
        # precise, just good enough for our purposes)
        if screenOutComments:
            line = self.Cscreener(line)  # remove C, C++ comments
        result = []
        currentToken = ''
        for c in line:
            if c.isspace():
                if len(currentToken) > 0:
                    result.append(currentToken)
                    currentToken = ''
            elif c.isalnum() or c == '_':
                currentToken = currentToken + c                
            else:
                if len(currentToken) > 0:
                    result.append(currentToken)
                    currentToken = ''
                result.append(c)
        return result                  
        

    def parseEnumFile(self,enumFileName,enumName):
        #
        # We are looking for a particular enum. The format we expect
        # is like this:
        #
        # enum <enumname> { <symbol> = <value>,
        #                   <symbol> = <value>,
        #                   ...
        #                   <symbol> = <value> } ;
        #
        # Of course this can freely flow across lines and there may be
        # C or C++ comments to navigate past. So, we essentially have
        # to tokenize and use a state machine to parse.
        state = 0
        symbol = None
        messageNumber = None
        try:
            f = open(enumFileName)
            for line in f:
                tokens = self.Ctokenize(line,True)  # screen out comments
                for token in tokens:
                    if state == 0 and token == 'enum':
                        state = 1
                    elif state == 1:
                        if token == enumName:
                            state = 2
                        else:
                            state = 0
                    elif state == 2:
                        if token == '{':
                            state = 3;
                        else:
                            state = 0;
                    elif state == 3:
                        if len(token) > 1: 
                            state = 4
                            symbol = token
                        elif token == '}':
                            state = 10 # ignore rest of file
                    elif state == 4:
                        if token == '=':
                            state = 5
                        elif token == '}':
                            state = 10 # ignore rest of file
                        else:
                            state = 3
                    elif state == 5:
                        if token.isdigit():
                            values = { 'enumSymbol': symbol, 'enumFile': enumFileName }
                            if int(token) > 0:  # ignore enums for 0
                                self.mergeEntry(token,values)
                            state = 6
                        elif token == '-':
                            state = 5  # skip unary minus sign before digits
                        else:
                            state = 3
                    elif state == 6:
                        if token == '}':
                            state = 10 # ignore rest of file
                        else:  # probably a comma
                            state = 3
                  
            f.close()
         
        except IOError as detail:
            print "Could not open " + enumFileName
            print detail

 
    def analyzeCodeReferences(self,directory):
        fileString = directory + "/*/*.cpp " + directory + "/*/*.h " + directory + "/*/*.y" 
        for key in self.dict:
            tableEntry = self.dict[key]
            if tableEntry['textsMatch'] == False:
                if 'enumSymbol' in tableEntry:
                    patternString = '"[' + key + "|" + tableEntry['enumSymbol'] + ']"'
                else:
                    patternString = key
                shellCmd = 'grep -H ' + patternString + " " + fileString
                p1 = subprocess.Popen(shellCmd, shell=True, stdout=subprocess.PIPE, close_fds=True)
                prevFileName = ""
                fileName = ""
                valueSet = set()
                for line in p1.stdout:
                    fileName = line[:line.find(':')]

                    if fileName != prevFileName and len(prevFileName) > 0:
                        entry = { 'listOfCodeReferences': valueSet }
                        self.mergeEntry(key,entry)
                        prevFileName = fileName
                        valueSet = set()
                        
                    line = line[len(fileName)+1:]  # remove file name part and colon
                    tokens1 = self.Ctokenize(line,False)  # don't screen out comments
                    found = False
                    if key in tokens1:
                        found = True
                    elif 'enumSymbol' in tableEntry and tableEntry['enumSymbol'] in tokens1:
                        found = True
                    if found:                   
                        # remove directory part of the name
                        valueSet.add(fileName[len(directory):].rstrip(':').strip('/'))
       
                # do the last one (if there was one)
                if len(fileName) > 0:
                    entry = { 'listOfCodeReferences': valueSet }
                    self.mergeEntry(key,entry)
                


    def analyzeTestReferences(self,directory):
        # print "analyzeTestReferences called for directory " + directory
        fileString = directory + "/*/EXPECTED*"
        for key in self.dict:
            if self.dict[key]['textsMatch'] == False:
                patternString = "[ERROR|WARNING]\[" + key + "\]"
                shellCmd = 'grep -l "' + patternString + '" ' + fileString
                p1 = subprocess.Popen(shellCmd, shell=True, stdout=subprocess.PIPE, close_fds=True)
                valueList = []
                for fileName in p1.stdout:
                    # remove directory part of the name and trailing '\n'
                    valueList.append(fileName[len(directory):].rstrip('\n').strip('/'))

                entry = { 'listOfTestReferences': valueList }
                self.mergeEntry(key,entry)
                

    def removeAngleBracketTerms(self,line):
        # replaces any text of the form "<stuff>"
        result = ''
        throwAway = ''
        i = 0
        state = 0
        while i < len(line):
            if state == 0:
                if line[i] == '<':
                    state = 1
                    throwAway = line[i]
                else:
                    result = result + line[i]
            elif state == 1:
                if line[i] == '>':
                    result = result + throwAway + line[i]
                    state = 0
                else:
                    state = 2
                    throwAway = throwAway + line[i]
            elif state == 2:
                if line[i] == '>':
                    state = 0
                    result = result + '.elided.'
                else:
                    throwAway = throwAway + '>' + line[i]
            i = i + 1

        # if we reached the end of the line after a '<', put the
        # throwaway text back in
        if state == 2:
            result = result + ' <' + throwAway
        #print "Before<: " + line
        #print "After<: " + result
        return result.rstrip() # ignore trailing spaces

    def removeDollarTerms(self,line):
        # removes text of the form $0~Datatype0 (where Datatype might
        # be String, Int, TableName etc.)
        result = ''
        throwAway = ''
        i = 0
        state = 0
        while i < len(line):
            if state == 0:
                if line[i] == '$':
                    state = 1;
                    throwAway = line[i]
                else:
                    result = result + line[i]
            elif state == 1:
                if line[i].isdigit():
                    state = 2
                    throwAway = throwAway + line[i]
                else:
                    result = result + throwAway + line[i]
                    throwAway = ''
                    state = 0
            elif state == 2:
                if line[i] == '~':
                    state = 3
                    throwAway = throwAway + line[i]
                else:
                    result = result + throwAway + line[i]
                    throwAway = ''
                    state = 0
            elif state == 3:
                if line[i].isalpha():
                    state = 4
                    throwAway = throwAway + line[i]
                else:
                    result = result + throwAway + line[i]
                    throwAway = ''
                    state = 0
            elif state == 4:
                if line[i].isalpha():
                    throwAway = throwAway + line[i]
                elif line[i].isdigit():
                    state = 0  # we reached the end of the dollar text
                    result = result + '.elided.'                  
                else: 
                    state = 0  # we reached the end of the dollar text
                    result = result + '.elided.' + line[i]
            i = i + 1

        # if we reached the end of the line then put the throwaway text
        # back in
        if state > 0 and state < 4:
            result = result + throwAway
        #print "Before$: " + line
        #print "After$: " + result
        return result.rstrip() # ignore trailing spaces          


    def compareText(self):
        # print "compareText called"
        for key in self.dict:
            attributes = self.dict[key]
            comparison = False
            if 'messageGuideText' in attributes:
                if 'errorMessageFileText' in attributes:
                    if attributes['messageGuideText'] == attributes['errorMessageFileText']:
                        comparison = True
                    else:
                        temp1 = self.removeAngleBracketTerms(attributes['messageGuideText'])
                        temp2 = self.removeDollarTerms(attributes['errorMessageFileText'])
                        if temp1 == temp2:
                            comparison = True
            self.mergeEntry(key,{ 'textsMatch': comparison })

    
    # Iterating through a Python dictionary gets keys out in hash order
    # which isn't useful to humans. This helper function gets the keys,
    # and places them in a list in numeric order
    def sortedNumericKeys(self,dictionary):
        numericKeys = []
        for key in self.dict:
            numericKeys.append(int(key))

        result = sorted(numericKeys)
        return result
            

    def reportResults(self,withCodeRefs):
        matchedCount = 0
        mismatchedCount = 0
        for keyN in self.sortedNumericKeys(self.dict):
            key = str(keyN)
            value = self.dict[key]
            if value['textsMatch']:
                matchedCount = matchedCount + 1
            else:
                mismatchedCount = mismatchedCount + 1
                if 'messageGuideText' in value:
                    if 'errorMessageFileText' in value:
                        print "Message " + key + " differs between code and Messages Guide:"
                        print "SqlciErrors.txt:    " + value['errorMessageFileText']
                        
                    else:
                        print "Message " + key + " appears in the Messages Guide but not the SqlciErrors.txt file:"
                    print "Message Guide text: " + value['messageGuideText']
                else:
                    if 'errorMessageFileText' in value:
                        print "Message " + key + " appears in the SqlciErrors.txt file but not the Messages Guide:"
                        print "SqlciErrors.txt:    " + value['errorMessageFileText']
                    else:
                        # must exist only in an enum
                        print "Message " + key + " does not appear in either the SqlciErrors.txt file nor the Messages Guide."
                        
                if 'enumSymbol' in value:
                    print "Enum symbol: " + value['enumSymbol'] + " (file " + value['enumFile'] + ")"

                # these tests shouldn't be necessary but do make the following code safe
                if not 'listOfCodeReferences' in value:
                    value['listOfCodeReferences'] = set()
                if not 'listOfTestReferences' in value:
                    value['listOfTestReferences'] = []
                        
                if withCodeRefs:
                    text = "This message "
                    if 'enumSymbol' in value:
                        text = text + "(or its enum symbol) "                            
                    if len(value['listOfCodeReferences']) == 0:
                        text = text + "does not seem to be referenced in C++ code." 
                    else:
                        text = text + "has possible references in " + str(len(value['listOfCodeReferences'])) + " files:" 
                    print text

                    for codeReference in value['listOfCodeReferences']:
                        print "  " + codeReference
  
                if len(value['listOfTestReferences']) == 0:
                    print "This message does not appear to be in any regress expected file."
                else:
                    print "This message possibly appears in the following " + str(len(value['listOfTestReferences'])) + " regress expected files:"
                    
                for testReference in value['listOfTestReferences']:
                    print "  " + testReference

                print " "

        print "Summary: There are " + str(matchedCount) + " matching messages and " + str(mismatchedCount) + " mismatching messages."
                        




# beginning of main


# process command line arguments

parser = argparse.ArgumentParser(
    description='This script checks the consistency of the Messages Guide and the code.')
parser.add_argument("--codeRefs", help='Looks for code references to mismatched messages; this option is quite slow and can take up to 10 seconds per message. Today there are about 2800 mismatched messages so count on six or seven hours.', action="store_true")

args = parser.parse_args()  # exits and prints help if args are incorrect

exitCode = 0

messagesTable = MessagesTable()

# check that $TRAF_HOME is set
mySQroot = os.getenv('TRAF_HOME')
if not mySQroot:
    print "$TRAF_HOME is not defined. Exiting."
    exit(1)

# parse the Messages Guide files
print
print datetime.datetime.ctime(datetime.datetime.now()) + ": reading Messages Guide"
messagesGuideChaptersDirectory = mySQroot + '/../../docs/messages_guide/src/asciidoc/_chapters'
for subdir, dirs, files in os.walk(messagesGuideChaptersDirectory):
    for file in files:
        filepath = subdir + os.sep + file
        if filepath.endswith(".adoc"):
            messagesTable.parseAdoc(filepath)

# parse the SqlciErrors.txt file
print
print datetime.datetime.ctime(datetime.datetime.now()) + ": reading SqlciErrors.txt"
eTextFileName = mySQroot + '/../sql/bin/SqlciErrors.txt'
messagesTable.parseMessagesFile(eTextFileName)

# parse the enum files
print
print datetime.datetime.ctime(datetime.datetime.now()) + ": reading enum files"
enumFileList = ( [ ['ustat/hs_const.h','USTAT_ERROR_CODES'],
    ['arkcmp/CmpErrors.h','ArkcmpErrorCode'],
    ['sqlcomp/CmpDDLCatErrorCodes.h','CatErrorCode'],
    ['optimizer/opt_error.h','OptimizerSQLErrorCode'],
    ['optimizer/UdrErrors.h','UDRErrors'],
    ['exp/ExpErrorEnums.h','ExeErrorCode'],
    ['sort/SortError.h','SortErrorEnum'],
    ['udrserv/udrdefs.h','UdrErrorEnum'] ] )
for entry in enumFileList:
    fileName = mySQroot + '/../sql/' + entry[0]
    messagesTable.parseEnumFile(fileName,entry[1])

# compare Messages Guide and code text
print
print datetime.datetime.ctime(datetime.datetime.now()) + ": comparing Messages Guide and SqlciError.txt text"
messagesTable.compareText()

# analyze code references
if args.codeRefs:
    print
    print datetime.datetime.ctime(datetime.datetime.now()) + ": looking for code references for mismatched messages (this may take a while)"
    sqlCodeDirectory = mySQroot + '/../sql'
    messagesTable.analyzeCodeReferences(sqlCodeDirectory)

# analyze test references
print
print datetime.datetime.ctime(datetime.datetime.now()) + ": looking for test examples for mismatched messages (this may take a while)"
regressDirectory = mySQroot + '/../sql/regress'
messagesTable.analyzeTestReferences(regressDirectory)


# report results
print
print datetime.datetime.ctime(datetime.datetime.now()) + ": generating report"
print
messagesTable.reportResults(args.codeRefs)

print
print datetime.datetime.ctime(datetime.datetime.now()) + ": done"

exit(exitCode)   


