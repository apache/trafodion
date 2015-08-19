#!/bin/sh

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

#set -vx

# This script installs Linux64 Trafodion ODBC library:
# libtrafodbc_l64.so
# By default driver files are installed in /usr/include/odbc (include
# files), /usr/lib (library file), and in /etc/odbc. A user can, if 
# needed, specify any specific directory for these set of files. 
# If the non-default directories do not exist, the script will create them.
# Before installing files, it backs up the old library to a SAV file if it
# is of the same major version. Different versions will be left alone.

PKG_DIR=
DIR_VALUE=
LIB_DIR=
LIB_NAME=
ETC_DIR=
#INCLUDE_DIR=
SAMPLE_DIR=
#package files, excluding this file
pkgfiles=(libtrafodbc_l64.so TRAFDSN MD5SUM LICENSE connect_test.cpp license.txt libicuucNv44.so.44 libicudataNv44.so.44)

#check whether the package directory has all files (or this script is invoked form "PkgTmp" dir)
function check_package {
  let len=${#pkgfiles[*]}
  let i=0
  while [ $i -lt $len ]; do
	  if [ ! -f ${pkgfiles[$i]} ]; then
		  echo " error! missing the file "" ${pkgfiles[$i]} ""in package!"
	     echo " exiting ..."
	     exit -2
	  fi
	  (( i++))
  done
  PKG_DIR=`pwd`
}

function license_check {

cat ./license.txt

echo -n "Do you accept the terms of the license (YES/NO): "
read OPTION
if [[ $OPTION = YES ]] || [[ $OPTION = yes ]]
then
	echo "Thank You...."
else
	echo "Please try again when you are ready to accept."
	echo "Exiting....."
	exit -4
fi

echo "Proceeding with install"
}

function do_prompt {
    default_value=$1
    prompt=$2
    echo -e $prompt
    while read in
    do
        # in case someone types *
	count=`echo $in | wc -w`
	if [ $count -gt "1" ]; then
               echo -e "error! only one value is allowed\n"
	       echo -e "retrying..."
	       in=
               echo -e $prompt
	       continue
	fi
        if [ "$in" ]; then
            DIR_VALUE=`echo $in|tr -d [:cntrl:] `
	else
            export DIR_VALUE=$default_value
        fi
        if [ $count = "0" ] || [ $count = "1" ]; then
            verify_dir "$DIR_VALUE"
            if [ $? -eq 0 ]; then
                break
            else
		echo -e "\nretrying..."
		in=
                echo -e $prompt
            fi
        fi
    done
}

function verify_dir {
    echo $1 | /bin/grep "^/.*" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "\n** error!  path name must begin with a '/'."
        return 1
    fi
    if ! [ -d $1 ]; then 
	mkdir -p $1
	if [[ $? != 0 ]]; then
		echo -e "\nFailed to create directory $1"
		echo -e "\nCheck the permissions!"
		return 1
	fi
    fi
}

function install_lib {
   #find the version of library in the package
   ver=`ls libtrafodbc_l64.so | cut -d . -f3`
   cd $DIR_VALUE
   if (( $? != 0 )); then
	echo -e "\nCan not cd to $DIR_VALUE"
        echo -e "exiting....\n"
	exit -2
   fi
   #make sure that only ONE libtrafodbc_l64.so exists!
   let lib_count=`echo libtrafodbc_l64.so | wc -w`  >/dev/null 2>&1
   if [ $lib_count -gt "1" ]; then
       echo -e "\nMore than one library of MAJOR version exist in $DIR_VALUE!!"
       echo -e "The files are: \c" 
       ls libtrafodbc_l64.so
       echo -e "\nBack them up and attempt install again "
       echo -e "Exiting...\n"
       exit -2
   fi
   if [ -f libtrafodbc_l64.so ]; then 
      #find the complete name of existing library
      lib=`ls libtrafodbc_l64.so`
      echo -e "\nWarning: a version:" $ver " of odbc library "
      echo -e "already exists! -> $DIR_VALUE/$lib \nsaving it to $DIR_VALUE/$lib.SAV"
      err=0
      mv -f $lib $lib.SAV
      (( err += $? ))
	  mv -f libicuucNv44.so.44 libicuucNv44.so.44.SAV     
      (( err += $? ))   
	  mv -f libicudataNv44.so.44 libicudataNv44.so.44.SAV     
      (( err += $? ))    
      #remove link
      rm -f libtrafodbc_l64.so.1
      (( err += $? ))
      if (( $err !=0 )); then
	  echo -e "\nError! can not clear previous installation"
	  echo -e "Check the permissions on $DIR_VALUE/$lib,"
	  echo "$DIR_VALUE/$lib.SAV, and $DIR_VALUE/libtrafodbc_l64.so.$ver"
          echo -e "Exiting...\n"
	  exit -2
      fi
   fi
   #copy library
   err=0
   cp -f $PKG_DIR/libicuucNv44.so.44  .
   (( err += $? ))
   chmod 555 libicuucNv44.so.44
   (( err += $? ))
   rm -f libicuucNv44.so
   (( err += $? ))
   ln -s libicuucNv44.so.44 libicuucNv44.so
   (( err += $? ))
   cp -f $PKG_DIR/libicudataNv44.so.44  .
   (( err += $? ))
   chmod 555 libicudataNv44.so.44
   (( err += $? ))
   rm -f libicudataNv44.so
   (( err += $? ))
   ln -s libicudataNv44.so.44 libicudataNv44.so
   (( err += $? ))
   cp -f $PKG_DIR/libtrafodbc_l64.so .
   (( err += $? ))
   ln -s libtrafodbc_l64.so libtrafodbc_l64.so.1
   (( err += $? ))
   rm -f libtrafodbc64.so
   (( err += $? ))
   ln -s libtrafodbc_l64.so.1 libtrafodbc64.so
   (( err += $? ))
   chmod 555 libtrafodbc64.so
   (( err += $? ))
   cd $PKG_DIR
   (( err += $? ))
   LIB_NAME=`ls libtrafodbc_l64.so`
   (( err += $? ))
   if (( $err != 0 )); then
     echo "driver installation failed.."
     echo "check the permission settings for $DIR_VALUE"
     echo "exiting.."
     exit -2
   fi
}

function install_drvr_lib {
   #find the version of library in the package
   ver=`ls libtrafodbc_l64_drvr.so | cut -d . -f3`
   cd $DIR_VALUE
   if (( $? != 0 )); then
        echo -e "\nCan not cd to $DIR_VALUE"
        echo -e "exiting....\n"
        exit -2
   fi
   #make sure that only ONE libtrafodbc_l64.so exists!
   let lib_count=`echo libtrafodbc_l64_drvr.so | wc -w`  >/dev/null 2>&1
   if [ $lib_count -gt "1" ]; then
       echo -e "\nMore than one library of MAJOR version: $ver exist in $DIR_VALUE!!"
       echo -e "The files are: \c"
       ls libtrafodbc_l64_drvr.so
       echo -e "\nBack them up and attempt install again "
       echo -e "Exiting...\n"
       exit -2
   fi
   if [ -f libtrafodbc_l64_drvr.so ]; then
      #find the complete name of existing library
      lib=`ls libtrafodbc_l64_drvr.so`
      echo -e "\nWarning: a version" $ver " of odbc library "
      echo -e "already exists! -> $DIR_VALUE/$lib \nsaving it to $DIR_VALUE/$lib.SAV"
      err=0
      mv -f $lib $lib.SAV
      (( err += $? ))
      #remove link
      rm -f libtrafodbc_l64_drvr.so.1
      (( err += $? ))
      if (( $err !=0 )); then
          echo -e "\nError! can not clear previous installation"
          echo -e "Check the permissions on $DIR_VALUE/$lib,"
          echo "$DIR_VALUE/$lib.SAV, and $DIR_VALUE/libtrafodbc_l64_drvr.so.1"
          echo -e "Exiting...\n"
          exit -2
      fi
   fi
   #copy library
   err=0
   cp -f $PKG_DIR/libtrafodbc_l64_drvr.so .
   (( err += $? ))
   ln -s libtrafodbc_l64_drvr.so libtrafodbc_l64_drvr.so.1
   (( err += $? ))
   rm -f libtrafodbc_drvr64.so
   (( err += $? ))
   ln -s libtrafodbc_l64_drvr.so.1 libtrafodbc_drvr64.so
   (( err += $? ))
   chmod 555 libtrafodbc_drvr64.so
   (( err += $? ))
   cd $PKG_DIR
   (( err += $? ))
   LIB_NAME=`ls libtrafodbc_l64_drvr.so `
   (( err += $? ))
   if (( $err != 0 )); then
     echo "driver installation failed.."
     echo "check the permission settings for $DIR_VALUE"
     echo "exiting.."
     exit -2
   fi
}

# copy etc files and include files
function install_files {
   err=0
   let start_index=$1
   let end_index=$2
   let index=start_index
   #TRAFDSN special case
   if (( index == 1 )); then 
     cp -f ${pkgfiles[index]} $DIR_VALUE/${pkgfiles[index]}
     (( err += $? ))
     (( index += 1 ))
   fi
   
   while (( index <= end_index ))
   do
     if [ -f $DIR_VALUE/${pkgfiles[index]} ] ; then
        cp -f $DIR_VALUE/${pkgfiles[index]} $DIR_VALUE/${pkgfiles[index]}.SAV
        (( err += $? ))
     fi
     cp -f ${pkgfiles[index]} $DIR_VALUE
     (( err += $? ))
     (( index += 1 ))
   done
   if (( $err != 0 )); then
	echo "error while copying files to $DIR_VALUE"
	echo "exiting..."
	exit -3
   fi
}

function copy_sample {
cp -f connect_test.cpp $DIR_VALUE/connect_test.cpp
if (( $? != 0 )); then
   echo "Error copying sample file to $DIR_VALUE"
   echo "Check the permission on $DIR_VALUE"
   exit -3
fi
}

function display_settings {
  echo -e "\nTRAFODBC driver has successfully been installed."
  echo -e "  * Library $LIB_NAME is installed on $LIB_DIR "
  echo -e "  * data source template file TRAFDSN has been copied onto $ETC_DIR"
  #echo -e "  * include files have been copied onto $INCLUDE_DIR"
  echo -e "  * sample file has been copied onto $SAMPLE_DIR\n"
}

function check_architecture {

echo "Verifying OS version"
ARCH=`uname -a`

case "$ARCH" in
	Linux*x86_64*)
		echo -e "Architecture verified";;	
	Linux*i686*)
		echo -e "You are installing a 64bit library on a 32bit linux"
		echo -n "installation. Do you want to proceed(yes/no)?"
		read OPTION
		if [[ $OPTION = YES ]] || [[ $OPTION = yes ]]
		then
			echo "Proceeding with install..."
		else
			echo "Exiting...."
			exit -4
		fi;;
	*)
		echo -e "We are unable to determine the architecture.  This"
		echo -e "means we could not determine what version of hardware"
		echo -e "you are using.  This could be problem."
		echo -n "Do you want to proceed(yes/no)?"
		read OPTION
		if [[ $OPTION = YES ]] || [[ $OPTION = yes ]]
		then
			echo "Proceeding with install..."
		else
			echo "Exiting...."
			exit -4
		fi;;
esac
}

#Beginning of Install
echo "\nBeginning Install of TRAFODBC Libraries\n"

#check_architecture
check_architecture

#check the package contents
check_package

# prompt user to accept license
license_check

#prompt for library
PROMPT="\nENTER a directory for library files\nOR hit Enter to use the default [/usr/lib64] : \c"
do_prompt /usr/lib64 "$PROMPT"
install_lib 
install_drvr_lib
#store for display_settings
LIB_DIR=$DIR_VALUE

#prompt for etc dir
PROMPT="\nENTER a directory for datasource template file\nOR hit Enter to use the default [/etc/odbc] : \c"
do_prompt /etc/odbc "$PROMPT"
# arg1 is start index into pkgFiles and arg2, the end_index 
install_files 1 3
#store for display_settings
ETC_DIR=$DIR_VALUE

#prompt for include dir
#PROMPT="\nENTER a directory for include files\nOR hit Enter to use the default [/usr/include/odbc] : \c"
#do_prompt /usr/include/odbc "$PROMPT"
# arg1 is start index into pkgFiles and arg2, the end_index 
#install_files 4 4
#store for display_settings
#INCLUDE_DIR=$DIR_VALUE

#prompt for sample dir
PROMPT="\nENTER a directory for sample\nOR hit Enter to use the default [/etc/odbc] : \c"
do_prompt /etc/odbc "$PROMPT"
#for now just copy this single file
copy_sample
#store for display_settings
SAMPLE_DIR=$DIR_VALUE

display_settings
