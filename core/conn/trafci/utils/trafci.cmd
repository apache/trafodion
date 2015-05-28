@echo off&Title Trafodion Command Interface

CLS
set MY_SQROOT=
set CMD_LINE_ARGS=
set TRAFCI_QUERY_STRING=
set TRAFCI_CLASSPATH="##TRAFCI_CLASSPATH##"
set ROLE_ARGS=-r ""
set count=0

:set_args_loop

set /a count+=1
set CMD_LINE_ARGS=%CMD_LINE_ARGS% %1
shift /1
if %count%==16 goto after_set_args_loop

goto set_args_loop

:after_set_args_loop
set binlocation=%~dp0
set driver=%binlocation:~0,2%
%driver%
cd %binlocation%
java -classpath %TRAFCI_CLASSPATH% org.trafodion.ci.UserInterface %CMD_LINE_ARGS%

if not %ERRORLEVEL% EQU -9999 goto end_loop
echo.
echo Press any key to close this session
pause > nul

:end_loop
