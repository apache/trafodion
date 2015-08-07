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

dbToMove="hmon smon"
while getopts 'd:' parmOpt
do
    case $parmOpt in
    d)  dbToMove="${OPTARG}" ;;
    ?)  echo "Invalid option specified.   Only -d is allowed."
        exit 0 ;;
    esac
done
shift $(($OPTIND - 1))

newLocation=$1

if [ ${#newLocation} -lt 1 ] ; then
	echo "Syntax:  clouderMoveDB.sh[-d <database>] <newLocation>"
	echo "This script will create <newLocation> and then tell PostgreSQL"
	echo "for Cloudera to move the PostgreSQL <database> DB."
	echo "-d <database> to specify which DB to move."
	echo "    default is ${dbToMove}.  This is the large DB."
	echo "Note:  You have to be root to run this script."
	exit 0
fi

if [ $UID -ne 0 ] ; then
	echo "This script must be run as root."
	exit 0
fi

#
#  If not present, create new location directory.
#
if [ ! -d ${newLocation} ] ; then
	mkdir -p ${newLocation}
fi

if [ ! -d ${newLocation} ] ; then
	echo "Could not create ${newLocation}."
	exit 0
fi

#
#  Make sure it has proper ownership.
#
clouderaUser=cloudera-scm
wantUserGroup=${clouderaUser}:${clouderaUser}
userGroup=$(ls -ld ${newLocation} | awk '{print $3 ":" $4}')
if [ X${userGroup} != X${wantUserGroup} ] ; then
	chown ${wantUserGroup} ${newLocation}
fi
userGroup=$(ls -ld ${newLocation} | awk '{print $3 ":" $4}')
if [ X${userGroup} != X${wantUserGroup} ] ; then
	echo "Can not give ownership to ${clouderaUser} for ${newLocation}"
	exit 0
fi

#
#  Setup POSTGRESQL password file so that we get no prompting.
#  If you know the cloudera-scm password, define/export CLOUDERA_PGPASS and it will be used.
#  otherwise, we will fetch it.
#
export PGPASSFILE=./pgpassfile
tempFile=./tempFile
rm -f ${PGPASSFILE} ${tempFile}

if [ ${#CLOUDERA_PGPASS} -eq 0 ] ; then
	cloudera_genpassfile="/var/lib/cloudera-scm-server-db/data/generated_password.txt"
	if [ ! -f ${cloudera_genpassfile} ] ; then
		echo "Could not find cloudera-scm generated password."
		exit 0
	fi

	CLOUDERA_PGPASS=$(head -1 ${cloudera_genpassfile})
	unset cloudera_genpassfile
fi

cloudera_dbprops="/etc/cloudera-scm-server/db.properties"
if [ ! -f ${cloudera_dbprops} ] ; then
	echo "Could not find Cloudera db.properties files."
	exit 0
fi
hostPort=$(grep com.cloudera.cmf.db.host ${cloudera_dbprops} | awk -F= '{print $2}')
if [ ${#hostPort} -eq 0 ] ; then
	echo "Could not find com.cloudera.cmf.db.host in Cloudera db.properties file."
	exit 0
fi
unset cloudera_dbprops

echo "${hostPort}:*:${clouderaUser}:${CLOUDERA_PGPASS}" > ${PGPASSFILE}
if [ ! -f ${PGPASSFILE} ] ; then
	echo "Could not create temporary psql password files ${PGPASSFILE}"
	exit 0
fi
chmod 600 ${PGPASSFILE}

#
# Following will make sure our password and temporary files are removed.
cleanPASSFILE()
{
	rm -f ${PGPASSFILE} ${tempFile}
}
trap cleanPASSFILE ERR INT

#
# We're ready to go.
# Get list of tablespace.
phost=$(echo ${hostPort} | awk -F: '{print $1}')
pport=$(echo ${hostPort} | awk -F: '{print $2}')

for currDB in ${dbToMove}
do
    if [ $(psql -h ${phost} -p ${pport} -U ${clouderaUser} -l -q -t | grep -c ${currDB}) -eq 0 ] ; then
        echo "Db ${currDB} was not found in the PostgreSQL."
        cleanPASSFILE
        exit 0
    fi
done

dataSpace="data_space"
currDB=$(echo ${dbToMove} | awk '{print $1}')
psql -h ${phost} -p ${pport} -U ${clouderaUser} -d ${currDB} -q -t -c \\db > ${tempFile}

if [ $(grep -c ${dataSpace} ${tempFile}) -gt 0 ] ; then
	if [ $(grep ${dataSpace} ${tempFile} | awk '{print $5}') != ${newLocation} ] ; then
		echo "${dataSpace} is already defined and does not match ${newLocation}."
		grep ${dataSpace} ${tempFile}
		cleanPASSFILE
		exit 0
	fi
else
	psql -h ${phost} -p ${pport} -U ${clouderaUser} -d ${currDB} <<-EOT > ${tempFile}
		CREATE TABLESPACE ${dataSpace} LOCATION '${newLocation}';
		GRANT ALL ON TABLESPACE ${dataSpace} TO PUBLIC;
		\q
		EOT
	if [ $(grep -c 'CREATE TABLESPACE' ${tempFile}) -ne 1 ] ; then
		echo "Error creating tablespace ${dataSpace}"
		cat ${tempFile}
		cleanPASSFILE
		exit 0
	fi
	exit 0
fi

#
# Tablespace exists.
# Now get list of tables and indexes and setup move.
#
for currDB in ${dbToMove}
do
    echo "ALTER DATABASE ${currDB} SET default_tablespace = ${dataSpace};" > ${tempFile}
    while read -r parm1 parm2 objectName parmRest ; do
        if [ ${#objectName} -gt 0 ] ; then
            echo "ALTER TABLE ${objectName} set tablespace ${dataSpace};" >> ${tempFile}
        fi
    done <<< "$(psql -h ${phost} -p ${pport} -U ${clouderaUser} -d ${currDB} -q -t -c \\dt)"
    while read -r parm1 parm2 objectName parmRest ; do
        if [ ${#objectName} -gt 0 ] ; then
            echo "ALTER INDEX ${objectName} set tablespace ${dataSpace};" >> ${tempFile}
        fi
    done <<< "$(psql -h ${phost} -p ${pport} -U ${clouderaUser} -d ${currDB} -q -t -c \\di)"

    #
    # Ready to apply changes.
    #
    psql -h ${phost} -p ${pport} -U ${clouderaUser} -d ${currDB} -a -f ${tempFile}
    #cat ${tempFile}
done

cleanPASSFILE
