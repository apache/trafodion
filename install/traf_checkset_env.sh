#!/bin/bash
#
# make sure the environment can build the code of trafodion or not.
# must configure the yum repo right before execute this script.
# run this script with root permission.

#default path
MY_JVM_PATH=${MY_JVM_PATH-"/usr/lib/jvm"}
MY_JAVA_VER=${MY_JAVA_VER-"java-1.7.0-openjdk"}
MY_SUDO=${MY_SUDO-"sudo"}
MY_YUM=${MY_YUM-"${MY_SUDO} yum -y"}

MY_LOCAL_SW_DIST=${MY_LOCAL_SW_DIST-${HOME}/local_software_tools}
MY_INSTALL_SW_DIST=${MY_INSTALL_SW_DIST-${HOME}/installed_software_tools}
MY_DOWNLOAD_SW_DIST=${MY_DOWNLOAD_SW_DIST-${HOME}/download_software_tools}

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
    echo "WARN: local sofrware tools aren't exist, should be downloading all
    software tools from internet, it's very slowlly! While you can
    prepare local software, and set environment MY_LOCAL_SW_DIST to
    change default software path, and run this script again. The 
    default local software directory is [${MY_LOCAL_SW_DIST}]. You
    can set environment MY_LOCAL_SW_DIST to change it."
    echo "Do you want continue? Enter y/n (default:n):"
    read YN
    case ${YN} in
	Y|y|Yes|YES)
	    ;;
	*)
	    echo "Download following build tools from internet:"
	    for i in `seq ${#local_sws[@]}`
	    do
		printf "%2d.%15s: %s\n" ${i} ${local_sws[${i}-1]} ${http_sws[${i}-1]}
	    done
	    exit 1
	    ;;
    esac
else
    # check the local software's source exist or not
    for local_sw in ${local_sws[@]}
    do
	local_file=`ls ${MY_LOCAL_SW_DIST} | grep ${local_sw}`
	if [ "x${local_file}" = "x" ]; then
            echo "WARN: [${local_sw}] source file not exist in directory [${MY_LOCAL_SW_DIST}]"
	fi
    done
fi

# check the permission.
(${MY_SUDO} touch /etc/trafpermise) >>${LOGFILE}2>&1
if [ "x$?" != "x0" ]; then
    echo "ERROR: you must run this script with sudo without password permission."
    exit 1
fi

# check the based command
basecmds=(yum lsb_release awk cut uname)
for basecmd in ${basecmds[@]}  
do
    basecmdpath=`which ${basecmd}`
    if [ "x$?" != "x0" ]; then
	case ${basecmd} in
	    yum)
		echo "ERROR: yum must install first by yourself."
		exit 1
		;;
	    lsb_release)
		(${MY_YUM} install redhat-lsb) >>${LOGFILE}2>&1
		if [ "x$?" = "x0" ]; then
		    echo "ERROR: yum repo server has a error when run command [${MY_YUM} install redhat-lsb]."
		    exit 1
		fi
		;;
	    *)
		echo "ERROR: command [${basecmd}] not exist, make sure you have installed,and the path command added to path rightly."
		exit 1
	esac
    fi
    echo "INFO: command ${basecmd} exist"
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
	echo "ERROR: os distributor is [${osdistributor}], but only support RedHat or CentOS now."
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
	echo "ERROR: os version is [$osver], but only support 6.x now."
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
echo "INFO: remove pdsh and qt-dev trafodion scripts get confused."
(${MY_YUM} erase pdsh) >>${LOGFILE}2>&1
(${MY_YUM} erase qt-dev) >>${LOGFILE}2>&1

# check and set the java
echo "INFO: check and set java environment"
javadir=`\ls -L ${MY_JVM_PATH} | grep "${MY_JAVA_VER}-"`
javapath="${MY_JVM_PATH}/${javadir}"
dirs="ASSEMBLY_EXCEPTION  bin  include  jre  jre-abrt  lib  LICENSE" \
				     " tapset  THIRD_PARTY_README"
if [ ! -d ${javapath} ]; then
     echo "ERROR: java dir [${javapath}] isn't right"
     exit 1
fi

javahome=`grep JAVA_HOME ~/.bashrc | wc -l`
if [ "x${javahome}" = "x0" ]; then
    echo -en "export JAVA_HOME=${javapath}\n" >> $HOME/.bashrc
    echo -en "export PATH=\$PATH:\$JAVA_HOME/bin\n" >> $HOME/.bashrc
fi

# check and set locale
local_locale=`locale | grep -v "xen_US.UTF-8"`
if [ "${local_locale}" != "x" ]; then
    echo "WARN: locale is not right, this script modify the locale to "\
         "\"en_US.UTF-8\" from "
    echo "[${local_locale}]"
    lang_set=`grep LANG ~/.bashrc | wc -l`
    if [ "x${lang_set}" = "x0" ]; then
	echo -en "export LANG=\"en_US.UTF-8\"\n" >> $HOME/.bashrc
    fi
    lc_all_set=`grep LC_ALL ~/.bashrc | wc -l`
    if [ "x${lc_all_set}" = "x0" ]; then
	echo -en "export LC_ALL=\"en_US.UTF-8\"\n" >> $HOME/.bashrc
    fi
fi

# check error
if (("${#faillibs[@]}" > "0")); then
    echo "ERROR: libs [${faillibs[@]}] aren't found in your repo server."
    exit 1;
fi

source $HOME/.bashrc
echo $'\n'"Build environment is created SUCCESS!"

echo $'\n'"Install the required Build tools start!"
export MY_LOCAL_SW_DIST
if [ ! -e ${MY_DOWNLOAD_SW_DIST} ]; then
    echo "INFO: mkdir [${MY_DOWNLOAD_SW_DIST}]"
    mkdir ${MY_DOWNLOAD_SW_DIST}
fi
echo "INFO: install tools with command:"
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
    echo -en "export TOOLSDIR=${MY_INSTALL_SW_DIST}\n" >> $HOME/.bashrc
    echo -en "export PATH=\$PATH:\$TOOLSDIR/apache-maven-3.3.3/bin\n" >> $HOME/.bashrc
fi

