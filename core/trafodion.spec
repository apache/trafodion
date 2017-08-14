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

Summary:	Trafodion: Transactional SQL-on-Hadoop DBMS
Name:		%{name}
Version:	%{version}
Release:	%{release}
AutoReqProv:	no
License:	Apache version 2.0 -  http://www.apache.org/licenses/LICENSE-2.0
Group:		Applications/Databases
Source0:        %{name}-%{version}.tar.gz
BuildArch:	%{_arch}
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}
URL:            http://trafodion.incubator.apache.org


%define _binary_filedigest_algorithm 1
%define _source_filedigest_algorithm 1
%define _binary_payload w9.gzdio
%define _source_payload w9.gzdio

Requires: audit-libs
Requires: apr
Requires: apr-util
Requires: coreutils
Requires: cracklib
Requires: expect
Requires: gawk
Requires: glib2
Requires: glibc
Requires: gmp
Requires: gnuplot
Requires: groff
Requires: gzip
Requires: keyutils-libs
Requires: libcom_err
Requires: libgcc
Requires: libxml2
Requires: lsof
Requires: lzo
Requires: ncurses
Requires: openssl
Requires: pam
Requires: pcre
Requires: pdsh
Requires: perl
Requires: perl-DBD-SQLite
Requires: perl-DBI
Requires: perl-Module-Pluggable
Requires: perl-Params-Validate
Requires: perl-Pod-Escapes
Requires: perl-Pod-Simple
Requires: perl-Time-HiRes
Requires: perl-version
Requires: protobuf
Requires: python
Requires: readline
Requires: sqlite
Requires: snappy
Requires: xerces-c
Requires: zlib

Prefix: /home/trafodion
Prefix: /etc

%description
Apache Trafodion for Operational Big Data combining the power of transactional SQL and Apache HBase with the elastic scalability of Hadoop.


%prep
%setup -b 0 -n %{name}-%{version} -c


%pre -n %{name}
if ! getent group trafodion > /dev/null
then
  /usr/sbin/groupadd trafodion > /dev/null 2>&1
fi
if ! getent passwd trafodion > /dev/null
then
  /usr/sbin/useradd --shell /bin/bash -m trafodion -g trafodion --home /home/trafodion > /dev/null 2>&1
fi
if getent group hbase > /dev/null
then
  /usr/sbin/usermod -a -G hbase trafodion > /dev/null 2>&1
fi
if getent group hive > /dev/null
then
  /usr/sbin/usermod -a -G hive trafodion > /dev/null 2>&1
fi
if getent group hadoop > /dev/null
then
  /usr/sbin/usermod -a -G hadoop trafodion > /dev/null 2>&1
fi
chmod go+rx /home/trafodion


%build
# don't build debug info package
%define debug_package:

%install
cd %{_builddir}
mv -f %{name}-%{version}/sysinstall/* %{buildroot}/
rmdir %{name}-%{version}/sysinstall
mkdir -p %{buildroot}/home/trafodion/%{name}-%{version}
mv -f %{name}-%{version}/* %{buildroot}/home/trafodion/%{name}-%{version}/

%post
mkdir -p /etc/trafodion/
echo "TRAF_HOME=/home/trafodion/%{name}-%{version}" > /etc/trafodion/trafodion_config
echo "source /etc/trafodion/conf/trafodion-env.sh" >> /etc/trafodion/trafodion_config
echo "source /etc/trafodion/conf/traf-cluster-env.sh" >> /etc/trafodion/trafodion_config


%clean
/bin/rm -rf %{buildroot}

%files
/etc/init.d/trafodion
/etc/security/limits.d/trafodion.conf
/etc/sudoers.d/trafodion
%defattr(-,trafodion,trafodion)
/home/trafodion/.bashrc
/home/trafodion/%{name}-%{version}

%changelog
* Mon Oct 18 2016 Steve Varnau
- ver 2.1
