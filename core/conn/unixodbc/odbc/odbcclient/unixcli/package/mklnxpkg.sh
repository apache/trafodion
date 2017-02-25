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

#create a release file directory

FORCE=false
# ICU path must be set in environment

while [ $# -gt 0 ]
do
        case "$1" in
                -f)
                        FORCE=true;;
                -*)
                        echo "usage: mkpkg.sh [-f]"
                        exit 2;;
                *)
                        break;;
        esac
shift
done

if [[ ! -d relfiles ]]
then
	mkdir relfiles
	if (( $? != 0 ))
	then
	   echo -e "Can not create relfiles directory, exiting... \n"
	   exit 2
	fi
fi

cd relfiles

###############################################################################
#                LINUX 64 bit driver packaging begins                         #
###############################################################################

if [[ -d linux64 ]]
then
	if [[ $FORCE == "false" ]]
	then
		echo "Previous linux64 package files exists!"
		echo "You must remove these files to proceed. Delete (y/n)?"
		read STATE
		if [[ $STATE == "Y" ]] || [[ $STATE == "y" ]]
		then
			rm -rf linux64
			if [[ $? != 0 ]]
			then
				 echo -e "can not remove linux64 dir, exiting...\n"
				 exit 2
			fi
		fi
	else
		  rm -rf linux64
		  if [[ $? != 0 ]]
		  then
			echo -e "cannot remove linux64 dir, exiting...\n"
			exit 2
		  fi
	fi
fi

libname=`ls ../../libtrafodbc_l64.so`
libdrvrname=`ls ../../libtrafodbc_l64_drvr.so`
if [ ! -z $libname ]
then
    mkdir linux64
    if (( $? != 0 ))
    then
	echo -e "Can not create linux64 directory, exiting....\n"
	exit 2
    fi
    cd linux64
    let err=0
    cp ../../../libtrafodbc_l64.so ${libname##*/}
    ((err += $?))
    cp ${ICU}/linux64/lib/libicuuc.so.44.0 libicuuc.so.44
    ((err += $?))
    cp ${ICU}/linux64/lib/libicudata.so.44.0 libicudata.so.44
    ((err += $?))
    md5sum ./${libname##*/} >  MD5SUM
    ((err += $?))
    if [ ! -z $libdrvrname ]
    then
        cp ../../../libtrafodbc_l64_drvr.so ${libdrvrname##*/}
        ((err += $?))
		md5sum ./${libdrvrname##*/} >>  MD5SUM
		((err += $?))
    fi
    cp ../../TRAFDSN    TRAFDSN
    ((err += $?))
    cp ../../trafodbclnx_install.sh   install.sh
    ((err += $?))
    chmod 755  install.sh
    ((err += $?))
    cp ../../connect_test.cpp   connect_test.cpp
    ((err += $?))
    cp ../../runconnect_test.sh   runconnect_test.sh
    ((err += $?))
    cp $TRAF_HOME/../../licenses/Apache license.txt
    ((err += $?))
    cd ..
    # do the tar
    cp -rf linux64 PkgTmp
    ((err += $? ))
    tar -czf ../trafodbc_Linux64_pkg.tar.gz PkgTmp
    ((err += $? ))
    if (( $err == 0 ))
    then
       echo -e "Successfully created and distributed TRAF_ODBC_Linux_Driver_64.tar.gz to conn/clients folder\n"
       rm -rf PkgTmp
    else
       echo -e "Error while creating ODBC LINUX64 package!! \n \n"
    fi
else
	echo "Cannot find linux library..."
	echo -e "ODBC LINUX64 packaging has not been done!!!\n"
fi

