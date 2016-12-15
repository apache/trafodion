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

#
# Install Trafodion Command Interface Client
#
TRAFCIJARPREFIX=trafci
TARGETDIR=classes
LIBDIR=../lib
JAVAC=$JAVA_HOME/bin/javac
JAVAE=$JAVA_HOME/bin/java
JAR=$JAVA_HOME/bin/jar
PERL=perl
PYTHON=python
INSTALL_ROOT=$TRAF_HOME/trafci
INSTALL_LIBDIR=$INSTALL_ROOT/lib
INSTALL_SAMPLESDIR=$INSTALL_ROOT/samples
INSTALL_BINDIR=$INSTALL_ROOT/bin
EXPORTLIBDIR=$TRAF_HOME/export/lib
CP=/bin/cp
TEMPSRC=../temp

sed 's/\#\#TRAFCI_CLASSPATH\#\#/\${TRAF_HOME}\/export\/lib\/jdbcT4-${TRAFODION_VER}.jar:\${TRAF_HOME}\/trafci\/lib\/trafci.jar/g' trafci.sh > lnxplatform.sh
mv -f lnxplatform.sh $TEMPSRC/trafci.sh
	
echo "Creating trafci install folder in $TRAF_HOME "
if [ ! -d "$INSTALL_LIBDIR" ]; then mkdir -p -m 775 $INSTALL_LIBDIR; fi

echo "Copying library files to trafci install folder "
$CP $LIBDIR/$TRAFCIJARPREFIX.jar $INSTALL_LIBDIR

if [ ! -d "$INSTALL_LIBDIR/$PERL" ]; then mkdir -p -m 775 $INSTALL_LIBDIR/$PERL; fi
$CP $LIBDIR/$PERL/Session.pm $INSTALL_LIBDIR/$PERL

if [ ! -d "$INSTALL_LIBDIR/$PYTHON" ]; then mkdir -p -m 775 $INSTALL_LIBDIR/$PYTHON; fi
$CP $LIBDIR/$PYTHON/Session.py $INSTALL_LIBDIR/$PYTHON

if [ ! -d "$INSTALL_BINDIR" ]; then mkdir -p -m 775 $INSTALL_BINDIR; fi

echo "Copying binary files to trafci install folder"
$CP $TEMPSRC/trafci.sh trafci.cmd trafci.pl trafci.py trafci-perl.pl trafci-python.py ciencr.sh trafci $INSTALL_BINDIR

if [ ! -d "$INSTALL_SAMPLESDIR" ]; then mkdir -p -m 775 $INSTALL_SAMPLESDIR; fi

echo "Copying samples to trafci install folder"
$CP ../samples/* $INSTALL_SAMPLESDIR

chmod 775 $INSTALL_LIBDIR/*
chmod 775 $INSTALL_SAMPLESDIR/*
chmod 775 $INSTALL_BINDIR/*
