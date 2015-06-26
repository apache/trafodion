#!/bin/ksh


function init {

PACKROOT=${BUILDDIR%/*}
PACKDRIVER="U:"
SRCDIR=$PACKDRIVER/odbc/src/odbc
LIBDIR=$PACKDRIVER/odbc/lib 
subst $PACKDRIVER $PACKROOT
export PATH=$PATH:$ISCMDBLD_PATH
if [ ! -d "$SRCDIR" ] 
then
  echo "Error: $SRCDIR does not exist" 
  exit 1
fi


}

function fini {
subst $PACKDRIVER /D
}

function package_win32_release {

echo
echo ===============================
echo     PACKAGE WIN32 RELEASE
echo ===============================
echo

INSTALLDIR="$SRCDIR/Install"
INSTALLFILENAME=HPODBC.msi
HPSIGNDIR=C:/Progra~1/Hewlett-Packard/HPCSS/HPCSS/HPSignClient1.4
OUTSIGNDIR=$LIBDIR/Release/signed
SAVEPATH=$PATH
PATH=$ISCMDBLD_PATH\;$PATH

if [ ! -d "$OUTSIGNDIR" ] 
then
  mkdir -p $OUTSIGNDIR
fi

attrib -r "$INSTALLDIR/*.*" /s

echo
echo "Building Media for $INSTALLFILENAME ..."
FILENAME="$INSTALLDIR/Win32 ODBC Installer MSI/Win32 ODBC MSI Installer.ism"
ISCmdBld.exe -p "$FILENAME"
if [ "$?" -ne 0 ] 
then 
  echo "Error Building Media for $INSTALLFILENAME"
  exit 1
fi

echo
echo "Moving $INSTALLFILENAME ..."
rm -f $LIBDIR/Release/$INSTALLFILENAME

mv "$INSTALLDIR//Win32 ODBC Installer MSI/Win32 ODBC MSI Installer/PROJECT_ASSISTANT/SINGLE_MSI_IMAGE/DiskImages/DISK1/HP ODBC 3.0.msi" $LIBDIR/Release/$INSTALLFILENAME
if [ "$?" -ne 0 ] 
then 
  echo "Error Moving $INSTALLFILENAME"
  exit 1
fi

#echo
#echo "Digitally Signing installer ..."
#rm -f $OUTSIGNDIR/*
#$HPSIGNDIR/HPSign.bat -r Neoview_NDCS -c $HPSIGNDIR/HPSign.conf -i $LIBDIR/Release/$INSTALLFILENAME -o $OUTSIGNDIR -obj Executable_batch_Sign_Local
#if [ "$?" -ne 0 ] 
#then 
#  echo "Error Digitally Signing $INSTALLFILENAME"
#  exit 1
#fi
#cp $OUTSIGNDIR/"$INSTALLFILENAME" $LIBDIR/Release/"$INSTALLFILENAME"
#if [ "$?" -ne 0 ] 
#then 
#  echo "Error Moving Digitally Signed $INSTALLFILENAME"
#  exit 1
#fi
#rm -f $OUTSIGNDIR/*

PATH=$SAVEPATH

}

function package_win64_release {

echo
echo ===============================
echo     PACKAGE WIN64 RELEASE
echo ===============================
echo

INSTALLDIR="$SRCDIR/Install"
INSTALLFILENAME=HPODBC64.msi
HPSIGNDIR=C:/Progra~1/Hewlett-Packard/HPCSS/HPCSS/HPSignClient1.4
OUTSIGNDIR=$LIBDIR/Release/signed
SAVEPATH=$PATH
PATH=$ISCMDBLD_PATH\;$PATH

attrib -r "$INSTALLDIR/*.*" /s

echo
echo "Building Media for $INSTALLFILENAME ..."
FILENAME="$INSTALLDIR/Win64 ODBC Installer MSI/Win64 ODBC MSI Installer.ism"
ISCmdBld.exe -p "$FILENAME"
if [ "$?" -ne 0 ] 
then 
  echo "Error Building Media for $INSTALLFILENAME"
  exit 1
fi

echo
echo "Moving $INSTALLFILENAME ..."
rm -f $LIBDIR/Release/$INSTALLFILENAME

mv "$INSTALLDIR/Win64 ODBC Installer MSI/Win64 ODBC MSI Installer/PROJECT_ASSISTANT/SINGLE_MSI_IMAGE/DiskImages/DISK1/HP ODBC64 3.0.msi" $LIBDIR/Release/$INSTALLFILENAME
if [ "$?" -ne 0 ] 
then 
  echo "Error Moving $INSTALLFILENAME"
  exit 1
fi

#echo
#echo "Digitally Signing installer ..."	
#rm -f $OUTSIGNDIR/*
#$HPSIGNDIR/HPSign.bat -r Neoview_NDCS -c $HPSIGNDIR/HPSign.conf -i $LIBDIR/Release/$INSTALLFILENAME -o $OUTSIGNDIR -obj Executable_batch_Sign_Local
#if [ "$?" -ne 0 ] 
#then 
#  echo "Error Digitally Signing $INSTALLFILENAME"
#  exit 1
#fi
#cp $OUTSIGNDIR/"$INSTALLFILENAME" $LIBDIR/Release/"$INSTALLFILENAME"
#if [ "$?" -ne 0 ] 
#then 
#  echo "Error Moving Digitally Signed $INSTALLFILENAME"
#  exit 1
#fi
#rm -f $OUTSIGNDIR/*

PATH=$SAVEPATH

}

function test_win32 {

# To be implemented later

}

function test_win64 {

# To be implemented later

}
init
package_win32_release
package_win64_release
fini

exit 0
