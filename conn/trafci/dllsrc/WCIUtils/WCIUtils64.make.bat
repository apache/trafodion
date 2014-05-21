@echo off
del CIUtils.obj
del CIUtils.exp
del CIUtils.lib
del WCIUtils64.dll

@echo on
echo.
echo  Adding Java2 SDK include paths to the MSVC C++ Compiler Include Path
echo  If its different on your PC, please edit the WNCIUtils.make.bat file.
echo.
set include=%include%;C:\Program Files\Java\jdk1.6.0_30\include;C:\Program Files\Java\jdk1.6.0_30\include\win32;


del org_trafodion_ci_WCIUtils.h

javah -jni -o org_trafodion_ci_WCIUtils.h -classpath C:\Temp\trafci-Build\classes org.trafodion.ci.WCIUtils

rem  === See ReadMe.txt for cl options

rem cl /O2 /GL /EHsc /W3 /Wp64 /TP -LD CIUtils.cpp


rem Building 64 bit dll without MSVC RT dependency by linking it statically(/MT)
cl /O2 /GL /EHsc /W3 /Wp64 /TP /MT -LD CIUtils.cpp
 

@echo off

rem ===  The next command will create a smaller file with a MSVC RT dependency
rem ===  and also create a manifest file to be used with mt. Adds /MD and /Zi to the 
rem ===  cl options.
rem ===
rem cl /O2 /GL /EHsc /MD /W3 /Wp64 /Zi /TP -LD CIUtils.cpp
rem

@echo on
rename CIUtils.dll WCIUtils64.dll
rem cp WCIUtils64.dll ../../src 
