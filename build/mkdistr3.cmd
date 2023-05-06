@echo off
setlocal enabledelayedexpansion

pushd ..\src

set "buildConf=release static3"

if not -%1 == - goto %1

for /f "tokens=* delims=" %%p in ('"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -property installationPath -latest') do set vspath=%%p

echo %vspath%

call "%vspath%\VC\Auxiliary\Build\vcvars32.bat"

del build.log
for %%p in ("%buildConf%^|Win32") do devenv.com "rqpf.sln" /build %%p /Out build.log
if errorlevel 1 goto error

call "%vspath%\VC\Auxiliary\Build\vcvars64.bat"

del build64.log
for %%p in ("%buildConf%^|x64") do devenv.com "rqpf.sln" /build %%p /Out build64.log
if errorlevel 1 goto error

:pub
echo %buildConf%
set tmpd="%temp%\~rqprls"
rd /s /q %tmpd%
md %tmpd%
call :copyfiles "RQP_Def.farconfig RQP_Unicode.farconfig" ..\build %tmpd%
call :copyfiles "*.hlf *.lng changelog.txt" ..\docs %tmpd%
call :copyfiles *.md .. %tmpd%
md %tmpd%\bassplugs
echo Put BASS plugins (*.dll) into this folder>%tmpd%\bassplugs\readme.txt

for /f "tokens=2,3,4,5 delims=, " %%a in ('findstr "FILEVERSION" "rqpf.rc"') do set buildver=%%a%%b%%c%%d
echo !buildver!

call :pack x86 "%buildConf%" !buildver!_Far3
call :pack x64 "%buildConf%" !buildver!_Far3

rd /s /q %tmpd%
goto :eof

:pack
set archpath=%1
set archpath=%archpath:x86=.%
echo %archpath%
call :copyfiles tags.dll ..\lib\bass\%archpath% %tmpd%
call :copyfiles rqp*.dll "%archpath%\%~2" %tmpd%
7z a -r -t7z ..\rqp_v%3_%1.7z %tmpd%\*
if errorlevel 1 goto error
del %tmpd%\*.dll
goto :eof


:copyfiles
::echo %~2
pushd "%~2"
for %%p in (%~1) do (
	echo %~2\%%~p ==^> %~3
	copy /b "%%~p" "%~3"
)
popd
goto :eof

:error
echo Errorlevel %errorlevel%!
pause