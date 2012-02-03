@echo off
setlocal

rem
rem  This is the Windows apccontrol file.
rem

rem Assign parameters to named variables
SET command=%1
SET sbindir=%5

rem Strip leading and trailing quotation marks from paths.
rem This is easily accomplished on NT, but Win95/98/ME
rem require an evil little trick with 'FOR'.
SET sbindir=%sbindir:"=%
IF "%sbindir%" == "" FOR %%A IN (%5) DO SET sbindir=%%A

rem Paths to important executables
SET APCUPSD="%sbindir%\apcupsd"
SET SHUTDOWN="%sbindir%\shutdown"
SET BACKGROUND="%sbindir%\background"

rem Only do popups on Win95/98/ME/NT. All other platforms support 
rem balloon notifications which are provided by apctray.
SET POPUP=echo
VER | FIND /I "Windows 95" > NUL
IF NOT ERRORLEVEL 1 SET POPUP=%BACKGROUND% "%sbindir%\popup"
VER | FIND /I "Windows 98" > NUL
IF NOT ERRORLEVEL 1 SET POPUP=%BACKGROUND% "%sbindir%\popup"
VER | FIND /I "Windows ME" > NUL
IF NOT ERRORLEVEL 1 SET POPUP=%BACKGROUND% "%sbindir%\popup"
VER | FIND /I "Windows NT" > NUL
IF NOT ERRORLEVEL 1 SET POPUP=%BACKGROUND% "%sbindir%\popup"

rem
rem This piece is to substitute the default behaviour with your own script,
rem   perl, C program, etc.
rem
rem You can customize any command by creating an executable file (may be a
rem   script or a compiled program) and naming it the same as the %1 parameter
rem   passed by apcupsd to this script. We will accept files with any extension
rem   included in PATHEXT (*.exe, *.bat, *.cmd, etc).
rem
rem After executing your script, apccontrol continues with the default action.
rem   If you do not want apccontrol to continue, exit your script with exit 
rem   code 99. E.g. "exit /b 99".
rem
rem WARNING: please be aware that if you add any commands before the shutdown
rem   in the downshutdown) case and your command errors or stalls, it will
rem   prevent your machine from being shutdown, so test, test, test to
rem   make sure it works correctly.
rem
rem The apccontrol.bat file will be replaced every time apcupsd is installed,
rem   so do NOT make event modifications in this file. Instead, override the
rem   event actions using event scripts as described above.
rem

rem Use CALL here because event script might be a batch file itself
CALL ".\%command%" 2> NUL

rem This is retarded. "IF ERRORLEVEL 99" means greater-than-or-
rem equal-to 99, so we have to synthesize an == using two IFs. 
rem Ahh, the glory of Windows batch programming. At least they 
rem gave us a NOT op.
IF NOT ERRORLEVEL 99 GOTO :events
IF NOT ERRORLEVEL 100 GOTO :done

:events

rem
rem powerout, onbattery, offbattery, mainsback events occur
rem   in that order.
rem

IF "%command%" == "commfailure"   GOTO :commfailure
IF "%command%" == "commok"        GOTO :commok
IF "%command%" == "powerout"      GOTO :powerout
IF "%command%" == "onbattery"     GOTO :onbattery
IF "%command%" == "offbattery"    GOTO :offbattery
IF "%command%" == "mainsback"     GOTO :mainsback
IF "%command%" == "failing"       GOTO :failing
IF "%command%" == "timeout"       GOTO :timeout
IF "%command%" == "loadlimit"     GOTO :loadlimit
IF "%command%" == "runlimit"      GOTO :runlimit
IF "%command%" == "doshutdown"    GOTO :doshutdown
IF "%command%" == "annoyme"       GOTO :annoyme
IF "%command%" == "emergency"     GOTO :emergency
IF "%command%" == "changeme"      GOTO :changeme
IF "%command%" == "remotedown"    GOTO :remotedown
IF "%command%" == "startselftest" GOTO :startselftest
IF "%command%" == "endselftest"   GOTO :endselftest
IF "%command%" == "battdetach"    GOTO :battdetach
IF "%command%" == "battattach"    GOTO :battattach

echo Unknown command '%command%'
echo.
echo Usage: %0 command
echo.
echo Warning: this script is intended to be launched by
echo apcupsd and should never be launched by users.
GOTO :done

:commfailure
   %POPUP% "Communications with UPS lost."
   GOTO :done

:commok
   %POPUP% "Communciations with UPS restored."
   GOTO :done

:powerout
   GOTO :done

:onbattery
   %POPUP% "Power failure. Running on UPS batteries."
   GOTO :done

:offbattery
   %POPUP% "Power has returned. No longer running on UPS batteries."
   GOTO :done

:mainsback
   GOTO :done

:failing
   %POPUP% "UPS battery power exhaused. Doing shutdown."
   GOTO :done

:timeout
   %POPUP% "UPS battery runtime limit exceeded. Doing shutdown."
   GOTO :done

:loadlimit
   %POPUP% "UPS battery discharge limit reached. Doing shutdown."
   GOTO :done

:runlimit
   %POPUP% "UPS battery runtime percent reached. Doing shutdown."
   GOTO :done

:doshutdown
rem
rem  If you want to try to power down your UPS, uncomment
rem    out the following lines, but be warned that if the
rem    following shutdown -h now doesn't work, you may find
rem    the power being shut off to a running computer :-(
rem  Also note, we do this in the doshutdown case, because
rem    there is no way to get control when the machine is
rem    shutdown to call this script with --killpower. As
rem    a consequence, we do both killpower and shutdown
rem    here.
rem  Note that Win32 lacks a portable way to delay for a
rem    given time, so we use the trick of pinging a
rem    non-existent IP address with a given timeout.
rem
rem   %APCUPSD% /kill
rem   ping -n 1 -w 5000 10.255.255.254 > NUL
rem   %POPUP% "Doing %APCUPSD% --killpower"
rem   %APCUPSD% --killpower
rem   ping -n 1 -w 12000 10.255.255.254 > NUL
rem
   %SHUTDOWN% -h now
   GOTO :done

:annoyme
   %POPUP% "Power problems: please logoff."
   GOTO :done

:emergency
   %POPUP% "Emergency shutdown initiated."
   GOTO :done

:changeme
   %POPUP% "Emergency! UPS batteries have failed: Change them NOW"
   GOTO :done

:remotedown
   %POPUP% "Shutdown due to master state or comms lost."
   GOTO :done

:startselftest
   %POPUP% "Self-test starting"
   GOTO :done

:endselftest
   %POPUP% "Self-test completed"
   GOTO :done

:battdetach
   %POPUP% "Battery disconnected"
   GOTO :done

:battattach
   %POPUP% "Battery reattached"
   GOTO :done

:done
rem That's all, folks

