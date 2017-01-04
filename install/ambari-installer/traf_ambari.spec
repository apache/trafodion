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

Summary:	Ambari management pack for Trafodion
Name:		traf_ambari
Version:	%{version}
Release:	%{release}
AutoReqProv:	no
License:	TBD
Group:		Applications/Databases
Source0:        ambari_rpm.tar.gz
BuildArch:	noarch
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}
URL:            http://trafodion.incubator.apache.org

Requires: ambari-server

Prefix: /opt/trafodion

%description
This extension enables management of Trafodion via Ambari.


%prep
%setup -b 0 -n %{name} -c

%build

%install
cd %{_builddir}
mkdir -p %{buildroot}/opt/trafodion
cp -rf %{name}/* %{buildroot}/opt/trafodion

%post
$RPM_INSTALL_PREFIX/mpack-install/am_install.sh "$RPM_INSTALL_PREFIX"

%clean
/bin/rm -rf %{buildroot}

%files
/opt/trafodion/traf-mpack/
/opt/trafodion/mpack-install/

%changelog
* Fri Oct 21 2016 Steve Varnau
- ver 1.0
