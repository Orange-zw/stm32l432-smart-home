@echo off
setlocal

set "ARMAR=D:\MDK_keil5\arm\ARMCLANG\bin\armar.exe"
set "OBJDIR=%~dp0"
for %%I in ("%OBJDIR%..") do set "ROOTDIR=%%~fI"
set "OUTDIR=%ROOTDIR%\CONNECT"
set "MADE_ANY=0"
set "MADE_ESP=0"
set "MADE_E4G=0"

if not exist "%ARMAR%" (
  echo ERROR: armar.exe not found: "%ARMAR%"
  exit /b 1
)

if not exist "%OUTDIR%" (
  mkdir "%OUTDIR%"
  if errorlevel 1 exit /b %errorlevel%
)

if exist "%OBJDIR%esp8266.o" (
  call :MAKE_ONE "esp8266.o" "ESP8266.lib"
  if errorlevel 1 exit /b %errorlevel%
  set "MADE_ANY=1"
  set "MADE_ESP=1"
)

if exist "%OBJDIR%e4g.o" (
  call :MAKE_ONE "e4g.o" "E4G.lib"
  if errorlevel 1 exit /b %errorlevel%
  set "MADE_ANY=1"
  set "MADE_E4G=1"
)

if "%MADE_ANY%"=="0" (
  echo ERROR: no object found. Need esp8266.o and/or e4g.o in "%OBJDIR%"
  exit /b 1
)

if "%MADE_ESP%"=="1" (
  if exist "%ROOTDIR%\HARDWARE\ESP8266\esp8266.c" (
    attrib -r "%ROOTDIR%\HARDWARE\ESP8266\esp8266.c" >nul 2>&1
    del /q "%ROOTDIR%\HARDWARE\ESP8266\esp8266.c"
    if errorlevel 1 exit /b %errorlevel%
  )
)

if "%MADE_E4G%"=="1" (
  if exist "%ROOTDIR%\HARDWARE\E4G\E4G.c" (
    attrib -r "%ROOTDIR%\HARDWARE\E4G\E4G.c" >nul 2>&1
    del /q "%ROOTDIR%\HARDWARE\E4G\E4G.c"
    if errorlevel 1 exit /b %errorlevel%
  )
)

endlocal
exit /b 0

:MAKE_ONE
set "OBJ=%OBJDIR%%~1"
set "OUT=%OUTDIR%\%~2"

"%ARMAR%" --create "%OUT%" "%OBJ%"
if errorlevel 1 exit /b %errorlevel%

"%ARMAR%" -t "%OUT%"
exit /b 0
