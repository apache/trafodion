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

##############################################################
# Set / Calculate standard environmental values
# Note: Do NOT put local customizations here. See sqf/LocalSettingsTemplate.sh
##############################################################
# Adding build-time dependencies:
#  * Add environment variable in the last section of this file.
#  * Talk to the infra team about getting the tool/library/etc installed on build/test machines.
#  * Add an entry in sqf/LocalSettingsTemplate.sh to make path configurable.
##############################################################

#Product version (Trafodion or derivative product)
export TRAFODION_VER_PROD="Apache Trafodion"
# Trafodion version (also update file ../sql/common/copyright.h)
export TRAFODION_VER_MAJOR=2
export TRAFODION_VER_MINOR=0
export TRAFODION_VER_UPDATE=0
export TRAFODION_VER="${TRAFODION_VER_MAJOR}.${TRAFODION_VER_MINOR}.${TRAFODION_VER_UPDATE}"

# Product copyright header
export PRODUCT_COPYRIGHT_HEADER="2015 Apache Software Foundation"
##############################################################
# Trafodion authentication:
#    Set TRAFODION_ENABLE_AUTHENTICATION to YES to enable
# authentication in the Trafodion environment.
# LDAP configuration must also be setup--see
# $MY_SQROOT/sql/scripts/traf_authentication_config for details.
##############################################################
export TRAFODION_ENABLE_AUTHENTICATION=NO


# default SQ_IC to TCP if it is not set in sqenv.sh. Values are
# IBV for infiniband, TCP for tcp
export SQ_IC=${SQ_IC:-TCP}
export MPI_IC_ORDER=$SQ_IC

# use sock
#export SQ_TRANS_SOCK=1

if [[ -z "$SQ_VERBOSE" ]]; then
  SQ_VERBOSE=0
fi
# temp variable for 64 bit cluster testing
# export SQ_WDT_KEEPALIVETIMERVALUE=900
#envvar to limit the number of memory arenas
export MALLOC_ARENA_MAX=1

# set this to 0 to use GNU compiler
export SQ_USE_INTC=0

if [[ "$SQ_BUILD_TYPE" = "release" ]]; then
  SQ_BTYPE=
else
  SQ_BTYPE=d
fi
export SQ_BTYPE
export SQ_MBTYPE=$SQ_MTYPE$SQ_BTYPE

# To enable code coverage, set this to 1
if [[ -z "$SQ_COVERAGE" ]]; then
  SQ_COVERAGE=0
elif [[ $SQ_COVERAGE -eq 1 ]]; then
  if [[ "$SQ_VERBOSE" == "1" ]]; then
    echo "*****************************"
    echo "*  Code coverage turned on  *"
    echo "*****************************"
    echo
  fi
fi
export SQ_COVERAGE

# Set default build parallelism
# Can be overridden on make commandline
cpucnt=$(grep processor /proc/cpuinfo | wc -l)
#     no number means unlimited, and will swamp the system
export MAKEFLAGS="-j$cpucnt"

# For now set up the TOOLSDIR, it may be overwritten later when the
# .trafodion configuration file is loaded.
if [ -z "$TOOLSDIR" ]; then
  if [[ -n "$CLUSTERNAME" ]]; then
    export TOOLSDIR=${TOOLSDIR:-/home/tools}
    export MY_UDR_ROOT=/home/udr
  else
    export TOOLSDIR=${TOOLSDIR:-/opt/home/tools}
    export MY_UDR_ROOT=$PWD
  fi
fi


# Use JAVA_HOME if set, else look for installed openjdk, finally toolsdir
REQ_JDK_VER="1.7.0_67"
if [[ -z "$JAVA_HOME" && -d "${TOOLSDIR}/jdk${REQ_JDK_VER}" ]]; then
  export JAVA_HOME="${TOOLSDIR}/jdk${REQ_JDK_VER}"
elif [[ -z "$JAVA_HOME" && -d /usr/lib/jvm/java-1.7.0-openjdk.x86_64/ ]]; then
  export JAVA_HOME="/usr/lib/jvm/java-1.7.0-openjdk.x86_64"
elif [[ -z "$JAVA_HOME" ]]; then
  echo "Please set JAVA_HOME to version jdk${REQ_JDK_VER}"
fi



export SQ_PDSH=/usr/bin/pdsh
export PDSH="$SQ_PDSH -R exec"
export PDSH_SSH_CMD="ssh -q -n %h"
export SQ_PDCP=/usr/bin/pdcp
export PDCP="$SQ_PDCP -R ssh"
export TAR_DOWNLOAD_ROOT=$HOME/sqllogs
export CACERTS_DIR=$HOME/cacerts

# Get redhat major version
# Examples:
# Red Hat Enterprise Linux Server release 6.3 (Santiago)
# CentOS release 6.5 (Final)
if [[ "$SUSE_LINUX" == "false" ]]; then
   export RH_MAJ_VERS=$(sed -r 's/^.* release ([0-9]+).[0-9]+ .*/\1/' /etc/redhat-release)
else
   export RH_MAJ_VERS=6
fi
export MY_SQROOT=$PWD
export SQ_HOME=$PWD

export HBASE_TRXDIR=$MY_SQROOT/export/lib
export HBASE_TRX_JAR=hbase-trx-cdh5_3-${TRAFODION_VER}.jar
export UTIL_JAR=trafodion-utility-${TRAFODION_VER}.jar
if [[ "$SQ_HBASE_DISTRO" = "HDP" ]]; then
    export HBASE_TRX_JAR=hbase-trx-hdp2_2-${TRAFODION_VER}.jar
fi

# check for workstation env
# want to make sure SQ_VIRTUAL_NODES is set in the shell running sqstart
# so we can determine if we are on a workstation or not
if [[ -e ${MY_SQROOT}/etc/ms.env ]] ; then
  VIRT_NODES=$(awk '/SQ_VIRTUAL_NODES=/ { fields=split($0,virt,"=");  if ( fields == 2 ) { virtnodes=virt[2];}} END {print  virtnodes}' < $MY_SQROOT/etc/ms.env)
  if [[ -n "$VIRT_NODES" ]] ; then
     export SQ_VIRTUAL_NODES="$VIRT_NODES"
  fi
fi

export SQ_IDTMSRV=1

export MY_MPI_ROOT="$MY_SQROOT"
export MPI_ROOT="$MY_SQROOT/opt/hpmpi"

unset MPI_CC
export MPI_CXX=/usr/bin/g++
export MPICH_CXX=$MPI_CXX

export PATH=$MPI_ROOT/bin:$MY_SQROOT/export/bin"$SQ_MBTYPE":$MY_SQROOT/sql/scripts:$MY_SQROOT/tools:$MY_SQROOT/trafci/bin:$PATH
# The guiding principle is that the user's own software is preferred over anything else;
# system customizations are likewise preferred over default software.

CC_LIB_RUNTIME=/lib64:/usr/lib64
VARLIST=""


# need these to link

MPILIB=linux_amd64

# There are minor differences between Hadoop 1 and 2, right now
# this is visible in the libhdfs interface.
unset USE_HADOOP_1


#-------------------------------------------------------------------------------
# Setup for Hadoop integration
#
# The section below is meant to be customized for an installation, other
# parts of this file should not be dependent on a specific installation
#
# Set the following environment variables, based on the Hadoop distro:
#-------------------------------------------------------------------------------

# Native libraries and include directories (the latter needed only to build from source)

# HADOOP_LIB_DIR           directory of HDFS library libhdfs.so
# HADOOP_INC_DIR           directory with header files for libhdfs
# THRIFT_LIB_DIR           directory of Thrift library libthrift.so
# THRIFT_INC_DIR           directory with header files for thrift
# CURL_LIB_DIR           directory of Thrift library libcurl.so
# CURL_INC_DIR           directory with header files for curl
# LOC_JVMLIBS              directory of the JNI C++ DLL libjvm.so
# LOG4CXX_LIB_DIR          directory of log4cxx library lib4cxx.so
# LOG4CXX_INC_DIR          directory with header files for log4cxx

# Elements of the CLASSPATH for Trafodion

# HADOOP_JAR_DIRS          directories with Hadoop jar files to be included
# HADOOP_JAR_FILES         individual Hadoop entries for the class path
# HBASE_JAR_FILES          individual HBase entries for the class path
# HIVE_JAR_DIRS            directories with Hive jar files to be included
# HIVE_JAR_FILES           individual Hive entries for the class path

# Configuration directories

# HADOOP_CNF_DIR           directory with Hadoop configuration files
# HBASE_CNF_DIR            directory with HBase config file hbase-site.xml
# HIVE_CNF_DIR             directory with Hive config file hive-site.xml

# ---+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
if [[ "$SQ_MTYPE" == 64 ]]; then
  export LOC_JVMLIBS=$JAVA_HOME/jre/lib/amd64/server
else
  export LOC_JVMLIBS=$JAVA_HOME/jre/lib/i386/server
fi

if [[ -e $MY_SQROOT/sql/scripts/sw_env.sh ]]; then
  # we are on a development system where install_local_hadoop has been
  # executed
  # ----------------------------------------------------------------
  [[ $SQ_VERBOSE == 1 ]] && echo "Sourcing in $MY_SQROOT/sql/scripts/sw_env.sh"
  . $MY_SQROOT/sql/scripts/sw_env.sh
  #echo "YARN_HOME = $YARN_HOME"

  # native library directories and include directories
  export HADOOP_LIB_DIR=$YARN_HOME/lib/native
  export HADOOP_INC_DIR=$YARN_HOME/include
  if [ -z $THRIFT_LIB_DIR ]; then
    export THRIFT_LIB_DIR=$TOOLSDIR/thrift-0.9.0/lib
  fi
  if [ -z $THRIFT_INC_DIR ]; then
    export THRIFT_INC_DIR=$TOOLSDIR/thrift-0.9.0/include
  fi
  export CURL_INC_DIR=/usr/include
  export CURL_LIB_DIR=/usr/lib64
  # directories with jar files and list of jar files
  export HADOOP_JAR_DIRS="$YARN_HOME/share/hadoop/common
                          $YARN_HOME/share/hadoop/common/lib
                          $YARN_HOME/share/hadoop/mapreduce
                          $YARN_HOME/share/hadoop/hdfs"
  export HADOOP_JAR_FILES=
  export HBASE_JAR_FILES=
  HBASE_JAR_DIRS="$HBASE_HOME/lib"
  for d in $HBASE_JAR_DIRS; do
    HBASE_JAR_FILES="$HBASE_JAR_FILES $d/*.jar"
  done

  export HIVE_JAR_DIRS="$HIVE_HOME/lib"
  export HIVE_JAR_FILES="$HIVE_HOME/share/hadoop/mapreduce/hadoop-mapreduce-client-core-*.jar"

  # suffixes to suppress in the classpath (set this to ---none--- to add all files)
  export SUFFIXES_TO_SUPPRESS="-sources.jar -tests.jar"

  # Configuration directories

  export HADOOP_CNF_DIR=$MY_SQROOT/sql/local_hadoop/hadoop/etc/hadoop
  export HBASE_CNF_DIR=$MY_SQROOT/sql/local_hadoop/hbase/conf
  export HIVE_CNF_DIR=$MY_SQROOT/sql/local_hadoop/hive/conf

elif [[ -d /opt/cloudera/parcels/CDH ]]; then
  # we are on a cluster with Cloudera parcels installed
  # -------------------------------------------

  # native library directories and include directories
  export HADOOP_LIB_DIR=/opt/cloudera/parcels/CDH/lib/hadoop/lib/native
  export HADOOP_INC_DIR=/opt/cloudera/parcels/CDH/include

  ### Thrift not supported on Cloudera yet (so use TOOLSDIR download)
  export THRIFT_LIB_DIR=$TOOLSDIR/thrift-0.9.0/lib
  export THRIFT_INC_DIR=$TOOLSDIR/thrift-0.9.0/include


  export CURL_INC_DIR=/usr/include
  export CURL_LIB_DIR=/usr/lib64

  # directories with jar files and list of jar files
  # (could try to reduce the number of jars in the classpath)
  export HADOOP_JAR_DIRS="/opt/cloudera/parcels/CDH/lib/hadoop
                          /opt/cloudera/parcels/CDH/lib/hadoop/lib"
  export HADOOP_JAR_FILES="/opt/cloudera/parcels/CDH/lib/hadoop/client/hadoop-hdfs-*.jar"
  export HBASE_JAR_FILES="/opt/cloudera/parcels/CDH/lib/hbase/hbase-*-security.jar
                          /opt/cloudera/parcels/CDH/lib/hbase/hbase-client.jar
                          /opt/cloudera/parcels/CDH/lib/hbase/hbase-common.jar
                          /opt/cloudera/parcels/CDH/lib/hbase/hbase-server.jar
                          /opt/cloudera/parcels/CDH/lib/hbase/hbase-examples.jar
                          /opt/cloudera/parcels/CDH/lib/hbase/hbase-protocol.jar
                          /opt/cloudera/parcels/CDH/lib/hbase/lib/htrace-core.jar
                          /opt/cloudera/parcels/CDH/lib/hbase/lib/zookeeper.jar
                          /opt/cloudera/parcels/CDH/lib/hbase/lib/$protobuf-*.jar
                          /opt/cloudera/parcels/CDH/lib/hbase/lib/snappy-java-*.jar
                          /opt/cloudera/parcels/CDH/lib/hbase/lib/high-scale-lib-*.jar
                          /opt/cloudera/parcels/CDH/lib/hbase/hbase-hadoop-compat.jar "
  export HIVE_JAR_DIRS="/opt/cloudera/parcels/CDH/lib/hive/lib"
  export HIVE_JAR_FILES="/opt/cloudera/parcels/CDH/lib/hadoop-mapreduce/hadoop-mapreduce-client-core.jar"

  # suffixes to suppress in the classpath (set this to ---none--- to add all files)
  export SUFFIXES_TO_SUPPRESS="-sources.jar -tests.jar"

  # Configuration directories

  export HADOOP_CNF_DIR=/etc/hadoop/conf
  export HBASE_CNF_DIR=/etc/hbase/conf
  export HIVE_CNF_DIR=/etc/hive/conf

elif [[ -n "$(ls /usr/lib/hadoop/hadoop-*cdh*.jar 2>/dev/null)" ]]; then
  # we are on a cluster with Cloudera installed
  # -------------------------------------------
  
  # native library directories and include directories
  export HADOOP_LIB_DIR=/usr/lib/hadoop/lib/native
  export HADOOP_INC_DIR=/usr/include

  ### Thrift not supported on Cloudera yet (so use TOOLSDIR download)
  export THRIFT_LIB_DIR=$TOOLSDIR/thrift-0.9.0/lib
  export THRIFT_INC_DIR=$TOOLSDIR/thrift-0.9.0/include


  export CURL_INC_DIR=/usr/include
  export CURL_LIB_DIR=/usr/lib64

  # directories with jar files and list of jar files
  # (could try to reduce the number of jars in the classpath)
  export HADOOP_JAR_DIRS="/usr/lib/hadoop
                          /usr/lib/hadoop/lib"
  export HADOOP_JAR_FILES="/usr/lib/hadoop/client/hadoop-hdfs-*.jar"
  export HBASE_JAR_FILES="/usr/lib/hbase/hbase-*-security.jar
                          /usr/lib/hbase/hbase-client.jar
                          /usr/lib/hbase/hbase-common.jar
                          /usr/lib/hbase/hbase-server.jar
                          /usr/lib/hbase/hbase-examples.jar
                          /usr/lib/hbase/hbase-protocol.jar
                          /usr/lib/hbase/lib/htrace-core.jar
                          /usr/lib/hbase/lib/zookeeper.jar
                          /usr/lib/hbase/lib/protobuf-*.jar
                         /usr/lib/hbase/lib/snappy-java-*.jar
                         /usr/lib/hbase/lib/high-scale-lib-*.jar
                         /usr/lib/hbase/hbase-hadoop-compat.jar "
  export HIVE_JAR_DIRS="/usr/lib/hive/lib"
  export HIVE_JAR_FILES="/usr/lib/hadoop-mapreduce/hadoop-mapreduce-client-core.jar"

  # suffixes to suppress in the classpath (set this to ---none--- to add all files)
  export SUFFIXES_TO_SUPPRESS="-sources.jar -tests.jar"

  # Configuration directories

  export HADOOP_CNF_DIR=/etc/hadoop/conf
  export HBASE_CNF_DIR=/etc/hbase/conf
  export HIVE_CNF_DIR=/etc/hive/conf

elif [[ -n "$(ls /etc/init.d/ambari* 2>/dev/null)" ]]; then
  # we are on a cluster with Hortonworks installed
  # ----------------------------------------------

  # native library directories and include directories
  export HADOOP_LIB_DIR=/usr/hdp/current/hadoop-client/lib/native/Linux-*-${SQ_MTYPE}:/usr/hdp/current/hadoop-client/lib/native
  export HADOOP_INC_DIR=/usr/include
  # The supported HDP version, HDP 1.3 uses Hadoop 1
  export USE_HADOOP_1=1

  ### Thrift not supported on Hortonworks yet (so use TOOLSDIR download)
  export THRIFT_LIB_DIR=$TOOLSDIR/thrift-0.9.0/lib
  export THRIFT_INC_DIR=$TOOLSDIR/thrift-0.9.0/include
  export CURL_INC_DIR=/usr/include
  export CURL_LIB_DIR=/usr/lib64

  # directories with jar files and list of jar files
  export HADOOP_JAR_DIRS="/usr/hdp/current/hadoop-client
                          /usr/hdp/current/hadoop-client/lib"
  export HADOOP_JAR_FILES="/usr/hdp/current/hadoop-client/client/hadoop-hdfs-*.jar"
  export HBASE_JAR_FILES="/usr/hdp/current/hbase-client/hbase-*-security.jar
                          /usr/hdp/current/hbase-client/lib/hbase-common.jar
                          /usr/hdp/current/hbase-client/lib/hbase-client.jar
                          /usr/hdp/current/hbase-client/lib/hbase-server.jar
                          /usr/hdp/current/hbase-client/lib/hbase-protocol.jar
                          /usr/hdp/current/hbase-client/lib/htrace-core*.jar
                          /usr/hdp/current/hbase-client/lib/zookeeper.jar
                          /usr/hdp/current/hbase-client/lib/protobuf-*.jar
                         /usr/hdp/current/hbase-client/lib/snappy-java-*.jar
                         /usr/hdp/current/hbase-client/lib/high-scale-lib-*.jar
                         /usr/hdp/current/hbase-client/lib/hbase-hadoop-compat-*-hadoop2.jar "

  export HIVE_JAR_DIRS="/usr/hdp/current/hive-client/lib"
  export HIVE_JAR_FILES="/usr/hdp/current/hadoop-mapreduce-client/hadoop-mapreduce-client-core*.jar"

  export HBASE_TRX_JAR=hbase-trx-hdp2_2-${TRAFODION_VER}.jar

  # Configuration directories

  export HADOOP_CNF_DIR=/etc/hadoop/conf
  export HBASE_CNF_DIR=/etc/hbase/conf
  export HIVE_CNF_DIR=/etc/hive/conf

elif [[ -d /opt/mapr ]]; then
  # we are on a MapR system
  # ----------------------------------------------------------------

  # We tried this with MapR 3.1, which has hadoop-0.20.2, hbase-0.94.13, hive-0.12

  [[ $SQ_VERBOSE == 1 ]] && echo "Found /opt/mapr, this is a MapR distro"

  # Note that hadoopversion and hiveversion are not officially
  # supported by MapR, only hbaseversion is. We recommend creating
  # these files to guide Trafodion to the right version, if necessary.
  if [[ -r /opt/mapr/hadoop/hadoopversion ]]; then
    MAPR_HADOOP_VERSION=$(cat /opt/mapr/hadoop/hadoopversion)
    MAPR_HADOOPDIR=/opt/mapr/hadoop/hadoop-${MAPR_HADOOP_VERSION}
  else
    MAPR_HADOOPDIR=$(echo /opt/mapr/hadoop/hadoop-*)
  fi
  if [[ -r /opt/mapr/hbase/hbaseversion ]]; then
    MAPR_HBASE_VERSION=$(cat /opt/mapr/hbase/hbaseversion)
    MAPR_HBASEDIR=/opt/mapr/hbase/hbase-${MAPR_HBASE_VERSION}
  else
    MAPR_HBASEDIR=$(echo /opt/mapr/hbase/hbase-*)
  fi
  if [[ -r /opt/mapr/hive/hiveversion ]]; then
    MAPR_HIVE_VERSION=$(cat /opt/mapr/hive/hiveversion)
    MAPR_HIVEDIR=/opt/mapr/hive/hive-${MAPR_HIVE_VERSION}
  else
    MAPR_HIVEDIR=$(echo /opt/mapr/hive/hive-*)
  fi

  ### Thrift not supported on MapR (so use TOOLSDIR download)
  export THRIFT_LIB_DIR=$TOOLSDIR/thrift-0.9.0/lib
  export THRIFT_INC_DIR=$TOOLSDIR/thrift-0.9.0/include
  export CURL_INC_DIR=/usr/include
  export CURL_LIB_DIR=/usr/lib64

  # native library directories and include directories
  if [[ -r $MAPR_HADOOPDIR/lib/native/Linux-amd64-64/libhdfs.so ]]; then
    export HADOOP_LIB_DIR=$MAPR_HADOOPDIR/lib/native/Linux-amd64-64
  else
    export HADOOP_LIB_DIR=$MAPR_HADOOPDIR/c++/Linux-amd64-64/lib
  fi
  export HADOOP_INC_DIR=/build-not-supported-on-MapR-yet
  export USE_HADOOP_1=1

  # directories with jar files and list of jar files
  export HADOOP_JAR_DIRS="$MAPR_HADOOPDIR/lib"
  export HADOOP_JAR_FILES="$MAPR_HADOOPDIR/client/hadoop-hdfs-*.jar"
  export HBASE_JAR_FILES="$MAPR_HBASEDIR/hbase-*.jar
                          $MAPR_HBASEDIR/lib/zookeeper.jar
                          $MAPR_HBASEDIR/lib/protobuf-*.jar
                          $MAPR_HBASEDIR/lib/snappy-java-*.jar "
  export HIVE_JAR_DIRS="$MAPR_HIVEDIR/lib"
  # Could not find a hadoop-mapreduce-client-core*.jar on my MapR test cluster,
  # this jar file is required by other distros.

  # Configuration directories

  export HADOOP_CNF_DIR=$MAPR_HADOOPDIR/conf
  export HBASE_CNF_DIR=$MAPR_HBASEDIR/conf
  export HIVE_CNF_DIR=$MAPR_HIVEDIR/conf

  # HBase-trx jar with some modifications to work with MapR HBase 0.94.13
  export HBASE_TRX_JAR=hbase-trx-mapr4_0-trx-${TRAFODION_VER}.jar

else

  # try some other options

  function vanilla_apache_usage {

  cat <<EOF

    If you are ready to build Trafodion, perform one of the following options:

      make all         (Build Trafodion, DCS, and REST) OR
      make package     (Build Trafodion, DCS, REST, and Client drivers)  OR
      make package-all (Build Trafodion, DCS, REST, Client drivers, and Tests)

    If Trafodion has been built and you want test: 

       Execute the install_local_hadoop script which performs a single node
       install using a popular Hadoop distribution 

          cd $MY_SQROOT/sql/scripts
          install_local_hadoop [-p <port option>]
          install_traf_components 
          configure Trafodion and start the processes
          HAVE FUN!

   You can also choose to install_local_hadoop before building Trafodion.

EOF
  }

  # Hadoop/HBase/Hive install directories, as determined by this script,
  # when using an Apache installation without one of the distros
  APACHE_HADOOP_HOME=
  APACHE_HBASE_HOME=
  APACHE_HIVE_HOME=

  if [ -f $HADOOP_PREFIX/etc/hadoop/core-site.xml ]; then
    APACHE_HADOOP_HOME=$HADOOP_PREFIX
    export HADOOP_CNF_DIR=$HADOOP_PREFIX/etc/hadoop
  fi
  if [ -f $HBASE_HOME/conf/hbase-site.xml ]; then
    [[ $SQ_VERBOSE == 1 ]] && echo "HBASE_HOME is set to $HBASE_HOME, this is vanilla Apache"
    APACHE_HBASE_HOME=$HBASE_HOME
    export HBASE_CNF_DIR=$HBASE_HOME/conf
  fi

  APACHE_HIVE_HOME=$HIVE_HOME
  export HIVE_CNF_DIR=$HIVE_HOME/conf

  for cp in `echo $CLASSPATH | sed 's/:/ /g'`
  do
    if [ -f $cp/core-site.xml ]; then
      export HADOOP_CNF_DIR=$cp
      APACHE_HADOOP_HOME=$(dirname $(dirname $cp))
    fi
    if [ -f $cp/hbase-site.xml ]; then
      [[ $SQ_VERBOSE == 1 ]] && echo "Found $cp/hbase-site.xml in CLASSPATH, this is vanilla Apache"
      export HBASE_CNF_DIR=$cp
      APACHE_HBASE_HOME=`dirname $cp`
    fi
    if [ -f $cp/hive-site.xml ]; then
      export HIVE_CNF_DIR=$cp
      APACHE_HIVE_HOME=`dirname $cp`
    fi
  done

  # sometimes, conf file and lib files don't have the same parent,
  # try to handle some common cases, where the libs are under /usr/lib
  if [ ! -d $APACHE_HADOOP_HOME/lib/ -a -d /usr/lib/hadoop ]; then
    APACHE_HADOOP_HOME=/usr/lib/hadoop
  fi
  if [ ! -d $APACHE_HBASE_HOME/lib -a -d /usr/lib/hbase ]; then
    APACHE_HBASE_HOME=/usr/lib/hbase
  fi
  if [ ! -d $APACHE_HIVE_HOME/lib -a -d /usr/lib/hive ]; then
    APACHE_HIVE_HOME=/usr/lib/hive
  fi

  if [ ! -d $APACHE_HADOOP_HOME/lib ]; then
    echo "**** ERROR: Unable to determine location of Hadoop lib directory"
  fi

  if [ ! -d $APACHE_HBASE_HOME/lib ]; then
    echo "**** ERROR: Unable to determine location of HBase lib directory"
  fi

  if [[ -d $TOOLSDIR/thrift-0.9.0 ]]; then
    # this is mostly for a build environment, where we need
    # thrift from TOOLSDIR
    export THRIFT_LIB_DIR=$TOOLSDIR/thrift-0.9.0/lib
    export THRIFT_INC_DIR=$TOOLSDIR/thrift-0.9.0/include
  fi

  if [ -n "$HBASE_CNF_DIR" -a -n "$HADOOP_CNF_DIR" -a \
       -d $APACHE_HADOOP_HOME/lib -a -d $APACHE_HBASE_HOME/lib ]; then
    # We are on a system with Apache HBase, probably without a distro
    # ---------------------------------------------------------------


    [ "$SQ_VERBOSE" == 1 ] && echo "Found both HBase and Hadoop config for vanilla Apache"

    # native library directories and include directories
    if [ -f /usr/lib/hadoop/lib/native/libhdfs.so ]; then
      export HADOOP_LIB_DIR=/usr/lib/hadoop/lib/native
    elif [ -f $APACHE_HADOOP_HOME/lib/native/libhdfs.so ]; then
      export HADOOP_LIB_DIR=$APACHE_HADOOP_HOME/lib/native
    else
      echo '**** ERROR: Could not find Hadoop native library libhdfs.so in ' $APACHE_HADOOP_HOME/lib/native
    fi

    if [ -f /usr/include/hdfs.h ]; then
      export HADOOP_INC_DIR=/usr/include
    elif [ -f $APACHE_HADOOP_HOME/include/hdfs.h ]; then
      export HADOOP_INC_DIR=$APACHE_HADOOP_HOME/include
    fi

    # directories with jar files and list of jar files
    export HADOOP_JAR_DIRS="$APACHE_HADOOP_HOME/share/hadoop/common
                            $APACHE_HADOOP_HOME/share/hadoop/common/lib
                            $APACHE_HADOOP_HOME/share/hadoop/mapreduce
                            $APACHE_HADOOP_HOME/share/hadoop/hdfs"
    export HADOOP_JAR_FILES=


    export HBASE_JAR_FILES="$APACHE_HBASE_HOME/lib/hbase-common-*.jar
                            $APACHE_HBASE_HOME/lib/hbase-client-*.jar
                            $APACHE_HBASE_HOME/lib/hbase-server-*.jar
                            $APACHE_HBASE_HOME/lib/hbase-protocol-*.jar
                            $APACHE_HBASE_HOME/lib/hbase-examples-*.jar
                            $APACHE_HBASE_HOME/lib/htrace-core-*.jar
                            $APACHE_HBASE_HOME/lib/zookeeper-*.jar
                            $APACHE_HBASE_HOME/lib/protobuf-*.jar
                            $APACHE_HBASE_HOME/lib/snappy-java-*.jar
                            $APACHE_HBASE_HOME/lib/high-scale-lib-*.jar
                            $APACHE_HBASE_HOME/lib/hbase-hadoop-compat-*-hadoop2.jar "

    export HIVE_JAR_DIRS="$APACHE_HIVE_HOME/lib"

    export HBASE_TRX_JAR=hbase-trx-hbase_98_4-${TRAFODION_VER}.jar

    # end of code for Apache Hadoop/HBase installation w/o distro
  else
    # print usage information, not enough information about Hadoop/HBase
    vanilla_apache_usage
    NEEDS_HADOOP_INSTALL=1
  fi

fi

# ---+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
# end of customization variables

# Common for local workstations, Cloudera, Hortonworks and MapR

if [ -z $ZOOKEEPER_DIR ]; then
  export ZOOKEEPER_DIR=$TOOLSDIR/zookeeper-3.4.5
fi
if [ -z $MPICH_ROOT ]; then
  export MPICH_ROOT=$TOOLSDIR/dest-mpich-3.0.4
fi
export PROTOBUFS=/usr

# LOG4CXX
if [[ -d $TOOLSDIR/apache-log4cxx-0.10.0 ]]
then
  export LOG4CXX_LIB_DIR=$TOOLSDIR/apache-log4cxx-0.10.0/lib
  export LOG4CXX_INC_DIR=$TOOLSDIR/apache-log4cxx-0.10.0/include
else
  export LOG4CXX_LIB_DIR=/lib64:/usr/lib64
  export LOG4CXX_INC_DIR=/usr/include/log4cxx
fi

# For now, set the QT_TOOLKIT envvar if the required version exists in the
# download location
if [[ -z "$QT_TOOLKIT" && -d $TOOLSDIR/Qt-4.8.5-64 ]]; 
then
   # QT_TOOLKIT is optional, if the directory doesn't exist
   # then we won't build the compiler GUI
   export QT_TOOLKIT="$TOOLSDIR/Qt-4.8.5-64"
fi


# for debugging
export LD_BIND_NOW=true

export MPI_TMPDIR=$PWD/tmp
if [[ -d $MPI_TMPDIR ]]; then
  if [[ "$SQ_VERBOSE" == "1" ]]; then
    echo "Pre-existing directory found for MPI_TMPDIR: $MPI_TMPDIR"
    echo
  fi
else
  if [[ "$SQ_VERBOSE" == "1" ]]; then
    echo "Creating directory for MPI_TMPDIR: $MPI_TMPDIR"
    echo
  fi
  mkdir $MPI_TMPDIR
fi

# Add man pages
export MANPATH=$MANPATH:$MY_SQROOT/export/share/man

# Control lunmgr verbosity
export SQ_LUNMGR_VERBOSITY=1

# 32 bit workaround
#export OLDGETBUF=1
# Control SQ default startup behavior (c=cold, w=warm, if removed sqstart will autocheck)
export SQ_STARTUP=r

# Alternative logging capability in monitor
export SQ_MON_ALTLOG=0

# Monitor sync thread responsiveness timeout
# default 15 mins
export SQ_MON_SYNC_TIMEOUT=900

# set to 0 to disable phandle verifier
export SQ_PHANDLE_VERIFIER=1

# set to 0 to disable or 1 to enable configuration of DTM as a persistent process
# must re-execute 'sqgen' to effect change
export SQ_DTM_PERSISTENT_PROCESS=1

# Check the state of the node with the cluster manager during regroup
export SQ_WDT_CHECK_CLUSTER_STATE=0

# Perl libraries used by Seaquest (e.g. sqgen components)
export PERL5LIB=$MY_SQROOT/export/lib

# Enable SQ_PIDMAP if you want to get a record of process activity.
# This can be useful in troubleshooting problems.  There is an overhead cost
# incurred each time a process is started so do not enable this if performance
# is critical.
# Log process start/end messages in $MY_SQROOT/tmp/monitor.map
export SQ_PIDMAP=1

#Enable RMS (SQL Run time statistics)
export SQ_START_RMS=1

# Enable QVP (Query Validation Process).
export SQ_START_QVP=1

# Set to 1 to enable QMM (Query Matching Monitor process for MVQR).
export SQ_START_QMM=0

# SSD provider scripts
export LMP_POLL_FREQ=60
export LSP_POLL_FREQ=60
export SMP_POLL_FREQ=60
export SSP_POLL_FREQ=60

# Compression provider scripts
export CSP_POLL_FREQ=60
export CMP_POLL_FREQ=10

# need this to use new phandle
export SQ_NEW_PHANDLE=1

unset SQ_SEAMONSTER

# Uncomment to set ssh connection option used by the 'sqnodestatus' script
#export SQ_MON_SSH_OPTIONS=' -o "ConnectTimeout 1" -o "ConnectionAttempts 3" '

export ENABLE_EMBEDDED_ARKCMP=1

# make sure we get HUGE pages in cores
echo "0x73" > /proc/$$/coredump_filter

# The "unset SQ_EVLOG_NONE" command below does nothing when followed by the
# "export SQ_EVLOG_NONE" command. It comes into play when SeaPilot is
# enabled, in which case "export SQ_EVLOG_NONE" will be deleted from this
# script and "unset SQ_EVLOG_NONE" will remain, to clean up shells where
# "export SQ_EVLOG_NONE" was executed prior to enabling SeaPilot. The
# "unset SQ_SEAPILOT_SUSPENDED" command serves the same purpose for the
# reverse situation; that is, to clean up shells where
# "export SQ_SEAPILOT_SUSPENDED" was executed prior to disabling SeaPilot.
# The "unset SQ_EVLOG_TMP" and "unset SQ_EVLOG_STDERR" commands serve the
# same purpose as "unset SQ_EVLOG_NONE".
#
# Change the SQ_EVLOG_NONE export to SQ_EVLOG_STDERR to have events go to
# STDERR. Alternatively, change the SQ_EVLOG_NONE export to SQ_EVLOG_TMP
# to use the temporary EVLOG logging mechanism.
unset SQ_EVLOG_NONE
unset SQ_EVLOG_TMP
unset SQ_EVLOG_STDERR
export SQ_EVLOG_NONE=1


function ckillall {
  # echo "In the ckillall function"
  export SQ_PS1=$PS1
  $MY_SQROOT/sql/scripts/ckillall
  unset SQ_PS1
}
export -f ckillall

source tools/sqtools.sh

#######################
# BUILD Tools/Libraries
######################

# Standard tools expected to be installed and found in PATH
# Tool availability are checked during the build (make) step
export ANT=ant
export AR=ar
export FLEX=flex
export CXX=g++

# Maven may not yet be in the path. Check and report an error
export MAVEN=mvn
if [[ -z "$(which $MAVEN 2> /dev/null)" ]]; then
  echo " *** ERROR unable to find mvn (MAVEN) in path"
  echo " *** please include it in your path before attempting your build"
fi

# Non-standard or newer version tools
if [ -z $BISON ]; then
  export BISON="${TOOLSDIR}/bison_3_linux/bin/bison"     # Need 1.3 version or later
fi
if [ -z $LLVM ]; then
  export LLVM="${TOOLSDIR}/dest-llvm-3.2"
fi
if [ -z $UDIS86 ]; then
  export UDIS86="${TOOLSDIR}/udis86-1.7.2"
fi
if [ -z $ICU ]; then
  export ICU="${TOOLSDIR}/icu4.4"
fi

#######################
# Developer Local over-rides  (see sqf/LocalSettingsTemplate.sh)
# Cannot rely on this, the connectivity build overwrites the .trafodion file
######################
if [[ -r ~/.trafodion ]]; then
  [[ $SQ_VERBOSE == 1 ]] && echo "Sourcing local settings file ~/.trafodion"
  source ~/.trafodion
fi

# PROTOBUFS may include local over-rides
export PROTOBUFS_LIB=$PROTOBUFS/lib
export PROTOBUFS_INC=$PROTOBUFS/include

######################
# Library Path may include local over-rides
# Put Hadoop native dir before Trafodion, so that an available libhdfs will
# be picked up there, otherwise use the libhdfs distributed with Trafodion.
export LD_LIBRARY_PATH=$CC_LIB_RUNTIME:$MPI_ROOT/lib/$MPILIB:$HADOOP_LIB_DIR:$MY_SQROOT/export/lib"$SQ_MBTYPE":$LOC_JVMLIBS:$LOG4CXX_LIB_DIR:.

######################
# classpath calculation may include local over-rides

# check for previous invocation of this script in this shell
PREV_SQ_CLASSPATH=$SQ_CLASSPATH
SQ_CLASSPATH=

# set up SQ_CLASSPATH for use with Base clients and libhdfs
# From the HBase manual: Minimally, a client of HBase needs the hbase, hadoop,
# log4j, commons-logging, commons-lang, and ZooKeeper jars in its CLASSPATH connecting to a cluster
# Hortonworks seems to have a hadoop-client.jar

# expand jar files in list of directories
for d in $HADOOP_JAR_DIRS; do
  HADOOP_JAR_FILES="$HADOOP_JAR_FILES $d/*.jar"
done

for d in $HIVE_JAR_DIRS; do
  HIVE_JAR_FILES="$HIVE_JAR_FILES $d/*.jar"
done

# assemble all of them into a classpath
for j in $HBASE_JAR_FILES $HADOOP_JAR_FILES $HIVE_JAR_FILES; do
  if [[ -f $j ]]; then

    # eliminate jars with unwanted suffixes
    SUPPRESS_FILE=0
    for s in $SUFFIXES_TO_SUPPRESS; do
      if [[ ${j%${s}} != $j ]]; then
        SUPPRESS_FILE=1
      fi
    done
    # also eliminate ant jar that may be
    # incompatible with system ant command
    [[ $j =~ /ant- ]] && SUPPRESS_FILE=1

    # finally, add the jar to the classpath
    if [[ $SUPPRESS_FILE -eq 0 ]]; then
      SQ_CLASSPATH=$SQ_CLASSPATH:$j
    fi
  fi
done

# remove the leading colon from the classpath
SQ_CLASSPATH=${SQ_CLASSPATH#:}

# add Hadoop and HBase config dirs to classpath, if they exist
if [[ -n "$HADOOP_CNF_DIR" ]]; then SQ_CLASSPATH="$SQ_CLASSPATH:$HADOOP_CNF_DIR"; fi
if [[ -n "$HBASE_CNF_DIR"  ]]; then SQ_CLASSPATH="$SQ_CLASSPATH:$HBASE_CNF_DIR";  fi
if [[ -n "$HIVE_CNF_DIR"   ]]; then SQ_CLASSPATH="$SQ_CLASSPATH:$HIVE_CNF_DIR";   fi
if [[ -n "$SQ_CLASSPATH"   ]]; then SQ_CLASSPATH="$SQ_CLASSPATH:";   fi
SQ_CLASSPATH=${SQ_CLASSPATH}${HBASE_TRXDIR}:\
${HBASE_TRXDIR}/${HBASE_TRX_JAR}:\
$MY_SQROOT/export/lib/trafodion-sql-${TRAFODION_VER}.jar:\
$MY_SQROOT/export/lib/trafodion-utility-${TRAFODION_VER}.jar:\
$MY_SQROOT/export/lib/trafodion-dtm-${TRAFODION_VER}.jar:\
$MY_SQROOT/export/lib/jdbcT4.jar:\
$MY_SQROOT/export/lib/jdbcT2.jar

# Check whether the current shell environment changed from a previous execution of this
# script.
SQ_CLASSPATH=$(remove_duplicates_in_path "$SQ_CLASSPATH")
if [[  ( -n "$PREV_SQ_CLASSPATH" )
    && ( "$PREV_SQ_CLASSPATH" != "$SQ_CLASSPATH" ) ]]; then
  cat <<EOF
The environment changed from a previous execution of this script.
This is not supported. To change environments, do the following:
  sqstop
  <make any changes, e.g. update Hadoop, HBase, MySQL>
  start a new shell and source in sqenv.sh
  rm \$MY_SQROOT/etc/ms.env
  sqgen
  start a new shell and source in sqenv.sh
  sqstart
EOF
# export an env var that can be checked by subsequent scripts
# (exit is not a good option here since we source in this file,
# so exit would kill the entire shell)
export CHANGED_SQ_ENV_RESTART_SHELL=1
fi

# take anything from the existing classpath, but not the part that was
# added by previous invocations of this script in this shell (assuming it
# produced the same classpath).

# Note: There will be unwanted classpath entries if you do the
# following: a) source in this file;
#            b) prepend to the classpath;
#            c) source this file in again

USER_CLASSPATH=${CLASSPATH##${SQ_CLASSPATH}}
USER_CLASSPATH=$(remove_duplicates_in_path ${USER_CLASSPATH#:})

if [[ -n "$USER_CLASSPATH" ]]; then
  # new info, followed by the original classpath
  export CLASSPATH="${SQ_CLASSPATH}:${USER_CLASSPATH}"
else
  export CLASSPATH="${SQ_CLASSPATH}"
fi
export CLASSPATH=$(remove_duplicates_in_path "${CLASSPATH}:")

PATH=$(remove_duplicates_in_path "$PATH")
MANPATH=$(remove_duplicates_in_path "$MANPATH")

####################
# Check/Report on key variables
###################

# Check variables that should refer to real directories
VARLIST="MY_SQROOT $VARLIST JAVA_HOME PERL5LIB MPI_TMPDIR"

if [[ "$SQ_VERBOSE" == "1" ]]; then
  echo "Checking variables reference existing directories ..."
fi
for AVAR in $VARLIST; do
  AVALUE="$(eval "echo \$$AVAR")"
  if [[ "$SQ_VERBOSE" == "1" ]]; then
    printf '%s =\t%s\n' $AVAR $AVALUE
  fi
  if [[ ! -d $AVALUE ]]; then
    echo
    echo "*** WARNING: $AVAR directory not found: $AVALUE"
    echo
  fi
done

if [[ "$SQ_VERBOSE" == "1" ]]; then
  printf "\nPATH="
  echo $PATH | sed -e's/:/ /g' | fmt -w2 | xargs printf '\t%s\n'
  echo
  printf "\nLD_LIBRARY_PATH="
  echo $LD_LIBRARY_PATH | sed -e's/:/ /g' | fmt -w2 | xargs printf '\t%s\n'
  echo
  printf "\nCLASSPATH=\n"
  echo $CLASSPATH | sed -e's/:/ /g' | fmt -w2 | xargs printf '\t%s\n'
  echo
fi
