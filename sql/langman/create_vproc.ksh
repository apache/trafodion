#######################################################################
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 1997-2014 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# @@@ END COPYRIGHT @@@
#######################################################################
#/*
# ******************************************************************************
# *
# * File:         create_vproc.ksh
# * Description:  Script file to generate vproc.java file which
# *               will be included in mxlangman.jar file.
# *
# * Created:      05/24/2002
# * Language:     Shell
# *
# *
# ******************************************************************************
# */
#

#
# We get the vproc information from /bin/vproc.h file
# Return error if that file does not exist
#
if [ ! -r ../bin/vproc.h ]; then
  echo " ../bin/vproc.h file does not exist or not readable."
  exit 1;
fi

#
# get the langman vproc values from /bin/vproc.h file
#
prodnum=`grep "^#define PRODNUMMXLANGMAN" ../bin/vproc.h | awk '{print $3}'`
date=`grep "^#define DATE1LANGMAN" ../bin/vproc.h | awk '{print $3}'`
label=`grep "^#define LANGMAN_CC_LABEL" ../bin/vproc.h | awk '{print $3}'`

if [ "X$prodnum" = "X" -o "X$date" = "X" -o "X$label" = "X" ]; then
  echo "Either PRODNUMMXLANGMAN or DATE1LANGMAN or LANGMAN_CC_LABEL \c"
  echo "is not set in ../bin/vproc.h."
  echo "Check ..bin/vproc.h file."
  exit 1;
fi


#
# Concatenate to form a single string
#
VPROC=\"$prodnum"_"$date"_"$label\"

# Create the java file
echo  "//"
echo  "// File Name: vproc.java"
echo  "//"
echo  "// This file prints the VPROC information for mxlangman.jar"
echo  "// This file is created by the build script during langman build."
echo  "// If you need to change this file, make changes to create_vproc.sh"
echo  "//"
echo  ""
echo  "package com.tandem.sqlmx;"
echo  "public class vproc"
echo  "{"
echo  "  static String vproc_str = $VPROC;"
echo
echo  "  public static void main(String[] args)"
echo  "  {"
echo  "    System.out.println(\"VPROC for Trafodion Language Manager : \" + vproc_str);"
echo  "  }"
echo  "}"

