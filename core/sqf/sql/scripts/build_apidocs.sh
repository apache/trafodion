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
# script to build Javadoc and Doxygen sites
#
# Currently this is invoked from the mvn site goals of
# those documents that have associated apidocs
#

WHICH_DOC=
TGT_DIR=
TGT_SUBDIR=

while [[ $# > 0 ]]
  do
    case "$1" in
      -h )
            echo ""
            echo "Usage: $0 [-h] [-o dir] [project ...]"
            echo ""
            echo "-o specifies the output/target directory, default is ."
            echo "projects are sql_javadoc or udr_doxygen for now, default is all"
            exit 1
            ;;

      -o )
            shift
            if [[ $# > 0 ]]; then
              TGT_DIR=$1
            else
              echo "expected directory after -o flag"
              exit 98
            fi
            ;;

      ** )
            WHICH_DOC="${WHICH_DOC} $1"
            ;;

    esac
    shift
  done

# set default parameters, if needed
if [[ -z "$TGT_DIR" ]]; then
  TGT_DIR=.
fi

if [[ -z "$WHICH_DOC" ]]; then
  WHICH_DOC="sql_javadoc udr_doxygen"
fi

# create output dir and get the absolute path name
mkdir -p $TGT_DIR
cd $TGT_DIR
TGT_DIR=$PWD

for d in $WHICH_DOC
do
  APIDOC_DIR=

  case $d in
    sql_javadoc)

         echo "Building Javadocs for SQL project"
         cd $TRAF_HOME/../sql
         mvn javadoc:javadoc
         APIDOC_DIR=target/site/apidocs
         TGT_SUBDIR="/tmudr_javadoc"
    ;;

    udr_doxygen)

         echo "Building Doxygen for TMUDF C++ interface"
         cd $TRAF_HOME/../sql/sqludr
         APIDOC_DIR=tmudr_2.0.1/html
         rm -rf $APIDOC_DIR
         doxygen doxygen_tmudr*.config
         TGT_SUBDIR="/tmudr_doxygen"
    ;;

    *)
    echo "Unrecognized apidoc to build: $d"
    exit 99
    ;;
  esac

  if [[ -n "$TGT_DIR" ]]; then
    echo "Moving $d apidocs from $APIDOC_DIR to ${TGT_DIR}${TGT_SUBDIR}"
    if [[ ! -d ${TGT_DIR}${TGT_SUBDIR} ]]; then
      mkdir -p ${TGT_DIR}${TGT_SUBDIR}
    else
      rm -rf ${TGT_DIR}${TGT_SUBDIR}
    fi
    mv -f $APIDOC_DIR ${TGT_DIR}${TGT_SUBDIR}
  fi
done
