#!/bin/bash

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
         cd $MY_SQROOT/../sql
         mvn javadoc:javadoc
         APIDOC_DIR=target/site/apidocs
         TGT_SUBDIR="/tmudr_javadoc"
    ;;

    udr_doxygen)

         echo "Building Doxygen for TMUDF C++ interface"
         cd $MY_SQROOT/../sql/sqludr
         doxygen doxygen_tmudr*.config
         APIDOC_DIR=tmudr_2.0.1/html
         TGT_SUBDIR="/tmudr_doxygen"
    ;;

    *)
    echo "Unrecognized apidoc to build: $d"
    exit 99
    ;;
  esac

  if [[ -n "$TGT_DIR" ]]; then
    echo "Moving $d apidocs to ${TGT_DIR}${TGT_SUBDIR}"
    if [[ ! -d ${TGT_DIR}${TGT_SUBDIR} ]]; then
      mkdir -p ${TGT_DIR}${TGT_SUBDIR}
    fi
    mv -f $APIDOC_DIR ${TGT_DIR}${TGT_SUBDIR}
  fi
done
