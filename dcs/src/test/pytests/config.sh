#!/bin/bash
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
#------------------------------------------------------------------------------------
# @(#) config.sh
#
# PURPOSE:
#    Configure tests for Python ODBC testing
#
# MODIFIED:
#    02/26/2014 A.C - Created
#
#------------------------------------------------------------------------------------

export BASE_DIR=`dirname $0`
export DRVR_TAR_LOC=""
export DSN=""
export ODBCHOME="/usr"
export ODBCHOME_LIB="/usr/lib64"
export ODBC_TRACE="Off"
export DB_CATALOG="TRAFODION"
export DB_SCHEMA="T2QA"
export DB_USER="SOMEUSER"
export DB_PASSWORD="SOMEPASSWORD"
export PRINT_FILE_CONTENTS="false"
if [ "$BASE_DIR" = "." ]; then BASE_DIR=$(pwd); fi


function Program_Help {
    echo ""
    echo "Description: Configure Python ODBC test suite(s)"
    echo "Usage:       $0 -d <dsn> [OPTIONS]"
    echo ""
    echo "NOTE: If the -t option is NOT used then it's assumed the directory $BASE_DIR/odbc_driver has the appropriate ODBC driver installed."
    echo ""
    echo "Options:"
    echo "  -t <tar_location>        If tar file exists locally  : /ABSOLUTE_PATH_TO/TRAF_ODBC_Linux_Driver_64.tar.gz"
    echo "                           If tar file exists remotely : scp:jenkins@downloads.trafodion.org:/ABSOLUTE_PATH_TO/TRAF_ODBC_Linux_Driver_64.tar.gz"
    echo "                                                         scp:downloads.trafodion.org:/ABSOLUTE_PATH_TO/TRAF_ODBC_Linux_Driver_64.tar.gz"
    echo "                                                         http://downloads.trafodion.org/TRAF_ODBC_Linux_Driver_64.tar.gz"
    echo "  -d <dsn>                 DSN to connect to           : <Fully_Qualified_Domain_Name_Of_Machine>:<Port>"
    echo "  -o <odbchome>            Driver Manager Home directory. Sets environment variable ODBCHOME. (Default is /usr)"
    echo "  -L <odbchome_lib>        Driver Manager library directory. (Default is /usr/lib64)"
    echo "  -r                       Set Driver Manager trace ON! (Default is OFF)"
    echo "  -c <catalog>             Database catalog name to use. (Default is TRAFODION)"
    echo "  -s <schema>              Database schema name to use. (Default is T2QA) "
    echo "  -u <user>                Database user name. (Default is SOMEUSER)"
    echo "  -p <password>            Database user password. (Default is SOMEPASSWORD)"
    echo "  -v                       Echo contents of configured files"
    echo "  -h                       Print this usage message"
    echo ""
    exit
}

function Download_Install_Driver {
  # function expects driver to be installed @ $BASE_DIR/odbc_driver
  cd "$BASE_DIR/odbc_driver"
  mkdir tmp
  cd tmp

  # downloads and untars odbc driver installer to tmp directory
  if [ -f "$DRVR_TAR_LOC" ]
  then
    # tar file is located locally
    tar xvf "$DRVR_TAR_LOC"
  elif [ $(echo "$DRVR_TAR_LOC" | egrep -c '^scp:') -eq 1 ]
  then
    # fetch tar file via scp
    scp ${DRVR_TAR_LOC##scp:} .
    tar xvf ${DRVR_TAR_LOC##*\/}
  elif [ $(echo "$DRVR_TAR_LOC" | egrep -c '^http:') -eq 1 ]
  then
    # fetch file via wget
    wget ${DRVR_TAR_LOC}
    tar xvf ${DRVR_TAR_LOC##*\/}
  else
    # unknown option
    echo ""
    echo "ERROR: Do not know how to fetch driver from $DRVR_TAR_LOC. Please check to make sure you are using absolute paths."
    echo ""
    exit 1
  fi

  cd PkgTmp

  # run odbc driver installer
  ./install.sh <<- EOF
YES
$BASE_DIR/odbc_driver
$BASE_DIR/odbc_driver
$BASE_DIR/odbc_driver
$BASE_DIR/odbc_driver
EOF

  # list odbc_driver directory
  echo ""
  echo "INFO: ODBC Driver has been installed and configured"
  if [ "$PRINT_FILE_CONTENTS" = "true" ]
  then
    echo "INFO: Contents of $BASE_DIR/odbc_driver"
    echo ""
    ls "$BASE_DIR/odbc_driver"
  fi
  echo ""
} # end Download_Install_Driver function

function Setup_ODBC_Config {
  cd "$BASE_DIR"

  # set up odbcinst.ini
  sed -e "s%TEMPLATE_PATH_TO_DRIVER%$BASE_DIR/odbc_driver%g" -e "s%TEMPLATE_TRACE_FLAG%$ODBC_TRACE%g" .odbcinst.ini.tmpl > odbcinst.ini
  echo ""
  echo "INFO: File odbcinst.ini has been configured"
  if [ "$PRINT_FILE_CONTENTS" = "true" ]
  then
    echo "INFO: Contents of odbcinst.ini"
    echo ""
    cat odbcinst.ini
  fi
  echo ""

  # set up odbc.ini
  sed -e "s/TEMPLATE_CATALOG/$DB_CATALOG/g" -e "s/TEMPLATE_SCHEMA/$DB_SCHEMA/g" -e "s/TEMPLATE_DSN/$DSN/g" .odbc.ini.tmpl > odbc.ini
  echo ""
  echo "INFO: File odbc.ini has been configured"
  if [ "$PRINT_FILE_CONTENTS" = "true" ]
  then
    echo "INFO: Contents of odbc.ini"
    echo ""
    cat odbc.ini
  fi
  echo ""
} # end Setup_ODBC_Config function

function Setup_PyODBC_Config {
  cd "$BASE_DIR"

  # set up env.sh
  sed -e "s%TEMPLATE_ODBC_LIB%$BASE_DIR/odbc_driver:$ODBCHOME_LIB%g" -e "s%TEMPLATE_ODBC_HOME%$ODBCHOME%g" -e "s%TEMPLATE_ODBC_SYS_INI%$BASE_DIR%g" .env.sh.tmpl > env.sh
  echo ""
  echo "INFO: File env.sh has been configured"
  if [ "$PRINT_FILE_CONTENTS" = "true" ]
  then
    echo "INFO: Contents of env.sh"
    echo ""
    cat env.sh
  fi
  echo ""

  # set up config.ini
  sed -e "s/TEMPLATE_CATALOG/$DB_CATALOG/g" -e "s/TEMPLATE_SCHEMA/$DB_SCHEMA/g" -e "s/TEMPLATE_DSN/$DSN/g" -e "s/TEMPLATE_USER/$DB_USER/g" -e "s/TEMPLATE_PASSWORD/$DB_PASSWORD/g" .config.ini.tmpl > config.ini
  echo ""
  echo "INFO: File config.ini has been configured"
  if [ "$PRINT_FILE_CONTENTS" = "true" ]
  then
    echo "INFO: Contents of config.ini"
    echo ""
    cat config.ini
  fi
  echo ""

  # set up tox.ini
  sed -e "s%TEMPLATE_ODBC_HOME%$ODBCHOME%g" -e "s%TEMPLATE_ODBC_LIB%$BASE_DIR/odbc_driver:$ODBCHOME_LIB%g" .tox.ini.tmpl > .tox.ini.tmpl2

  # check to see if http_proxy set in the environment
  PROXY_INFO=""
  PROXY_INFO="$(env | egrep 'http_proxy=' | cut -d'=' -f2)"
  if [ -z "$PROXY_INFO" ]
  then
    sed -e '/TEMPLATE_HTTP_PROXY/d' -e '/TEMPLATE_HTTPS_PROXY/d' -e '/TEMPLATE_FTP_PROXY/d' .tox.ini.tmpl2 > tox.ini
  else
    sed -e "s%TEMPLATE_HTTP_PROXY%http_proxy=$PROXY_INFO%g" -e "s%TEMPLATE_HTTPS_PROXY%https_proxy=$PROXY_INFO%g" -e "s%TEMPLATE_FTP_PROXY%ftp_proxy=$PROXY_INFO%g" .tox.ini.tmpl2 > tox.ini
    echo "" >> env.sh
    echo "export http_proxy=$PROXY_INFO" >> env.sh
    echo "export https_proxy=$PROXY_INFO" >> env.sh
    echo "export ftp_proxy=$PROXY_INFO" >> env.sh
  fi
  rm .tox.ini.tmpl2

  echo "INFO: File tox.ini has been configured"
  if [ "$PRINT_FILE_CONTENTS" = "true" ]
  then
    echo "INFO: Contents of tox.ini"
    echo ""
    cat tox.ini
  fi
  echo ""
} # end Setup_PyODBC_Config function


#----------
# Main
#----------

# Process command line arguments
if [ $# -eq 0 ]
then
  Program_Help
  exit 1
else
  while getopts c:d:o:L:p:s:t:u:rv opt
  do
    case "$opt" in
      c) # catalog
         DB_CATALOG="$OPTARG"
         ;;
      d) # DSN
         DSN="$OPTARG"
         ;;
      L) # ODBCHOME library
         ODBCHOME_LIB="$OPTARG"
         ;;
      o) # ODBCHOME
         ODBCHOME="$OPTARG"
         ;;
      p) # Database user's password
         DB_PASSWORD="$OPTARG"
         ;;
      r) # ODBC Driver trace ON
         ODBC_TRACE="On"
         ;;
      s) # Database Schema
         DB_SCHEMA="$OPTARG"
         ;;
      t) # ODBC Driver tar file location
         DRVR_TAR_LOC="$OPTARG"
         ;;
      u) # Database user
         DB_USER="$OPTARG"
         ;;
      v) # print contents of files configured
         PRINT_FILE_CONTENTS="true"
         ;;
     \?) # Unknown option
         Program_Help
         exit 1
         ;;
    esac
  done
fi

# Print out the options set
echo ""
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo "@                                                                                                                     "
echo -n "@  Driver Tar File Location         : "
if [ -z "$DRVR_TAR_LOC" ]; then echo "Not needed"; else echo "$DRVR_TAR_LOC"; fi
echo "@  Driver Manager Home Directory    : $ODBCHOME"
echo "@  Driver Manager Library Directory : $ODBCHOME_LIB"
echo "@  DSN                              : $DSN"
echo "@  Database User                    : $DB_USER"
echo "@  Database Password                : $DB_PASSWORD"
echo "@                                                                                                                     "
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo ""

# check to see if odbc_driver directory exists
if [ ! -L "$BASE_DIR/odbc_driver/libtrafodbc_drvr64.so" -a -z "$DRVR_TAR_LOC" ]
then
  echo ""
  echo "ERROR: Directory $BASE_DIR/odbc_driver has to contain the Trafodion ODBC driver or the option -t <tar_location> has to be specified!"
  echo ""
  exit 1
elif [ ! -L "$BASE_DIR/odbc_driver/libtrafodbc_drvr64.so" ]
then
  rm -rf "$BASE_DIR/odbc_driver"
  mkdir "$BASE_DIR/odbc_driver"
  Download_Install_Driver
fi

# clean up directory
rm "$BASE_DIR/*.ini" 2>/dev/null
rm "$BASE_DIR/env.sh" 2>/dev/null
rm "$BASE_DIR/*.pyc" 2>/dev/null

# run setup
Setup_ODBC_Config
Setup_PyODBC_Config

