# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

FROM centos:centos6.6
MAINTAINER Trafodion Community <dev@trafodion.apache.org>

LABEL Vendor="Apache Trafodion"
LABEL version=stable

# download and install environment dependencies
RUN	yum install -y epel-release alsa-lib-devel ant ant-nodeps boost-devel cmake \
	     device-mapper-multipath dhcp flex gcc-c++ gd git glibc-devel \
       glibc-devel.i686 graphviz-perl gzip java-1.7.0-openjdk-devel \
  		 apr-devel apr-util-devel \
       libX11-devel libXau-devel libaio-devel \
       libcurl-devel libibcm.i686 libibumad-devel libibumad-devel.i686 \
       libiodbc libiodbc-devel librdmacm-devel librdmacm-devel.i686 \
       libxml2-devel lua-devel lzo-minilzo \
       net-snmp-devel net-snmp-perl openldap-clients openldap-devel \
       openldap-devel.i686 openmotif openssl-devel openssl-devel.i686 \
       openssl-static perl-Config-IniFiles perl-Config-Tiny \
       perl-DBD-SQLite perl-Expect perl-IO-Tty perl-Math-Calc-Units \
       perl-Params-Validate perl-Parse-RecDescent perl-TermReadKey \
       perl-Time-HiRes protobuf-compiler protobuf-devel \
       readline-devel saslwrapper sqlite-devel \
       unixODBC unixODBC-devel uuid-perl wget xerces-c-devel xinetd \
  && yum -y erase pdsh \
	&& yum clean all

# set environment
ENV	JAVA_HOME /usr/lib/jvm/java-1.7.0-openjdk.x86_64
ENV PATH $PATH:$JAVA_HOME/bin
