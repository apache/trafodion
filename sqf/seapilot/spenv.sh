#!/bin/bash

# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2009-2014 Hewlett-Packard Development Company, L.P.
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

###########################################################################
## spenv.sh :
##      This file is used to setup the required variables to both build
##      seapilot and to bring up the appropriate seapilot specific services
##      such as qpid, lighthouse.
##
##      This file should be sourced in after sqenv.sh
##
###########################################################################

if [ -z "${MY_SQROOT}" ] ; then
    export MY_SPROOT="$PWD"
else
    export MY_SPROOT="${MY_SQROOT}/seapilot"
fi

export SP_EXPORT_LIB="${MY_SPROOT}/export/lib${SQ_MBTYPE}"
export QPID_ROUTE_DIR="/usr/bin"
export QPID_PORTS_VER="1.0.0"

# Create seapilot logs directory
export SP_LOGS="${MY_SPROOT}/logs"
if [[ ! -d "$SP_LOGS" ]]; then
    mkdir -p $SP_LOGS
fi



###########################################################################
## qpid ports assignment :
##      The default qpid ports assignment is stored in $HOME/.qpidports.
##      This value is used as the starting port number for Qpid brokers
##      in the manageability messaging network.
###########################################################################

  export QPID_NODE_PORT=65500
  # Get value of QPID port from file.  If file doesn't exist, is empty,
  # or is not a number, then assign to new value.
  typeset QPID_PORT_IN_FILE=$(cat $HOME/.qpidports 2>/dev/null)
  if [[ ! -e $HOME/.qpidports ]] ||
     [[ -z $QPID_PORT_IN_FILE ]] ||
     ! [ $QPID_PORT_IN_FILE -eq $QPID_PORT_IN_FILE ] 2>/dev/null; then 
    echo $QPID_NODE_PORT > $HOME/.qpidports
    echo $QPID_PORTS_VER > $HOME/.qpidportsver  # assign version.
  fi

# To call this function you must first set the following variables:
# QPID_NODE_PORT, SQ_MTYPE
function add_to_env_files {

    # Verify that the QPID_NODE_PORT environment variable is set
    if [[ -z "$QPID_NODE_PORT" ]]; then
	echo "ERROR: The QPID_NODE_PORT environment variable is not set."
        return;
    fi

    # Verify that the QPID_NODE_PORT environment variable is a non-negative integer
    if [[ ! "$QPID_NODE_PORT" -ge 0 ]]; then
	printf '%s\t%s\t%s\t%s\n' "$(date)" "$HOSTNAME" "$MY_SQROOT" "${QPID_NODE_PORT}" >> $HOME/.qpidports_errors
	echo "ERROR: The QPID_NODE_PORT environment variable is out of range: $QPID_NODE_PORT."
    fi

    # Create the following files if they do not exist
    typeset ENVFILES_CREATE="	$MY_SQROOT/sql/scripts/mon.env \
				$MY_SQROOT/sql/scripts/shell.env"

    for ENVFILE in $ENVFILES_CREATE; do
	add_to_env_file 1 $ENVFILE
    done

    # Do not create these files if they do not already exist
    typeset ENVFILES_APPEND="$MY_SQROOT/etc/ms.env"

    for ENVFILE in $ENVFILES_APPEND; do
	add_to_env_file 0 $ENVFILE
    done
}

function add_to_env_file {
    typeset CREATE=$1
    typeset FILE=$2
 
    typeset ENVDIR=$(dirname $FILE)
    if [[ ! -f "$FILE" && "$CREATE" -eq 1 ]]; then
	mkdir -p $ENVDIR
	touch $FILE
	RET=$?
	if [[ $? -ne 0 ]]; then
	    echo "Unable to create file: $FILE"
            return
	fi

        # File created, add the QPID_NODE_PORT variable.
        echo >> $FILE
        echo "# qpid connection node port" >> $FILE
        echo "QPID_NODE_PORT=$QPID_NODE_PORT" >> $FILE
    fi

    if [[ -f "$FILE" ]]; then
        # If the file exists, check to see if
        # QPID_NODE_PORT is already defined, if not add the variable,
        # otherwise, synchronize the value of the QPID_NODE_PORT variable
        grep -qa 'QPID_NODE_PORT' $FILE
        if [[ $? -ne 0 ]]; then
            echo >> $FILE
            echo "# qpid connection node port" >> $FILE
            echo "QPID_NODE_PORT=$QPID_NODE_PORT" >> $FILE
        else
            typeset QPID_NODE_PORT_VALUE_IN_FILE=$(grep -a 'QPID_NODE_PORT' $FILE | tail -1 | cut -d '=' -f 2)
            if [[ "$QPID_NODE_PORT_VALUE_IN_FILE" != "$QPID_NODE_PORT" ]]; then
                sed -i "s/QPID_NODE_PORT=$QPID_NODE_PORT_VALUE_IN_FILE/QPID_NODE_PORT=$QPID_NODE_PORT/g" $FILE
            fi
        fi
    fi
}


    export QPID_NODE_PORT=$(cat $HOME/.qpidports)
        export QPID_INSTANCE_PORT=$((QPID_NODE_PORT + 1))
        if [ -n "${MY_SQROOT}" ] ; then
    	  add_to_env_files  
        fi
    
        if [[ -s $HOME/.qpidportsver ]]; then
            export QPID_NODE_PORT_VER=$(cat $HOME/.qpidportsver)
        else 
            export QPID_NODE_PORT_VER="0.0.0"
        fi
        
        # Get the number of nodes in the instance for use by UNA.
        TempList=$(grep 'node-id=.[0-9]*;node-name=.[^A-Za-z].[0-9]*' $MY_SQROOT/sql/scripts/sqconfig | grep -v "\#" | cut -d"=" -f3 | cut -d";" -f1 | sort -u)
        if [[ $TempList = "" ]]; then
          # This is a development environment.
          export NUMBER_INSTANCE_NODES=1
          export UNA_NODE_RANGE=0
        else
          export NUMBER_INSTANCE_NODES=$(echo $TempList | wc -w)
          let UNA_NODE_RANGE=$NUMBER_INSTANCE_NODES-1
          export UNA_NODE_RANGE
        fi
