Prerequisite:

openssl >= 0.9.8
Microsoft Visual Studio >= 2010
64bit odbccp32.lib from Microsoft Visual Studio 2008 or previous.
InstallShield >= 2012

Build steps

1. In order to make the build script be able to find these Prerequisite,
 some variables on the top of build_os.bat and pkg.bat need to be set accordingly.

set variable OPENSSL_PATH to point to openssl binary folder, for example:
	set OPENSSL_PATH=E:\OpenSSL101e_bin

set variable ODBCCP32_64_PATH point to Visual Studio 8 PlatformSDK libary amd64 folder, for example:
	set ODBCCP32_64_PATH="E:\Program Files (x86)\Microsoft Visual Studio 8\VC\PlatformSDK\Lib\amd64"

set variable MSBUILD_PATH to point to msbuild system, for example:
	set MSBUILD_PATH=C:\Windows\Microsoft.NET\Framework64\v4.0.30319

set variable BUILDDIR to point to conn\odbc folder of trafodion source tree. for example:
	set BUILDDIR=E:


the path for installshield need to be set in pkg.bat

set variable PACKDRIVER to conn folder of trafodion source tree. for example:
	set PACKDRIVER=E:

set variable ISCMDBLD_PATH to Installshield install folder, for example:
	set ISCMDBLD_PATH="E:\Program Files (x86)\Installshield\2012\System"

2. to build, open a cmd window, and type

	cd winodbc\odbcclient
	build_os.bat
	pkg.bat

the final Win ODBC driver installer package can be found at %PACKDRIVER%\odbc\lib