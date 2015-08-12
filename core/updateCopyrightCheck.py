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
#  This script checks for files that have been changed in a given
#  workspace. It then checks that the copyrights in the file end
#  in the current year. If they don't, it returns an error or
#  optionally updates them.
#
#  If the environment variable UPDATE_COPYRIGHTS is set to YES,
#  or if the --update option is specifed, then the script
#  updates copyrights that need updating. No error is
#  returned in that case (unless some other more severe error is
#  found). If the environment variable is absent or set to something
#  other than YES, and the --update option is not specified, then
#  an out-of-date copyright causes an error.
#
import os
import sys
import re
import datetime
import subprocess
# import argparse  requires Python 2.7 unfortunately


class LineParser:
    def __init__(self):
        # require that the entire string match the pattern
        # (note the $ at the end of the pattern)
        pattern = r'#[\s]+(?P<status>(new file:|modified:))' + \
            r'[\s]+(?P<filename>[\S]*)\n$'
        self.lineParser = re.compile(pattern)
        # the parseState is not really used now, but it might
        # be useful for debugging
        self.parseState = "INIT"

    def parseLine(self, line):
        """Parse a line from a 'git status' command"""

        fileName = None
        if line == "# Changes to be committed:\n":
            self.parseState = "TO_BE_COMMITTED"
        elif line == "# Changed but not updated:\n":
            self.parseState = "CHANGED_NOT_UPDATED"
        elif line == "# Untracked files:\n":
            self.parseState = "UNTRACKED"
        else:
            m = self.lineParser.match(line)
            if m:
                fileName = m.group('filename')

        result = fileName
        return result


# beginning of main

# process command line arguments

# Note: It's easiest to use the argparse module, but unfortunately
# it is only installed as part of Python with 2.7, and many
# workstations are on 2.6. I've left the original argparse code
# here, commented out, so we can use it in the future as
# Python 2.7 and later versions become more common.

#parser = argparse.ArgumentParser(
#    description='This script checks for out-of-date copyrights.',
#    epilog='''If the environment variable UPDATE_COPYRIGHTS=YES is present,
#    the script behaves as if --update was specified.''')
#
# --update takes no arguments
#line = 'If specified, out-of-date copyrights are automatically updated.'
#parser.add_argument('--update', action='store_true', help=line)
#
# --directory takes one argument, the directory name
#parser.add_argument('--directory',
#                    help='Defaults to the present working directory.')
#
#args = parser.parse_args()  # exits and prints help if args are incorrect


# the code below does the equivalent of the argparse code above

doUsage = False
expectingDirectory = False
first = True


class Args:
    def __init__(self):
        self.directory = ""
        self.update = False

args = Args()

for arg in sys.argv:
    if first:
        first = False
    elif expectingDirectory:
        # make sure the argument doesn't begin with minus
        # since we are expecting a directory name
        if arg[0] == "-":
            doUsage = True
        else:
            args.directory = arg
        expectingDirectory = False
    else:
        if arg == "--update":
            args.update = True
        elif arg == "--directory":
            expectingDirectory = True
        else:  # -h, --help, or invalid argument
            doUsage = True

if expectingDirectory:  # if --directory at end lacks argument
   doUsage = True

if doUsage:
    print "usage: updateCopyrightCheck.py [-h] [--update] " + \
        "[--directory DIRECTORY]"
    print
    print "This script checks for out-of-date copyrights."
    print
    print "optional arguments:"
    print "  -h, --help            show this help message and exit"
    print "  --update              If specified, out-of-date " + \
        "copyrights are automatically"
    print "                        updated."
    print "  --directory DIRECTORY Defaults to the present working directory."
    print
    print "If the environment variable UPDATE_COPYRIGHTS=YES " + \
        "is present, the script"
    print "behaves as if --update was specified."
    print
    exit(0)

# end of argparse replacement code

directory = args.directory
if directory:  # if a directory was specified, switch to it
    os.chdir(directory)

update = args.update
if not update:
    if os.environ.__contains__('UPDATE_COPYRIGHTS'):
        if os.environ['UPDATE_COPYRIGHTS'] == 'YES':
            update = True

# initialize counters

filesLackingCopyright = 0
filesCorrectCopyright = 0
filesIncorrectCopyright = 0
filesUpdatedCopyright = 0
filesMultipleCopyright = 0
filesFailedUpdate = 0

# get the current year and set up pattern matching strings

currentYear = str(datetime.datetime.now().year)
matchString = r'\(C\) Copyright [0-9\-]*' + currentYear + \
    r' Hewlett-Packard Development Company'

reCheckYear = re.compile(matchString)
matchString2 = r'\(C\) Copyright (?P<startYear>([0-9]+\-)?)' + \
    r'(?P<oldYear>[0-9]+) Hewlett-Packard Development Company'
reCaptureOldYear = re.compile(matchString2)

# get the set of files changed

cmd = ['git', 'status']
gitProcess = subprocess.Popen(cmd, stdout=subprocess.PIPE)

# for each file
#    read it
#    if it lacks copyright, mention that
#    else if the copyright is wrong
#       update the file to replace the copyright
#       mention that we did so

print
lineParser = LineParser()
for line in gitProcess.stdout:
    parseResult = lineParser.parseLine(line)
    if parseResult:
        #  The copyright line of interest looks like:
        #
        #  // (x) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
        #
        #  (It is actually (C), not (x); I obfuscated the comment so this
        #  script wouldn't mistakenly report itself as having multiple
        #  copyrights :-)
        #
        #  Of course the comment delimiter will vary with the file language.
        cmdGrep = ['grep',
                   '(C) Copyright [0-9\-]*'
                   + ' Hewlett-Packard Development Company',
                   parseResult]
        grepProcess = subprocess.Popen(cmdGrep, stdout=subprocess.PIPE)
        matchCount = 0
        needsEdit = 1
        savedLine = None
        for copyrightLine in grepProcess.stdout:
            matchCount = matchCount + 1
            savedLine = copyrightLine
            m = reCheckYear.search(copyrightLine)
            if m:
                needsEdit = False

        if matchCount == 0:
            filesLackingCopyright = filesLackingCopyright + 1
            print '*** File ' + parseResult + \
                ' lacks a copyright; please add one.'
        elif matchCount > 1:
            filesMultipleCopyright = filesMultipleCopyright + 1
            print '*** File ' + parseResult + \
                ' appears to have multiple copyrights; please check manually.'
        elif needsEdit:
            # exactly one copyright line, as expected, but old copyright
            filesIncorrectCopyright = filesIncorrectCopyright + 1
            if update:   # try to update it if --update option was specified
                print '*** Updating file ' \
                    + parseResult + ' to current copyright.'
                m2 = reCaptureOldYear.search(savedLine)
                if m2:
                    filesUpdatedCopyright = filesUpdatedCopyright + 1
                    filesIncorrectCopyright = filesIncorrectCopyright - 1
                    oldYear = m2.group('oldYear')
                    startYear = m2.group('startYear')
                    if len(startYear) > 0:
                        # it's yyyy-zzzz, just replace zzzz with 2015
                        substituteCommand = '/(C) Copyright [0-9\-]* ' \
                            + 'Hewlett-Packard Development Company/' \
                            + 's/' + oldYear + '/' + currentYear + '/g\n'
                    else:  # it's just zzzz, replace with zzzz-2015
                        substituteCommand = '/(C) Copyright [0-9\-]* ' \
                            + 'Hewlett-Packard Development Company/' \
                            + 's/' + oldYear + '/' + oldYear + '-' \
                            + currentYear + '/g\n'
                    writeCommand = 'w\n'
                    cmdEd = ['ed', '-s', parseResult]
                    edProcess = subprocess.Popen(cmdEd, stdin=subprocess.PIPE)
                    edProcess.stdin.write(substituteCommand)
                    edProcess.stdin.write(writeCommand)
                    edProcess.stdin.close()
                    print '*** Update completed.'
                else:
                    filesFailedUpdate = filesFailedUpdate + 1
                    print '*** Update failed; please check manually.'
            else:  # --update not specified
                print '*** File ' + parseResult + \
                    ' has an incorrect copyright; please update.'
        else:
            print 'File ' + parseResult + ' has the correct copyright.'
            filesCorrectCopyright = filesCorrectCopyright + 1


# print out a count of files having correct copyrights,
# the number updated, and the number missing copyrights etc.

exitCode = 0

print
print "Number of files lacking copyright: " + str(filesLackingCopyright)
print "Number of files with correct copyright: " + str(filesCorrectCopyright)
print "Number of files with updated copyright: " + str(filesUpdatedCopyright)
if filesIncorrectCopyright > 0:
    print "Number of files with incorrect copyrights: " + \
        str(filesIncorrectCopyright)
    exitCode = 1
if filesFailedUpdate > 0:
    print "Number of files where copyright update failed: " + \
        str(filesFailedUpdate)
    exitCode = 1
if filesMultipleCopyright > 0:
    print "Number of files with multiple copyrights: " + \
        str(filesMultipleCopyright)
    exitCode = 1
print

exit(exitCode)
