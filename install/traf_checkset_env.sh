#!/bin/bash
#
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
# make sure the environment can build the code of trafodion or not.
# must configure the yum repo right before execute this script.
# run this script with normal user, while must has sudo permission.

function usage {

cat <<EOF

Syntax: $0 [ -y ] [ -h | -help | --help | -v | -version | --version ]
Usage:

    1. You can create building environment with this script 
    2. You must have sudo privileges before running this script.
    3. This script will change the following environment:
        a) Checking and installing basic commands(like as yum lsb_release awk cut uname).
        b) Checking the os version.
        c) Installing all libraries which is depended by Trafodion.
        d) Removing pdsh and qt-dev if had installed.
        e) Checking and setting the java environment.
        f) Checking and setting locale(modify the LANG and LC_LANG to en_US.UTF-8)
        g) Changing $HOME/.bashrc(Add JAVA_HOME, LANG, LC_LANG and TOOLSDIR, modify PATH)
        h) Calling the script traf_tools_setup.sh to install the depended tools.
    4. Getting more information from the link:
        https://cwiki.apache.org/confluence/display/TRAFODION/Create+Build+Environment
    5. It's best to download all the necessary softwares and set the environment variable "MY_SW_HOME" before running this script.
    6. You can set optional environment variables as following:
        a) MY_JVM_PATH: set the specific JVM path
        b) MY_JAVA_VER: set the specific JDK version name
        c) MY_SUDO: set the specific sudo command
        d) MY_YUM: set the command for installing packages 
        e) MY_LOCAL_SW_DIST:
    7. This script maybe create the directory "${MY_DOWNLOAD_SW_DIST}" if not exits.

EOF
}

#default path
MY_JVM_PATH=${MY_JVM_PATH-"/usr/lib/jvm"}
MY_JAVA_VER=${MY_JAVA_VER-"java-1.7.0-openjdk"}
MY_SUDO=${MY_SUDO-"sudo"}
MY_YUM=${MY_YUM-"${MY_SUDO} yum -y"}

# for setup tools, 
MY_SW_HOME=${MY_SW_HOME-"/add_your_local_shared_folder_here"}
MY_LOCAL_SW_DIST=${MY_LOCAL_SW_DIST-${MY_SW_HOME}/local_software_tools}
MY_INSTALL_SW_DIST=${MY_INSTALL_SW_DIST-${MY_SW_HOME}/installed_software_tools}
MY_DOWNLOAD_SW_DIST=${MY_DOWNLOAD_SW_DIST-${MY_SW_HOME}/download_software_tools}

MY_IMPLICIT_Y="n"
while [ $# -gt 0 ];
do
  case $1 in
      -y) MY_IMPLICIT_Y="y"
	  ;;
      -h|-help|--help)
	  usage
	  exit 0
	  ;;
      -v|-version|--version)
	  echo "version 0.0.0.1"
	  exit 0
	  ;;
      *)  echo "ERROR: Unexpected argument $1"
	  usage
	  exit 1
	  ;;
  esac
  shift
done


CHECKLOG=${LOGFILE-$(pwd)/$0.log}
local_sws=(udis llvm mpich bison icu zookeeper thrift apache-maven protobuf apache-log4cxx hadoop)
http_sws=(
http://sourceforge.net/projects/udis86/files/udis86/1.7/udis86-1.7.2.tar.gz
http://llvm.org/releases/3.2/llvm-3.2.src.tar.gz
http://www.mpich.org/static/downloads/3.0.4/mpich-3.0.4.tar.gz
http://ftp.gnu.org/gnu/bison/bison-3.0.tar.gz
http://download.icu-project.org/files/icu4c/4.4/icu4c-4_4-src.tgz
https://archive.apache.org/dist/zookeeper/zookeeper-3.4.5/zookeeper-3.4.5.tar.gz
http://archive.apache.org/dist/thrift/0.9.0/thrift-0.9.0.tar.gz
http://archive.apache.org/dist/maven/maven-3/3.3.3/binaries/apache-maven-3.3.3-bin.tar.gz
https://github.com/google/protobuf/releases/download/v2.5.0/protobuf-2.5.0.tar.gz
https://dist.apache.org/repos/dist/release/logging/log4cxx/0.10.0/apache-log4cxx-0.10.0.tar.gz
http://archive.apache.org/dist/hadoop/common/hadoop-2.6.0/hadoop-2.6.0.tar.gz
)

# check the local software directory
if [ ! -d ${MY_LOCAL_SW_DIST} ]; then
    echo "WARNING: Local software tools aren't present. Will download all tools from the internet. This will be very slow. If you do have the local software tools present, set the environment variable MY_LOCAL_SW_DIST to point to them and run this script again. The default local software directory is [${MY_LOCAL_SW_DIST}]. Do you want to continue? Enter y/n (default: n):"
    if [ "x$MY_IMPLICIT_Y" = "xn" ]; then
	read YN
	case ${YN} in
	    Y|y|Yes|YES)
		;;
	    *)
		echo "Downloading the following build tools from the internet:"
		for i in `seq ${#local_sws[@]}`
		do
		    printf "%2d.%15s: %s\n" ${i} ${local_sws[${i}-1]} ${http_sws[${i}-1]}
		done
		exit 1
		;;
	esac
    fi
else
    # check the local software's source exist or not
    for local_sw in ${local_sws[@]}
    do
	local_file=`ls ${MY_LOCAL_SW_DIST} | grep ${local_sw}`
	if [ "x${local_file}" = "x" ]; then
            echo "WARNING: [${local_sw}] source file does not exist in directory [${MY_LOCAL_SW_DIST}]"
	fi
    done
fi

# check the permission.
(${MY_SUDO} touch /etc/trafpermise) >>${LOGFILE}2>&1
if [ "x$?" != "x0" ]; then
    echo "ERROR: you must run this script with sudo without password permission."
    exit 1
fi

# check some basic commands
basecmds=(yum lsb_release awk cut uname)
for basecmd in ${basecmds[@]}  
do
    basecmdpath=`which ${basecmd}`
    if [ "x$?" != "x0" ]; then
	case ${basecmd} in
	    yum)
		echo "ERROR: You must first install yum."
		exit 1
		;;
	    lsb_release)
		(${MY_YUM} install redhat-lsb) >>${LOGFILE}2>&1
		if [ "x$?" = "x0" ]; then
		    echo "ERROR: yum repo server has an error when running command [${MY_YUM} install redhat-lsb]."
		    exit 1
		fi
		;;
	    *)
		echo "ERROR: command [${basecmd}] does not exist. Make sure you have installed it, and have added it to the command path."
		exit 1
	esac
    fi
    echo "INFO: command ${basecmd} exists"
done

osname=`uname -o`
cpuarch=`uname -m`
echo "INFO: os is [${osname}], cpu architecture is [${cpuarch}]"

osdistributor=`lsb_release -i | cut -d : -f 2-`
case ${osdistributor} in
    *RedHat*|*CentOS*)
	echo "INFO: os distributor id is [${osdistributor}]"
	;;
    *)
	echo "ERROR: The OS distribution is [${osdistributor}], but Trafodion only supports RedHat and CentOS presently."
	exit 1
esac

osver=`lsb_release -r | cut -d : -f 2-`
osvermajor=`echo $osver | cut -d . -f 1-1`
osverminor=`echo $osver | cut -d . -f 2-2`
case ${osvermajor} in
    6|7)
	echo "INFO: os version is [$osver]"
	;;
    *)
	echo "ERROR: OS version is [$osver]. Only 6.x versions are presently supported."
	exit 1
esac

# install all libs
dependlibs=(
	    epel-release alsa-lib-devel ant ant-nodeps apr-devel	\
	    apr-util-devel boost-devel cmake device-mapper-multipath	\
	    dhcp doxygen flex gcc-c++ gd git glibc-devel		\
	    glibc-devel.i686 graphviz-perl gzip ${MY_JAVA_VER}-devel	\
	    libX11-devel libXau-devel libaio-devel libcurl-devel	\
	    libibcm.i686 libibumad-devel libibumad-devel.i686 libiodbc	\
	    libiodbc-devel librdmacm-devel librdmacm-devel.i686		\
	    libxml2-devel lua-devel lzo-minilzo net-snmp-devel		\
	    net-snmp-perl openldap-clients openldap-devel		\
	    openldap-devel.i686 openmotif openssl-devel			\
	    openssl-devel.i686 perl-Config-IniFiles perl-Config-Tiny	\
	    perl-DBD-SQLite perl-Expect perl-IO-Tty perl-Math-Calc-Units \
	    perl-Params-Validate perl-Parse-RecDescent perl-TermReadKey	\
	    perl-Time-HiRes protobuf-compiler protobuf-devel		\
	    readline-devel rpm-build saslwrapper sqlite-devel unixODBC	\
	    unixODBC-devel uuid-perl wget xerces-c-devel xinetd		\
	    ncurses-devel)
faillibs=()
echo "INFO: install dependent library start"
for dependlib in ${dependlibs[@]}
do
    (${MY_YUM} install ${dependlib}) >>${LOGFILE}2>&1
    if [ "x$?" != "x0" ]; then
	faillibs=("${faillibs[@]}" "${dependlib}")
	echo "ERROR: install dependent library [${dependlib}] fail"
    else
	echo "INFO: install dependent library [${dependlib}] success"
    fi
done
echo "INFO: install dependent library finish"

# remove pdsh and qt-dev
echo "INFO: remove pdsh and qt-dev commands to avoid problems with trafodion scripts"
(${MY_YUM} erase pdsh) >>${LOGFILE}2>&1
(${MY_YUM} erase qt-dev) >>${LOGFILE}2>&1

# check and set the java
echo "INFO: check and set java environment"
javadir=`\ls -L ${MY_JVM_PATH} | grep "${MY_JAVA_VER}-" | head -n 1`
javapath="${MY_JVM_PATH}/${javadir}"

if [ ! -d ${javapath} ]; then
     echo "ERROR: java dir [${javapath}] isn't right"
     exit 1
fi

source $HOME/.bashrc
javahome=`grep JAVA_HOME ~/.bashrc | wc -l`
if [ "x${javahome}" = "x0" ]; then
    echo -en "\n# Added by traf_checkset_env.sh of trafodion\n" >> $HOME/.bashrc
    echo -en "export JAVA_HOME=${javapath}\n" >> $HOME/.bashrc
    echo -en "export PATH=\$PATH:\$JAVA_HOME/bin\n" >> $HOME/.bashrc
    export JAVA_HOME=${javapath}
    export PATH=$PATH:$JAVA_HOME/bin
else
    java_version=`${JAVA_HOME}/bin/java -version 2>&1 | awk 'NR==1{ gsub(/"/,""); print $3}'`
    case ${java_version} in
	1.7.*)
	    echo "INFO: java version is [$java_version]"
	    ;;
	*)
	    echo "ERROR: java version is [$java_version]. Only 1.7.x versions are presently supported."
	    exit 1
    esac
fi

# check and set locale
local_locale=`locale | grep -v "en_US.UTF-8" | head -n 1`
if [ "x${local_locale}" != "x" ]; then
    lang_set=`grep LANG ~/.bashrc | awk -F= '{ print $2 }'`
    if [ "x${lang_set}" != "xen_US.UTF-8" ]; then
	echo -en "\n# Added by traf_checkset_env.sh of trafodion\n" >> $HOME/.bashrc
	echo -en "export LANG=\"en_US.UTF-8\"\n" >> $HOME/.bashrc
	export LANG="en_US.UTF-8"
    fi
    lc_all_set=`grep LC_TIME ~/.bashrc | awk -F= '{ print $2 }'`
    if [ "x${lc_all_set}" != "xen_US.UTF-8" ]; then
	echo -en "\n# Added by traf_checkset_env.sh of trafodion\n" >> $HOME/.bashrc
	echo -en "export LC_ALL=\"en_US.UTF-8\"\n" >> $HOME/.bashrc
	export LC_ALL="en_US.UTF-8"
    fi
fi

# check error
if (("${#faillibs[@]}" > "0")); then
    echo "ERROR: libs [${faillibs[@]}] aren't found in your repo server."
    exit 1;
fi

echo $'\n'"Build environment is created SUCCESS!"

echo $'\n'"Install the required Build tools start!"
export MY_LOCAL_SW_DIST
if [ ! -e ${MY_DOWNLOAD_SW_DIST} ]; then
    echo "INFO: mkdir [${MY_DOWNLOAD_SW_DIST}]"
    mkdir ${MY_DOWNLOAD_SW_DIST}
fi
echo "INFO: install tools with command:"\
"    [./traf_tools_setup.sh -d ${MY_DOWNLOAD_SW_DIST} -i ${MY_INSTALL_SW_DIST}]]"
./traf_tools_setup.sh -d ${MY_DOWNLOAD_SW_DIST} -i ${MY_INSTALL_SW_DIST}
if [ "x$?" = "x0" ]; then
    echo $'\n'"Install the required Build tools SUCCESS!"
else
    echo "ERROR: Install the required Build tools FAIL.
    the execute command:
     [./traf_tools_setup.sh -d ${MY_DOWNLOAD_SW_DIST} -i ${MY_INSTALL_SW_DIST}]"
fi

tooldirexist=`grep TOOLSDIR ~/.bashrc | wc -l`
if [ "x${tooldirexist}" = "x0" ]; then
    echo -en "\n# Added by traf_checkset_env.sh of trafodion\n" >> $HOME/.bashrc
    echo -en "export TOOLSDIR=${MY_INSTALL_SW_DIST}\n" >> $HOME/.bashrc
    echo -en "export PATH=\$PATH:\$TOOLSDIR/apache-maven-3.3.3/bin\n" >> $HOME/.bashrc
fi

