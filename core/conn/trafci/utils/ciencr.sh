#! /bin/ksh
############################################################################
#                                                                         ##
#   TRAFCI (Java EncryptUtil) exec script for Linux/Unix.                 ##
#                                                                         ##
#   Usage: #ciencr.sh { see user manual }                                 ##
#                                                                         ##
############################################################################
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

CUR=.
CURR_DIR=`pwd`

if [ ${TRAF_HOME} ]; then
   #echo "INFO: TRAFCIHOME is set to ${TRAF_HOME}/trafci"
   export TRAFCIHOME=${TRAF_HOME}/trafci
else
   echo "ERROR: \$TRAF_HOME is not set. Please run sqenv.sh first."
   exit 3
fi

#get user dir
export USERHOME=`awk 'BEGIN {print ENVIRON["HOME"]}'`

#echo "INFO: Current Directory is "$CURR_DIR

# Verify JAVA_HOME environment variable is set and is version 1.5 or greater
if [ $JAVA_HOME ]; then
    # Verify the minimum version of java
    MIN_JAVA_VERSION=105
    MIN_JAVA_DOT_VERSION=1.5
    JAVA_EXE=$JAVA_HOME/bin/java
    if [ -f ${JAVA_EXE} ]; then
        $JAVA_EXE -version 2>java_tmp.ver
        VERSION=`cat java_tmp.ver | grep "java version" | awk '{ print substr($3, 2, length($3)-2); }'`
        rm -f java_tmp.ver
        VERSION=`echo $VERSION | awk '{ print substr($1, 1, 3); }' | sed -e 's;\.;0;g'`
        if [ $VERSION ]; then
           if [[ $VERSION -lt $MIN_JAVA_VERSION ]]; then
              echo "ERROR: Java version is less than the minimun required version of jdk"$MIN_JAVA_DOT_VERSION". Please resolve before continuing."
                exit 3
           fi
       fi
    else
       echo "ERROR: java executable not found at $JAVA_EXE"
        exit 2
    fi
else
    echo "ERROR: JAVA_HOME environment variable must be set. Please resolve before continuing."
    exit 3
fi

# Setting the classpath seperator
un=`uname | cut -c1-6`
if [[ $un = "CYGWIN" ]]; then
    separator=";"
elif [[ $un = "Linux" ]]; then
    separator=":"
else
    separator=":"
fi

# Set the CLASSPATH
# Note: Setting conf dir also in classpath
CLASSPATH=$TRAFCIHOME/lib/trafci.jar

if [ ! -e "${USERHOME}/.trafciconf/security.props" ]; then
	#echo "********** Starting EncryptUtil **************************************"
  #echo $JAVA_EXE -cp $CLASSPATH org.trafodion.ci.pwdencrypt.EncryptUtil -o install
  $JAVA_EXE -cp $CLASSPATH org.trafodion.ci.pwdencrypt.EncryptUtil -o install
  export ERR=$?
  echo "-o install setting completed";
  #echo "**********************************************************************"
fi

runTRAFCI()
{
    #echo "TRAFCIHOME        : "$TRAFCIHOME
    #echo "JAVA_HOME         : "$JAVA_HOME
    #echo "CLASSPATH         : "$CLASSPATH
    #echo "USERHOME          : "$USERHOME
    #echo
    #echo "ciencr parameters : " "$@"

    #echo "********** Starting EncryptUtil **************************************"
    #echo $JAVA_EXE -cp $CLASSPATH org.trafodion.ci.pwdencrypt.EncryptUtil "$@"
    $JAVA_EXE -cp $CLASSPATH org.trafodion.ci.pwdencrypt.EncryptUtil "$@"
    export ERR=$?
    #echo "**********************************************************************"
}

runTRAFCI "$@"

#echo End ciencr Processing.
if [ $ERR = "0" ]; then
	echo "Operation succeeded!"
	#echo "Exit Status: "$ERR
elif [ $ERR = "5" ]; then
	echo "Operation failed!"
	#echo "Exit Status: "$ERR
fi
exit $ERR
