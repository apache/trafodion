#!/bin/ksh

# This script has to be run on PC under a MKS environment 
# where Visual Studio 2010 and InstallShield 2012 are setup.
# Make sure $BUILDDIR is setup correctly before running this script.

function init {

SRCDIR=$BUILDDIR/src/odbc
LIBDIR=$BUILDDIR/lib 
PATH=$MSBUILD_PATH\;$PATH
DATE=`date +%y%m%d`

if [ ! -d "$SRCDIR" ] 
then
  echo "Error: $SRCDIR does not exist" 
  exit 1
fi

}

function display_setup {

echo
echo ===============================
echo     BUILD SETUP
echo ===============================
echo
echo BUILDDIR=$BUILDDIR
echo

}

function build_clean {

echo
echo ===============================
echo     CLEAN UP
echo ===============================
echo

rm -Rf $LIBDIR/Release/*
rm -Rf $LIBDIR/x64/Release/* 
 
rm -Rf $SRCDIR/odbcclient/drvr35/Release/*
rm -Rf $SRCDIR/odbcclient/drvr35/TCPIPV4/Release/*
rm -Rf $SRCDIR/odbcclient/drvr35/TCPIPV6/Release/*
rm -Rf $SRCDIR/odbcclient/drvr35adm/Release/*
rm -Rf $SRCDIR/odbcclient/drvr35trace/Release/*
rm -Rf $SRCDIR/odbcclient/drvr35msg/Release/*
rm -Rf $SRCDIR/odbcclient/Drvr35Res/Release/*
rm -Rf $SRCDIR/odbcclient/TranslationDll/Release/*
rm -Rf $SRCDIR/Install/SetCertificateDirReg/SetCertificateDirReg/Release/*
rm -Rf $SRCDIR/Install/UpdateDSN/UpdateDSN/Release/*
 
rm -Rf $SRCDIR/odbcclient/drvr35/x64/Release/*
rm -Rf $SRCDIR/odbcclient/drvr35/TCPIPV4/x64/Release/*
rm -Rf $SRCDIR/odbcclient/drvr35/TCPIPV6/x64/Release/*
rm -Rf $SRCDIR/odbcclient/drvr35adm/x64/Release/*
rm -Rf $SRCDIR/odbcclient/drvr35trace/x64/Release/*
rm -Rf $SRCDIR/odbcclient/drvr35msg/x64/Release/*
rm -Rf $SRCDIR/odbcclient/Drvr35Res/x64/Release/*
rm -Rf $SRCDIR/odbcclient/TranslationDll/x64/Release/*
rm -Rf $SRCDIR/Install/SetCertificateDirReg/SetCertificateDirReg/x64/Release/*
rm -Rf $SRCDIR/Install/UpdateDSN/UpdateDSN/x64/Release/*  

}

#create_version_file function to update version string
# $1 should be filename
# $2 should be library version
# $3 should be timestamp
# $4 product name (ie HPODBC or HPODBC_DRVR)
function create_version_file {
echo
echo ===============================
echo     UPDATE VPROC
echo ===============================
echo

echo "// $3 Version File generate $2, svn revision $SVN_REVISION" > $1
echo "extern char* VprocString=\"$3 Version $MAJOR_VER.$MINOR_VER.$BUG_VER Release $REL_MAJ_VER.$REL_MIN_VER.$REL_SP_VER (Build release [$SVN_REVISION])\";" >> $1
echo "extern \"C\" void $3_Version_"$MAJOR_VER"_"$MINOR_VER"_"$BUG_VER"_Release_"$REL_MAJ_VER"_"$REL_MIN_VER"_"$REL_SP_VER"_Build_release_"$SVN_REVISION" ()" >> $1
echo "{ }" >> $1
}

function build_win32_release {

echo
echo ===============================
echo     BUILD WIN32 RELEASE
echo ===============================
echo

echo
echo "Building Drvr35Msg - Win32 Release..."
cd $SRCDIR/odbcclient/Drvr35Msg
msbuild.exe /t:rebuild Drvr35Msg.vcxproj /p:Platform=Win32 /p:Configuration=Release
case "$?" in
0) echo "Building Drvr35Msg - Win32 Release Successfully Completed.";;
*) echo "Building Drvr35Msg - Win32 Release Failed.";
   exit 1;;
esac

echo
echo "Building Drvr35 - Win32 Release..."
cd $SRCDIR/odbcclient/Drvr35
msbuild.exe /t:rebuild Drvr35.vcxproj /p:Platform=Win32 /p:Configuration=Release
case "$?" in
0) echo "Building Drvr35 - Win32 Release Successfully Completed.";;
*) echo "Building Drvr35 - Win32 Release Failed.";
   exit 1;;
esac

echo
echo "Building Drvr35Adm - Win32 Release..."
cd $SRCDIR/odbcclient/Drvr35Adm
msbuild.exe /t:rebuild Drvr35Adm.vcxproj /p:Platform=Win32 /p:Configuration=Release
case "$?" in
0) echo "Building Drvr35Adm - Win32 Release Successfully Completed.";;
*) echo "Building Drvr35Adm - Win32 Release Failed.";
   exit 1;;
esac

echo
echo "Building Drvr35Trace - Win32 Release..."
cd $SRCDIR/odbcclient/Drvr35Trace
msbuild.exe /t:rebuild Drvr35Trace.vcxproj /p:Platform=Win32 /p:Configuration=Release
case "$?" in
0) echo "Building Drvr35Trace - Win32 Release Successfully Completed.";;
*) echo "Building Drvr35Trace - Win32 Release Failed.";
   exit 1;;
esac

echo
echo "Building TCPIPV4 - Win32 Release..."
cd $SRCDIR/odbcclient/Drvr35/TCPIPV4
msbuild.exe /t:rebuild TCPIPV4.vcxproj /p:Platform=Win32 /p:Configuration=Release
case "$?" in
0) echo "Building TCPIPV4 - Win32 Release Successfully Completed.";;
*) echo "Building TCPIPV4 - Win32 Release Failed.";
   exit 1;;
esac

echo
echo "Building TCPIPV6 - Win32 Release..."
cd $SRCDIR/odbcclient/Drvr35/TCPIPV6
msbuild.exe /t:rebuild TCPIPV6.vcxproj /p:Platform=Win32 /p:Configuration=Release
case "$?" in
0) echo "Building TCPIPV6 - Win32 Release Successfully Completed.";;
*) echo "Building TCPIPV6 - Win32 Release Failed.";
   exit 1;;
esac

echo
echo "Building TranslationDll - Win32 Release..."
cd $SRCDIR/odbcclient/TranslationDll
msbuild.exe /t:rebuild TranslationDll.vcxproj /p:Platform=Win32 /p:Configuration=Release	
case "$?" in
0) echo "Building TranslationDll - Win32 Release Successfully Completed.";;
*) echo "Building TranslationDll - Win32 Release Failed.";
   exit 1;;
esac

echo "Building Drvr35Res - Win32 Release..."
cd $SRCDIR/odbcclient/Drvr35Res 
msbuild.exe /t:rebuild Drvr35Res.vcxproj /p:Platform=Win32 /p:Configuration=Release	
case "$?" in
0) echo "Building Drvr35Res - Win32 Release Successfully Completed.";;
*) echo "Building Drvr35Res - Win32 Release Failed.";
   exit 1;;
esac

echo    
echo "Building SetCertificateDirReg InstallHelper - Win32 Release..."
cd $SRCDIR/Install/SetCertificateDirReg/SetCertificateDirReg
msbuild.exe /t:rebuild SetCertificateDirReg.vcxproj /p:Platform=Win32 /p:Configuration=Release
case "$?" in
0) echo "Building SetCertificateDirReg InstallHelper - Win32 Release Successfully Completed.";;
*) echo "Building SetCertificateDirReg InstallHelper - Win32 Release Failed.";
   exit 1;;
esac

echo
echo "Building UpdateDSN InstallHelper - Win32 Release..."
cd $SRCDIR/Install/UpdateDSN/UpdateDSN
msbuild.exe /t:rebuild UpdateDSN.vcxproj /p:Platform=Win32 /p:Configuration=Release
case "$?" in
0) echo "Building UpdateDSN InstallHelper - Win32 Release Successfully Completed.";;
*) echo "Building UpdateDSN InstallHelper - Win32 Release Failed.";
   exit 1;;
esac

}

function build_win64_release {

echo
echo ===============================
echo     BUILD WIN64 RELEASE
echo ===============================
echo

echo
echo "Building Drvr35Msg - Win64 Release..."
cd $SRCDIR/odbcclient/Drvr35Msg
msbuild.exe /t:rebuild Drvr35Msg.vcxproj /p:Platform=x64 /p:Configuration=Release
case "$?" in
0) echo "Building Drvr35Msg - Win64 Release Successfully Completed.";;
*) echo "Building Drvr35Msg - Win64 Release Failed.";
   exit 1;;
esac

echo
echo "Building Drvr35 - Win64 Release..."
cd $SRCDIR/odbcclient/Drvr35
msbuild.exe /t:rebuild Drvr35.vcxproj /p:Platform=x64 /p:Configuration=Release
case "$?" in
0) echo "Building Drvr35 - Win64 Release Successfully Completed.";;
*) echo "Building Drvr35 - Win64 Release Failed.";
   exit 1;;
esac

echo    
echo "Building Drvr35Adm - Win64 Release..."
cd $SRCDIR/odbcclient/Drvr35Adm
msbuild.exe /t:rebuild Drvr35Adm.vcxproj /p:Platform=x64 /p:Configuration=Release
case "$?" in
0) echo "Building Drvr35Adm - Win64 Release Successfully Completed.";;
*) echo "Building Drvr35Adm - Win64 Release Failed.";
   exit 1;;
esac    

echo
echo "Building Drvr35Trace - Win64 Release..."
cd $SRCDIR/odbcclient/Drvr35Trace
msbuild.exe /t:rebuild Drvr35Trace.vcxproj /p:Platform=x64 /p:Configuration=Release
case "$?" in
0) echo "Building Drvr35Trace - Win64 Release Successfully Completed.";;
*) echo "Building Drvr35Trace - Win64 Release Failed.";
   exit 1;;
esac

echo
echo "Building TCPIPV4 - Win64 Release..."
cd $SRCDIR/odbcclient/Drvr35/TCPIPV4
msbuild.exe /t:rebuild TCPIPV4.vcxproj /p:Platform=x64 /p:Configuration=Release
case "$?" in
0) echo "Building TCPIPV4 - Win64 Release Successfully Completed.";;
*) echo "Building TCPIPV4 - Win64 Release Failed.";
   exit 1;;
esac

echo
echo "Building TCPIPV6 - Win64 Release..."
cd $SRCDIR/odbcclient/Drvr35/TCPIPV6
msbuild.exe /t:rebuild TCPIPV6.vcxproj /p:Platform=x64 /p:Configuration=Release
case "$?" in
0) echo "Building TCPIPV6 - Win64 Release Successfully Completed.";;
*) echo "Building TCPIPV6 - Win64 Release Failed.";
   exit 1;;
esac

echo
echo "Building TranslationDll - Win64 Release..."
cd $SRCDIR/odbcclient/TranslationDll
msbuild.exe /t:rebuild TranslationDll.vcxproj /p:Platform=x64 /p:Configuration=Release	
case "$?" in
0) echo "Building TranslationDll - Win64 Release Successfully Completed.";;
*) echo "Building TranslationDll - Win64 Release Failed.";
   exit 1;;
esac

echo
echo "Building Drvr35Res - Win64 Release..."
cd $SRCDIR/odbcclient/Drvr35Res 
msbuild.exe /t:rebuild Drvr35Res.vcxproj /p:Platform=x64 /p:Configuration=Release	
case "$?" in
0) echo "Building Drvr35Res - Win64 Release Successfully Completed.";;
*) echo "Building Drvr35Res - Win64 Release Failed.";
       exit 1;;
esac

echo    
echo "Building SetCertificateDirReg InstallHelper - Win64 Release..."
cd $SRCDIR/Install/SetCertificateDirReg/SetCertificateDirReg
msbuild.exe /t:rebuild SetCertificateDirReg.vcxproj /p:Platform=x64 /p:Configuration=Release
case "$?" in
0) echo "Building SetCertificateDirReg InstallHelper - Win64 Release Successfully Completed.";;
*) echo "Building SetCertificateDirReg InstallHelper - Win64 Release Failed.";
   exit 1;;
esac

echo  
echo "Building UpdateDSN InstallHelper - Win64 Release..."
cd $SRCDIR/Install/UpdateDSN/UpdateDSN
msbuild.exe /t:rebuild UpdateDSN.vcxproj /p:Platform=x64 /p:Configuration=Release
case "$?" in
0) echo "Building UpdateDSN InstallHelper - Win64 Release Successfully Completed.";;
*) echo "Building UpdateDSN InstallHelper - Win64 Release Failed.";
   exit 1;;
esac

}


function test_win32 {

# To be implemented later

}

function test_win64 {

# To be implemented later

}


init
#display_setup

build_clean

create_version_file "$SRCDIR/odbcclient/drvr35/version_drvr.cpp" "$DATE" "HPODBC_DRVR"

build_win32_release
build_win64_release

$BUILDDIR/src/odbc/BuildOdbcDrvr/package.ksh 2>&1

echo
echo ===============================
echo     ODBCDRVR BUILD COMPLETED!
echo ===============================
echo

exit 0

