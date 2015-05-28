#!/bin/ksh 
# this is the master script used mainly to easily
# capture input/output

function init {

DATE=`date +%y%m%d%H%M%S`
STARTDIR=`pwd`
LOGFILE=$STARTDIR/logfile.$DATE

. ./buildinfo

}

function get_source {

if [ ! -d "$SVN_DIR" ] 
then
  mkdir -p "$SVN_DIR"
fi

if [ ! -d "$SVN_DIR" ] 
then
  echo "Error: $SVN_DIR does not exist" 
  exit 1
fi

cd $SVN_DIR

if [ -z "$SVN_REVISION" ]
then
  SVN_REVISION=`svn info $SVN_PATH | awk '/Revision:/ {print $2}'`
fi

if [ -z "$SVN_REVISION" ]
then
  echo "Error: $SVN_REVISION is not set"
  exit 1
fi

rm -Rf build_$SVN_REVISION

# sparse tree checkout here
svn -r $SVN_REVISION checkout --depth=empty $SVN_PATH build_$SVN_REVISION 
cd build_$SVN_REVISION

svn --depth=empty update conn                     # create empty conn
svn --set-depth=infinity update conn/odbc         # get everything under conn/odbc
svn --set-depth=infinity update conn/security_dll # get everything under conn/security_dll
svn --depth=empty update sql                      # create empty sql
svn --set-depth=infinity update sql/cli           # get everything under sql/cli
svn --set-depth=infinity update sql/common        # get everything under sql/common

# setup $BUILDDIR
cd conn/odbc
export BUILDDIR=`pwd`

cd $STARTDIR

}

function update_version {

for proj in drvr35 drvr35adm  drvr35msg drvr35trace TCPIPV4 TCPIPV6 TranslationDll Drvr35Res
do

    case "$proj" in
    drvr35msg) 
        projpath=$proj
        resource=DrvMsg35.rc 
        ;;
    TCPIPV4) 
        projpath=drvr35/$proj
        resource=TCPIPV4.RC
        ;;
    TCPIPV6) 
        projpath=drvr35/$proj
        resource=TCPIPV6.RC 
        ;;
    *) 
        projpath=$proj
        resource=$proj.rc 
        ;;
	esac
   
    perl $BUILDDIR/src/odbc/BuildOdbcDrvr/update_version.pl $BUILDDIR/src/odbc/odbcclient/$projpath/$resource $MAJOR_VER $MINOR_VER $SP_VER $SVN_REVISION
    if [ "$?" -ne 0 ]
    then 
      echo
      echo ============================================================================
      echo "BUILD FAILED: Can't update version in $resource."
      echo ============================================================================
      echo
      exit 1
    fi
              
done

cd $STARTDIR

}

init
get_source
update_version

# could add a command line switch to specify logfile at some point?
$BUILDDIR/src/odbc/BuildOdbcDrvr/build_odbc_driver.ksh 2>&1 | tee $LOGFILE
RETVAL=`cat $LOGFILE | awk '$0~/^ODBCDRVR BUILD COMPLETED!/ {print 0}'`
if [ "$RETVAL" == "0" ]; then
   cp -f $BUILDDIR/lib/release/HPODBC64.msi c:/share/
   cp -f $BUILDDIR/lib/release/HPODBC.msi c:/share/
   echo
   echo ============================================================================
   echo "BUILD WINDOWS DRIVER $SVN_REVISION COMPLETED SUCCESSFULLY"
   echo ============================================================================
   echo
else
   echo
   echo ============================================================================
   echo "BUILD WINDOWS DRIVER FAILED: See $LOGFILE for more information."
   echo ============================================================================
   echo   
fi
