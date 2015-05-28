#!/bin/sh

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
INSTALL_ROOT=$MY_SQROOT/trafci
INSTALL_LIBDIR=$INSTALL_ROOT/lib
INSTALL_SAMPLESDIR=$INSTALL_ROOT/samples
INSTALL_BINDIR=$INSTALL_ROOT/bin
EXPORTLIBDIR=$MY_SQROOT/export/lib
CP=/bin/cp
TEMPSRC=../temp

sed 's/\#\#TRAFCI_CLASSPATH\#\#/\${MY_SQROOT}\/export\/lib\/jdbcT4.jar:\${MY_SQROOT}\/trafci\/lib\/trafci.jar/g' trafci.sh > lnxplatform.sh
mv -f lnxplatform.sh $TEMPSRC/trafci.sh
	
echo "Creating trafci install folder in $MY_SQROOT "
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
