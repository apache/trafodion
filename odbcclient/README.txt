Prerequisite:

openssl >= 0.9.8
Microsoft Visual Studio >= 2010
64bit odbccp32.lib from Microsoft Visual Studio 2008 or previous.
InstallShield >= 2012
git > 1.9

Build steps

1. In order to make the build script be able to find these Prerequisite,
 some variables on the top of build_os.bat and pkg.bat need to be set accordingly.

set variable OPENSSL_PATH to point to openssl binary folder, for example:
	set OPENSSL_PATH=E:\OpenSSL101e_bin
the folder should have structure like this:
	2013/10/31  18:13    <DIR>          .
	2013/10/31  18:13    <DIR>          ..
	2013/10/31  18:13    <DIR>          bin
	2013/02/11  08:26           435,662 changes.txt
	2013/10/31  18:13    <DIR>          exp
	2013/02/11  08:26            46,614 faq.txt
	2013/10/31  18:13    <DIR>          include
	2013/10/31  18:13    <DIR>          lib
	2013/02/11  09:59         1,647,616 libeay32.dll
	2013/02/11  09:59           351,744 libssl32.dll
	2013/02/11  08:02             6,279 license.txt
	2013/02/11  08:26            27,423 news.txt
	2004/05/10  16:23            30,423 OpenSSLhelp.chm
	2013/02/11  08:26             9,130 readme.txt
	2013/02/11  09:59           351,744 ssleay32.dll
	2013/10/31  18:13            20,790 unins000.dat
	2013/10/31  18:11           715,038 unins000.exe


set variable ODBCCP32_64_PATH point to Visual Studio 8 PlatformSDK libary amd64 folder, for example:
	set ODBCCP32_64_PATH="E:\Program Files (x86)\Microsoft Visual Studio 8\VC\PlatformSDK\Lib\amd64"

set variable MSBUILD_PATH to point to msbuild system, for example:
	set MSBUILD_PATH=C:\Windows\Microsoft.NET\Framework64\v4.0.30319

set variable BUILDDIR to point to parent directory of win-odbc64 folder(trafodion windows driver source directory). for example:
	set BUILDDIR=E:

set variable GIT_BASH point to where git bash was installed.(from git install). for example:
	set GIT_BASH="C:\Program Files (x86)\Git\bin\sh.exe"
git need windows 

the path for installshield need to be set in pkg.bat

set variable PACKDRIVER to conn folder of trafodion source tree. for example:
	set PACKDRIVER=E:

set variable ISCMDBLD_PATH to Installshield install folder, for example:
	set ISCMDBLD_PATH="E:\Program Files (x86)\Installshield\2012\System"

2. to build, open a cmd window, and type

	cd E:\win-odbc64\odbcclient
	build_os.bat
	pkg.bat

the final Win ODBC driver installer package can be found at %PACKDRIVER%\lib\x64\Release